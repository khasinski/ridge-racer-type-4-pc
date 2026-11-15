"""ds_script.py frames 'pad script' [out]: replay an RR4_PAD_SCRIPT on
DuckStation and print the port's per-field trace format.

The script is the port's RR4_PAD_SCRIPT syntax (src/port/diagnostics.c):
a comma list of [2:]button@vsync[+frames][%period] or
[2:]button@mG/I:d[+frames][%period] (d VSyncs after each entry into
ModeGroup G / ModeIndex I). Analog steps are not supported.

Every field the emulator pauses at its blank; this reads the mode state as
the port's VSync hook does at that blank, then sets the buttons of the
script's next field before the exchange of this blank: the port's script
names the field whose receive buffer shows the press with one pad
(libpad stores port 1's bytes when it starts the next exchange), so the
controller holds it one exchange earlier. A pad in port 2 is connected when
a step names it (REGTEST_PAD2=1).

Output per field, as the port's RR4_TRACE=1 RR4_TRACE_RAND=1 lines:
  vsync <n> frame <FrameCounter> mode <g>/<i> sub <s> rand=<state>
  [pad <exchanges>]
where n = DuckStation frame - RR4_DS_FIELD_OFFSET (default 225, the boot
fields the port does not run), and with RR4_DS_PADLOG=1 also whether
libpad's vblank handler ran at the blank before it (the flag _padIntPad
sets). RR4_DS_PEEK=addr:len[,...] (hex PS1 addresses) appends those bytes
in hex, e.g. 8010c1a6:1 for the JogCon byte of pad 1's receive buffer
(the port's RR4_WATCH=PadPorts+6:1). REGTEST_PAD1_TYPE=JogCon (and the
other REGTEST_* variables of DUCKSTATION.md) select the controllers.
Absolute steps count port VSyncs (DuckStation frame - the offset).
Run it through run_capped.sh with RR4_DUCKSTATION and RR4_DISC set.
"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from duckstation import DuckStation

BUTTONS = {'select': 'Select', 'start': 'Start', 'up': 'Up', 'right': 'Right', 'down': 'Down',
           'left': 'Left', 'l2': 'L2', 'r2': 'R2', 'l1': 'L1', 'r1': 'R1', 'triangle': 'Triangle',
           'circle': 'Circle', 'cross': 'Cross', 'square': 'Square'}
FRAME_COUNTER, RAND_STATE, PAD_RAN = 0x800ABEAC, 0x80009010, 0x800ABCD0


def parse(script):
    steps = []
    for item in filter(None, (s.strip() for s in script.split(','))):
        port = 1
        if item.startswith('2:'):
            port, item = 2, item[2:]
        name, at = item.split('@')
        period = 0
        if '%' in at:
            at, p = at.split('%')
            period = int(p)
        count = 4
        if '+' in at:
            at, c = at.split('+')
            count = int(c)
        mode = None
        if at.startswith('m'):
            g, rest = at[1:].split('/')
            i, d = rest.split(':')
            mode, first = (int(g) << 8) | (int(i) & 0xFF), int(d)
        else:
            first = int(at)
        steps.append((port, BUTTONS[name.lower()], mode, first, count, period))
    return steps


def main():
    frames, script = int(sys.argv[1]), sys.argv[2]
    out = open(sys.argv[3], 'w') if len(sys.argv) > 3 else sys.stdout
    offset = int(os.environ.get('RR4_DS_FIELD_OFFSET', '225'))
    lead = int(os.environ.get('RR4_DS_LEAD', '1'))
    padlog = os.environ.get('RR4_DS_PADLOG') == '1'
    peeks = [(int(a, 16), int(n)) for a, n in
             (p.split(':') for p in os.environ.get('RR4_DS_PEEK', '').split(',') if p)]
    steps = parse(script)
    pad2 = any(s[0] == 2 for s in steps)
    anchor_mode, anchor_entry = -1, 0
    held = {1: set(), 2: set()}
    with DuckStation(pad2=pad2) as d:
        for frame in range(frames + offset + 1):
            v = frame - offset
            group, index, sub = d.mode()
            mode = (group << 8) | (index & 0xFF)
            if mode != anchor_mode:
                anchor_mode, anchor_entry = mode, v
            if v >= 1:
                line = 'vsync %d frame %u mode %u/%d sub %u rand=%08x' % (
                    v, d.u32(FRAME_COUNTER), group, index, sub, d.u32(RAND_STATE))
                if padlog:
                    line += ' pad %d' % d.u32(PAD_RAN)
                for a, n in peeks:
                    line += ' %x=%s' % (a, d.mem(a, n).hex())
                out.write(line + '\n')
            if padlog:
                d.poke(PAD_RAN, b'\0\0\0\0')
            # the buttons the controller sends at this blank's exchange
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
            for port in (1, 2):
                if want[port] != held[port]:
                    d.release(port)
                    if want[port]:
                        d.buttons(port=port, **{b: 1 for b in want[port]})
                    held[port] = want[port]
            d.advance(1)
    out.close()


if __name__ == '__main__':
    main()
