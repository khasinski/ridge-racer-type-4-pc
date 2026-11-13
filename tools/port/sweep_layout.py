"""Find globals whose host definition does not cover the original storage.

For every global defined in the port with a known PS1 address (main
executable symbol map or overlay symbol tables) this computes its size in
the original 32-bit layout and reports:

  ALIAS     another original symbol lies inside the object's PS1 extent
            and the port defines it as a separate object
  INTERIOR  another original symbol lies inside the extent and the port only
            references it (needs a decl that maps onto the owner)
  TRUNC     original initialized bytes follow the object before the next
            original symbol (the table was cut at a label boundary)
  OVERLAP   two port definitions overlap in the original address space
  SPAN      grouped report per owner: '*' marks interior symbols the port
            defines separately (stores through one name are not seen
            through the other)

usage: sweep_layout.py <decomp-root> [--json out.json]
Requires the libclang Python binding (pip install clang) and libclang.
LIBCLANG=<path to libclang.dylib / libclang.so / libclang.dll> picks the
library; without it the sweep tries the Xcode Command Line Tools, Xcode and
the usual Linux LLVM locations, then the binding's own search.
"""
import bisect, glob, json, os, re, sys
from multiprocessing import Pool
import clang.cindex as ci

def find_libclang():
    if os.environ.get('LIBCLANG'):
        return os.environ['LIBCLANG']
    candidates = [
        '/Library/Developer/CommandLineTools/usr/lib/libclang.dylib',
        '/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/libclang.dylib',
    ]
    candidates += sorted(glob.glob('/usr/lib/llvm-*/lib/libclang.so*'), reverse=True)
    candidates += sorted(glob.glob('/usr/lib*/libclang.so*'), reverse=True)
    candidates += sorted(glob.glob('/usr/lib/x86_64-linux-gnu/libclang*.so*'), reverse=True)
    return next((c for c in candidates if os.path.exists(c)), None)


LIBCLANG = find_libclang()
if LIBCLANG:
    ci.Config.set_library_file(LIBCLANG)
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '../..'))
ARGS = ['-std=c11', '-D__psyz', '-I' + ROOT + '/include', '-isystem',
        ROOT + '/third_party/psyz/psyz/include', '-w']
TK = ci.TypeKind
HOST_WORD_TYPES = {'uintptr_t', 'intptr_t', 'size_t', 'ptrdiff_t', 'u_long', 'ssize_t', 'PsyqULong'}


def typedef_name(t):
    while t.kind in (TK.TYPEDEF, TK.ELABORATED):
        if t.kind == TK.TYPEDEF:
            n = t.get_declaration().spelling
            if n in HOST_WORD_TYPES:
                return n
            t = t.get_declaration().underlying_typedef_type
        else:
            t = t.get_named_type()
    return None


_rec = {}


def ps1(t):
    """(size, align) in the original layout; None if unknown."""
    if typedef_name(t):
        return (4, 4)
    c = t.get_canonical()
    k = c.kind
    if k in (TK.BOOL, TK.CHAR_S, TK.CHAR_U, TK.SCHAR, TK.UCHAR):
        return (1, 1)
    if k in (TK.SHORT, TK.USHORT):
        return (2, 2)
    if k in (TK.INT, TK.UINT, TK.LONG, TK.ULONG, TK.FLOAT, TK.ENUM, TK.POINTER):
        return (4, 4)
    if k in (TK.LONGLONG, TK.ULONGLONG, TK.DOUBLE):
        return (8, 8)
    if k == TK.CONSTANTARRAY:
        e = ps1(c.element_type)
        return None if e is None else (e[0] * c.element_count, e[1])
    if k == TK.INCOMPLETEARRAY:
        e = ps1(c.element_type)
        return None if e is None else (0, e[1])
    if k == TK.RECORD:
        return record(c)
    return None


def record(c):
    d = c.get_declaration()
    if d.hash in _rec:
        return _rec[d.hash]
    if d.spelling == 'GpuDmaTag':
        return (4, 4)
    union = d.kind == ci.CursorKind.UNION_DECL
    packed = any(ch.kind == ci.CursorKind.PACKED_ATTR for ch in d.get_children())
    off = 0
    size = 0
    al = 1
    for f in c.get_fields():
        r = ps1(f.type)
        if r is None:
            _rec[d.hash] = None
            return None
        fs, fa = r
        if packed:
            fa = 1
        al = max(al, fa)
        if f.is_bitfield():
            w = f.get_bitfield_width()
            unit = fs * 8
            if union:
                size = max(size, fs * 8)
                continue
            if w == 0 or (not packed and off // unit != (off + w - 1) // unit):
                off = (off + unit - 1) // unit * unit
            off += w
            continue
        if union:
            size = max(size, fs * 8)
        else:
            off = (off + fa * 8 - 1) // (fa * 8) * (fa * 8) + fs * 8
    total = size if union else off
    total = (total + al * 8 - 1) // (al * 8) * (al * 8)
    _rec[d.hash] = (total // 8, al)
    return _rec[d.hash]


def visit(path):
    tu = ci.Index.create().parse(path, args=ARGS)
    defs, refs = [], set()
    for c in tu.cursor.get_children():
        if c.kind != ci.CursorKind.VAR_DECL or c.location.file is None:
            continue
        if os.path.relpath(c.location.file.name, ROOT).startswith('third_party'):
            continue
        refs.add(c.spelling)
        if c.storage_class == ci.StorageClass.EXTERN:
            continue
        r = ps1(c.type)
        defs.append(dict(name=c.spelling, file=os.path.relpath(c.location.file.name, ROOT), line=c.location.line,
                         static=c.storage_class == ci.StorageClass.STATIC, type=c.type.spelling,
                         size=None if r is None else r[0],
                         incomplete=c.type.get_canonical().kind == TK.INCOMPLETEARRAY))
    return defs, sorted(refs)


def load_symbols(decomp):
    syms = {}
    for line in open(os.path.join(decomp, 'configs/PAL/sym.main.txt')):
        m = re.match(r'\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+);(.*)', line)
        if m and 'type:func' not in m.group(3):
            syms[m.group(1)] = ('main', int(m.group(2), 16))
    funcs = set()
    for line in open(os.path.join(decomp, 'configs/PAL/sym.main.txt')):
        m = re.match(r'\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+);(.*)', line)
        if m and 'type:func' in m.group(3):
            funcs.add(int(m.group(2), 16))
    for p in glob.glob(os.path.join(decomp, 'configs/PAL/overlays/*.json')):
        d = json.load(open(p))
        if 'globals' not in d:
            continue
        # Overlays share one load window: each is its own address space.
        space = 'ovl%s' % (d.get('entry') or d.get('index') or os.path.basename(p))
        for n, a in d['globals'].items():
            syms[n] = (space, a)
    return syms


def load_data(decomp, labels=None):
    """Byte map of the main executable's data; fills `labels` with the
    `dlabel` names of the data files (unnamed D_ and jump-table labels the
    symbol map does not list)."""
    data = {}
    for p in glob.glob(os.path.join(decomp, 'asm/PAL/main/data/*.s')):
        pending = []
        for line in open(p):
            d = re.match(r'\s*dlabel\s+(\w+)', line)
            if d:
                pending.append(d.group(1))
                continue
            m = re.search(r'/\*\s*[0-9A-F]+\s+([0-9A-F]{8})(?:\s+[0-9A-F]+)?\s*\*/\s*\.(word|short|byte|asciz)\s+(.*)$', line)
            if not m:
                continue
            a = int(m.group(1), 16)
            if labels is not None:
                for n in pending:
                    labels.setdefault(n, a)
            pending = []
            kind, arg = m.group(2), m.group(3).split('/*')[0].strip()
            if kind == 'asciz':
                txt = bytes(arg[1:-1], 'utf-8').decode('unicode_escape').encode('latin-1') + b'\0'
                for i, b in enumerate(txt):
                    data[a + i] = b
                continue
            n = {'word': 4, 'short': 2, 'byte': 1}[kind]
            try:
                v = int(arg, 0)
            except ValueError:
                v = 0xFFFFFFFF  # relocation: nonzero
            for i in range(n):
                data[a + i] = (v >> (8 * i)) & 0xFF
    return data


OVERLAY_WINDOW = 0x80114878
ALLOWLIST = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sweep_layout_allow.txt')


def load_allowlist():
    """Reviewed decisions, one per line, each with a `# reason`:
      TRUNC|SPAN NAME     the finding is not a bug
      SIZE NAME 0xN       PS1 size of a host-enlarged object (bounds its extent)
      SYM NAME 0xADDR     main-executable address of an unnamed original object
                          the port defines under NAME"""
    allow, sizes, extra = {}, {}, {}
    if os.path.exists(ALLOWLIST):
        for n, line in enumerate(open(ALLOWLIST), 1):
            body, _, reason = line.partition('#')
            parts = body.split()
            if not parts:
                continue
            if not reason.strip():
                sys.exit(f'{ALLOWLIST}:{n}: entry needs a reason')
            if parts[0] in ('TRUNC', 'SPAN') and len(parts) == 2:
                allow[(parts[0], parts[1])] = reason.strip()
            elif parts[0] == 'SIZE' and len(parts) == 3:
                sizes[parts[1]] = int(parts[2], 0)
            elif parts[0] == 'SYM' and len(parts) == 3:
                extra[parts[1]] = int(parts[2], 0)
            else:
                sys.exit(f'{ALLOWLIST}:{n}: cannot parse {line.strip()!r}')
    return allow, sizes, extra


def main():
    decomp = sys.argv[1]
    syms = load_symbols(decomp)
    allow, sizes, extra = load_allowlist()
    for n, a in extra.items():
        syms.setdefault(n, ('main', a))
    labels = {}
    data = load_data(decomp, labels)
    files = sorted(glob.glob(ROOT + '/src/**/*.c', recursive=True))
    with Pool(10) as p:
        res = p.map(visit, files)
    defined, referenced = {}, set()
    for defs, refs in res:
        referenced.update(refs)
        for d in defs:
            if d['static']:
                continue
            defined.setdefault(d['name'], d)
    # Data-file labels the port uses by name are original object boundaries;
    # unused ones are still unowned data (they must not end a neighbour).
    for n, a in labels.items():
        if n in referenced or n in defined:
            syms.setdefault(n, ('main', a))
    # Port objects named after an unlabelled original address (D_8XXXXXXX in
    # the main executable) also bound the extents of their neighbours.
    for n in defined:
        m = re.fullmatch(r'D_(8[0-9A-F]{7})', n)
        if m and n not in syms and int(m.group(1), 16) < OVERLAY_WINDOW:
            syms[n] = ('main', int(m.group(1), 16))
    by_space = {}
    for n, (sp, a) in syms.items():
        by_space.setdefault(sp, []).append((a, n))
    for v in by_space.values():
        v.sort()
    findings = []
    for name, d in sorted(defined.items()):
        if name not in syms or d['size'] is None:
            continue
        sp, a = syms[name]
        lst = by_space[sp]
        addrs = [x[0] for x in lst]
        size = sizes.get(name, d['size'])
        i = bisect.bisect_right(addrs, a)
        nxt = addrs[i] if i < len(addrs) else None
        # interior symbols
        j = i
        while j < len(lst) and lst[j][0] < a + max(size, 1):
            other = lst[j][1]
            if other != name:
                if other in defined:
                    kind = 'OVERLAP' if lst[j][0] != a else 'ALIAS'
                    if lst[j][0] > a:
                        kind = 'ALIAS'
                    findings.append(dict(kind=kind, name=name, other=other, at=lst[j][0] - a, file=d['file'],
                                         other_file=defined[other]['file'], size=size))
                elif other in referenced:
                    findings.append(dict(kind='INTERIOR', name=name, other=other, at=lst[j][0] - a, file=d['file'], size=size))
            j += 1
        # truncation (main data only)
        if sp == 'main' and nxt is not None and nxt > a + size:
            tail = [data.get(x, 0) for x in range(a + size, nxt)]
            nz = sum(1 for b in tail if b)
            if nz and any(x in data for x in range(a + size, nxt)):
                findings.append(dict(kind='TRUNC', name=name, file=d['file'], size=size, gap=nxt - a - size, nonzero=nz,
                                     next=[n for ad, n in lst if ad == nxt][0]))
    used = set()
    kept = []
    for f in findings:
        key = ('SPAN' if f['kind'] != 'TRUNC' else 'TRUNC', f['name'])
        if key in allow:
            used.add(key)
        else:
            kept.append(f)
    for key in sorted(set(allow) - used):
        print(f"STALE    allowlist entry {key[0]} {key[1]} no longer matches a finding")
    findings = kept
    order = {'ALIAS': 0, 'OVERLAP': 1, 'TRUNC': 2, 'INTERIOR': 3}
    findings.sort(key=lambda f: (order[f['kind']], f['file'], f['name']))
    owners = {}
    for f in findings:
        if f['kind'] == 'TRUNC':
            print(f"TRUNC    {f['name']} ({f['file']}) size {f['size']:#x}, {f['nonzero']} nonzero of {f['gap']:#x} bytes before {f['next']}")
        else:
            owners.setdefault((f['name'], f['file'], f['size']), []).append(f)
    for (name, file, size), fs in sorted(owners.items(), key=lambda kv: kv[0][1]):
        sp, a = syms[name]
        lst = by_space[sp]
        i = bisect.bisect_right([x[0] for x in lst], a)
        extent = (lst[i][0] - a) if i < len(lst) else 0
        parts = ', '.join(f"{f['other']}@{f['at']:#x}{'*' if f['kind'] in ('ALIAS', 'OVERLAP') else ''}" for f in fs)
        print(f"SPAN     {name} ({file}) port size {size:#x} > original extent {extent:#x}: {parts}")
    counts = {}
    for f in findings:
        counts[f['kind']] = counts.get(f['kind'], 0) + 1
    print('summary', counts, 'allowlisted', len(used))
    if '--json' in sys.argv:
        json.dump(findings, open(sys.argv[sys.argv.index('--json') + 1], 'w'), indent=1)


if __name__ == '__main__':
    main()
