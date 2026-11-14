"""ds_calltime.py at until spec... : PS1 time of game and SDK calls on DuckStation's recompiler,
without breakpoints.

GDB breakpoints and single-stepping switch DuckStation to its interpreter,
whose timing differs from the recompiler the reference runs use by -45% to
+20% per routine (docs/ARCHITECTURE.md, PS1 time model). This tool instead
writes a small timing stub into kernel RAM that stays unused (0x8000A000,
0x8000C000, 0x8000F000) and points each function's first words at it
through GDB RAM writes, which DuckStation recompiles: the stub reads root
counter 0 (free-running at the system clock) at the call and at its return
and keeps the last 64 clocks per function. It boots the disc, installs the
stubs at DS field `at` (the executable is whole from field 282), runs to
`until` and prints per function the calls, min, median and the last values.

spec: name|0xaddr[:tail][:r8][:arg=reg] with names from the decomp's
sym.main.txt (RR4_DECOMP, default ~/Projects/ridge-racer-type-4-decomp).
tail: the function is an A0/B0/C0 BIOS trampoline (li t2; jr t2; li t1),
r8: restart root counter 2 at system clock / 8 at entry, for calls longer
than 65535 clocks (not nest-safe), arg=a1: log (clocks, a1 at entry) pairs.
The default stub is nest-safe and costs about 10 clocks of its own.
DUMMYBP=1 sets a never-hit breakpoint, for the same run on the interpreter.
Run it through run_capped.sh with RR4_DUCKSTATION, RR4_DISC and
RR4_DS_PORTS set (duckstation.py).
"""
import sys, os, struct, statistics
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from duckstation import DuckStation
R = {n: i for i, n in enumerate('zero at v0 v1 a0 a1 a2 a3 t0 t1 t2 t3 t4 t5 t6 t7 s0 s1 s2 s3 s4 s5 s6 s7 t8 t9 k0 k1 gp sp fp ra'.split())}
def r(op, rs, rt, rd, sh, fn): return (op << 26) | (R[rs] << 21) | (R[rt] << 16) | (R[rd] << 11) | (sh << 6) | fn
def i(op, rs, rt, imm): return (op << 26) | (R[rs] << 21) | (R[rt] << 16) | (imm & 0xFFFF)
def lui(rt, imm): return i(0x0F, 'zero', rt, imm)
def addiu(rt, rs, imm): return i(0x09, rs, rt, imm)
def ori(rt, rs, imm): return i(0x0D, rs, rt, imm)
def sw(rt, off, base): return i(0x2B, base, rt, off)
def lw(rt, off, base): return i(0x23, base, rt, off)
def lhu(rt, off, base): return i(0x25, base, rt, off)
def andi(rt, rs, imm): return i(0x0C, rs, rt, imm)
def addu(rd, rs, rt): return r(0, rs, rt, rd, 0, 0x21)
def sll(rd, rt, sh): return r(0, 'zero', rt, rd, sh, 0)
def jr(rs): return r(0, rs, 'zero', 'zero', 0, 0x08)
def j(addr): return (0x02 << 26) | ((addr >> 2) & 0x3FFFFFF)
NOP = 0
def hi(a): return ((a >> 16) + ((a & 0x8000) != 0)) & 0xFFFF
def lo(a): return a & 0xFFFF
def words(ws): return b''.join(struct.pack('<I', w) for w in ws)

def is_branch(w):
    op = w >> 26
    if op == 0: return (w & 0x3F) in (0x08, 0x09)  # jr jalr
    return op in (1, 2, 3, 4, 5, 6, 7) or (op in (0x10, 0x11, 0x12) and ((w >> 21) & 0x1F) == 8)

def uses(w, reg):
    n = R[reg]; op = w >> 26
    return ((w >> 21) & 0x1F) == n or ((w >> 16) & 0x1F) == n or (op == 0 and ((w >> 11) & 0x1F) == n)


def subu(rd, rs, rt): return r(0, rs, rt, rd, 0, 0x23)

def free_stub(stub, fn, moved, slot, log, tail=False, timer=0x1100, ring_mask=0xFF, reset_mode=None, arg=None):
    """Nest-safe timing stub at `stub` for fn: reads the free-running timer
    (RCnt0 at the system clock) at entry and at return. `moved` are fn's
    first words replaced by `j stub; nop` (ordinary instructions), or, with
    tail, a whole trampoline ending in jr + delay slot (an A0/B0 BIOS stub)
    whose callee returns to the stub. slot: +0 start count, +4 saved ra.
    log: +0 byte index, +4 sum, +8.. ring of words."""
    if not tail:
        assert len(moved) == 2
        for w in moved:
            assert not is_branch(w), hex(w)
    for w in moved:
        assert not uses(w, 'at') and not uses(w, 't8') and not uses(w, 't9'), hex(w)
    if reset_mode is not None:  # restart the counter at entry (not nest-safe)
        head = [lui('t8', 0x1F80), ori('t9', 'zero', reset_mode), sw('t9', timer + 4, 't8'), addiu('t9', 'zero', 0)]
    else:
        head = [lui('t8', 0x1F80), lhu('t9', timer, 't8')]
    head += [
            lui('at', hi(slot)), sw('t9', lo(slot), 'at'), sw('ra', lo(slot) + 4, 'at')]
    if arg:
        head += [sw(arg, lo(slot) + 8, 'at')]
    head += [None, None]  # ra = ret
    body = list(moved) + ([] if tail else [j(fn + 4 * len(moved)), NOP])
    n = len(head) + len(body)
    ret = stub + 4 * n
    head[-2] = lui('ra', hi(ret)); head[-1] = addiu('ra', 'ra', lo(ret))
    tailc = [lui('t8', 0x1F80), lhu('t9', timer, 't8'),
             lui('at', hi(slot)), lw('t8', lo(slot), 'at'), NOP,
             subu('t9', 't9', 't8'), andi('t9', 't9', 0xFFFF),
             lui('at', hi(log)), lw('t8', lo(log) + 4, 'at'), NOP,
             addu('t8', 't8', 't9'), sw('t8', lo(log) + 4, 'at'),
             lw('t8', lo(log), 'at'), NOP, andi('t8', 't8', ring_mask * 4 + 3), addu('t8', 't8', 'at'),
             sw('t9', lo(log) + 8, 't8')]
    if arg:  # pairs (clocks, the argument at entry)
        tailc += [lui('t9', hi(slot)), lw('t9', lo(slot) + 8, 't9'), NOP, sw('t9', lo(log) + 12, 't8')]
    tailc += [
             lw('t8', lo(log), 'at'), NOP, addiu('t8', 't8', 8 if arg else 4), sw('t8', lo(log), 'at'),
             lui('at', hi(slot)), lw('ra', lo(slot) + 4, 'at'), NOP, jr('ra'), NOP]
    return words(head + body + tailc)

SYMS = {}
for l in open(os.path.join(os.environ.get('RR4_DECOMP', os.path.expanduser('~/Projects/ridge-racer-type-4-decomp')), 'configs/PAL/sym.main.txt')):
    p = l.split()
    if len(p) >= 3 and p[1] == '=': SYMS[p[0]] = int(p[2].rstrip(';'), 16)
def addrs(k): return 0x8000C000 + k * 0xC0, 0x8000A000 + k * 16, 0x8000F000 + k * 0x110
def install(d, specs):
    for k, spec in enumerate(specs):
        nm, *fl = spec.split(':'); tail = 'tail' in fl; r8 = 'r8' in fl
        arg = next((f[4:] for f in fl if f.startswith('arg=')), None)
        fn = int(nm, 16) if nm.startswith('0x') else SYMS[nm]
        stub, slot, log = addrs(k)
        w = list(struct.unpack('<3I', d.mem(fn, 12)))
        moved = w[:3] if tail else w[:2]
        d.poke(log, b'\0' * 0x110)
        d.poke(stub, free_stub(stub, fn, moved, slot, log, tail=tail, ring_mask=0x3F,
                                    timer=0x1120 if r8 else 0x1100, reset_mode=0x200 if r8 else None, arg=arg))
        d.poke(fn, words([j(stub), NOP] + ([NOP] if tail else [])))
def report(d, specs, out=sys.stdout):
    for k, spec in enumerate(specs):
        stub, slot, log = addrs(k)
        sc = 8 if ':r8' in spec else 1
        idx, total = struct.unpack('<2I', d.mem(log, 8)); n = idx // 4
        ring = struct.unpack('<64I', d.mem(log + 8, 256))
        if ':arg=' in spec:
            n = idx // 8; m = min(n, 32)
            pairs = [(sc * ring[2 * ((n - m + i) % 32)], ring[2 * ((n - m + i) % 32) + 1]) for i in range(m)]
            v = [c for c, a in pairs]
            out.write('%-28s calls %5d pairs (clocks,arg): %s\n' % (spec, n, ' '.join('%d,%d' % p for p in pairs)))
            continue
        m = min(n, 64)
        v = [sc * ring[(n - m + i) % 64] for i in range(m)]
        out.write('%-28s calls %5d mean %8.1f min %6s median %8s max %6s | %s\n' % (spec, n, sc * total / n if n else 0,
                  min(v) if v else '-', statistics.median(v) if v else '-', max(v) if v else '-', ' '.join(map(str, v[-24:]))))
if __name__ == '__main__':
    at, until = int(sys.argv[1]), int(sys.argv[2]); specs = sys.argv[3:]
    with DuckStation() as d:
        if os.environ.get('DUMMYBP'): d._gsend('Z0,%x,4' % 0x8000DFF0)
        d.advance(at); install(d, specs)
        d.advance(until - at)
        report(d, specs)
