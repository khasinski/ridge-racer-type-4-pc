"""Reference runs on DuckStation for behaviour comparison with the port.

Drives our modified duckstation-regtest (AI control port + GDB stub):
boot the CUE paused, advance frames, press pad 1 buttons, take screenshots
and read PS1 RAM (mode(): ModeGroup/ModeIndex/SubmodeState).

    from duckstation import DuckStation
    d = DuckStation(); d.advance(400); d.press('Start'); print(d.mode())

Pad 2 (VS battle): DuckStation(pad2=True) connects a digital pad in port 2
(REGTEST_PAD2=1); drive it with press(name, port=2) / buttons(port=2, ...).
poke(addr, bytes) writes PS1 RAM through the GDB 'M' packet.

Other controllers: DuckStation(pad1_type='NeGcon') (or 'AnalogController',
'JogCon'; REGTEST_PAD1_TYPE / REGTEST_PAD2_TYPE) and bind({index:
value}, port=1) with that controller's bind indices (its buttons, then its half
axes; e.g. NeGcon 9 = steering right, 10 = I), as RR4_PAD_TYPE /
RR4_PAD_SCRIPT axis steps in the port.

Environment (see tools/port/DUCKSTATION.md for building the emulator):
  RR4_DUCKSTATION  the patched duckstation-regtest binary (default: one on PATH)
  RR4_DISC         the CUE of the PAL disc (required)
  RR4_DS_PORTS     "ai,gdb" port pair (default: a free pair in 47300-47999;
                   ai=/gdb= arguments override it)
  RR4_DS_ARGS      extra regtest arguments (e.g. -slowboot)
The REGTEST_* variables of DUCKSTATION.md pass through to the emulator.
Use it as a context manager (or call close()) so the emulator always exits.
"""
import socket, subprocess, time, os, struct, sys, shutil, shlex


def _binary():
    ds = os.environ.get('RR4_DUCKSTATION') or shutil.which('duckstation-regtest')
    if not ds:
        raise RuntimeError('set RR4_DUCKSTATION to the patched duckstation-regtest (tools/port/DUCKSTATION.md)')
    return ds


def _disc():
    cue = os.environ.get('RR4_DISC')
    if not cue:
        raise RuntimeError('set RR4_DISC to the CUE of the PAL disc')
    return cue


def _port_free(port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.bind(('127.0.0.1', port))
            return True
        except OSError:
            return False


def _ports(ai, gdb):
    if ai is None and gdb is None and os.environ.get('RR4_DS_PORTS'):
        ai, gdb = (int(x) for x in os.environ['RR4_DS_PORTS'].split(','))
    if ai is not None and gdb is not None:
        return ai, gdb
    if ai is not None or gdb is not None:
        raise ValueError('give both ports (ai= and gdb=) or neither')
    # another project uses 51735/51736; this range is free for RR4 runs
    start = 47300 + 2 * ((os.getpid() * 7) % 350)
    for i in range(350):
        a = 47300 + (start - 47300 + 2 * i) % 700
        if _port_free(a) and _port_free(a + 1):
            return a, a + 1
    raise RuntimeError('no free port pair in 47300-47999')


class DuckStation:
    def __init__(self, ai=None, gdb=None, log=None, pad2=False, pad1_type=None, pad2_type=None):
        ds, cue = _binary(), _disc()
        ai, gdb = _ports(ai, gdb)
        self.ports = (ai, gdb)
        self.ai = self.g = self.aif = None
        env = dict(os.environ, SDL_AUDIODRIVER='dummy')
        if pad2:
            env['REGTEST_PAD2'] = '1'
        if pad1_type:
            env['REGTEST_PAD1_TYPE'] = pad1_type
        if pad2_type:
            env['REGTEST_PAD2_TYPE'] = pad2_type
        extra = shlex.split(os.environ.get('RR4_DS_ARGS', ''))
        self.p = subprocess.Popen([ds, '-ai-port', str(ai), '-gdb', str(gdb), '-start-paused', '-frames', '10000000',
                                   *extra, '--', cue], stdout=log or subprocess.DEVNULL,
                                  stderr=subprocess.STDOUT, env=env, cwd=os.path.dirname(os.path.abspath(ds)))
        try:
            self.ai = self._connect(ai, 200)
            self.ai.settimeout(30)
            self.aif = self.ai.makefile('rb')
            self.greeting = self.aif.readline().decode().strip()
            if not self.greeting.startswith('OK'):
                raise RuntimeError('unexpected greeting %r on port %d' % (self.greeting, ai))
            self.ai.settimeout(None)
            self.g = self._connect(gdb, 100)
            self.g.settimeout(10)
            for s in (self.ai, self.g):
                s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        except BaseException:
            self.close()
            raise

    def _connect(self, port, tries):
        for _ in range(tries):
            if self.p.poll() is not None:
                raise RuntimeError('duckstation-regtest exited with %d before port %d opened' % (self.p.returncode, port))
            try:
                return socket.create_connection(('127.0.0.1', port))
            except OSError:
                time.sleep(0.1)
        raise RuntimeError('cannot connect to duckstation-regtest on port %d' % port)

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def cmd(self, line):
        # While the emulator is paused, regtest blocks up to 100 ms in its GDB
        # poll before it services the control port: a GDB read right after the
        # command wakes it (about 20 times faster per command).
        self.ai.sendall((line + '\n').encode())
        self._gsend('m80000000,4')
        return self.aif.readline().decode().strip()

    def advance(self, n):
        r = self.cmd(f'advance {n}')
        while True:
            st = self.cmd('status')
            if 'advance_remaining=0' in st:
                return r
            time.sleep(0.01)

    def buttons(self, port=1, **kw):
        verb = 'buttons2' if port == 2 else 'buttons'
        return self.cmd(verb + ' ' + ' '.join(f'{k}={v}' for k, v in kw.items()))

    def bind(self, values, port=1):
        verb = 'bind2' if port == 2 else 'bind'
        return self.cmd(verb + ' ' + ' '.join(f'{k}={v}' for k, v in sorted(values.items())))

    def release(self, port=1):
        return self.cmd('release2' if port == 2 else 'release')

    def press(self, name, frames=4, port=1):
        self.buttons(port=port, **{name: 1})
        self.advance(frames)
        self.release(port)

    def screenshot(self, path):
        """Write the displayed frame as a PPM. DuckStation's regtest reads its
        RGB5A1 display texture back and expands each 5-bit channel c to
        (c << 3) | (c & 7) (9 for c=1), while the port presents and captures
        round(c * 255 / 31) (8 for c=1), the full-range PS1 DAC level that
        DuckStation's own display shaders also show. Compare frames from the
        two at 5 bits (value >> 3 recovers c from either), or compare VRAM:
        `vram <path>` here, RR4_GPU_DUMP_AT in the port (GPUDUMP replay)."""
        self.ai.sendall(b'screenshot\n')
        self._gsend('m80000000,4')
        hdr = self.aif.readline().decode().split()
        if hdr[0] != 'SCREENSHOT':
            raise RuntimeError(hdr)
        w, h, pitch, size = map(int, hdr[1:5])
        data = self.aif.read(size)
        with open(path, 'wb') as f:
            f.write(b'P6\n%d %d\n255\n' % (w, h))
            for y in range(h):
                row = data[y * pitch:y * pitch + w * 4]
                f.write(bytes(b for i in range(0, len(row), 4) for b in row[i:i + 3]))

    # --- GDB remote protocol
    def _gsend(self, payload):
        cs = sum(payload.encode()) & 0xFF
        self.g.sendall(f'${payload}#{cs:02x}'.encode())
        while True:
            r = self._gread()
            if not (len(r) == 3 and r[0] in 'ST') and not r.startswith('T'):
                return r

    def _gread(self):
        buf = b''
        while True:
            ch = self.g.recv(1)
            if ch in (b'+',):
                continue
            if ch == b'$':
                break
        while True:
            ch = self.g.recv(1)
            if ch == b'#':
                self.g.recv(2)
                break
            buf += ch
        self.g.sendall(b'+')
        return buf.decode()

    def mem(self, addr, n):
        out = b''
        while n:
            k = min(n, 0x400)
            r = self._gsend(f'm{addr:x},{k:x}')
            try:
                out += bytes.fromhex(r)
            except ValueError:
                raise RuntimeError('gdb reply %r' % r[:80])
            addr += k
            n -= k
        return out

    def poke(self, addr, data):
        r = self._gsend(f'M{addr:x},{len(data):x}:{bytes(data).hex()}')
        if r != 'OK':
            raise RuntimeError('gdb reply %r' % r[:80])

    def u16(self, a):
        return struct.unpack('<H', self.mem(a, 2))[0]

    def s16(self, a):
        return struct.unpack('<h', self.mem(a, 2))[0]

    def u32(self, a):
        return struct.unpack('<I', self.mem(a, 4))[0]

    def mode(self):
        return (self.u16(0x800F4C6A), self.s16(0x800F3A26), self.u16(0x800F4D8A))

    def audiodump(self, path=None):
        """Append the mixed SPU output (raw 44100 Hz s16 stereo) to path from
        now on; without a path, close the dump. Set DUCKSTATION_SPU_REG_TRACE=1
        (or DUCKSTATION_SPU_KEY_TRACE=1) in the environment before creating
        the instance, with log= a file, to log SPU register writes; compare
        with the port's PSYZ_SPU_REG_TRACE / RR4_AUDIO_CAPTURE."""
        return self.cmd('audiodump ' + path if path else 'audiodump')

    def ticks(self):
        """(system ticks, frame number) of the emulated clock (33.8688 MHz)."""
        r = self.cmd('ticks').split()
        return int(r[1].split('=')[1]), int(r[2].split('=')[1])

    def close(self):
        for s in (self.aif, self.ai, self.g):
            try:
                if s:
                    s.close()
            except OSError:
                pass
        self.aif = self.ai = self.g = None
        if self.p.poll() is None:
            self.p.terminate()  # regtest shuts down on SIGTERM
            try:
                self.p.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.p.kill()
                self.p.wait()
