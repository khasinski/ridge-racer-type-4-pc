#include "rr/release.h"
#include "game/ui/uicnt.h"
#include "game/course/trkintp2.h"
#include "game/ui/uicode.h"
void EncodePasswordCode(u8 *a0, u8 *a1) {
    int s5, s2, s1, s0, t;
    unsigned char *s3, *v0, *tab, *p;
    /* The Japanese release's record password has five bytes (the PAL one
     * adds a language byte and random bits). */
    int n = RR_PAL_JP(6, 5);
    XorBytes37(a0, n);
    if (!RR_RELEASE_JP)
        a0[5] = a0[5] & 0xF;
    s5 = ChecksumBytes(a0, n);
    s2 = 0;
    s3 = a0;
    tab = PasswordCipherKey;
    s1 = ((s5 & 0xFF) % 11U) & 0xFF;
    do {
        v0 = tab + s1;
        s1++;
        p = s3 + s2;
        *p = AddOrSubtractByte(*p, *v0, 0);
        if (s1 >= 0xB) s1 = 0;
        s2++;
    } while (s2 < n);
    ByteToCodeChars(s5 & 0xFF, a1);
    s3 = a0;
    s2 = 0;
    s0 = 2;
    do {
        t = *s3;
        s3++;
        ByteToCodeChars(t, a1 + s0);
        s0 += 2;
        s2++;
    } while (s2 < n);
}
