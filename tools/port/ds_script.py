"""ds_script.py frames 'pad script' [out]: replay an RR4_PAD_SCRIPT on
DuckStation and print the port's per-field trace format.

The script is the port's RR4_PAD_SCRIPT syntax (src/port/diagnostics.c):
a comma list of [2:]button@vsync[+frames][%period] or
[2:]button@mG/I:d[+frames][%period] (d VSyncs after each entry into
ModeGroup G / ModeIndex I). Analog steps lx/ly/rx/ry=value@... hold a
DUAL SHOCK stick (REGTEST_PAD1_TYPE=AnalogController) as the port's do.

Every field the emulator pauses at its blank; this reads the mode state as
the port's VSync hook does at that blank, then sets the buttons of the
script's next field before the exchange of this blank: the port's script
names the field whose receive buffer shows the press with one pad
(libpad stores port 1's bytes when it starts the next exchange), so the
controller holds it one exchange earlier. The pause comes before the
blank (ds_frames.py), so no exchange has started and every controller
takes the same lead of one field: a DUAL SHOCK in analog mode, which
reads its buttons at the command byte, matches the port's receive buffer,
PlayerControllers and the Time Attack menus field for field with it (a
lead of two, from when the pause was taken to come after libpad's first
command bytes, put its presses a field early). The JogCon was not
checked again. RR4_DS_LEAD overrides the lead for both ports. A pad in
port 2 is connected when a step names it (REGTEST_PAD2=1).

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
RR4_DS_POKE=addr:hexbytes@vsync[,...] stores bytes (hex PS1 address, data
in memory order) at that field's blank, like the port's RR4_POKE, e.g.
800f3974:01000000@1000 for SaveProgress+0x54 (the Extra Trial row).
RR4_DS_CARD=slot:none|blank|path@vsync[,...] takes out or inserts a memory
card at that field's blank (the regtest's card verb), like the port's
RR4_CARD_SLOT.
DUMMYBP=1 sets a never-hit breakpoint, which runs the emulator's
interpreter with its memory checks: with DS_LOWRAM_LOG=<file> the patched
regtest then logs game code touching PS1 RAM 0..0xFFFF (null-pointer
accesses; ds_lowram_report.py <file> groups them by function).
RR4_DS_LOG=<file> keeps the emulator's output (the patched regtest's
DUCKSTATION_*_TRACE lines), which a run does not change.
Run it through run_capped.sh with RR4_DUCKSTATION and RR4_DISC set.
"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from duckstation import DuckStation

BUTTONS = {'select': 'Select', 'start': 'Start', 'up': 'Up', 'right': 'Right', 'down': 'Down',
           'left': 'Left', 'l2': 'L2', 'r2': 'R2', 'l1': 'L1', 'r1': 'R1', 'triangle': 'Triangle',
           'circle': 'Circle', 'cross': 'Cross', 'square': 'Square'}
FRAME_COUNTER, RAND_STATE, PAD_RAN = 0x800ABEAC, 0x80009010, 0x800ABCD0
# The DUAL SHOCK's sticks (the port's lx ly rx ry, -1..1): DuckStation's
# AnalogController binds its half axes after its 17 buttons (LLeft, LRight,
# LDown, LUp, RLeft, RRight, RDown, RUp); a negative value is left / up.
STICK_BINDS = {'lx': (17, 18), 'ly': (20, 19), 'rx': (21, 22), 'ry': (24, 23)}
ANALOG_SENSITIVITY = 1.33  # AnalogController's default "AnalogSensitivity"


def stick_bind(axis, value):
    """The bind that makes DuckStation send the stick byte the port sends
    for value: the port maps -1..1 linearly to 0x00..0xFF (as an SDL axis,
    src/port/diagnostics.c), DuckStation scales a half axis by its analog
    sensitivity to 0..255 and merges the two halves around 0x80."""
    low, high = STICK_BINDS[axis.lower()]
    byte = (max(-32768, min(32767, int(value * 32767))) + 32768) >> 8
    if byte >= 128:
        index, half = high, 2 * (byte - 128)
    else:
        index, half = low, 2 * (128 - byte) - 1
    return index, (half + 0.5) / (ANALOG_SENSITIVITY * 255)


def parse(script):
    steps = []
    for item in filter(None, (s.strip() for s in script.split(','))):
        port = 1
        if item.startswith('2:'):
            port, item = 2, item[2:]
        name, at = item.split('@')
        if '=' in name:
            axis, value = name.split('=')
            button = stick_bind(axis, float(value))
        else:
            button = BUTTONS[name.lower()]
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
        steps.append((port, button, mode, first, count, period))
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
        self.d, self.steps = d, parse(script)
        self.offset = field_offset() if offset is None else offset
        self.lead = {p: int(os.environ.get('RR4_DS_LEAD') or 1)
                     for p in (1, 2)}
        self.anchor_mode, self.anchor_entry = -1, 0
        self.held = {1: set(), 2: set()}
        self.frame = 0
        # RR4_DS_POKE=addr:hexbytes@v,...: PS1 RAM stores at the blank of
        # port vsync v, as the port's RR4_POKE stores between frames there.
        # RR4_DS_CARD=slot:none|blank|path@v,...: the regtest's card verb at
        # the blank of port vsync v, as the port's RR4_CARD_SLOT.
        self.cards = []
        for item in filter(None, os.environ.get('RR4_DS_CARD', '').split(',')):
            where, at = item.rsplit('@', 1)
            slot, what = where.split(':', 1)
            self.cards.append((int(at), int(slot), what))
        self.pokes = []
        for item in filter(None, os.environ.get('RR4_DS_POKE', '').split(',')):
            where, at = item.split('@')
            addr, data = where.split(':')
            self.pokes.append((int(at), int(addr, 16), bytes.fromhex(data)))

    def step(self):
        v = self.frame - self.offset
        group, index, sub = self.d.mode()
        mode = (group << 8) | (index & 0xFF)
        if mode != self.anchor_mode:
            self.anchor_mode, self.anchor_entry = mode, v
        return v, group, index, sub

    def advance(self, v):
        for at, addr, data in self.pokes:
            if at == v:
                self.d.poke(addr, data)
        for at, slot, what in self.cards:
            if at == v:
                self.d.cmd('card %d %s' % (slot, what))
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
                pressed = [b for b in want[port] if isinstance(b, str)]
                axes = dict(b for b in want[port] if not isinstance(b, str))
                if pressed:
                    self.d.buttons(port=port, **{b: 1 for b in pressed})
                if axes:
                    self.d.bind(axes, port=port)
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
    log = open(os.environ['RR4_DS_LOG'], 'w') if os.environ.get('RR4_DS_LOG') else None
    with DuckStation(pad2=pad2, log=log) as d:
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
