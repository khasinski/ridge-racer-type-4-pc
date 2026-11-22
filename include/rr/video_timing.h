#ifndef RR_VIDEO_TIMING_H
#define RR_VIDEO_TIMING_H

/* Video timing (Escape menu Graphics > Game timing, RR4_VIDEO=pal|ntsc).
 *
 * PAL is this disc as released: SetVideoMode(MODE_PAL),
 * 49.75 Hz fields, races at 25 frames a second. The PAL release is the
 * NTSC game with every value that counts frames converted to 25 frames a
 * second (5/6 of the NTSC count: durations, timers, frames per second in
 * time conversions, animation rates); the per-frame physics, AI and car
 * data are the NTSC ones, so on PAL the cars cover 5/6 of the NTSC
 * distance per second of real time.
 *
 * NTSC, the default, runs the PAL disc as the NTSC release does:
 * SetVideoMode(MODE_NTSC), 59.82 Hz fields, races at 30 frames a second,
 * at each place the PAL release changed for 25 frames a second the value of
 * the NTSC release (SLPS-01800, compared function by function), and the
 * NTSC values of the car records the PAL release retuned
 * (CarRecordUseNtscValues). Picture layout, language, content and fixes of
 * the PAL release stay as they are.
 *
 * The choice is made once at startup, before MainLoop; it is not changed
 * while the game runs. */
extern int VideoTimingNtsc;

/* The PAL release's value, or the NTSC release's under NTSC timing. */
#define RR_PAL_NTSC(pal, ntsc) (VideoTimingNtsc ? (ntsc) : (pal))

#endif
