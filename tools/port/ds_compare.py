#!/usr/bin/env python3
"""ds_compare.py: replay a scenario on DuckStation and the port, compare per field.

    ds_compare.py list
    ds_compare.py run <scenario> <out-dir> [--lowram] [--force-blanks] [--bin <port binary>]
                      [--refs <dir> [--save-ref]]

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

Both sides also log, per blank, whether libpad made its exchange and
whether the BIOS card driver took its turn (DuckStation: RR4_DS_PADLOG and
the driver's port byte, read between fields; the port: PSYZ_PAD_TRACE,
PSYZ_CARD_TRACE). The report lists the blanks where they differ, with the
port's distance to the boundary of the dispatch window that decided it
("margin"). DuckStation is deterministic (a run repeats to the clock) and,
on its exact timing (the default, tools/port/DUCKSTATION.md), raises
interrupts and the vblank at their clock; on the recompiler
(RR4_DS_TIMING=recompiler) they moved by up to about 50 clocks with the code
running, so the same port position could go either way there. A difference
within BAND_CLOCKS of a boundary, within the port's own resolution, is
marked, and --force-blanks reruns the port with such blanks forced to
DuckStation's outcome (PSYZ_BLANK_FORCE), adding those each rerun shows,
and reports where the scenario then first differs.

Environment: RR4_DUCKSTATION, RR4_DISC (as for duckstation.py), RR4_DECOMP
(the decompilation, for ds_lowram_report.py's names). Files in <out-dir>:
ds.txt, port.txt, port.log, report.txt (and lowram.log/.txt; port-force*.log
with --force-blanks).

--refs <dir> takes DuckStation's side from a stored reference,
<dir>/<scenario>/ds.txt (and saved.mcd for the scenarios that start from
the Save and Load scenario's card), instead of running it: DuckStation is
deterministic, so the trace only changes with the scenario. An autodrive
scenario's reference holds the input it replayed (masks.txt) and is used
only while the port chooses the same; otherwise DuckStation runs.
--save-ref stores what DuckStation ran there.

A scenario with the autodrive option (ta-full; vs-full, both pads in the
Versus race) runs the port first with
RR4_AUTODRIVE and replays the input it chose on DuckStation (ds_frames.py
masks, ds_script.py's RR4_DS_MASKS); --force-blanks reruns of it drive on
their own, so a forced blank that changes the input ends the comparison.

The absolute steps of the scenarios are port vsyncs (DuckStation frame -
295); the mode-anchored ones repeat at each entry into their mode, so a
flow that comes back to a mode uses absolute steps instead.
"""
import os, subprocess, shutil, sys

# The BIOS card driver's current port (PS1 0x7264), read between fields.
CARD_PORT = '80007264:1'
# How near a window's boundary a difference is marked, in clocks (see the
# docstring).
BAND_CLOCKS = 50

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
# Time Attack on another course: every course unlocked (SaveProgress's
# cleared-course bits at +0x0C, poked in the main menu), the course menu's
# choice stepped there with right (or left past 4, wrapping) 40 fields apart
# (choice -> course: 0 4 2 1 6 3 5 7).
TA_COURSE = lambda choice: (BOOT + ',' + TA_STICKS.replace('circle@m1/50:200', ','.join(
    '%s@m1/50:%d+6' % ('right' if choice <= 4 else 'left', 100 + 40 * i)
    for i in range(choice if choice <= 4 else 8 - choice)) + ',circle@m1/50:%d' % (
    100 + 40 * (choice if choice <= 4 else 8 - choice) + 60)))
# Versus with two pads: player 1, then player 2 loads garage data from their
# card (slot 1, slot 2) in the car select.
VS_LOAD = '2:select@100,start@m2/4:700+4%300,start@m1/4:150,down@m1/4:450,down@m1/4:500,start@m1/4:550,down@2600,circle@2700,down@3000,circle@3100,circle@3500,up@3800,circle@3900,circle@4500,down@4800,circle@4900,down@5300,down@5360,circle@5450,up@5700,circle@5800,circle@6000,2:down@6300,2:circle@6400,2:down@6700,2:circle@6800,2:circle@7200,2:up@7500,2:circle@7600,2:circle@8200,2:down@8500,2:circle@8600'
CAR_POSE = [('800abfc0:4', 'PlayerCars+200:4'), ('800ac074:2', 'PlayerCars+380:2')]
TA_COURSE_OPTS = {'pad': 'dualshock', 'watch': CAR_POSE,
                  'pokes': 'SaveProgress+12:4=255@1200', 'ds_pokes': '800f392c:ff000000@1200'}
RACE_STATE = [('800ff6b0:4', 'RaceState:4'), ('800ac07c:4', 'PlayerCars+388:4')]
# Time Attack driven by RR4_AUTODRIVE (the port's run first, its input
# replayed on DuckStation: ds_frames.py masks, RR4_DS_MASKS): three laps,
# the replay to its end, the results and the retry's race.
TA_AUTO = BOOT + ',down@m1/4:450,start@m1/4:550,circle@m1/50:200,circle@m1/51:200,circle@m1/51:260,circle@m1/54:200+4%120'
# Versus with two pads, both cars driven by RR4_AUTODRIVE over the whole
# race (no laps poked), the two-player replay to its end and the results.
VS_AUTO = ('2:select@100,start@m2/4:700+4%300,start@m1/4:150,down@m1/4:450,down@m1/4:500,'
           'start@m1/4:550,circle@m1/56:150,circle@m1/56:300,2:circle@m1/56:450,'
           '2:circle@m1/56:600,circle@m1/57:200')
# the second player's car (PS1 records 800 bytes apart, the host's 816)
CAR2_POSE = [('800ac2e0:4', 'PlayerCars+1016:4'), ('800ac394:2', 'PlayerCars+1196:2'),
             ('800ac39c:4', 'PlayerCars+1204:4')]
LOW_RAM = [('80000004:4', 'Ps1LowRamImage+4:4')]
# The Grand Prix to its ending: the throttle held and nine laps' distance
# poked into each race (as regress.sh gp), the stage poked to the seventh
# in the first race (SelectionState+0x17, so that race's result is the
# seventh's), the eighth race, the ending's scenes, the epilogue's credits
# over course 8, the ending movie and the title after it.
GP_STAGE = [('800ad4ef:1', 'SelectionState+23:1'), ('800f3914:4', 'CourseIndex:4')]
GP_LAPS = ','.join('PlayerCars+388+=TrackLength@m1/47:%d' % d for d in range(700, 1600, 100))

# name: (fields, pads, options). Options: pokes (port RR4_POKE), ds_pokes
# (RR4_DS_POKE; sym_pokes: the port's pokes on DuckStation too, through
# RR4_DS_SYMPOKE, mode anchors and globals as values included), watch [(ds addr:len, port name:len[, mode prefix: compared
# only in those modes, e.g. an overlay's variable)]], pad (dualshock),
# cards (slot 1, slot 2): 'blank' (DuckStation's blank card), 'none',
# 'unformatted', 'saved' (the save the save-load scenario makes), 'full'
# (15 blocks of other files); default ('blank', 'none'); autodrive:
# RR4_AUTODRIVE's input (the port first, then DuckStation). card_steps
# [(slot, 'none', vsync)]: the card pulled out at that field.
SCENARIOS = {
    'options': (7000, BOOT + ',' + MENU_DOWNS(6) + ',start@m1/4:750,' + OPTIONS, {}),
    # the options' Music Player (mode 2/9): its XA tracks over the course
    # scenes, stepped right twice and left once, then back to the options
    'options-music': (6200, BOOT + ',' + MENU_DOWNS(6) + ',start@m1/4:750,' +
                      ','.join('down@m2/2:%d' % (300 + 50 * i) for i in range(5)) +
                      ',circle@m2/2:600,right@m2/9:700,right@m2/9:1300,left@m2/9:1900,cross@m2/9:2500', {}),
    # a DUAL SHOCK's controller setup: the control type stepped (A, B, C),
    # its vibration and analog pages, then back out of the options
    'options-dualshock': (4700, BOOT + ',' + MENU_DOWNS(6) + ',start@m1/4:750,circle@m2/2:300,'
                          'right@m2/2:500,right@m2/2:560,down@m2/2:700,right@m2/2:800,down@m2/2:900,'
                          'right@m2/2:1000,circle@m2/2:1200,down@m2/2:1400,circle@m2/2:1500,'
                          'cross@m2/2:1800,cross@m2/2:2100', {'pad': 'dualshock'}),
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
    # the second Extra Trial (CourseChoiceTable[1][1] unlocked instead of
    # the first): its menus, car and race
    'extra-trial-2': (6000, BOOT + ',' + MENU_DOWNS(7) + ',start@m1/4:800,circle@m1/79:300,'
                      'circle@m1/80:300,circle@m1/80:600,cross@m1/82:100+20000',
                      {'pokes': 'SaveProgress+84:4=1@900,CourseChoiceTable+40:2=0@900', 'sym_pokes': True,
                       'watch': CAR_POSE + [('800f3914:4', 'CourseIndex:4')]}),
    # the Extra Trial's race to its end (the throttle held, nine laps'
    # distance poked), its replay, the results and the retry's race
    'extra-trial-race': (10300, BOOT + ',' + MENU_DOWNS(7) + ',start@m1/4:800,circle@m1/79:300,'
                         'circle@m1/80:300,circle@m1/80:600,cross@m1/82:100+20000,start@m1/22:300,'
                         'circle@m1/83:300+4%154',
                         {'pokes': 'SaveProgress+84:4=1@900,CourseChoiceTable+36:2=0@900,' +
                          ','.join('PlayerCars+388+=TrackLength@m1/82:%d' % d for d in range(700, 1600, 100)),
                          'sym_pokes': True, 'watch': CAR_POSE + RACE_STATE}),
    'dualshock-ta': (12000, 'start@m1/4:150,' + TA_STICKS, {'pad': 'dualshock', 'watch': CAR_POSE}),
    'dualshock-movie-ta': (6000, BOOT + ',' + TA_STICKS, {'pad': 'dualshock', 'watch': CAR_POSE}),
    # the courses no other scenario loads (0 every race above, 5 the Extra Trial's)
    'ta-course4': (4500, TA_COURSE(1), TA_COURSE_OPTS),
    'ta-course2': (4500, TA_COURSE(2), TA_COURSE_OPTS),
    'ta-course1': (4500, TA_COURSE(3), TA_COURSE_OPTS),
    'ta-course6': (4500, TA_COURSE(4), TA_COURSE_OPTS),
    'ta-course3': (4500, TA_COURSE(5), TA_COURSE_OPTS),
    'ta-course7': (4500, TA_COURSE(7), TA_COURSE_OPTS),
    'gp-ending': (26500, BOOT + ',start@m1/4:400,' + GP + ',circle@m1/33:200+4%154',
                  {'pokes': GP_LAPS + ',SelectionState+23:1=6@7700', 'sym_pokes': True,
                   'watch': CAR_POSE + RACE_STATE + GP_STAGE}),
    # the garage's team car select and history screen stepped to other
    # teams and cars than garage-save's
    'garage-team': (6500, BOOT + ',' + MENU_DOWNS(3) + ',start@m1/4:600,circle@m1/40:300,'
                    'right@m1/64:150,right@m1/64:200,circle@m1/64:300,right@m1/66:150,'
                    'right@m1/66:200,left@m1/66:260,circle@m1/66:400', {}),
    # the Grand Prix with the second team (Racing Team Solvalou, hard) and
    # the second maker (Lizard): its first race to the results, nine laps'
    # distance poked, and the next heat's menus
    'gp-team2': (10500, BOOT + ',start@m1/4:400,' + GP.replace(
                     'circle@m1/8:200+4%154', 'circle@m1/8:200+4%154,right@m1/8:300').replace(
                     'circle@m1/10:200+4%154', 'circle@m1/10:200+4%154,right@m1/10:490'),
                 {'pokes': GP_LAPS, 'sym_pokes': True, 'watch': CAR_POSE + RACE_STATE}),
    # Versus with other cars and another course than vs-full's, both cars
    # driven by RR4_AUTODRIVE into the race
    'vs-picks': (9000, VS_AUTO.replace('circle@m1/56:150', 'right@m1/56:100,circle@m1/56:150').replace(
                     '2:circle@m1/56:450', '2:right@m1/56:400,2:circle@m1/56:450').replace(
                     'circle@m1/57:200', 'right@m1/57:100,right@m1/57:140,circle@m1/57:200'),
                 {'autodrive': 2, 'watch': CAR_POSE + RACE_STATE + CAR2_POSE + [('800f3914:4', 'CourseIndex:4')]}),
    'ta-full': (32000, TA_AUTO, {'autodrive': True, 'watch': CAR_POSE + RACE_STATE}),
    'vs-full': (28500, VS_AUTO, {'autodrive': 2, 'watch': CAR_POSE + RACE_STATE + CAR2_POSE}),
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
    if opts.get('sym_pokes'):
        env['RR4_DS_SYMPOKE'] = opts['pokes']
    env['RR4_DS_PEEK'] = ','.join([w[0] for w in opts.get('watch', [])] + [CARD_PORT])
    env['RR4_DS_PADLOG'] = '1'
    env.update(extra or {})
    return env


def run_ds(out, fields, pads, opts, name='ds.txt', extra=None):
    with open(os.path.join(out, name.replace('.txt', '.log')), 'w') as log:
        subprocess.run(capped(sys.executable, os.path.join(HERE, 'ds_script.py'), str(fields), pads,
                              os.path.join(out, name)), env=ds_env(opts, extra), stdout=log,
                       stderr=subprocess.STDOUT, check=False)


# --refs: the stored reference of the scenario being run (see the docstring)
REF = None


def make_saved_card(out):
    """The save-load scenario on DuckStation up to its save, on a card file."""
    card = os.path.join(out, 'saved.mcd')
    if os.path.exists(card):
        os.remove(card)
    if REF and os.path.exists(os.path.join(REF, 'saved.mcd')):
        shutil.copy(os.path.join(REF, 'saved.mcd'), card)
        return card
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


def run_port(out, binary, fields, pads, opts, card_env, card_dirs, force='', log_name='port.log'):
    run = os.path.join(out, 'port')
    shutil.rmtree(run, ignore_errors=True)
    os.makedirs(os.path.join(run, 'cfg'))
    for name, src in card_dirs.items():
        shutil.copytree(src, os.path.join(run, name))
    env = dict(os.environ, PSYZ_HEADLESS='1', SDL_AUDIODRIVER='dummy',
               RR4_CONFIG_HOME=os.path.join(run, 'cfg'), RR4_FRAME_LIMIT=str(fields),
               RR4_PAD_SCRIPT=pads, RR4_TRACE='1', RR4_TRACE_RAND='1',
               PSYZ_PAD_TRACE='1', PSYZ_CARD_TRACE='1', PSYZ_BLANK_FORCE=force)
    for k in ('PSYZ_CARD_UNPLUG', 'PSYZ_CARD_UNFORMATTED', 'RR4_CARD_SLOT'):
        env.pop(k, None)
    env.update(card_env)
    if opts.get('pad'):
        env['RR4_PAD_TYPE'] = opts['pad']
    if opts.get('autodrive'):
        env['RR4_AUTODRIVE'] = str(int(opts['autodrive']))  # 2: both Versus pads
        env['RR4_AUTODRIVE_LOG'] = os.path.join(out, log_name.replace('.log', '.ad'))
    if opts.get('pokes'):
        env['RR4_POKE'] = opts['pokes']
    if opts.get('watch'):
        env['RR4_WATCH'] = ','.join(w[1] for w in opts['watch'])
    with open(os.path.join(out, log_name), 'w') as log:
        subprocess.run(capped(binary, os.environ['RR4_DISC']), cwd=run, env=env, stdout=log,
                       stderr=subprocess.STDOUT, check=False)
    with open(os.path.join(out, log_name)) as log, \
            open(os.path.join(out, log_name.replace('.log', '.txt')), 'w') as t:
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


def ds_blanks(path):
    """{blank: (libpad exchange, card driver turn)} from DuckStation's trace:
    line v+1 tells about blank v (the flag libpad's handler sets, the
    driver's port changing from line v); {} without RR4_DS_PADLOG."""
    rows = []
    for line in open(path):
        t = line.split()
        if not t or t[0] != 'vsync' or 'pad' not in t:
            continue
        port = next((x for x in t if x.startswith(CARD_PORT.split(':')[0] + '=')), None)
        rows.append((int(t[1]), t[t.index('pad') + 1] != '0', port))
    return {v: (pad, port is not None and port != nport)
            for (v, _, port), (_, pad, nport) in zip(rows, rows[1:])}


def port_blanks(path):
    """{blank: (exchange, card turn, '+since/cycles', pad margin, card margin)}
    from the port's PSYZ_PAD_TRACE / PSYZ_CARD_TRACE lines."""
    rows, cur = {}, None
    for line in open(path, errors='replace'):
        if line.startswith('pad: blank '):
            t = line.split()
            m = t[t.index('margin') + 1:t.index('margin') + 3] if 'margin' in t else ['-1', '-1']
            cur = int(t[2])
            rows[cur] = [t[4] != '0', False, t[9], int(m[0]), int(m[1])]
        elif line.startswith('card: vblank port') and cur in rows and 'times out' not in line:
            # the vblank where the driver times out a transfer to an empty
            # port keeps its port, as DuckStation's port byte does there
            rows[cur][1] = True
    return rows


def blank_diffs(out, log_name='port.log'):
    """[(blank, 'pad'|'card', DuckStation's outcome, the port's, margin, where)]."""
    a, b = ds_blanks(os.path.join(out, 'ds.txt')), port_blanks(os.path.join(out, log_name))
    diffs = []
    for v in sorted(set(a) & set(b)):
        for k, name in ((0, 'pad'), (1, 'card')):
            if a[v][k] != b[v][k]:
                diffs.append((v, name, a[v][k], b[v][k], b[v][3 + k], b[v][2]))
    return diffs


def in_band(d):
    return 0 <= d[4] <= BAND_CLOCKS


def describe(d):
    return '%d %s (DuckStation %s, port %s; port %s, margin %s)%s' % (
        d[0], 'libpad exchange' if d[1] == 'pad' else 'card driver turn',
        'yes' if d[2] else 'no', 'yes' if d[3] else 'no', d[5], d[4] if d[4] >= 0 else '-',
        ': within %d clocks of the boundary' % BAND_CLOCKS if in_band(d) else '')


def first_field_diff(out, opts, port_txt='port.txt'):
    a, b = columns(os.path.join(out, 'ds.txt'), opts, True), columns(os.path.join(out, port_txt), opts, False)
    return next((v for v in sorted(set(a) & set(b)) if a[v] != b[v]), None)


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
    if ds_blanks(os.path.join(out, 'ds.txt')) and os.path.exists(os.path.join(out, 'port.log')):
        diffs = blank_diffs(out)
        first = first_field_diff(out, opts)
        before = [d for d in diffs if first is None or d[0] < first]
        lines.append('blanks: %d libpad exchange / card driver turn differences, %d before the first '
                     'field that differs, %d of those within %d clocks of a window boundary'
                     % (len(diffs), len(before), sum(map(in_band, before)), BAND_CLOCKS))
        lines.extend('  ' + describe(d) for d in before[:8])
    return '\n'.join(lines)


def force_blanks(out, binary, fields, pads, opts, card_env, card_dirs, rounds=6):
    """Rerun the port with the differences within the band (BAND_CLOCKS) before
    the first field that differs forced to DuckStation's outcome, adding
    the ones each rerun shows, until a rerun shows none new."""
    forced, lines = {}, []
    log_name, txt = 'port.log', 'port.txt'
    for k in range(rounds + 1):
        first = first_field_diff(out, opts, txt)
        shown = ','.join('%d%s' % (v, n[0]) for v, n in sorted(forced)) or 'none'
        if first is None:
            lines.append('forced %s: every field the same' % shown)
            break
        new = [d for d in blank_diffs(out, log_name) if d[0] < first and in_band(d) and
               (d[0], d[1]) not in forced]
        lines.append('forced %s: first field that differs %d' % (shown, first))
        if not new or k == rounds:
            break
        for d in new:
            forced[(d[0], d[1])] = d[2]
        by = {}
        for (v, n), x in forced.items():
            by.setdefault(v, []).append('%s=%d' % (n, x))
        spec = ','.join('%d:%s' % (v, ':'.join(sorted(f))) for v, f in sorted(by.items()))
        log_name = 'port-force%d.log' % (k + 1)
        txt = log_name.replace('.log', '.txt')
        run_port(out, binary, fields, pads, opts, card_env, card_dirs, spec, log_name)
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
    global REF
    if '--refs' in sys.argv:
        REF = os.path.join(os.path.abspath(sys.argv[sys.argv.index('--refs') + 1]), name)
    extra, card_env, card_dirs = prepare_cards(out, opts)
    fresh = {k: v for k, v in extra.items() if k.startswith('REGTEST_CARD') and v.endswith('.mcd')}
    keep = {k: open(v, 'rb').read() for k, v in fresh.items()}
    if opts.get('autodrive'):
        # the port drives first; DuckStation replays its input
        run_port(out, binary, fields, pads, opts, card_env, card_dirs)
        import ds_frames
        ds_frames.make_masks(os.path.join(out, 'port.txt'), os.path.join(out, 'port.ad'),
                             os.path.join(out, 'masks.txt'), 1)
        extra = dict(extra, RR4_DS_MASKS=os.path.join(out, 'masks.txt'))
    def same(a, b):
        return os.path.exists(a) and os.path.exists(b) and open(a, 'rb').read() == open(b, 'rb').read()
    stored = REF and os.path.exists(os.path.join(REF, 'ds.txt')) and '--save-ref' not in sys.argv and (
        not opts.get('autodrive') or same(os.path.join(REF, 'masks.txt'), os.path.join(out, 'masks.txt')))
    if stored:
        shutil.copy(os.path.join(REF, 'ds.txt'), os.path.join(out, 'ds.txt'))
        print('DuckStation: the stored reference', REF)
    else:
        if REF and opts.get('autodrive'):
            print('DuckStation: no stored reference with this input, running it')
        run_ds(out, fields, pads, opts, extra=extra)
        if REF and '--save-ref' in sys.argv:
            os.makedirs(REF, exist_ok=True)
            for f in ('ds.txt', 'masks.txt', 'saved.mcd'):
                if os.path.exists(os.path.join(out, f)):
                    shutil.copy(os.path.join(out, f), os.path.join(REF, f))
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
    if not opts.get('autodrive'):
        run_port(out, binary, fields, pads, opts, card_env, card_dirs)
    report = compare(out, opts)
    if '--force-blanks' in sys.argv:
        report += '\n' + force_blanks(out, binary, fields, pads, opts, card_env, card_dirs)
    open(os.path.join(out, 'report.txt'), 'w').write(report + '\n')
    print(report)


if __name__ == '__main__':
    main()
