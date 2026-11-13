#!/usr/bin/env python3
"""Compare the PAL disc with another Ridge Racer Type 4 release.

    tools/port/region_diff.py PAL.cue OTHER.cue [--decomp DIR] [--section S]...

Reads both images directly (MODE2/2352 .bin next to the .cue), and reports
what docs/REGIONS.md records:

  boot   SYSTEM.CNF boot file, executable header, overlay load address
  exe    every PAL main-executable function (decomp configs/PAL/functions.txt)
         matched in the other executable: identical, relocated only (branch/
         call targets and data addresses moved), immediate constants changed
         (li / slti / sltiu / andi operands), or structurally different
  ovl    the same for the nine overlays (configs/PAL/overlays/*.json)
  r4     R4.BIN directory: entry counts, GetR4FileIndex group bases and which
         entries are byte-identical between the discs
  str    R4.STR video streams (frames, sectors per frame, size) and XA audio

Nothing is written; the report goes to stdout. --decomp defaults to
~/Projects/ridge-racer-type-4-decomp.
"""
import argparse
import collections
import difflib
import glob
import hashlib
import json
import os
import struct
import sys

EXE_BASE = 0x80010000
REG = ['zero', 'at', 'v0', 'v1', 'a0', 'a1', 'a2', 'a3', 't0', 't1', 't2', 't3',
       't4', 't5', 't6', 't7', 's0', 's1', 's2', 's3', 's4', 's5', 's6', 's7',
       't8', 't9', 'k0', 'k1', 'gp', 'sp', 'fp', 'ra']
IMM_OPS = {8: 'addi', 9: 'addiu', 10: 'slti', 11: 'sltiu', 12: 'andi', 13: 'ori',
           14: 'xori', 0x20: 'lb', 0x21: 'lh', 0x23: 'lw', 0x24: 'lbu',
           0x25: 'lhu', 0x28: 'sb', 0x29: 'sh', 0x2b: 'sw', 4: 'beq', 5: 'bne',
           6: 'blez', 7: 'bgtz', 1: 'bcond', 0x32: 'lwc2', 0x3a: 'swc2'}


class Disc:
    """ISO9660 files of a MODE2/2352 image (Form 1 and Form 2 sectors)."""

    def __init__(self, cue):
        self.cue = cue
        name = None
        with open(cue, encoding='latin-1') as f:
            for line in f:
                line = line.strip()
                if line.upper().startswith('FILE '):
                    name = line[5:].rsplit(' ', 1)[0].strip('"')
                    break
        if not name:
            sys.exit('%s: no FILE line' % cue)
        self.bin = open(os.path.join(os.path.dirname(cue), name), 'rb')
        pvd = self.sector(16)[24:24 + 2048]
        if pvd[1:6] != b'CD001':
            sys.exit('%s: no ISO9660 volume descriptor' % cue)
        self.files = {}
        root = pvd[156:190]
        self._walk(struct.unpack_from('<I', root, 2)[0],
                   struct.unpack_from('<I', root, 10)[0], '')

    def sector(self, lba):
        self.bin.seek(lba * 2352)
        return self.bin.read(2352)

    def read(self, lba, size):
        out = bytearray()
        while len(out) < size:
            s = self.sector(lba)
            out += s[24:24 + (2324 if s[18] & 0x20 else 2048)]
            lba += 1
        return bytes(out[:size])

    def _walk(self, lba, size, prefix):
        data = self.read(lba, size)
        i = 0
        while i < len(data):
            n = data[i]
            if n == 0:
                i = (i // 2048 + 1) * 2048
                continue
            rec = data[i:i + n]
            i += n
            name = rec[33:33 + rec[32]].decode('ascii', 'replace')
            if name in ('\x00', '\x01'):
                continue
            elba = struct.unpack_from('<I', rec, 2)[0]
            esize = struct.unpack_from('<I', rec, 10)[0]
            if rec[25] & 2:
                self._walk(elba, esize, prefix + name + '/')
            else:
                self.files[prefix + name.split(';')[0]] = (elba, esize)

    def file(self, name):
        lba, size = self.files[name]
        return self.read(lba, size)

    def boot(self):
        for line in self.file('SYSTEM.CNF').decode('latin-1').splitlines():
            key, _, value = line.partition('=')
            if key.strip().upper() == 'BOOT':
                return value.strip().split('\\')[-1].split(';')[0]
        return None


def r4_entries(r4):
    n = struct.unpack_from('<I', r4, 0)[0]
    offs = struct.unpack_from('<%dI' % (n + 1), r4, 4)
    return [r4[offs[i] * 2048:offs[i + 1] * 2048] for i in range(n)]


def dis(x, masked):
    """One MIPS word as text; masked drops relocatable operands."""
    op = x >> 26
    rs, rt, imm = (x >> 21) & 31, (x >> 16) & 31, x & 0xffff
    simm = imm - 0x10000 if imm & 0x8000 else imm
    if x == 0:
        return 'nop'
    if op in (2, 3):
        return ('j', 'jal')[op - 2] + (' T' if masked else ' %08x' % (0x80000000 | ((x & 0x3ffffff) << 2)))
    if op == 0xf:
        return 'lui %s,%s' % (REG[rt], 'H' if masked else hex(imm))
    if op in IMM_OPS:
        # Constants survive the mask: li (addiu/ori from $zero), compares,
        # logic immediates and stack offsets. Other immediates may be the low
        # half of a moved address.
        keep = (not masked or (rs == 0 and op in (9, 13)) or op in (10, 11, 12, 14) or rs == 29)
        return '%s %s,%s,%s' % (IMM_OPS[op], REG[rt], REG[rs], simm if keep else 'I')
    if op == 0:
        return 'special%02x %s,%s,%s,%d' % (x & 63, REG[rs], REG[rt], REG[(x >> 11) & 31], (x >> 6) & 31)
    return '%08x' % x


def is_constant(text):
    head = text.split(' ', 1)[0]
    return (head in ('slti', 'sltiu', 'andi', 'xori') or
            (head in ('addiu', 'ori', 'addi') and ',zero,' in text))


def match_functions(funcs, read_a, words_b, base_b, expect_delta):
    """Classify each (name, addr, size) of A against the word list of B."""
    masked_b = [dis(x, True) for x in words_b]
    index = collections.defaultdict(list)
    for i in range(len(masked_b) - 3):
        index[tuple(masked_b[i:i + 4])].append(i)
    rows, delta = [], expect_delta
    for name, addr, size in funcs:
        n = size // 4
        if n == 0:
            continue
        wa = [read_a(addr + 4 * k) for k in range(n)]
        ma = [dis(x, True) for x in wa]
        cands = index.get(tuple(ma[:4]), []) if n >= 4 else range(len(masked_b) - n + 1)
        hits = [c for c in cands if masked_b[c:c + n] == ma]
        if hits:
            c = min(hits, key=lambda c: abs(base_b + 4 * c - addr - delta))
            delta = base_b + 4 * c - addr
            wb = list(words_b[c:c + n])
            kind = 'identical' if wb == wa else 'relocated'
            rows.append((name, addr, size, kind, base_b + 4 * c, []))
            continue
        # Same shape when constants are masked too?  Then only constants moved.
        loose_a = [t.split(' ')[0] for t in ma]
        start = (addr + delta - base_b) // 4
        window = range(max(0, start - 256), min(len(masked_b) - n, start + 256) + 1)
        found = None
        for c in sorted(window, key=lambda c: abs(c - start)):
            if [t.split(' ')[0] for t in masked_b[c:c + n]] == loose_a:
                pairs = [(x, y) for x, y in zip(ma, masked_b[c:c + n]) if x != y]
                if pairs and all(is_constant(x) for x, _ in pairs):
                    found = c, pairs
                    break
        if found:
            c, pairs = found
            delta = base_b + 4 * c - addr
            rows.append((name, addr, size, 'constants', base_b + 4 * c, pairs))
            continue
        # Structural change: align against the neighbourhood.
        win = masked_b[max(0, start - 64):start + n + 64]
        sm = difflib.SequenceMatcher(None, ma, win, autojunk=False)
        changed = n - sum(b.size for b in sm.get_matching_blocks())
        rows.append((name, addr, size, 'structural', None, changed))
    return rows


def report_functions(title, rows):
    counts = collections.Counter(r[3] for r in rows)
    print('%s: %d functions: %s' % (title, len(rows),
          ', '.join('%s %d' % (k, counts[k]) for k in ('identical', 'relocated', 'constants', 'structural'))))
    for name, addr, size, kind, other, extra in rows:
        if kind == 'constants':
            shown = '; '.join('%s -> %s' % (x, y.split(',')[-1]) for x, y in extra[:6])
            more = ' (+%d)' % (len(extra) - 6) if len(extra) > 6 else ''
            print('  const  %-32s %08x %#6x  %s%s' % (name, addr, size, shown, more))
    for name, addr, size, kind, other, extra in rows:
        if kind == 'structural':
            print('  struct %-32s %08x %#6x  ~%d instructions differ' % (name, addr, size, extra))


def exe_words(exe):
    return struct.unpack_from('<%dI' % ((len(exe) - 0x800) // 4), exe, 0x800)


def r4_group_bases(exe, getr4_addr):
    """GetR4FileIndex switch: the five 'addiu v0,a1,imm' returns in case order."""
    out, addr = [], getr4_addr
    for _ in range(32):
        x = struct.unpack_from('<I', exe, addr - EXE_BASE + 0x800)[0]
        if x >> 16 == 0x24a2:
            out.append(x & 0xffff)
        addr += 4
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('pal')
    ap.add_argument('other')
    ap.add_argument('--decomp', default=os.path.expanduser('~/Projects/ridge-racer-type-4-decomp'))
    ap.add_argument('--section', action='append', choices=['boot', 'exe', 'ovl', 'r4', 'str'])
    a = ap.parse_args()
    sections = a.section or ['boot', 'exe', 'ovl', 'r4', 'str']
    pal, oth = Disc(a.pal), Disc(a.other)
    pal_exe, oth_exe = pal.file(pal.boot()), oth.file(oth.boot())
    funcs = []
    with open(os.path.join(a.decomp, 'configs/PAL/functions.txt')) as f:
        for line in f:
            if line.strip():
                name, addr, size = line.split()
                funcs.append((name, int(addr, 16), int(size, 16)))
    read_pal = lambda addr: struct.unpack_from('<I', pal_exe, addr - EXE_BASE + 0x800)[0]
    exe_rows = None

    def exe_match():
        nonlocal exe_rows
        if exe_rows is None:
            exe_rows = match_functions(funcs, read_pal, exe_words(oth_exe), EXE_BASE, 0)
        return exe_rows

    if 'boot' in sections:
        for name, d, exe in (('PAL', pal, pal_exe), ('other', oth, oth_exe)):
            pc, text, tsize = struct.unpack_from('<I4xII', exe, 0x10)
            ovl = struct.unpack_from('<I', exe, 0x800 + 0x2F0)[0]
            print('%-5s boot=%s exe=%d bytes pc0=%08x text=%08x+%#x overlay base=%08x' %
                  (name, d.boot(), len(exe), pc, text, tsize, ovl))
            print('      files: %s' % ', '.join('%s %d' % (k, v[1]) for k, v in sorted(d.files.items())))

    if 'exe' in sections:
        report_functions('main executable', exe_match())

    pal_r4 = oth_r4 = None
    if 'ovl' in sections or 'r4' in sections:
        pal_r4, oth_r4 = r4_entries(pal.file('R4.BIN')), r4_entries(oth.file('R4.BIN'))

    if 'ovl' in sections:
        for jf in sorted(glob.glob(os.path.join(a.decomp, 'configs/PAL/overlays/*.json'))):
            j = json.load(open(jf))
            if 'functions' not in j:
                continue
            entry = j.get('entry', j.get('index'))
            base = j.get('load_address', 0x80114878)
            img_a, img_b = pal_r4[entry], oth_r4[entry]
            read_a = lambda addr, img=img_a, base=base: struct.unpack_from('<I', img, addr - base)[0]
            words_b = struct.unpack_from('<%dI' % (len(img_b) // 4), img_b, 0)
            fl = [(f['symbol'], f['address'], f['size']) for f in j['functions']]
            report_functions('overlay %d %s (module id %d/%d, %d/%d bytes)' %
                             (entry, j.get('name', os.path.basename(jf)[:-5]),
                              struct.unpack_from('<I', img_a)[0], struct.unpack_from('<I', img_b)[0],
                              len(img_a), len(img_b)),
                             match_functions(fl, read_a, words_b, base, 0))

    if 'r4' in sections:
        rows = {r[0]: r for r in exe_match()}
        g = rows['GetR4FileIndex']
        pal_bases = r4_group_bases(pal_exe, g[1])
        oth_bases = r4_group_bases(oth_exe, g[4]) if g[4] else []
        print('R4.BIN entries: PAL %d, other %d' % (len(pal_r4), len(oth_r4)))
        print('GetR4FileIndex bases (groups 1,2,3,4,0): PAL %s other %s' %
              ([hex(x) for x in pal_bases], [hex(x) for x in oth_bases]))
        hashes = collections.defaultdict(list)
        for j, e in enumerate(oth_r4):
            hashes[hashlib.md5(e).digest()].append(j)
        names = ['course/car'] + ['group %d' % k for k in (1, 2, 3, 4, 0)]
        for k, (lo, hi) in enumerate(zip([0] + pal_bases, pal_bases + [len(pal_r4)])):
            olo = ([0] + oth_bases)[k]
            ohi = (oth_bases + [len(oth_r4)])[k]
            same = [i for i in range(lo, hi) if hashes.get(hashlib.md5(pal_r4[i]).digest())]
            at_slot = [i for i in range(lo, hi) if olo + i - lo < ohi and pal_r4[i] == oth_r4[olo + i - lo]]
            print('  %-10s PAL %3d entries [%d..%d], other %3d [%d..%d]: %d identical somewhere, %d at the same slot' %
                  (names[k], hi - lo, lo, hi - 1, ohi - olo, olo, ohi - 1, len(same), len(at_slot)))

    if 'str' in sections:
        for name, d in (('PAL', pal), ('other', oth)):
            lba, size = d.files['R4.STR']
            d.bin.seek(lba * 2352)
            streams, xa = [], collections.Counter()
            for k in range(size // 2048):
                s = d.bin.read(2352)
                if s[18] & 0x04:
                    xa[s[17]] += 1
                    continue
                if s[24:26] != b'\x60\x01':
                    continue
                kind = s[26:28].hex()
                chunk, chunks, frame = struct.unpack_from('<HHI', s, 28)
                w, h = struct.unpack_from('<HH', s, 40)
                if chunk:
                    continue
                if streams and streams[-1][0] == kind and frame == streams[-1][2] + 1:
                    streams[-1][2], streams[-1][4] = frame, k
                else:
                    streams.append([kind, frame, frame, k, k, w, h])
            for kind, f0, f1, s0, s1, w, h in streams:
                print('%-5s video %s frames %d..%d %dx%d, %.2f sectors/frame' %
                      (name, kind, f0, f1, w, h, (s1 - s0) / max(1, f1 - f0)))
            print('%-5s XA audio sectors by channel: %s' % (name, dict(sorted(xa.items()))))


if __name__ == '__main__':
    main()
