#include "rr/ps1_sizes.h"
#include "rr/vehicle_table.h"
#include "game/car/car.h"
#include "game/car/carsetup.h"
#include "game/car/carlane.h"
#include "game/race/racesamp.h"
#include "game/race/raceinit.h"
#include "rr/bios_devices.h"
#include "rr/overlay667_course_initializer.h"
#include "rr/race_setup.h"
#include "rr/record_banks.h"
extern RaceVehicle PlayerCars[];
extern int TrackLength;
extern int TotalRaceTime;

void InitRaceCars(int a0)
{
    int s1, s2, i, t;
    RaceVehicle **s0;
    RaceVehicle *vehicle;
    PlayerCars[1].flags1E4 = 0;
    InitCarTable(a0);
    CarViewportBankOffset = 0;
    switch (a0) {
    case 0:
    default:
        i = 0;
        InitPlayerCar(PlayerCars, i, i);
        SetupRaceMemoryArena(0, 0);
        break;
    case 1:
        InitPlayerCar(PlayerCars, 0, 7);
        InitComCars(7);
        SetupRaceMemoryArena(0, 0);
        break;
    case 2:
        i = 0;
        InitPlayerCar(PlayerCars, i, i);
        i = 1;
        InitPlayerCar(PlayerCars + 1, i, i);
        SetupRaceMemoryArena(1, 0);
        break;
    case 3:
        InitPlayerCar(PlayerCars, 0, NumCars - 2);
        InitPlayerCar(PlayerCars + 1, 1, NumCars - 1);
        InitComCars(NumCars - 2);
        SetupRaceMemoryArena(1, 0);
        break;
    case 4:
        s1 = 0;
        s2 = 0;
        if (NumCars > 0) {
            do {
                s8 participant = LinkEntryState[s1 / 2][s1 % 2];
                s0 = &CarTable[s1];
                if (participant == 1) {
                    InitPlayerCar(*s0, s1, s2);
                    s2++;
                } else {
                    (*s0)->flags1E4 = 0;
                    vehicle = *s0;
                    t = TrackLength;
                    vehicle->half1E8 = -1;
                    (*s0)->first = -(t * 300);
                }
                s1++;
            } while (s1 < NumCars);
        }
        LinkPlayerCount = s2;
        SetupRaceMemoryArena(2, 0);
        break;
    case 5:
        PsyqBzeroPs1(PlayerCars, sizeof(PlayerCars[0]), RR_PS1_RACE_VEHICLE_BYTES);
        InitComCars(7);
        SetupRaceMemoryArena(1, 0);
        SavedRaceState = 2;
        break;
    case 6:
        InitPlayerCar(PlayerCars, 0, 1);
        InitComCars(1);
        SetupRaceMemoryArena(0, 0);
        break;
    }
    TotalRaceTime = 0;
}
