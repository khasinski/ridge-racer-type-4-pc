#!/bin/sh
# Disc-based regression runs: replay fixed scenarios with your PAL disc image
# and compare 1x frame captures and per-field game-state traces against a
# reference recorded from a given commit.
#
#   tools/port/regress.sh record [-b <binary>] [<commit>] [<scenario>...]
#   tools/port/regress.sh check  [-b <binary>] <ref-commit> [<scenario>...]
#   tools/port/regress.sh list
#
# record builds <commit> (default HEAD) in a private worktree and stores its
# captures and traces as the reference for that commit. check builds the
# current work tree (or runs -b <binary>) through the same scenarios and
# compares; it exits 1 when anything differs and keeps the differing frames.
# -b <binary> skips the build (record then labels the reference with the
# <commit> given, so the binary must be built from it).
#
# Scenarios (all scripted from power-on with a fresh, empty memory card):
#   attract  no input: opening, title, menu idle, attract demo race
#   ta       menus -> Time Attack race (auto-driven, laps fast-forwarded)
#            -> replay (skipped after 300 fields) -> retry -> second race
#   gp       Grand Prix: team and car selection, first race, replay,
#            results, the second race starting
#   vs       two-player VS race (split screen), replay, retry, second race
#
# Each run is headless (PSYZ_HEADLESS=1: the CPU rasterizer at 1x),
# silent (SDL_AUDIODRIVER=dummy) and goes through tools/port/run_capped.sh,
# so it waits for a machine-wide run slot and is killed past the memory
# limit. The disc's CD timing is the PS1's (PSYZ_CD_TIMING is not set).
#
# Environment:
#   RR4_DISC               the CUE file (default: the first *.cue under
#                          ../ridge-racer-type-4-decomp/raw/ next to the repo)
#   RR4_REGRESS_DIR        references, worktrees and builds
#                          (default ${XDG_CACHE_HOME:-~/.cache}/rr4-regress)
#   RR4_REGRESS_STRIDE     capture every n-th VSync (default 30)
#   RR4_REGRESS_OUT        where check writes its results
#                          (default $RR4_REGRESS_DIR/check/<ref>-vs-<label>)
#
# Builds use -DCMAKE_BUILD_TYPE=Release -DRR4_DIAGNOSTICS=ON
# -DRR4_EXPORT_SYMBOLS=ON (the test hooks and RR4_WATCH need both).
# A reference is only comparable with the same scenario definitions: check
# refuses a scenario whose parameters (params.txt) changed since the record.
set -eu

root=$(cd "$(dirname "$0")/../.." && pwd)
store=${RR4_REGRESS_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/rr4-regress}
stride=${RR4_REGRESS_STRIDE:-30}
capped="$root/tools/port/run_capped.sh"
all_scenarios="attract ta gp vs"

die() { echo "regress.sh: $*" >&2; exit 2; }

# --- scenarios ----------------------------------------------------------
# scenario <name>: sets frames, pads, pokes and extra (env assignments).
# Pad and poke steps are anchored to modes (ModeGroup/ModeIndex), see
# RR4_PAD_SCRIPT and RR4_POKE in src/port/main.c.
boot='start@m2/4:100+4%100,start@m1/4:150'
scenario() {
    pads='' pokes='' extra=''
    case $1 in
    attract)
        frames=12000
        ;;
    ta)
        frames=6600
        pads="$boot,down@m1/4:450,start@m1/4:550,circle@m1/50:200,circle@m1/51:200"
        pads="$pads,circle@m1/51:260,circle@m1/54:200+4%120,start@m1/22:300"
        for d in 700 800 900 1000 1100 1200; do
            pokes="$pokes${pokes:+,}PlayerCars+388+=TrackLength@m1/53:$d"
        done
        extra=RR4_AUTODRIVE=1
        ;;
    gp)
        frames=12600
        pads='start@m2/4:700+4%300,start@m1/4:200+4%150,start@m0/2:100+4%150'
        for m in 6 8 10 11 12 14 16 18 34 48; do
            pads="$pads,circle@m1/$m:200+4%154"
        done
        pads="$pads,circle@m0/4:200+4%154,start@m1/22:120,down@m1/43:200"
        pads="$pads,circle@m1/43:300+4%154"
        for d in 700 800 900 1000 1100 1200 1300 1400 1500; do
            pokes="$pokes${pokes:+,}PlayerCars+388+=TrackLength@m1/47:$d"
        done
        extra=RR4_AUTODRIVE=1
        ;;
    vs)
        frames=7000
        pads='2:select@100,start@m2/4:700+4%300,start@m1/4:150,down@m1/4:450'
        pads="$pads,down@m1/4:500,start@m1/4:550,circle@m1/56:150,circle@m1/56:300"
        pads="$pads,2:circle@m1/56:450,2:circle@m1/56:600,circle@m1/57:200"
        pads="$pads,start@m1/22:120,circle@m1/58:300,circle@m1/58:500"
        pads="$pads,circle@m1/58:700+4%150,start@m1/22:300"
        pokes='PlayerCars+1204+=TrackLength@m1/38:700,PlayerCars+388+=TrackLength@m1/38:750'
        pokes="$pokes,PlayerCars+1204+=TrackLength@m1/38:800,PlayerCars+388+=TrackLength@m1/38:850"
        pokes="$pokes,PlayerCars+1204+=TrackLength@m1/38:900,PlayerCars+1204+=TrackLength@m1/38:1000"
        extra=RR4_AUTODRIVE=1
        ;;
    *)
        die "unknown scenario $1 (one of: $all_scenarios)"
        ;;
    esac
}

# Globals printed with every traced field besides the mode state: race
# state, distance along the course of both player cars and the first two
# opponents, and the BIOS rand() state (RR4_TRACE_RAND).
watch='RaceState,PlayerCars+388,PlayerCars+1204,ComCars+388,ComCars+1188'

params() {
    scenario "$1"
    printf 'frames=%s\nstride=%s\npads=%s\npokes=%s\nextra=%s\nwatch=%s\n' \
        "$frames" "$stride" "$pads" "$pokes" "$extra" "$watch"
}

# --- disc, builds -------------------------------------------------------
find_disc() {
    if [ -n "${RR4_DISC:-}" ]; then
        [ -f "$RR4_DISC" ] || die "RR4_DISC=$RR4_DISC does not exist"
        disc=$RR4_DISC
        return
    fi
    disc=$(find "$root/../ridge-racer-type-4-decomp/raw" -name '*.cue' 2> /dev/null | sort | head -1)
    [ -n "$disc" ] || die "no disc image: set RR4_DISC=<path to the PAL .cue>"
}

configure_build() { # <source> <build-dir>
    cmake -S "$1" -B "$2" -DCMAKE_BUILD_TYPE=Release -DRR4_DIAGNOSTICS=ON \
        -DRR4_EXPORT_SYMBOLS=ON -DBUILD_TESTING=OFF > "$2.configure.log" 2>&1 ||
        { tail -20 "$2.configure.log" >&2; die "configure failed ($2.configure.log)"; }
    cmake --build "$2" --parallel > "$2.build.log" 2>&1 ||
        { grep -E 'error' "$2.build.log" | head -20 >&2; die "build failed ($2.build.log)"; }
}

# Check out <sha> with its own PsyZ and SDL commits, cloned from the local
# submodules (no network), and build it once.
build_commit() { # <sha> -> bin
    src="$store/src/$1"
    build="$store/build/$1"
    bin="$build/ridge-racer-type-4-pc"
    [ -x "$bin" ] && return
    mkdir -p "$store/src" "$store/build"
    if [ ! -d "$src" ]; then
        git -C "$root" worktree add -d "$src" "$1" > /dev/null
        git -C "$src" submodule init third_party/psyz > /dev/null
        git -C "$src" config submodule.third_party/psyz.url "$root/third_party/psyz"
        git -C "$src" -c protocol.file.allow=always submodule update third_party/psyz > /dev/null
        git -C "$src/third_party/psyz" submodule init external/SDL > /dev/null
        git -C "$src/third_party/psyz" config submodule.external/SDL.url \
            "$root/third_party/psyz/external/SDL"
        git -C "$src/third_party/psyz" -c protocol.file.allow=always \
            submodule update external/SDL > /dev/null
    fi
    echo "building $1 in $build"
    configure_build "$src" "$build"
}

build_worktree() { # -> bin
    build="$root/build-regress"
    bin="$build/ridge-racer-type-4-pc"
    echo "building the work tree in $build"
    configure_build "$root" "$build"
}

# --- runs ---------------------------------------------------------------
to_png() { # <ppm> <png>
    if command -v magick > /dev/null 2>&1; then
        magick "$1" "$2"
    elif command -v sips > /dev/null 2>&1; then
        sips -s format png "$1" --out "$2" > /dev/null 2>&1
    else
        return 1
    fi
}

sha256() {
    if command -v sha256sum > /dev/null 2>&1; then sha256sum "$@"; else shasum -a 256 "$@"; fi
}

# run_scenario <name> <bin> <out-dir> <keep-png 0|1>: run.log, trace.txt,
# frames.txt ("<vsync> <sha256>"), and with keep-png the frames as PNG.
run_scenario() {
    name=$1 sbin=$2 sout=$3 keep=$4
    scenario "$name"
    rm -rf "$sout"
    mkdir -p "$sout/run/cfg"
    params "$name" > "$sout/params.txt"
    (
        cd "$sout/run"
        # shellcheck disable=SC2086 # extra is a list of assignments
        env PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy RR4_CONFIG_HOME="$sout/run/cfg" \
            RR4_FRAME_LIMIT="$frames" RR4_PAD_SCRIPT="$pads" RR4_POKE="$pokes" \
            RR4_TRACE=1 RR4_TRACE_RAND=1 RR4_WATCH="$watch" \
            RR4_CAPTURE=cap RR4_CAPTURE_AT="$stride-$frames/$stride" $extra \
            "$capped" "$sbin" "$disc" > ../run.log 2>&1
    ) || echo "$name: the game exited with status $? (see $sout/run.log)" >&2
    grep '^vsync ' "$sout/run.log" > "$sout/trace.txt" || true
    : > "$sout/frames.txt"
    for f in $(cd "$sout/run" && ls cap.*.ppm 2> /dev/null | sort -t. -k2 -n); do
        v=${f#cap.}; v=${v%.ppm}
        echo "$v $(sha256 "$sout/run/$f" | cut -d' ' -f1)" >> "$sout/frames.txt"
        if [ "$keep" = 1 ]; then
            mkdir -p "$sout/frames"
            to_png "$sout/run/$f" "$sout/frames/$v.png" || cp "$sout/run/$f" "$sout/frames/$v.ppm"
            rm -f "$sout/run/$f"
        fi
    done
    [ "$keep" = 1 ] && rm -rf "$sout/run"
    # A run that dies early leaves a short trace: say so here, check compares.
    last=$(tail -1 "$sout/trace.txt" | cut -d' ' -f2)
    [ "${last:-0}" -ge "$frames" ] ||
        echo "$name: only ${last:-0} of $frames fields traced (see $sout/run.log)" >&2
    echo "$name: $(wc -l < "$sout/trace.txt" | tr -d ' ') fields, $(wc -l < "$sout/frames.txt" | tr -d ' ') frames"
}

run_all() { # <bin> <dir> <keep> <scenario...>
    rbin=$1 dir=$2 keep=$3
    shift 3
    pids=
    for s in "$@"; do
        run_scenario "$s" "$rbin" "$dir/$s" "$keep" &
        pids="$pids $!"
    done
    for p in $pids; do wait "$p" || true; done
}

compare_scenario() { # <name> <ref-dir> <cur-dir> -> 0 same, 1 differs
    name=$1 cref=$2 ccur=$3
    rc=0
    if ! cmp -s "$cref/params.txt" "$ccur/params.txt"; then
        echo "$name: scenario parameters changed since the reference; record it again"
        diff "$cref/params.txt" "$ccur/params.txt" | sed 's/^/  /' | head -10
        return 1
    fi
    if cmp -s "$cref/trace.txt" "$ccur/trace.txt"; then
        echo "$name: state trace identical ($(wc -l < "$ccur/trace.txt" | tr -d ' ') fields)"
    else
        rc=1
        first=$(diff "$cref/trace.txt" "$ccur/trace.txt" | grep -m1 '^[<>] vsync' | cut -d' ' -f3)
        echo "$name: state trace differs from vsync ${first:-?} ($(diff "$cref/trace.txt" "$ccur/trace.txt" | grep -c '^>') fields)"
        diff "$cref/trace.txt" "$ccur/trace.txt" > "$ccur/trace.diff" || true
        head -6 "$ccur/trace.diff" | sed 's/^/  /'
    fi
    # join wants lexical order; differs.txt goes back to VSync order.
    sort -k1,1 "$cref/frames.txt" > "$ccur/ref-frames.sorted"
    sort -k1,1 "$ccur/frames.txt" > "$ccur/frames.sorted"
    join "$ccur/ref-frames.sorted" "$ccur/frames.sorted" | awk '$2 != $3 {print $1}' |
        sort -n > "$ccur/differs.txt"
    missing=$(join -v1 "$ccur/ref-frames.sorted" "$ccur/frames.sorted" | wc -l | tr -d ' ')
    extra=$(join -v2 "$ccur/ref-frames.sorted" "$ccur/frames.sorted" | wc -l | tr -d ' ')
    rm -f "$ccur/ref-frames.sorted" "$ccur/frames.sorted"
    nd=$(wc -l < "$ccur/differs.txt" | tr -d ' ')
    if [ "$nd" = 0 ] && [ "$missing" = 0 ] && [ "$extra" = 0 ]; then
        echo "$name: captures identical ($(wc -l < "$ccur/frames.txt" | tr -d ' ') frames)"
        rm -rf "$ccur/run"
    else
        rc=1
        echo "$name: $nd captures differ, $missing missing, $extra extra (first differing vsync $(head -1 "$ccur/differs.txt"))"
        mkdir -p "$ccur/frames"
        for v in $(head -20 "$ccur/differs.txt"); do
            to_png "$ccur/run/cap.$v.ppm" "$ccur/frames/$v.png" 2> /dev/null ||
                cp "$ccur/run/cap.$v.ppm" "$ccur/frames/$v.ppm"
            if command -v magick > /dev/null 2>&1 && [ -f "$cref/frames/$v.png" ]; then
                magick compare "$cref/frames/$v.png" "$ccur/frames/$v.png" \
                    "$ccur/frames/$v.diff.png" 2> /dev/null || true
            fi
        done
        echo "  reference frames: $cref/frames, differing frames (first 20): $ccur/frames"
        rm -rf "$ccur/run"
    fi
    return $rc
}

# --- commands -----------------------------------------------------------
cmd=${1:-}
[ $# -gt 0 ] && shift
given_bin=
if [ "${1:-}" = -b ]; then
    [ $# -ge 2 ] || die "-b needs a binary"
    given_bin=$(cd "$(dirname "$2")" && pwd)/$(basename "$2")
    [ -x "$given_bin" ] || die "$2 is not an executable"
    shift 2
fi

resolve() { # <commit-ish> -> sha
    git -C "$root" rev-parse --verify --quiet "$1^{commit}" || die "unknown commit $1"
}

case $cmd in
list)
    for s in $all_scenarios; do
        scenario "$s"
        echo "$s: $frames fields"
    done
    if [ -d "$store/refs" ]; then
        echo "references in $store/refs:"
        for d in "$store/refs"/*/; do
            [ -d "$d" ] || continue
            echo "  $(basename "$d") $(sed -n 's/^describe: //p' "$d/meta.txt" 2> /dev/null)"
        done
    fi
    ;;
record)
    commit=HEAD
    if [ $# -gt 0 ] && ! echo " $all_scenarios " | grep -q " $1 "; then
        commit=$1
        shift
    fi
    sha=$(resolve "$commit")
    # shellcheck disable=SC2086 # one word per scenario
    [ $# -gt 0 ] || set -- $all_scenarios
    for s in "$@"; do scenario "$s"; done
    find_disc
    if [ -n "$given_bin" ]; then bin=$given_bin; else build_commit "$sha"; fi
    ref="$store/refs/$sha"
    mkdir -p "$ref"
    {
        echo "commit: $sha"
        echo "describe: $(git -C "$root" describe --tags --always "$sha")"
        echo "binary: $bin"
        echo "recorded: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    } > "$ref/meta.txt"
    echo "recording $* for $sha into $ref"
    run_all "$bin" "$ref" 1 "$@"
    ;;
check)
    [ $# -gt 0 ] || die "check needs the reference commit"
    sha=$(resolve "$1")
    shift
    ref="$store/refs/$sha"
    [ -d "$ref" ] || die "no reference for $sha: run tools/port/regress.sh record $sha"
    # shellcheck disable=SC2086 # one word per scenario
    [ $# -gt 0 ] || set -- $all_scenarios
    for s in "$@"; do
        scenario "$s"
        [ -f "$ref/$s/frames.txt" ] || die "the reference for $sha has no $s: record $sha $s"
    done
    find_disc
    if [ -n "$given_bin" ]; then
        bin=$given_bin
        label=$(basename "$(dirname "$bin")")
    else
        build_worktree
        label=$(git -C "$root" describe --tags --always --dirty)
    fi
    out=${RR4_REGRESS_OUT:-$store/check/$(git -C "$root" rev-parse --short "$sha")-vs-$label}
    rm -rf "$out"
    mkdir -p "$out"
    echo "checking $* against $sha ($ref) into $out"
    run_all "$bin" "$out" 0 "$@"
    status=0
    for s in "$@"; do
        compare_scenario "$s" "$ref/$s" "$out/$s" || status=1
    done
    if [ $status = 0 ]; then echo "regress: all identical"; else echo "regress: differences (see $out)"; fi
    exit $status
    ;;
*)
    sed -n '2,/^set -eu/p' "$0" | sed '$d; s/^# \{0,1\}//'
    exit 2
    ;;
esac
