#!/usr/bin/env python3
"""ds_compare.py: replay a scenario on DuckStation and the port, compare per field.

    ds_compare.py list
    ds_compare.py run <scenario> <out-dir> [--lowram] [--bin <port binary>]

Each scenario is an RR4_PAD_SCRIPT (with its pokes, controller and card)
played on the patched duckstation-regtest through ds_script.py and on the
port (headless, silent, PSYZ_CARD_UNPLUG=2: DuckStation's blank card in
slot 1, slot 2 empty), both through run_capped.sh. The traces (mode,
FrameCounter, rand, and the scenario's watched memory, read at DuckStation's
PS1 address and at the port's symbol) are compared field by field: the
report gives, for each, whether every field matches, else the first field
that differs and whether the sequence of values is still the same (only
the fields differ). --lowram adds a DuckStation run on the interpreter
logging game accesses to PS1 RAM 0..0xFFFF (DS_LOWRAM_LOG, DUMMYBP=1),
grouped by ds_lowram_report.py into lowram.txt.

Environment: RR4_DUCKSTATION, RR4_DISC (as for duckstation.py), RR4_DECOMP
(the decompilation, for ds_lowram_report.py's names). Files in <out-dir>:
ds.txt, port.txt, port.log, report.txt (and lowram.log/.txt).

The absolute steps of the scenarios are port vsyncs (DuckStation frame -
295); the mode-anchored ones repeat at each entry into their mode, so a
flow that comes back to a mode uses absolute steps instead.
"""
import os, subprocess, shutil, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))

BOOT = 'start@m2/4:100+4%100,start@m1/4:150'
# The Grand Prix regression scenario's inputs (regress.sh gp) after the
# main menu, with the throttle held in the race instead of RR4_AUTODRIVE.
GP = ','.join(['start@m0/2:100+4%150'] +
              ['circle@m1/%d:200+4%%154' % m for m in (6, 8, 10, 11, 12, 14, 16, 18, 34, 48)] +
              ['circle@m0/4:200+4%154', 'start@m1/22:120', 'down@m1/43:200',
               'circle@m1/43:300+4%154', 'cross@m1/47:100+20000'])
GP_RULES = GP.replace('start@m0/2:100+4%150,', '')
OPTIONS = ('circle@m2/2:300,circle@m2/2:700,down@m2/2:1000,down@m2/2:1050,down@m2/2:1100,'
           'circle@m2/2:1200,right@m2/2:1500,right@m2/2:1530,right@m2/2:1560,circle@m2/2:1700,'
           'up@m2/2:2000,circle@m2/2:2100,circle@m2/2:2400,right@m2/2:2500,circle@m2/2:2600,'
           'cross@m2/2:2800,down@m2/2:3100,down@m2/2:3150,circle@m2/2:3300,left@m2/2:3600,'
           'circle@m2/2:3800,down@m2/2:4100,down@m2/2:4150,circle@m2/2:4300')
MENU_DOWNS = lambda n, at=400: ','.join('down@m1/4:%d' % (at + 50 * i) for i in range(n))
SAVE_LOAD = (BOOT + ',' + MENU_DOWNS(5) + ',start@m1/4:700,up@2400,up@2460,circle@2600,up@2900,'
             'circle@3000,circle@3500,up@3800,up@3860,up@3920,circle@4050,up@4300,circle@4400,'
             'circle@5000,up@5300,up@5360,circle@5500,up@5800,circle@5900,circle@6600,up@6900,'
             'circle@7000,circle@7500,circle@7900')
TA_STICKS = ('down@m1/4:450,start@m1/4:550,circle@m1/50:200,circle@m1/51:200,circle@m1/51:260,'
             'circle@m1/54:200+4%120,start@m1/22:300,cross@m1/53:100+20000,lx=0.6@m1/53:400+60,'
             'lx=-0.45@m1/53:700+90,lx=1@m1/53:1100+30,ly=-0.5@m1/53:1300+40,lx=-1@m1/53:1500+25,'
             'rx=0.7@m1/53:1700+20')
# Versus with two pads: player 1, then player 2 loads garage data from their
# card (slot 1, slot 2) in the car select.
VS_LOAD = '2:select@100,start@m2/4:700+4%300,start@m1/4:150,down@m1/4:450,down@m1/4:500,start@m1/4:550,down@2600,circle@2700,down@3000,circle@3100,circle@3500,up@3800,circle@3900,circle@4500,down@4800,circle@4900,down@5300,down@5360,circle@5450,up@5700,circle@5800,circle@6000,2:down@6300,2:circle@6400,2:down@6700,2:circle@6800,2:circle@7200,2:up@7500,2:circle@7600,2:circle@8200,2:down@8500,2:circle@8600'
CAR_POSE = [('800abfc0:4', 'PlayerCars+200:4'), ('800ac074:2', 'PlayerCars+380:2')]
LOW_RAM = [('80000004:4', 'Ps1LowRamImage+4:4')]

# name: (fields, pads, options). Options: pokes (port RR4_POKE), ds_pokes
# (RR4_DS_POKE), watch [(ds addr:len, port name:len[, mode prefix: compared
# only in those modes, e.g. an overlay's variable)]], pad (dualshock),
# cards (slot 1, slot 2): 'blank' (DuckStation's blank card), 'none',
# 'unformatted', 'saved' (the save the save-load scenario makes), 'full'
# (15 blocks of other files); default ('blank', 'none'). card_steps
# [(slot, 'none', vsync)]: the card pulled out at that field.
SCENARIOS = {
    'options': (7000, BOOT + ',' + MENU_DOWNS(6) + ',start@m1/4:750,' + OPTIONS, {}),
    'options-gp': (16000, BOOT + ',' + ','.join('down@%d' % (1437 + 50 * i) for i in range(6)) +
                   ',start@1787,' + OPTIONS + ',start@6857,' + GP, {'watch': CAR_POSE}),
    'records': (6500, BOOT + ',' + MENU_DOWNS(4) + ',start@m1/4:650,down@2364,circle@2464,right@2900,'
                'right@3000,down@3100,cross@3300,down@3700,circle@3800,right@4200,right@4300,'
                'cross@4500,down@5000,down@5060,circle@5160', {}),
    'editor-gp': (16500, BOOT + ',down@1437,down@1487,down@1537,start@1637,down@2253,circle@2353,'
                  'circle@3000,up@3500,circle@3600,right@4000,right@4100,circle@4300,right@4600+40,'
                  'down@4700+50,circle@4800+60,right@4820+30,right@5000+300,down@5000+300,'
                  'up@5400+10,left@5420+10,circle@5500,up@5900,circle@6000,down@6400,down@6460,'
                  'circle@6560,start@7021,' + GP,
                  {'watch': CAR_POSE + [('8012767c:2', 'ovl662_editor_cursor_state+20:2', '3/')]}),
    'garage-save': (7800, BOOT + ',' + MENU_DOWNS(3) + ',start@m1/4:600,circle@m1/40:300,circle@2629,'
                    'right@2929,right@3029,left@3129,cross@3329,down@3629,circle@3729,circle@4029,'
                    'right@4129,circle@4229,up@4429,right@4529,circle@4629,circle@5029,up@5229,'
                    'circle@5329,circle@5829,down@6029,circle@6129', {}),
    'save-load': (8500, SAVE_LOAD, {}),
    'load-saved': (5000, BOOT + ',' + MENU_DOWNS(5) + ',start@m1/4:700,up@2400,up@2460,circle@2600,'
                   'up@2900,circle@3000,circle@3800,circle@4200', {'cards': ('saved', 'none')}),
    'overwrite': (5000, BOOT + ',' + MENU_DOWNS(5) + ',start@m1/4:700,up@2400,up@2460,up@2520,circle@2600,'
                  'up@2900,circle@3000,up@3300,circle@3400,circle@4000,circle@4400', {'cards': ('saved', 'none')}),
    'unformatted': (5600, BOOT + ',' + MENU_DOWNS(5) + ',start@m1/4:700,up@2400,up@2460,up@2520,circle@2600,'
                    'up@2900,circle@3000,circle@3700,up@4000,up@4060,up@4120,circle@4300,up@4600,circle@4700,'
                    'circle@5300', {'cards': ('unformatted', 'none')}),
    'full-card': (4600, BOOT + ',' + MENU_DOWNS(5) + ',start@m1/4:700,up@2400,up@2460,up@2520,circle@2600,'
                  'up@2900,circle@3000,circle@3600,circle@4000', {'cards': ('full', 'none')}),
    'card-pulled': (5600, SAVE_LOAD, {'card_steps': [(1, 'none', 4470)]}),
    'two-cards': (8500, SAVE_LOAD, {'cards': ('blank', 'blank')}),
    'slot2-only': (8500, SAVE_LOAD, {'cards': ('none', 'blank')}),
    'vs-two-cards': (9000, VS_LOAD, {'cards': ('saved', 'saved')}),
    'vs-slot2-only': (9000, VS_LOAD, {'cards': ('none', 'saved')}),
    'gp-rules': (24500, BOOT + ',start@m1/4:400,' + GP_RULES + ',start@m1/47:700,down@m1/47:800,'
                 'start@m1/47:1000,circle@m0/6:300,down@20300,circle@m1/50:200,circle@m1/51:200,'
                 'circle@m1/51:260,circle@m1/54:200+4%120,start@m1/22:300,cross@m1/53:100+20000',
                 {'watch': LOW_RAM + [('800ac000:4', 'PlayerCars+264:4')]}),
    'extra-trial': (12000, BOOT + ',' + MENU_DOWNS(7) + ',start@m1/4:800,circle@m1/79:300,'
                    'circle@m1/80:300,circle@m1/80:600,cross@m1/82:100+20000',
                    {'pokes': 'SaveProgress+84:4=1@900,CourseChoiceTable+36:2=0@900',
                     'ds_pokes': '800f3974:01000000@900,800abe2c:0000@900', 'watch': CAR_POSE}),
    'dualshock-ta': (12000, 'start@m1/4:150,' + TA_STICKS, {'pad': 'dualshock', 'watch': CAR_POSE}),
    'dualshock-movie-ta': (6000, BOOT + ',' + TA_STICKS, {'pad': 'dualshock', 'watch': CAR_POSE}),
}


def capped(*cmd):
    return [os.path.join(HERE, 'run_capped.sh')] + list(cmd)


def ds_env(opts, extra=None):
    env = dict(os.environ)
    env.pop('RR4_DS_FIELD_OFFSET', None)
    if opts.get('pad') == 'dualshock':
        env['REGTEST_PAD1_TYPE'] = 'AnalogController'
    if opts.get('ds_pokes'):
        env['RR4_DS_POKE'] = opts['ds_pokes']
    if opts.get('watch'):
        env['RR4_DS_PEEK'] = ','.join(w[0] for w in opts['watch'])
    env.update(extra or {})
    return env


def run_ds(out, fields, pads, opts, name='ds.txt', extra=None):
    with open(os.path.join(out, name.replace('.txt', '.log')), 'w') as log:
        subprocess.run(capped(sys.executable, os.path.join(HERE, 'ds_script.py'), str(fields), pads,
                              os.path.join(out, name)), env=ds_env(opts, extra), stdout=log,
                       stderr=subprocess.STDOUT, check=False)


def make_saved_card(out):
    """The save-load scenario on DuckStation up to its save, on a card file."""
    card = os.path.join(out, 'saved.mcd')
    if os.path.exists(card):
        os.remove(card)
    run_ds(out, 5100, SAVE_LOAD, {}, 'saved-card.txt', {'REGTEST_CARD1': card})
    return card


def make_full_card(out):
    """A card of 15 one-block files of another game (no room left)."""
    src = os.path.join(out, 'full-bu00')
    shutil.rmtree(src, ignore_errors=True)
    os.makedirs(src)
    for k in range(15):
        with open(os.path.join(src, 'BESLES-%05dFILL' % k), 'wb') as f:
            f.write(bytes([0x53, 0x43, 0x11, 0x01]) + bytes(8188))
    card = os.path.join(out, 'full.mcd')
    subprocess.run([sys.executable, os.path.join(HERE, 'mcd.py'), 'import', src, card], check=True)
    return card, src


def prepare_cards(out, opts):
    """(DuckStation REGTEST_CARD1/2 values, port env, port card dirs)."""
    kinds = opts.get('cards', ('blank', 'none'))
    ds, env, dirs = {}, {'PSYZ_CARD_UNPLUG': '', 'PSYZ_CARD_UNFORMATTED': ''}, {}
    made = {}
    for slot, kind in enumerate(kinds, 1):
        if kind == 'none':
            ds['REGTEST_CARD%d' % slot] = 'none'
            env['PSYZ_CARD_UNPLUG'] += str(slot)
        elif kind == 'blank':
            ds['REGTEST_CARD%d' % slot] = '' if slot == 1 else 'blank'
        elif kind == 'unformatted':
            path = os.path.join(out, 'ds-card%d.mcd' % slot)
            open(path, 'wb').write(bytes(131072))
            ds['REGTEST_CARD%d' % slot] = path
            env['PSYZ_CARD_UNFORMATTED'] += str(slot)
        elif kind in ('saved', 'full'):
            if kind not in made:
                if kind == 'saved':
                    card = make_saved_card(out)
                    src = os.path.join(out, 'saved-bu00')
                    shutil.rmtree(src, ignore_errors=True)
                    subprocess.run([sys.executable, os.path.join(HERE, 'mcd.py'), 'export', card, src],
                                   check=True)
                else:
                    card, src = make_full_card(out)
                made[kind] = (card, src)
            card, src = made[kind]
            path = os.path.join(out, 'ds-card%d.mcd' % slot)
            shutil.copy(card, path)
            ds['REGTEST_CARD%d' % slot] = path
            dirs['bu%d0' % (slot - 1)] = src
    if opts.get('card_steps'):
        ds['RR4_DS_CARD'] = ','.join('%d:%s@%d' % st for st in opts['card_steps'])
        env['RR4_CARD_SLOT'] = ds['RR4_DS_CARD']
    return ds, {k: v for k, v in env.items() if v}, dirs


def run_port(out, binary, fields, pads, opts, card_env, card_dirs):
    run = os.path.join(out, 'port')
    shutil.rmtree(run, ignore_errors=True)
    os.makedirs(os.path.join(run, 'cfg'))
    for name, src in card_dirs.items():
        shutil.copytree(src, os.path.join(run, name))
    env = dict(os.environ, PSYZ_HEADLESS='1', SDL_AUDIODRIVER='dummy',
               RR4_CONFIG_HOME=os.path.join(run, 'cfg'), RR4_FRAME_LIMIT=str(fields),
               RR4_PAD_SCRIPT=pads, RR4_TRACE='1', RR4_TRACE_RAND='1')
    for k in ('PSYZ_CARD_UNPLUG', 'PSYZ_CARD_UNFORMATTED', 'RR4_CARD_SLOT'):
        env.pop(k, None)
    env.update(card_env)
    if opts.get('pad'):
        env['RR4_PAD_TYPE'] = opts['pad']
    if opts.get('pokes'):
        env['RR4_POKE'] = opts['pokes']
    if opts.get('watch'):
        env['RR4_WATCH'] = ','.join(w[1] for w in opts['watch'])
    with open(os.path.join(out, 'port.log'), 'w') as log:
        subprocess.run(capped(binary, os.environ['RR4_DISC']), cwd=run, env=env, stdout=log,
                       stderr=subprocess.STDOUT, check=False)
    with open(os.path.join(out, 'port.log')) as log, open(os.path.join(out, 'port.txt'), 'w') as t:
        t.writelines(l for l in log if l.startswith('vsync '))


def columns(path, opts, ds):
    """Per field: {vsync: {column: value}} with mode, frame, rand and the watches."""
    rows = {}
    for line in open(path):
        t = line.split()
        if not t or t[0] != 'vsync':
            continue
        row = {'mode': t[5] + ' ' + t[7], 'frame': t[3]}
        for x in t:
            if x.startswith('rand='):
                row['rand'] = x[5:]
        for k, (d, p, *only) in enumerate(opts.get('watch', [])):
            size = int(d.split(':')[1])
            key = (d.split(':')[0] + '=') if ds else (p.rsplit(':', 1)[0] + '=')
            v = next((x[len(key):] for x in t if x.startswith(key)), None)
            if only and not row['mode'].startswith(only[0]):
                v = None
            elif v is not None:
                v = int.from_bytes(bytes.fromhex(v), 'little') if ds else int(v) & ((1 << (8 * size)) - 1)
            row['watch%d %s' % (k, p)] = v
        rows[int(t[1])] = row
    return rows


def compare(out, opts):
    a, b = columns(os.path.join(out, 'ds.txt'), opts, True), columns(os.path.join(out, 'port.txt'), opts, False)
    fields = sorted(set(a) & set(b))
    lines = ['fields compared: %d (DuckStation %d, port %d)' % (len(fields), len(a), len(b))]
    for col in (a[fields[0]] if fields else {}):
        bad = [v for v in fields if a[v].get(col) != b[v].get(col)]

        def changes(rows):
            seq, prev = [], None
            for v in fields:
                if rows[v].get(col) != prev:
                    prev = rows[v].get(col)
                    seq.append(prev)
            return seq
        if not bad:
            lines.append('%s: every field the same' % col)
        else:
            same = changes(a) == changes(b)
            lines.append('%s: %d fields differ, first %d (DuckStation %s, port %s); the sequence of values is %s'
                         % (col, len(bad), bad[0], a[bad[0]].get(col), b[bad[0]].get(col),
                            'the same' if same else 'different'))
    return '\n'.join(lines)


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in ('list', 'run'):
        sys.exit(__doc__)
    if sys.argv[1] == 'list':
        for name, (fields, _, opts) in SCENARIOS.items():
            print('%-20s %6d fields %s' % (name, fields, ' '.join('%s=%s' % kv for kv in opts.items()
                                                                if kv[0] in ('pad', 'cards', 'card_steps'))))
        return
    name, out = sys.argv[2], os.path.abspath(sys.argv[3])
    binary = os.path.join(ROOT, 'build', 'ridge-racer-type-4-pc')
    if '--bin' in sys.argv:
        binary = os.path.abspath(sys.argv[sys.argv.index('--bin') + 1])
    fields, pads, opts = SCENARIOS[name]
    os.makedirs(out, exist_ok=True)
    extra, card_env, card_dirs = prepare_cards(out, opts)
    fresh = {k: v for k, v in extra.items() if k.startswith('REGTEST_CARD') and v.endswith('.mcd')}
    keep = {k: open(v, 'rb').read() for k, v in fresh.items()}
    run_ds(out, fields, pads, opts, extra=extra)
    if '--lowram' in sys.argv:
        log = os.path.join(out, 'lowram.log')
        if os.path.exists(log):
            os.remove(log)
        for k, v in fresh.items():  # the same cards again
            open(v, 'wb').write(keep[k])
        run_ds(out, fields, pads, opts, 'ds-lowram.txt', dict(extra, DUMMYBP='1', DS_LOWRAM_LOG=log))
        with open(os.path.join(out, 'lowram.txt'), 'w') as f:
            subprocess.run([sys.executable, os.path.join(HERE, 'ds_lowram_report.py'), log] +
                           ([os.environ['RR4_DECOMP']] if os.environ.get('RR4_DECOMP') else []),
                           stdout=f, check=False)
    run_port(out, binary, fields, pads, opts, card_env, card_dirs)
    report = compare(out, opts)
    open(os.path.join(out, 'report.txt'), 'w').write(report + '\n')
    print(report)


if __name__ == '__main__':
    main()
