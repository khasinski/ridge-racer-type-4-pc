"""ds_lowram_report.py <log> [decomp-root]: group a DS_LOWRAM_LOG by function.

The patched duckstation-regtest (DUCKSTATION.md) writes one line per new
(pc, address) pair when code touches PS1 RAM 0..0xFFFF while the game's code
is running or is the caller (ra): "R|W pc=... addr=... ra=... frame=...".
Game code there is a null-pointer access (or the SDK's own kernel patches);
kernel code with a game return address is a BIOS call. Lines are grouped as
G (game pc, by function) or K (kernel pc, by the calling game function), with
the first frame and the addresses touched. Function names come from the
decompilation's configs/PAL/sym.main.txt (decomp-root, default
../ridge-racer-type-4-decomp next to this repository).
"""
import bisect, collections, os, re, sys

root = sys.argv[2] if len(sys.argv) > 2 else os.path.join(
    os.path.dirname(os.path.abspath(__file__)), '../../../ridge-racer-type-4-decomp')
funcs = []
for line in open(os.path.join(root, 'configs/PAL/sym.main.txt')):
    m = re.match(r'\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+);.*type:func', line)
    if m:
        funcs.append((int(m.group(2), 16), m.group(1)))
funcs.sort()
starts = [a for a, _ in funcs]


def name(a):
    if a >= 0x80114878:
        return 'overlay+%x' % (a - 0x80114878)
    i = bisect.bisect_right(starts, a) - 1
    return funcs[i][1] if i >= 0 else '?'


def game(a):
    return 0x80010000 <= a < 0x80200000


groups = collections.OrderedDict()
for line in open(sys.argv[1]):
    m = re.match(r'(\w) pc=(\w+) addr=(\w+) ra=(\w+) frame=(\d+)', line)
    if not m:
        continue
    kind, pc, addr, ra, frame = m.group(1), int(m.group(2), 16), int(m.group(3), 16), int(m.group(4), 16), int(m.group(5))
    key = ('G', name(pc), kind) if game(pc) else ('K', name(ra) if game(ra) else '%08x' % ra, kind)
    entry = groups.setdefault(key, [set(), frame])
    entry[0].add(addr & 0x1FFFFFFF)
for (where, func, kind), (addrs, frame) in groups.items():
    a = sorted(addrs)
    print(where, kind, func, 'first@%d' % frame, 'n=%d' % len(a), ' '.join('%#x' % x for x in a[:8]), '...' if len(a) > 8 else '')
