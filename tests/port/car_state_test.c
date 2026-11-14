/* Regression for native car records growing past the PS1 array size.
 * Entering a mode with no opponents must clear every byte of every slot,
 * including the last car and the tail of its pointer-bearing control. */
#include <stdio.h>
#include <string.h>
#include "rr/player_cars.h"
#include "rr/scene_camera_state.h"
#include "rr/menu_setup.h"
#include "rr/race_setup.h"
#include "game/car/carlane.h"

int main(void) {
    static RaceSpecEntry30 specs[11];
    RaceContext.carSpecs540 = specs;
    GameModeType = 0;
    SelectedTeamIndex = 0;
    for (int pass = 0; pass < 2; pass++) {
        memset(ComCars, pass ? 0x5A : 0xA5, sizeof ComCars);
        InitComCars(0);
        const unsigned char *bytes = (const unsigned char *)ComCars;
        for (size_t i = 0; i < sizeof ComCars; i++) {
            if (bytes[i]) {
                fprintf(stderr, "Opponent state survived reset at byte %zu of %zu\n", i, sizeof ComCars);
                return 1;
            }
        }
    }
    return 0;
}
