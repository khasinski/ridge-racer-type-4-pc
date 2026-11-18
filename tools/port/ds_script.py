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
controller holds it one exchange earlier. DuckStation pauses after libpad
has sent a blank's first command bytes: a DUAL SHOCK or JogCon reads its
buttons at the command byte (a digital pad and a neGcon only at the
buttons' byte), so for those the buttons are set one blank earlier still.
RR4_DS_LEAD overrides the lead (1, or 2 for those controllers) for both
ports. A pad in port 2 is connected when
a step names it (REGTEST_PAD2=1).

Output per field, as the port's RR4_TRACE=1 RR4_TRACE_RAND=1 lines:
  vsync <n> frame <FrameCounter> mode <g>/<i> sub <s> rand=<state>
  [pad <exchanges>]
where n = DuckStation frame - 295, the BIOS boot the port does not run
(field_offset(); RR4_DS_FIELD_OFFSET overrides it). Run the port with the
same cards: PSYZ_CARD_UNPLUG=2 for DuckStation's default (a blank card in
slot 1, slot 2 empty). With RR4_DS_PADLOG=1 also whether libpad's vblank handler ran at the blank before it (the flag _padIntPad
sets). RR4_DS_PEEK=addr:len[,...] (hex PS1 addresses) appends those bytes
in hex, e.g. 8010c1a6:1 for the JogCon byte of pad 1's receive buffer
(the port's RR4_WATCH=PadPorts+6:1). REGTEST_PAD1_TYPE=JogCon (and the
other REGTEST_* variables of DUCKSTATION.md) select the controllers.
Absolute steps count port VSyncs (DuckStation frame - the offset).
DUMMYBP=1 sets a never-hit breakpoint, which runs the emulator's
interpreter with its memory checks: with DS_LOWRAM_LOG=<file> the patched
regtest then logs game code touching PS1 RAM 0..0xFFFF (null-pointer
accesses; ds_lowram_report.py <file> groups them by function).
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


def field_offset():
    """DuckStation frames before the port's first VSync: the BIOS boot. The
    game's first CD command comes just after DuckStation's vblank 296 with
    or without cards, and line n read at the pause after frame n + 295 is
    the moment of port vsync n (ds_frames.py). RR4_DS_FIELD_OFFSET
    overrides it."""
    return int(os.environ.get('RR4_DS_FIELD_OFFSET') or 295)


class Driver:
    """Plays a pad script on d one field at a time: step() returns the
    port's field number v of the blank the emulator is paused at (and the
    mode read there), and advance() sets the buttons for the next exchange
    and runs one field."""

    def __init__(self, d, script, offset=None):
        late = ('AnalogController', 'JogCon')
        self.d, self.steps = d, parse(script)
        self.offset = field_offset() if offset is None else offset
        self.lead = {p: int(os.environ.get('RR4_DS_LEAD') or
                            (2 if os.environ.get('REGTEST_PAD%d_TYPE' % p) in late else 1))
                     for p in (1, 2)}
        self.anchor_mode, self.anchor_entry = -1, 0
        self.held = {1: set(), 2: set()}
        self.frame = 0

    def step(self):
        v = self.frame - self.offset
        group, index, sub = self.d.mode()
        mode = (group << 8) | (index & 0xFF)
        if mode != self.anchor_mode:
            self.anchor_mode, self.anchor_entry = mode, v
        return v, group, index, sub

    def advance(self, v):
        # the buttons the controller sends at this blank's exchange
        want = {1: set(), 2: set()}
        target = {p: v + self.lead[p] for p in (1, 2)}
        for port, button, smode, first, count, period in self.steps:
            t = (target[port] if smode is None else
                 target[port] - self.anchor_entry if smode == self.anchor_mode else -1) - first
            if t < 0:
                continue
            if period:
                t %= period
            if t < count:
                want[port].add(button)
        for port in (1, 2):
            if want[port] != self.held[port]:
                self.d.release(port)
                if want[port]:
                    self.d.buttons(port=port, **{b: 1 for b in want[port]})
                self.held[port] = want[port]
        self.d.advance(1)
        self.frame += 1


def main():
    frames, script = int(sys.argv[1]), sys.argv[2]
    out = open(sys.argv[3], 'w') if len(sys.argv) > 3 else sys.stdout
    padlog = os.environ.get('RR4_DS_PADLOG') == '1'
    peeks = [(int(a, 16), int(n)) for a, n in
             (p.split(':') for p in os.environ.get('RR4_DS_PEEK', '').split(',') if p)]
    pad2 = any(s[0] == 2 for s in parse(script))
    with DuckStation(pad2=pad2) as d:
        if os.environ.get('DUMMYBP'):
            d._gsend('Z0,801ffff0,4')
        drv = Driver(d, script)
        for _ in range(frames + drv.offset + 1):
            v, group, index, sub = drv.step()
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
            drv.advance(v)
    out.close()


if __name__ == '__main__':
    main()
