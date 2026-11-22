"""ds_frames.py: a regress.sh scenario on DuckStation, field by field, with timing marks.

    ds_frames.py run <fields> <trace.txt> <log>     (environment below)
    ds_frames.py masks <port-trace> <autodrive-log> <out> [lead]
    ds_frames.py timeline <log> [from to]

run plays the scenario on the patched regtest (tools/port/DUCKSTATION.md)
and writes, per field, the port's RR4_TRACE=1 RR4_TRACE_RAND=1 line with
regress.sh's RR4_WATCH globals (RaceState, PlayerCars+388, PlayerCars+1204,
ComCars+388, ComCars+1188: host offsets, read at the PS1 ones), so the
two traces diff directly. The port's first field starts at DuckStation's
vblank 296 (the BIOS boot; the game's first CD command lands 603 clocks
after it), so port vsync n's blank is DuckStation's vblank n + 296, and
line n is read at the pause just before that vblank (DuckStation frame
n + 295 ends there): line n and port vsync n are the same moment. Compare
with the port run headless with PSYZ_CARD_UNPLUG=2.

  DSF_PADS    the scenario's RR4_PAD_SCRIPT (mode anchors included); the
              controller sends a step's buttons DSF_LEAD (default 1) fields
              early, as tools/port/ds_script.py does
  DSF_POKES   its RR4_POKE (PlayerCars/ComCars/TrackLength names; host
              RaceVehicle offsets past the first car are mapped to the PS1's
              800-byte records)
  DSF_CARDPORT=1  append "cport <n>": the BIOS card driver's port byte
              (PS1 0x7264) as the line is read, the port it served at the
              blank before; the port's PSYZ_CARD_TRACE prints its own
  RR4_DS_CARD slot:none|blank|path@vsync,...: the card taken out or
              inserted at that field's blank, as ds_script.py does
  DSF_MASKS   pad 1 masks per port vsync ("vsync mask [mask2]", pad 2's
              in a two-pad Versus race), RR4_AUTODRIVE's
              input made with `masks` from the port's trace and its
              RR4_AUTODRIVE_LOG (lead 1, with one pad or two: lead 2 put
              the Versus scenario's second race a frame off at its first
              steering after 6680)
  DSF_MARKS   name[:ret][@mG/I],...: stubs at these functions' entries (and
              returns with :ret) write a marker to EXP2 0x1F802090, which
              the patched regtest logs as XMARK with its tick (no effect on
              emulation but the stub's ~15 clocks; unlike ds_mark.py's root
              counter 2 writes it does not move the sequencer). Unanchored
              marks are installed at DuckStation field DSF_MARK_AT (default
              600), @mG/I ones at each entry into that mode (overlay code).
              Names come from the decomp's sym.main.txt and overlay JSONs.
  DSF_EXC=1   also mark every exception at the vector (I_STAT, DICR and the
              cause) and its two returns (rfe), for interrupt dispatch times
  DSF_PEEK    addr:len,... (hex PS1 addresses): append those bytes in hex
              as ds_script.py's RR4_DS_PEEK does (800abef8:4 for the first
              player car's position; the port's RR4_WATCH=PlayerCars+0)
  DSF_PADLOG=1  append "pad <n>": whether libpad's vblank handler ran at
              the blank before the field (its flag, read and cleared between
              fields: no effect on the emulation)
  DSF_PAUSES=1  log "PAUSE <n> tick <t>" at each field's pause (the trace
              line n is read there)
  DSF_A0=1    also log each BIOS A0 call's number (rand 2f, srand 30)
  DSF_FUNCAT  at that port vsync, print the mode function's address

timeline prints the log's marks, CD commands and completions per field with
their clocks after the field's vblank (numbered as the port's vsyncs:
DuckStation's VBLANK lines say frame n + 296 for it) (EXC lines: interrupts; RET94/98:
exception returns; RET9c: DICR at the exception). Run `run` through
tools/port/run_capped.sh-compatible slots (duckstation.py takes one) with
RR4_DUCKSTATION and RR4_DISC set.
"""
import bisect, json, os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ds_calltime as dc
from duckstation import DuckStation

FIELD_OFFSET = 295
BUTTONS = {'select': 'Select', 'start': 'Start', 'up': 'Up', 'right': 'Right', 'down': 'Down',
           'left': 'Left', 'l2': 'L2', 'r2': 'R2', 'l1': 'L1', 'r1': 'R1', 'triangle': 'Triangle',
           'circle': 'Circle', 'cross': 'Cross', 'square': 'Square'}
MASKBITS = [(0x0100, 'Select'), (0x0800, 'Start'), (0x1000, 'Up'), (0x2000, 'Right'), (0x4000, 'Down'),
            (0x8000, 'Left'), (0x0001, 'L2'), (0x0002, 'R2'), (0x0004, 'L1'), (0x0008, 'R1'),
            (0x0010, 'Triangle'), (0x0020, 'Circle'), (0x0040, 'Cross'), (0x0080, 'Square')]
FRAME_COUNTER, RAND_STATE, MODE_FUNC_TABLE = 0x800ABEAC, 0x80009010, 0x8009E9F0
PAD_RAN = 0x800ABCD0  # libpad's flag that its vblank handler ran
HOST_CAR, PS1_CAR = 816, 800  # RaceVehicle
RACE_MODES = {'1/26', '1/38', '1/47', '1/53', '1/82'}  # RR4_AUTODRIVE's
SYM = dict(dc.SYMS)
OVL = {}
_ovl = os.path.join(os.environ.get('RR4_DECOMP', os.path.expanduser('~/Projects/ridge-racer-type-4-decomp')),
                    'configs/PAL/overlays')
for _f in (os.listdir(_ovl) if os.path.isdir(_ovl) else []):
    if _f.endswith('.json') and _f != 'names.json':
        for _fn in json.load(open(os.path.join(_ovl, _f))).get('functions', []):
            OVL[_fn['symbol']] = _fn['address']
CARD_PORT = 0x80007264  # the BIOS card driver's current port (DSF_CARDPORT)
WATCH = [('RaceState', 'RaceState', 0), ('PlayerCars+388', 'PlayerCars', 388),
         ('PlayerCars+1204', 'PlayerCars', 1204), ('ComCars+388', 'ComCars', 388),
         ('ComCars+1188', 'ComCars', 1188)]


def ps1_offset(off):
    return off // HOST_CAR * PS1_CAR + off % HOST_CAR


def anchor(at):
    if at.startswith('m'):
        g, rest = at[1:].split('/')
        i, d = rest.split(':')
        return (int(g) << 8) | (int(i) & 0xFF), int(d)
    return None, int(at)


def parse_pads(script):
    steps = []
    for item in filter(None, (s.strip() for s in script.split(','))):
        port = 1
        if item.startswith('2:'):
            port, item = 2, item[2:]
        name, at = item.split('@')
        period = count = 0
        if '%' in at:
            at, p = at.split('%')
            period = int(p)
        count = 4
        if '+' in at:
            at, c = at.split('+')
            count = int(c)
        mode, first = anchor(at)
        steps.append((port, BUTTONS[name.lower()], mode, first, count, period))
    return steps


def parse_pokes(script):
    out = []
    for item in filter(None, (x.strip() for x in script.split(','))):
        lhs, at = item.rsplit('@', 1)
        add = '+=' in lhs
        name, val = lhs.split('+=' if add else '=')
        size = 4
        if ':' in name:  # name[+offset]:size, as the port's RR4_POKE
            name, n = name.split(':')
            size = int(n)
        base, off = (name.split('+') + ['0'])[:2]
        mode, first = anchor(at)
        out.append((SYM[base] + ps1_offset(int(off, 0)), add, val, mode, first, size))
    return out


def s32(b):
    return struct.unpack('<i', b)[0]


def I(op, rs, rt, imm):
    return (op << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFF)


K0, K1 = 26, 27


def install_exception_marks(d):
    """XMARK (I_STAT << 16) | 0x8000 | (Cause & 0x7C) at the vector, the DICR
    at offset 9c before it, and the return address at the two rfe sites."""
    x = 0x8000D000
    code = [I(0x0F, 0, K0, 0x1F80), I(0x23, K0, K1, 0x10F4), 0, I(0x2B, K0, K1, 0x209C),
            I(0x23, K0, K1, 0x1070), 0, I(0x0C, K1, K1, 0x7FF),
            (K1 << 16) | (K1 << 11) | (16 << 6),          # sll k1, k1, 16
            0x40000000 | (K0 << 16) | (13 << 11), 0,      # mfc0 k0, Cause
            I(0x0C, K0, K0, 0x7C), (K1 << 21) | (K0 << 16) | (K1 << 11) | 0x25,  # or k1, k1, k0
            I(0x0D, K1, K1, 0x8000), I(0x0F, 0, K0, 0x1F80), I(0x2B, K0, K1, 0x2090),
            I(0x0F, 0, K0, 0x8000), I(0x09, K0, K0, 0x0C80), (K0 << 21) | 8, 0]
    if d.mem(0x80000080, 16) != dc.words([0x3C1A0000, 0x275A0C80, 0x03400008, 0]):
        raise SystemExit('unexpected exception vector')
    d.poke(x, dc.words(code))
    d.poke(0x80000080, dc.words([dc.j(x), 0]))
    for k, (site, off) in enumerate(((0x800065CC, 0x2094), (0x80001010, 0x2098))):
        if d.mem(site, 8) != dc.words([(K0 << 21) | 8, 0x42000010]):
            raise SystemExit('unexpected exception return at %08x' % site)
        r = x + 0x80 + k * 0x20
        d.poke(r, dc.words([I(0x0F, 0, K1, 0x1F80), I(0x2B, K1, K0, off), (K0 << 21) | 8, 0x42000010]))
        d.poke(site, dc.words([dc.j(r), 0]))


def install_a0_marks(d):
    """XMARK at offset 9a with the function number of each BIOS A0 call
    (rand 0x2F, srand 0x30, ...)."""
    stub = 0x8000D200
    if d.mem(0x800000A0, 16) != dc.words([0x3C080000, 0x250805C4, 0x01000008, 0]):
        raise SystemExit('unexpected A0 vector')
    t1, t8 = 9, 24
    d.poke(stub, dc.words([I(0x0F, 0, t8, 0x1F80), I(0x28, t8, t1, 0x209A),
                           0x3C080000, 0x250805C4, dc.j(0x800000A8), 0]))
    d.poke(0x800000A0, dc.words([dc.j(stub), 0]))


def install_mark(d, k, spec):
    name, *flags = spec.split(':')
    fn = int(name, 16) if name.startswith('0x') else (SYM.get(name) or OVL[name])
    stub = 0x8000C000 + k * 0x60
    moved = list(struct.unpack('<2I', d.mem(fn, 8)))
    if moved[0] == dc.j(stub):
        return
    for w in moved:
        if dc.is_branch(w) or dc.uses(w, 't8') or dc.uses(w, 't9'):
            raise SystemExit('%s: its first instructions cannot be moved (%08x)' % (name, w))
    code = [dc.lui('t8', 0x1F80), dc.ori('t9', 'zero', 0x100 + k), dc.sw('t9', 0x2090, 't8')]
    if 'ret' in flags:  # not nest-safe
        slot = 0x8000D300 + 4 * k
        back = stub + 4 * 11
        code += [dc.lui('t8', dc.hi(slot)), dc.sw('ra', dc.lo(slot), 't8'),
                 dc.lui('ra', dc.hi(back)), dc.addiu('ra', 'ra', dc.lo(back))]
        code += moved + [dc.j(fn + 8), dc.NOP]
        code += [dc.lui('t8', 0x1F80), dc.ori('t9', 'zero', 0x200 + k), dc.sw('t9', 0x2090, 't8'),
                 dc.lui('t8', dc.hi(slot)), dc.lw('ra', dc.lo(slot), 't8'), dc.NOP, dc.jr('ra'), dc.NOP]
    else:
        code += moved + [dc.j(fn + 8), dc.NOP]
    d.poke(stub, dc.words(code))
    d.poke(fn, dc.words([dc.j(stub), dc.NOP]))


def run(fields, trace_path, log_path):
    os.environ['DUCKSTATION_CD_TRACE'] = '1'
    steps = parse_pads(os.environ.get('DSF_PADS', ''))
    pokes = parse_pokes(os.environ.get('DSF_POKES', ''))
    masks, masks2 = {}, {}
    if os.environ.get('DSF_MASKS'):
        for line in open(os.environ['DSF_MASKS']):
            v, m, *m2 = line.split()
            masks[int(v)] = int(m)
            if m2:
                masks2[int(v)] = int(m2[0])
    marks = [m for m in os.environ.get('DSF_MARKS', '').split(',') if m]
    # the stubs (0x60 bytes each from 0x8000C000) reach 0x8000D000 past 42,
    # and the game hangs where the marks go in (48 did, 39 did not)
    if len(marks) > 42:
        raise SystemExit('DSF_MARKS: %d marks, at most 42 fit' % len(marks))
    mark_at = int(os.environ.get('DSF_MARK_AT', '600'))
    lead = int(os.environ.get('DSF_LEAD', '1'))
    func_at = int(os.environ['DSF_FUNCAT']) if os.environ.get('DSF_FUNCAT') else None
    pad2 = any(s[0] == 2 for s in steps)
    padlog = os.environ.get('DSF_PADLOG') == '1'
    cardport = os.environ.get('DSF_CARDPORT') == '1'
    peeks = [(int(a, 16), int(n)) for a, n in
             (p.split(':') for p in os.environ.get('DSF_PEEK', '').split(',') if p)]
    cards = []  # RR4_DS_CARD as in ds_script.py: the card verb at port vsync v's blank
    for item in filter(None, os.environ.get('RR4_DS_CARD', '').split(',')):
        where, at = item.rsplit('@', 1)
        slot, what = where.split(':', 1)
        cards.append((int(at), int(slot), what))
    anchor_mode, anchor_entry = -1, 0
    held = {1: set(), 2: set()}
    with open(log_path, 'w') as lf, open(trace_path, 'w') as out, DuckStation(log=lf, pad2=pad2) as d:
        for k, m in enumerate(marks):
            name = m.split('@')[0].split(':')[0]
            lf.write('MARK %d %s\nMARK %d %s.ret\n' % (0x100 + k, name, 0x200 + k, name))
        lf.flush()
        for frame in range(fields + FIELD_OFFSET + 1):
            v = frame - FIELD_OFFSET
            group, index, sub = d.mode()
            mode = (group << 8) | (index & 0xFF)
            entered = mode != anchor_mode
            if entered:
                anchor_mode, anchor_entry = mode, v
            if frame == mark_at:
                if os.environ.get('DSF_EXC') == '1':
                    install_exception_marks(d)
                if os.environ.get('DSF_A0') == '1':
                    install_a0_marks(d)
                for k, m in enumerate(marks):
                    if '@' not in m:
                        install_mark(d, k, m)
            if entered:
                for k, m in enumerate(marks):
                    if '@' in m:
                        spec, at = m.rsplit('@', 1)
                        g, i = at[1:].split('/')
                        if mode == ((int(g) << 8) | int(i)):
                            install_mark(d, k, spec)
            for addr, add, val, pmode, first, size in pokes:
                t = v if pmode is None else (v - anchor_entry if pmode == anchor_mode else -1)
                if t != first:
                    continue
                x = s32(d.mem(SYM[val], 4)) if val in SYM else int(val, 0)
                if add:
                    x += int.from_bytes(d.mem(addr, size), 'little', signed=True)
                d.poke(addr, (x & ((1 << (8 * size)) - 1)).to_bytes(size, 'little'))
            for at, slot, what in cards:
                if at == v:
                    d.cmd('card %d %s' % (slot, what))
            if v == func_at:
                names = {a: n for n, a in SYM.items()}
                names.update({a: n for n, a in OVL.items()})
                table = d.u32(MODE_FUNC_TABLE + 4 * group)
                fn = d.u32(table + 4 * index)
                sys.stderr.write('MODEFUNC %d/%d %08x %s\n' % (group, index, fn, names.get(fn)))
            if os.environ.get('DSF_PAUSES') == '1':
                lf.write('PAUSE %d tick %d\n' % (v, d.ticks()[0]))
            if v >= 1:
                line = 'vsync %d frame %u mode %u/%d sub %u' % (v, d.u32(FRAME_COUNTER), group, index, sub)
                for label, base, off in WATCH:
                    line += ' %s=%d' % (label, s32(d.mem(SYM[base] + ps1_offset(off), 4)))
                line += ' rand=%08x' % d.u32(RAND_STATE)
                if padlog:
                    line += ' pad %d' % d.u32(PAD_RAN)
                if cardport:
                    line += ' cport %d' % d.mem(CARD_PORT, 1)[0]
                for a, n in peeks:
                    line += ' %x=%s' % (a, d.mem(a, n).hex())
                out.write(line + '\n')
            if padlog:
                d.poke(PAD_RAN, b'\0\0\0\0')
            want = {1: set(), 2: set()}
            target = v + lead
            for port, button, smode, first, count, period in steps:
                t = (target if smode is None else
                     target - anchor_entry if smode == anchor_mode else -1) - first
                if t < 0:
                    continue
                if period:
                    t %= period
                if t < count:
                    want[port].add(button)
            for bit, button in MASKBITS:
                if masks.get(target, 0) & bit:
                    want[1].add(button)
                if masks2.get(target, 0) & bit:
                    want[2].add(button)
            for port in (1, 2):
                if want[port] != held[port]:
                    d.release(port)
                    if want[port]:
                        d.buttons(port=port, **{b: 1 for b in want[port]})
                    held[port] = want[port]
            d.advance(1)


def make_masks(trace_path, log_path, out_path, lead):
    """RR4_AUTODRIVE_LOG lines are written at each blank the driver runs
    (race mode, RaceState 2..3), with the state the trace line of that
    vsync shows; the controller sends the mask at that blank. Pad 2's
    lines ("ModeFrame mask 2", a Versus race with two pads) come after pad
    1's of the same blank and give a third column, pad 2's mask."""
    lines = [line.split() for line in open(log_path)]
    drive = [int(f[1]) for f in lines if len(f) == 2]
    drive2 = [int(f[1]) for f in lines if len(f) == 3]
    k = 0
    with open(out_path, 'w') as o:
        for line in open(trace_path):
            f = line.split()
            if not f or f[0] != 'vsync':
                continue
            state = dict(x.split('=', 1) for x in f if '=' in x)
            if f[5] in RACE_MODES and 2 <= int(state.get('RaceState', 0)) <= 3:
                if k < len(drive):
                    if drive2:
                        o.write('%d %d %d\n' % (int(f[1]) + lead, drive[k], drive2[k] if k < len(drive2) else 0))
                    else:
                        o.write('%d %d\n' % (int(f[1]) + lead, drive[k]))
                k += 1


def timeline(log_path, first=0, last=1 << 30):
    names, blanks, rows = {}, [], []
    for line in open(log_path):
        f = line.split()
        if not f:
            continue
        if f[0] == 'MARK':
            names[int(f[1])] = f[2]
        elif f[0] == 'VBLANK':
            blanks.append((int(f[2]), int(f[4])))
        elif f[0] in ('CDCMD', 'CDDONE'):
            rows.append((int(f[3]), f[0] + ' ' + f[1]))
        elif f[0] == 'XMARK':
            x, t = int(f[1], 16), int(f[3])
            if len(f) > 7 and f[7] == '9a':
                rows.append((t, 'A0:%02x' % x))
            elif len(f) > 7 and f[7] != '90':
                rows.append((t, 'RET' + f[7]))
            elif x & 0x8000:
                if (x & 0x7C) == 0:
                    rows.append((t, 'EXC istat=%03x' % (x >> 16)))
            else:
                rows.append((t, names.get(x, f[1])))
    rows.sort()
    ticks = [t for t, _ in blanks]
    cur = None
    for t, name in rows:
        i = bisect.bisect_right(ticks, t) - 1
        if i < 0:
            continue
        v = blanks[i][1] - FIELD_OFFSET - 1  # the vblank of port vsync v
        if not first <= v <= last:
            continue
        if v != cur:
            print('--- blank v%d' % v)
            cur = v
        print('  %7d  %s' % (t - ticks[i], name))


if __name__ == '__main__':
    cmd = sys.argv[1] if len(sys.argv) > 1 else ''
    if cmd == 'run':
        run(int(sys.argv[2]), sys.argv[3], sys.argv[4])
    elif cmd == 'masks':
        make_masks(sys.argv[2], sys.argv[3], sys.argv[4], int(sys.argv[5]) if len(sys.argv) > 5 else 0)
    elif cmd == 'timeline':
        timeline(sys.argv[2], *map(int, sys.argv[3:5]))
    else:
        sys.exit(__doc__)
