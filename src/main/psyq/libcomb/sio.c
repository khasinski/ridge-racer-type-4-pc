/* The game links the link cable driver under its early libsio names; they
 * are PsyZ libcomb (AddCOMB/_comb_control), the same driver on a serial
 * port whose cable runs over the network (psyz/link.h), or to nothing. */
#include <libcomb.h>
#include "psyq/libsio/siostart.h"
#include "rr/driver_control.h"

void AddSIO(void) { AddCOMB(); }

s32 _sio_control(s32 command, s32 operation, uintptr_t argument) {
    return (s32)_comb_control((u32)command, (u32)operation, argument);
}
