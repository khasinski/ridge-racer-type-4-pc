#include "rr/cpu_time.h"
#include "game/data/cardini.h"
#include "game/menu/menutra21.h"
#include "game/data/cardchk.h"
extern int CardPortCheckStateA[];
extern int CardPortCheckStateB[];
extern unsigned char CardCheckCounts[];
extern unsigned char CardFileCounts[];

void PollCardStatus(int a0, int a1, int a2) {
    RR_CPU_CYCLES(RR_CYCLES_POLL_CARD_STATUS);
    int r, flag, k, s3, t, v;
    int i;
    unsigned char *p0, *p1, *pp, *b0;

    k = a0 & 0xFF;
    r = CardUpdate(k, a1 & 0xFF);
    CardPortCheckStateA[k] = r;
    if (r != 0) {
        CardPortCheckStateB[k] = r;
        switch (r) {
        case -2: CardCheckCounts[0]++; break;
        case -1: CardCheckCounts[1]++; break;
        case 1: CardCheckCounts[2]++; break;
        case 2: CardCheckCounts[3] += 3; break;
        case -3: CardCheckCounts[4]++; break;
        case 3: CardCheckCounts[5]++; break;
        case 4: CardCheckCounts[6] += 3; break;
        case -4: CardCheckCounts[7]++; break;
        default: CardCheckCounts[8]++; break;
        }
    } else {
        CardPortCheckStateB[k] = 0;
    }
    i = 0;
    flag = 0;
    b0 = CardPortPollStatus;
    s3 = a0 & 0xFF;
    p0 = b0 + s3;
    p1 = CardPortUnformatted + s3;
loop:
    if (CardCheckCounts[i] >= 3) {
        switch (i) {
        case 0:
            *p0 = 0x14; *p1 = 0; flag = 1; break;
        case 1:
            *p0 = 0x1E; *p1 = 0; flag = 1; break;
        case 2:
            if (*p0 >= 0x10) {
                CardFileCounts[s3] = CountCardFiles(s3, a1 & 0xFF);
                *p0 = 0xF - CardFreeBlocks(CardFileCounts[s3]);
                *p1 = 0;
            }
            flag = 1; break;
        case 3:
            if (*p0 >= 0x10) { *p0 = 0x32; *p1 = 0; }
            flag = 1; break;
        case 4:
            *p0 = 0x14; *p1 = 1; flag = 1; break;
        case 5:
            if (*p0 >= 0x10) {
                CardFileCounts[s3] = CountCardFiles(s3, a1 & 0xFF);
                *p0 = 0xF - CardFreeBlocks(CardFileCounts[s3]);
                *p1 = 1;
            }
            flag = 1; break;
        case 6:
            if (*p0 >= 0x10) { *p0 = 0x32; *p1 = 1; }
            flag = 1; break;
        case 7:
            if (*p0 >= 0x10) { *p0 = 0x28; *p1 = 0; }
            flag = 1; break;
        case 8:
            if (*p0 >= 0x10) { *p0 = 0x3C; *p1 = 0; }
            flag = 1; break;
        }
    }
next:
    i++;
    if (i < 9) goto loop;
    if (flag == 1 || (short)a2 == 1) {
        i = 8;
        pp = CardCheckCounts + i;
        do {
            *pp-- = 0;
        } while (--i >= 0);
    }
}

/* Initial values of the globals this module owns. */
unsigned char CardCheckCounts[12];
unsigned char CardFileCounts[4];
int CardPortCheckStateA[2];
int CardPortCheckStateB[2];
