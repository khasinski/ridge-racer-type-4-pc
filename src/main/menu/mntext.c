#include "rr/release.h"
#include "game/ui/uipart1.h"
#include "rr/menu_selection.h"
#include "rr/save_slots.h"
#include "game/data/bankrec.h"
/* RODATA 0x80010e20 0x7c */
#include "rr/menu_text_rows.h"
#include "rr/menu_state_types.h"
#include "rr/time_record_types.h"
#include "rr/string_tail_copy.h"
#include "rr/menu_choice.h"
#include "game/menu/mntext.h"

extern char StrStarSpacePrefix[];
extern char LapTimeTemplateStr[];
extern u8 *MenuTextTemplates[];

/* Template 7 reads " CAR&COURSE CHANGE" in the Japanese release. */
static const u8 *MenuTextTemplate(s32 index) {
    if (RR_RELEASE_JP && index == 7)
        return (const u8 *)" CAR&COURSE CHANGE";
    return MenuTextTemplates[index];
}
extern char *MenuTransmissionLabels[];
extern char *MenuBlankLabelStr;
extern s16 ReverseCourse;
extern TimeRecord16 RaceRecords[2][8][5];
extern TimeRecord16 LapRecords[2][8][5];
extern s32 TotalRaceTime;

s32 InitMenuTextRows(MenuTextRow60 *rows) {
    u32 size = MenuTextRowCount * 60U;
    MenuTextRows = rows;
    PsyqBzero(rows, size);
    return size;
}

void SetMenuTextRow(s32 row, s32 position, s32 phase, s32 style, s32 label) {
    s32 templateIndex;
    s32 formatKind;
    s32 column;
    s32 value;
    s32 suffixIndex;

    if (row < MenuTextRowCount) {
        templateIndex = label & 0xFF;
        formatKind = label >> 8;
        MenuTextRows[row].rendering.phase = phase;
        column = 0;
        MenuTextRows[row].rendering.style = style;
        MenuTextRows[row].rendering.x = 0x11;
        MenuTextRows[row].rendering.y = (s16)position * 28 + 0x60;
        MenuTextRows[row].rendering.clut = 0x2638;
        if (label >= 0) {
            while (MenuTextTemplate(templateIndex)[column] != 0) {
                MenuTextRows[row].rendering.text[column] = MenuTextTemplate(templateIndex)[column];
                column++;
            }
        } else {
            for (; column < 0x16; column++) {
                MenuTextRows[row].rendering.text[column] = 0;
            }
            return;
        }
        switch (formatKind) {
        case 1:
        case 2:
        case 3:
            FormatDecimalDigits((char *)&MenuTextRows[row].rendering.text[4], formatKind, 0);
        case 4:
            FormatRaceTime((char *)&MenuTextRows[row].rendering.text[7], TotalRaceTime);
            break;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
            value = (formatKind - 5) % 5;
            FormatDecimalDigits((char *)&MenuTextRows[row].rendering.text[0], value + 1, 0);
            suffixIndex = value;
            if (value >= 3) {
                suffixIndex = 3;
            }
            CopyStringClearTail((char *)&MenuTextRows[row].rendering.text[1], MenuTransmissionLabels[suffixIndex]);
            CopyStringClearTail((char *)&MenuTextRows[row].rendering.text[4], LapTimeTemplateStr);
            if (formatKind < 10) {
                CopyStringClearTail((char *)&MenuTextRows[row].rendering.text[14], LapRecords[ReverseCourse][CourseIndex][value].name);
                FormatRaceTime((char *)&MenuTextRows[row].rendering.text[4], LapRecords[ReverseCourse][CourseIndex][value].time);
            } else {
                CopyStringClearTail((char *)&MenuTextRows[row].rendering.text[14], RaceRecords[ReverseCourse][CourseIndex][value].name);
                FormatRaceTime((char *)&MenuTextRows[row].rendering.text[4], RaceRecords[ReverseCourse][CourseIndex][value].time);
            }
            break;
        case 15:
            value = CarRecordPtrs[0]->stage1F3;
            FormatDecimalDigits((char *)&MenuTextRows[row].rendering.text[12], value + 1, 0);
            break;
        case 16:
            FormatDecimalDigits((char *)&MenuTextRows[row].rendering.text[8], SelectionState.fields.retries08[SelectionState.fields.phase17], 0);
            break;
        case 17:
            value = SelectionState.fields.result19;
            CopyStringClearTail((char *)&MenuTextRows[row].rendering.text[0], StrStarSpacePrefix);
            if (value == 0) {
                value = 1;
            }
            if (value < 9) {
                FormatDecimalDigits((char *)&MenuTextRows[row].rendering.text[1], value, 0);
                value--;
                if (value >= 3) {
                    value = 3;
                }
                CopyStringClearTail((char *)&MenuTextRows[row].rendering.text[2], MenuTransmissionLabels[value]);
            } else {
                CopyStringClearTail((char *)&MenuTextRows[row].rendering.text[1], MenuBlankLabelStr);
            }
            break;
        case 18:
        case 19:
        case 20:
        case 21:
            value = formatKind - 18;
            FormatDecimalDigits((char *)&MenuTextRows[row].rendering.text[1], value + 1, 0);
            if (value >= 3) {
                value = 3;
            }
            CopyStringClearTail((char *)&MenuTextRows[row].rendering.text[2], MenuTransmissionLabels[value]);
            break;
        case 30:
            MenuTextRows[row].rendering.clut = 0x2639;
            MenuTextRows[row].rendering.flags = 3;
            break;
        case 31:
            MenuTextRows[row].rendering.clut = 0x2639;
            MenuTextRows[row].rendering.flags = 2;
            MenuTextRows[row].rendering.text[0] = MenuTextTemplate(templateIndex)[0];
            MenuTextRows[row].rendering.text[1] = 0;
            break;
        }
    }
}

/* Initial values of the globals this module owns. */
u8 D_80010B84[106] = {32, 69, 78, 68, 32, 76, 73, 78, 75, 32, 66, 65, 84, 84, 76, 69, 32, 0, 0, 0, 32, 69, 78, 68, 32, 86, 83, 32, 66, 65, 84, 84, 76, 69, 32, 0, 0, 0, 0, 0, 42, 84, 82, 89, 32, 65, 71, 65, 73, 78, 33, 32, 0, 0, 0, 0, 42, 68, 79, 78, 39, 84, 32, 71, 73, 86, 69, 32, 85, 80, 32, 89, 69, 84, 46, 32, 0, 0, 0, 0, 42, 89, 79, 85, 32, 66, 76, 69, 87, 32, 73, 84, 32, 0, 0, 0, 42, 84, 79, 79, 32, 66, 65, 68, 32};
char StrStarSpacePrefix[540] = {42, 32, 0, 0, 42, 80, 76, 65, 89, 69, 82, 32, 0, 0, 0, 0, 42, 80, 76, 65, 89, 69, 82, 32, 49, 0, 0, 0, 32, 69, 78, 68, 32, 71, 82, 65, 78, 68, 32, 80, 82, 73, 88, 32, 0, 0, 0, 0, 42, 89, 79, 85, 32, 71, 79, 84, 32, 65, 78, 32, 69, 88, 84, 82, 65, 32, 67, 65, 82, 33, 0, 0, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 0, 0, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 0, 0, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 0, 0, 42, 89, 79, 85, 32, 76, 79, 83, 84, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 0, 0, 42, 89, 79, 85, 32, 70, 65, 73, 76, 69, 68, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 0, 0, 42, 80, 76, 69, 65, 83, 69, 32, 83, 69, 76, 69, 67, 84, 0, 0, 49, 115, 116, 32, 48, 48, 39, 48, 48, 34, 48, 48, 48, 32, 32, 0, 0, 0, 0, 0, 42, 83, 84, 65, 71, 69, 32, 49, 32, 32, 0, 0, 42, 84, 79, 84, 65, 76, 32, 48, 48, 39, 48, 48, 34, 48, 48, 48, 32, 32, 0, 0, 42, 76, 65, 80, 49, 32, 32, 48, 48, 39, 48, 48, 34, 48, 48, 48, 32, 32, 0, 0, 42, 32, 67, 65, 82, 32, 83, 84, 65, 71, 69, 32, 49, 32, 91, 93, 0, 0, 0, 0, 42, 67, 72, 65, 78, 67, 69, 32, 48, 0, 0, 0, 42, 80, 65, 83, 83, 87, 79, 82, 68, 32, 0, 0, 32, 69, 88, 73, 84, 32, 0, 0, 32, 67, 72, 65, 78, 71, 69, 32, 84, 82, 73, 65, 76, 0, 0, 0, 32, 67, 65, 82, 32, 38, 32, 67, 79, 85, 82, 83, 69, 32, 67, 72, 65, 78, 71, 69, 0, 0, 0, 0, 32, 82, 69, 84, 82, 89, 0, 0, 42, 89, 79, 85, 32, 87, 79, 78, 33, 33, 33, 0, 0, 0, 0, 0, 42, 89, 79, 85, 32, 81, 85, 65, 76, 73, 70, 73, 69, 68, 33, 32, 32, 32, 32, 0, 0, 0, 0, 0, 42, 67, 79, 78, 71, 82, 65, 84, 85, 76, 65, 84, 73, 79, 78, 83, 33, 33, 33, 0, 0, 0, 0, 0, 42, 84, 79, 84, 65, 76, 32, 84, 73, 77, 69, 32, 82, 69, 67, 79, 82, 68, 83, 0, 0, 0, 0, 0, 42, 76, 65, 80, 32, 84, 73, 77, 69, 32, 82, 69, 67, 79, 82, 68, 83, 0, 0, 0, 42, 82, 69, 83, 85, 76, 84, 0, 0, 0, 0, 0, 82, 69, 84, 73, 82, 69, 0, 0, 116, 104, 32, 0, 0, 0, 0, 0, 114, 100, 32, 0, 0, 0, 0, 0, 110, 100, 32, 0, 0, 0, 0, 0, 115, 116, 32};
char LapTimeTemplateStr[144] = {48, 48, 39, 48, 48, 34, 48, 48, 48, 32, 32};
u8 *MenuTextTemplates[34] = {(unsigned char *)&StrStarSpacePrefix[488], (unsigned char *)&StrStarSpacePrefix[468], (unsigned char *)&StrStarSpacePrefix[444], (unsigned char *)&StrStarSpacePrefix[420], (unsigned char *)&StrStarSpacePrefix[396], (unsigned char *)&StrStarSpacePrefix[380], (unsigned char *)&StrStarSpacePrefix[372], (unsigned char *)&StrStarSpacePrefix[348], (unsigned char *)&StrStarSpacePrefix[332], (unsigned char *)&StrStarSpacePrefix[324], (unsigned char *)&StrStarSpacePrefix[312], (unsigned char *)&StrStarSpacePrefix[300], (unsigned char *)&StrStarSpacePrefix[280], (unsigned char *)&StrStarSpacePrefix[260], (unsigned char *)&StrStarSpacePrefix[240], (unsigned char *)&StrStarSpacePrefix[228], (unsigned char *)&StrStarSpacePrefix[208], (unsigned char *)&StrStarSpacePrefix[192], (unsigned char *)&StrStarSpacePrefix[168], (unsigned char *)&StrStarSpacePrefix[144], (unsigned char *)&StrStarSpacePrefix[120], (unsigned char *)&StrStarSpacePrefix[96], (unsigned char *)&StrStarSpacePrefix[72], (unsigned char *)&StrStarSpacePrefix[48], (unsigned char *)&StrStarSpacePrefix[28], (unsigned char *)&StrStarSpacePrefix[16], (unsigned char *)&StrStarSpacePrefix[4], (unsigned char *)&StrStarSpacePrefix[0], (unsigned char *)&D_80010B84[96], (unsigned char *)&D_80010B84[80], (unsigned char *)&D_80010B84[56], (unsigned char *)&D_80010B84[40], (unsigned char *)&D_80010B84[20], (unsigned char *)D_80010B84};
char *MenuTransmissionLabels[4] = {(char *)&StrStarSpacePrefix[532], (char *)&StrStarSpacePrefix[524], (char *)&StrStarSpacePrefix[516], (char *)&StrStarSpacePrefix[508]};
char *MenuBlankLabelStr = (char *)&StrStarSpacePrefix[500];
s32 MenuTextRowCount;
MenuTextRow60 *MenuTextRows;
