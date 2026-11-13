# Contributing

This is a native C11 project. Preserve the intended game behavior and data
formats while adapting the code for modern 64-bit hosts. There is no byte-match
requirement, stock PS1 compiler requirement, PS1 checksum gate, source manifest
requirement or decompilation progress/debt ratchet.

## Development workflow

1. Repair the original modules in place under `src/main/` and `src/overlays/`;
   use subsystem headers under `include/` for shared declarations and ordinary
   C types and functions.
2. Remove or simplify unnecessary PS1 hardware/BIOS plumbing; use native C or
   our PsyZ fork where needed, preserving gameplay behavior.
3. Build with `cmake -S . -B build -DBUILD_TESTING=ON` and
   `cmake --build build --parallel`.
4. Run `ctest --test-dir build --output-on-failure` to execute the native test
   suite.
5. Validate changed game behavior with focused native tests or headless
   reproducible scenarios (`PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy`), and
   with the disc regression runs below when the change touches game logic,
   timing or rendering.
6. Run `tools/port/check.sh`: it compiles every game file with Clang (CC
   defaults to `clang`; other compilers are refused, the flags are
   Clang's) and a second time for 64-bit Windows (LLP64) when the MinGW-w64
   headers are there. It must report 0 failing in both passes.

Do not preserve register assignments, instruction scheduling, padding, linker
order or code addresses for compiler matching. Assembly or GTE macros that
perform real work must be translated to portable C or routed through PsyZ; do
not replace functionality with no-op stubs.

Serialized assets (`R4.BIN`, `R4.STR`, relative `.rel` archives, TIMs, VABs)
and save data retain their defined widths, offsets and endianness. Host
pointers must use real pointer types (`void *`, typed struct pointers,
`OT_TYPE *`), and host structures can be separated from raw 32-bit serialized
offsets.

Automated game runs go through `tools/port/run_capped.sh`, without a window
or sound (`PSYZ_HEADLESS=1` or `gpu`, `SDL_AUDIODRIVER=dummy`): it waits for
one of the machine-wide run slots and kills a run past its memory limit.

## Disc regression runs

CI has no disc image, so game-level regressions are checked locally with
`tools/port/regress.sh` and your PAL CUE/BIN image (`RR4_DISC=<.cue>`). It
plays fixed scenarios from power-on with an empty memory card: `attract`
(no input), `ta` (menus, a Time Attack race, results, replay, name entry),
`gp` (a Grand Prix race) and `vs` (a two-player race). Each run is headless
at 1x through `run_capped.sh` and records a frame capture every 30 VSyncs
and the game state of every field (mode, race state, car distances, the
BIOS `rand()` state).

```sh
tools/port/regress.sh record <commit>       # reference from a commit (built in a worktree)
tools/port/regress.sh check <commit>        # build the work tree and compare
tools/port/regress.sh check <commit> ta gp  # only some scenarios
tools/port/regress.sh check -b build/ridge-racer-type-4-pc <commit>
tools/port/regress.sh list                  # scenarios and stored references
```

`check` reports, per scenario, the first VSync where the state trace
differs and which captures differ, keeps the first 20 differing frames as
PNG (with a difference image when ImageMagick is installed; the reference
keeps all its frames), and exits 1 on any difference. With enhancements
off, a change should leave both identical; when a difference is intended
(a fixed bug), say so in the commit and record a new reference.
References, worktrees and builds live in `RR4_REGRESS_DIR` (default
`~/.cache/rr4-regress`); a reference takes about 70 MB, plus a worktree
and a build for each recorded commit.

## Layout sweep

`tools/port/sweep_layout.py <decomp-root>` finds host globals that do not
cover their original storage (aliases, interior symbols, truncated tables).
It needs the libclang Python binding (`pip install clang`) and the libclang
library: set `LIBCLANG` to `libclang.dylib`, `libclang.so` or
`libclang.dll`, or let it try the Xcode Command Line Tools, Xcode and
`/usr/lib/llvm-*/lib`. Accepted findings are listed in
`tools/port/sweep_layout_allow.txt`.

## Documentation

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md),
[docs/CODE_ORGANIZATION.md](docs/CODE_ORGANIZATION.md),
[docs/CONVENTIONS.md](docs/CONVENTIONS.md) and
[docs/RELEASING.md](docs/RELEASING.md). Open work (known bugs, fidelity gaps
and debt) is tracked in [docs/TECH_DEBT.md](docs/TECH_DEBT.md).
