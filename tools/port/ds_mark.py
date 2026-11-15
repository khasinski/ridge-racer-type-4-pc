"""ds_mark.py at until name... : when functions are entered on DuckStation's recompiler, without breakpoints.

At DS field `at` it points the first two instructions of each function at a
stub in kernel RAM the game leaves unused (0x8000C000) that writes a marker
(0x100 + its index) to root counter 2's count and goes on. The patched
regtest logs that write with DUCKSTATION_CD_TRACE (RCNT2 off 0), next to the
CD commands, their completions and the vblanks, all with the system tick;
this prints them from the first call of the first function, each with its
clocks since the previous line and its position in its field. Entry-to-entry
clocks are the [mark] values of include/rr/cpu_time.h and PsyZ's
timing_profile.h.

Root counter 2 is the sequencer's timer from SsStart (DS frame 426) on:
writing its count after that shifts the sequencer, so mark later functions
only to time them, not to follow the game after. A function whose first two
instructions branch or use t8/t9 (a BIOS trampoline such as PsyqBzero) cannot
be marked. Names come from the decomp's sym.main.txt (RR4_DECOMP), or give
an address (0x...). Run it through run_capped.sh with RR4_DUCKSTATION and
RR4_DISC set (duckstation.py); RR4_MARK_LOG keeps the raw log.
"""
import bisect, os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ds_calltime as dc
from duckstation import DuckStation

CLOCK = 33868800


def report(log, first, out=sys.stdout):
    names, blanks, rows = {}, [], []
    for line in open(log):
        f = line.split()
        if not f:
            continue
        if f[0] == 'MARK':
            names[int(f[1])] = f[2]
        elif f[0] == 'VBLANK':
            blanks.append(int(f[2]))
        elif f[0] == 'RCNT2' and f[2] == '0':
            rows.append((int(f[6]), names.get(int(f[4], 16), f[4])))
        elif f[0] in ('CDCMD', 'CDDONE'):
            rows.append((int(f[3]), f[0] + ' ' + f[1]))
    rows.sort()
    start = prev = None
    for t, name in rows:
        if start is None and name == first:
            start = t
        if start is None:
            continue
        i = bisect.bisect_right(blanks, t) - 1
        field = '%7.3f ms into its field' % ((t - blanks[i]) * 1e3 / CLOCK) if i >= 0 else ''
        out.write('%10.3f ms  +%9d clk  (%s)  %s\n' % ((t - start) * 1e3 / CLOCK,
                  t - prev if prev is not None else 0, field, name))
        prev = t


def main():
    at, until, names = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3:]
    os.environ['DUCKSTATION_CD_TRACE'] = '1'
    log = os.environ.get('RR4_MARK_LOG') or os.path.join(
        os.environ.get('TMPDIR', '/tmp'), 'ds_mark.%d.log' % os.getpid())
    with open(log, 'w') as lf, DuckStation(log=lf) as d:
        d.advance(at)
        for k, name in enumerate(names):
            fn = int(name, 16) if name.startswith('0x') else dc.SYMS[name]
            stub = 0x8000C000 + k * 0x40
            moved = list(struct.unpack('<2I', d.mem(fn, 8)))
            for w in moved:
                if dc.is_branch(w) or dc.uses(w, 't8') or dc.uses(w, 't9'):
                    raise SystemExit('%s: its first instructions cannot be moved (%08x)' % (name, w))
            code = [dc.lui('t8', 0x1F80), dc.ori('t9', 'zero', 0x100 + k), dc.sw('t9', 0x1120, 't8')]
            d.poke(stub, dc.words(code + moved + [dc.j(fn + 8), dc.NOP]))
            d.poke(fn, dc.words([dc.j(stub), dc.NOP]))
            lf.write('MARK %d %s\n' % (0x100 + k, name))
            lf.flush()
        d.advance(until - at)
    report(log, names[0])


if __name__ == '__main__':
    main()
