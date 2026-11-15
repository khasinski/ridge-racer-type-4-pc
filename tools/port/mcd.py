#!/usr/bin/env python3
"""Convert between a raw 128 KiB PS1 memory card image (.mcd/.mcr, as used
by DuckStation and other emulators) and a PsyZ card directory (bu00/, bu10/:
one host file per card file, named as on the card).

    mcd.py list   card.mcd
    mcd.py export card.mcd bu00/     # every file on the card into bu00/
    mcd.py import bu00/ card.mcd     # build a formatted card from bu00/
"""
import os, sys

FRAME, BLOCK, BLOCKS = 128, 8192, 16


def xor(b):
    x = 0
    for c in b[:127]:
        x ^= c
    return x


def frame(b):
    b = bytearray(b.ljust(127, b'\0'))
    b.append(xor(b))
    return bytes(b)


def read_card(path):
    with open(path, 'rb') as source:
        data = source.read(BLOCK * BLOCKS + 1)
    if len(data) != BLOCK * BLOCKS or data[:2] != b'MC':
        raise SystemExit('%s: not a raw 128 KiB memory card image' % path)
    files = []
    for i in range(1, BLOCKS):
        d = data[i * FRAME:(i + 1) * FRAME]
        state = int.from_bytes(d[0:4], 'little')
        if state != 0x51:  # first block of a file in use
            continue
        size = int.from_bytes(d[4:8], 'little')
        try:
            name = d[10:30].split(b'\0')[0].decode('ascii')
        except UnicodeDecodeError:
            raise SystemExit('%s: invalid filename in block %d' % (path, i))
        # Preserve genuine card names exactly; reject paths instead of silently
        # renaming saves or allowing export outside its destination directory.
        if (not name or name in ('.', '..') or
                any(c in name for c in '/\\:') or
                any(ord(c) < 32 or ord(c) == 127 for c in name)):
            raise SystemExit('%s: unsafe filename in block %d' % (path, i))
        chunks, blk, seen = [], i, set()
        while True:
            if blk < 1 or blk >= BLOCKS or blk in seen:
                raise SystemExit('%s: invalid block chain for %s' % (path, name))
            seen.add(blk)
            chunks.append(data[blk * BLOCK:(blk + 1) * BLOCK])
            nxt = int.from_bytes(data[blk * FRAME + 8:blk * FRAME + 10], 'little')
            if nxt == 0xFFFF:
                break
            blk = nxt + 1
        if size > len(chunks) * BLOCK or size == 0:
            raise SystemExit('%s: invalid size for %s' % (path, name))
        if any(existing == name for existing, _ in files):
            raise SystemExit('%s: duplicate filename %s' % (path, name))
        files.append((name, b''.join(chunks)[:size]))
    return files


def write_card(path, files):
    card = bytearray(b'\xff' * (BLOCK * BLOCKS))  # erased flash, as formatted
    card[0:BLOCK] = bytes(BLOCK)
    card[0:FRAME] = frame(b'MC')
    card[63 * FRAME:64 * FRAME] = frame(b'MC')  # write-test frame
    free = list(range(1, BLOCKS))
    for i in range(1, BLOCKS):
        card[i * FRAME:(i + 1) * FRAME] = frame(b'\xa0\0\0\0\0\0\0\0\xff\xff')
    for i in range(16, 36):  # broken sector list: none
        card[i * FRAME:(i + 1) * FRAME] = frame(b'\xff\xff\xff\xff\0\0\0\0\xff\xff')
    for name, body in files:
        n = (len(body) + BLOCK - 1) // BLOCK
        if n == 0 or n > len(free):
            raise SystemExit('%s does not fit' % name)
        blks, free = free[:n], free[n:]
        for k, b in enumerate(blks):
            state = 0x51 if k == 0 else (0x53 if k == n - 1 else 0x52)
            nxt = (blks[k + 1] - 1) if k + 1 < n else 0xFFFF
            d = state.to_bytes(4, 'little')
            d += (n * BLOCK if k == 0 else 0).to_bytes(4, 'little')
            d += nxt.to_bytes(2, 'little')
            d += name.encode('ascii')[:20] if k == 0 else b''
            card[b * FRAME:(b + 1) * FRAME] = frame(d)
            card[b * BLOCK:(b + 1) * BLOCK] = body[k * BLOCK:(k + 1) * BLOCK].ljust(BLOCK, b'\0')
    with open(path, 'wb') as destination:
        destination.write(card)


def main(argv):
    if len(argv) == 3 and argv[1] == 'list':
        for name, body in read_card(argv[2]):
            print('%-20s %6d bytes' % (name, len(body)))
    elif len(argv) == 4 and argv[1] == 'export':
        os.makedirs(argv[3], exist_ok=True)
        for name, body in read_card(argv[2]):
            with open(os.path.join(argv[3], name), 'wb') as destination:
                destination.write(body)
    elif len(argv) == 4 and argv[1] == 'import':
        files = [(n, open(os.path.join(argv[2], n), 'rb').read())
                 for n in sorted(os.listdir(argv[2]))
                 if not n.startswith('.') and os.path.isfile(os.path.join(argv[2], n))]
        write_card(argv[3], files)
    else:
        raise SystemExit(__doc__)


if __name__ == '__main__':
    main(sys.argv)
