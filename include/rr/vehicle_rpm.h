#ifndef RR_VEHICLE_RPM_H
#define RR_VEHICLE_RPM_H
#include "game/data/menuvars.h"
#include "rr/vehicle_drive.h"
extern s16 SelectedMakerIndex;
/* gear68 is nonnegative and below gearCountFC; each threshold index used by
 * that gear lies in0..5. A zero ordinary span takes the minimum-RPM path. The
 * special mode requires a nonzero low16 divisor after max-speed rounding.
 * Products wrap32 before signed division, steering magnitude is computed wide,
 * and the final halfword stores preserve the original RPM/latch wrapping.
 * The full canonical record is accessed through the GNU aliasable drive view. */
void UpdateEngineRpm(RaceVehicle *vehicle);
#endif
