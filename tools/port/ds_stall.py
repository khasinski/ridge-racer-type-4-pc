"""ds_stall.py <ds-log> [-hz]: DuckStation's clocks of marked calls without
their interrupts and their CPU stalls for the GPU's list DMA.

The log is a ds_frames.py run with DSF_MARKS (name:ret), DSF_EXC=1 and
DUCKSTATION_DMA_TRACE=1 (tools/port/DUCKSTATION.md: the regtest's DMAT and
DMAS lines; a slice's stall is its DMAS "used", the GPU channel's slices
outside an exception). Per frame (CdFileStep's entry to the next), as the
port's vsync: name=clocks (entry to return, the exceptions and the stalls
taken out; /n for n calls) and name.st=the stalls. -hz: per course render
(SubmitCourseHorizon, the pass programs' entries marked by address), each
pass's clocks the same way, and <log>.hzst with the passes' stalls. The
race frame's cost fits (src/main/system/cpuwork.c, submodel.c) take these
against the port's work counts of the same frames (RR4_MARK_WORK)."""
import sys, bisect


class Exc:
    """Interrupt exceptions from a DSF_EXC log: inside(t0, t1) is their time in [t0, t1)."""
    def __init__(self, path):
        iv = []; cur = None
        for l in open(path, errors='replace'):
            if not l.startswith('XMARK'): continue
            f = l.split()
            if len(f) < 8: continue
            x = int(f[1], 16); t = int(f[3]); off = f[7]
            if off == '90' and x & 0x8000 and (x & 0x7C) == 0 and x < 0x80000000:
                cur = t
            elif off in ('94', '98') and cur is not None:
                iv.append((cur, t)); cur = None
        self.s = [a for a, _ in iv]; self.iv = iv
        self.cum = [0]
        for a, b in iv: self.cum.append(self.cum[-1] + b - a)
    def inside(self, t0, t1):
        i = bisect.bisect_left(self.s, t0); tot = 0
        # an exception that began before t0 and ends after it
        if i > 0 and self.iv[i - 1][1] > t0: tot += min(self.iv[i - 1][1], t1) - t0
        j = bisect.bisect_left(self.s, t1)
        for a, b in self.iv[i:j]: tot += min(b, t1) - a
        return tot


log = sys.argv[1]; HZ = '-hz' in sys.argv
X = Exc(log)
names, blanks, ev, sl = {}, [], [], []
open_dma = {}
for line in open(log, errors='replace'):
    f = line.split()
    if not f: continue
    k = f[0]
    if k == 'MARK': names[int(f[1])] = f[2]
    elif k == 'VBLANK': blanks.append((int(f[2]), int(f[4])))
    elif k == 'XMARK':
        if len(f) > 7 and f[7] != '90': continue
        x, t = int(f[1], 16), int(f[3])
        if x >= 0x8000: continue
        ev.append((t, names.get(x, hex(x))))
    elif k == 'DMAT' and f[1] == '2':
        open_dma[2] = int(f[-1])
    elif k == 'DMAS' and f[1] == '2' and 2 in open_dma:  # the slice's own count (the tick leaves out pending ticks)
        t0 = open_dma.pop(2); sl.append((t0, int(f[3])))
    elif k in ('DMAE', 'DMAC') and f[1] == '2' and 2 in open_dma:
        t0 = open_dma.pop(2); sl.append((t0, int(f[-1]) - t0))
sl.sort()
# keep the slices outside the exceptions (an exception's time is taken out whole)
keep = [(a, d) for a, d in sl if X.inside(a, a + 1) == 0]
S = [a for a, _ in keep]; C = [0]
for _, d in keep: C.append(C[-1] + d)
def stall(t0, t1):
    i = bisect.bisect_left(S, t0); j = bisect.bisect_left(S, t1)
    return C[j] - C[i]
ev.sort(); T = [t for t, _ in blanks]
def vs(t):
    i = bisect.bisect_right(T, t) - 1
    return blanks[i][1], t - T[i]
if HZ:
    cur = None; o2 = open(log + '.hzst', 'w')
    for t, n in ev:
        if n == 'SubmitCourseHorizon': cur = [t, []]
        elif cur is not None and n.startswith('0x') and not n.endswith('.ret'): cur[1].append((n, t))
        elif cur is not None and n == 'SubmitCourseHorizon.ret':
            v = vs(cur[0])[0] - 296
            seq = cur[1] + [('end', t)]
            parts = [(a[0], b[1] - a[1] - X.inside(a[1], b[1]), stall(a[1], b[1])) for a, b in zip(seq, seq[1:])]
            print(v, t - cur[0], ' '.join('%s=%d' % (a, d - s) for a, d, s in parts), 'head=%d' % ((seq[0][1] - cur[0]) if cur[1] else 0))
            o2.write('%d %s\n' % (v, ' '.join('%s=%d' % (a, s) for a, _, s in parts)))
            cur = None
    sys.exit()
frames = []; cur = None; op = {}
for t, n in ev:
    ret = n.endswith('.ret'); b = n[:-4] if ret else n
    if b == 'CdFileStep' and not ret:
        if cur: frames.append(cur)
        v, off = vs(t); cur = {'v': v - 296, 'start': off, 't0': t, 'd': {}, 's': {}, 'c': {}}; op = {}
    if cur is None: continue
    if ret:
        if b in op:
            t0 = op.pop(b); s = stall(t0, t)
            cur['d'][b] = cur['d'].get(b, 0) + t - t0 - X.inside(t0, t) - s
            cur['s'][b] = cur['s'].get(b, 0) + s; cur['c'][b] = cur['c'].get(b, 0) + 1
        if b == 'DrawSync': cur['end'] = t - cur['t0']
    else: op[b] = t
if cur: frames.append(cur)
for fr in frames:
    print(fr['v'], 'start=%d' % fr['start'], 'frame=%s' % fr.get('end', '-'),
          ' '.join('%s=%d%s %s.st=%d' % (n, d, '' if fr['c'][n] == 1 else '/%d' % fr['c'][n], n, fr['s'][n]) for n, d in fr['d'].items()))
