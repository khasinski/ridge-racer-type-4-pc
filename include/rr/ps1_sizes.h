#ifndef RR_PS1_SIZES_H
#define RR_PS1_SIZES_H

/* PS1 sizes of game structures whose host layout is larger (pointers, the
 * host's alignment). The original clears them with the BIOS bzero, whose
 * time goes by the byte (PsyZ BIOS_FILL_BYTE_CYCLES, 132 clocks): the host
 * clears its own structure and spends the PS1 call's time (PsyqBzeroPs1).
 * The values are the decomp's (its RR_ASSERT_SIZE checks and the sizes
 * clang gives its sources for a 32-bit MIPS target). */
enum {
    RR_PS1_RACE_VEHICLE_BYTES = 0x320,
    RR_PS1_CAMERA_MOTION_BYTES = 0x70,
    RR_PS1_PRESENTATION_CONTROLLER_BYTES = 0x188,
    RR_PS1_MOTION_TRACK_BYTES = 0x2C,
    RR_PS1_SCROLL_TRACK_BYTES = 0x28,
    RR_PS1_SCENE_CAMERA_STATE_BYTES = 0x160,
    RR_PS1_MENU_TEXT_STATE_BYTES = 0x348,
    RR_PS1_INTRO_SCENE_STATE_BYTES = 0xBC,
    RR_PS1_STR_PLAYBACK_STATE_BYTES = 0x4C,
    RR_PS1_MENU_GLYPH_ROW_BYTES = 0x1C,
};

#endif
