#include "diagnostics_parse.h"

#include <psyz.h>
#include <psyz/input.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int Rr4DiagParseLong(const char *text, const char **end, int base, long *out) {
    char *stop;
    long value;

    if (!text || !*text || *text == ' ' || *text == '\t')
        return 0;
    errno = 0;
    value = strtol(text, &stop, base);
    if (stop == text || errno == ERANGE || (!end && *stop))
        return 0;
    if (end)
        *end = stop;
    *out = value;
    return 1;
}

/* An unsigned decimal component of a mode anchor. */
static int ParseDecimal(const char *text, const char **end, long *out) {
    return *text >= '0' && *text <= '9' && Rr4DiagParseLong(text, end, 10, out);
}

int Rr4DiagParseAnchor(const char *text, const char **end, int *mode, long *time) {
    long group, index = 0, delay = 0;
    const char *p;

    *mode = -1;
    if (*text != 'm')
        return Rr4DiagParseLong(text, end, 10, time);
    if (!ParseDecimal(text + 1, &p, &group))
        return 0;
    if (*p == '/' && !ParseDecimal(p + 1, &p, &index))
        return 0;
    if (*p == ':' && !Rr4DiagParseLong(p + 1, &p, 10, &delay))
        return 0;
    *mode = (int)((group << 8) | (index & 0xFF));
    *time = delay;
    if (end)
        *end = p;
    return end || !*p;
}

/* The next comma-separated item of a list: [*list, *item_end); *list moves
 * past the comma. Returns 0 at the end of the list. */
static int NextItem(const char **list, const char **item, size_t *length) {
    const char *comma;

    if (!*list || !**list)
        return 0;
    comma = strchr(*list, ',');
    *item = *list;
    *length = comma ? (size_t)(comma - *list) : strlen(*list);
    *list = comma ? comma + 1 : NULL;
    return 1;
}

/* Copies an item to a NUL-terminated buffer (truncated as before: 79 / 95
 * characters). */
static void CopyItem(char *out, size_t size, const char *item, size_t length) {
    if (length >= size)
        length = size - 1;
    memcpy(out, item, length);
    out[length] = '\0';
}

unsigned short Rr4DiagButtonMask(const char *name, size_t length) {
    static const struct {
        const char *name;
        unsigned short mask;
    } kButtons[] = {
        {"select", 0x0100}, {"l3", 0x0200}, {"r3", 0x0400}, {"start", 0x0800},
        {"up", 0x1000}, {"right", 0x2000}, {"down", 0x4000}, {"left", 0x8000},
        {"l2", 0x0001}, {"r2", 0x0002}, {"l1", 0x0004}, {"r1", 0x0008},
        {"triangle", 0x0010}, {"circle", 0x0020}, {"cross", 0x0040},
        {"square", 0x0080},
    };
    for (size_t i = 0; i < sizeof(kButtons) / sizeof(kButtons[0]); i++)
        if (strlen(kButtons[i].name) == length && !strncmp(kButtons[i].name, name, length))
            return kButtons[i].mask;
    return 0;
}

static int PadAxis(const char *name, size_t length) {
    static const char *const kNames[RR4_PAD_AXES] = {"lx", "ly", "rx", "ry", "steer", "i", "ii", "l"};
    for (int i = 0; i < RR4_PAD_AXES; i++)
        if (strlen(kNames[i]) == length && !strncmp(kNames[i], name, length))
            return i;
    return RR4_PAD_AXIS_NONE;
}

int Rr4DiagParsePadScript(const char *script, Rr4PadStep *steps, int max, int ports_used[2]) {
    const char *item;
    size_t length;
    int count = 0;

    while (count < max && NextItem(&script, &item, &length)) {
        char text[128];
        const char *at, *eq, *p;
        Rr4PadStep step;

        CopyItem(text, sizeof(text), item, length);
        p = text;
        step.port = 0;
        if ((p[0] == '1' || p[0] == '2') && p[1] == ':') {
            step.port = p[0] - '1';
            p += 2;
        }
        at = strchr(p, '@');
        if (!at) {
            fprintf(stderr, "RR4_PAD_SCRIPT: bad step \"%s\"\n", text);
            continue;
        }
        eq = memchr(p, '=', (size_t)(at - p));
        step.axis = RR4_PAD_AXIS_NONE;
        step.value = 0.0f;
        step.mask = 0;
        if (eq) {
            char *value_end;
            step.axis = PadAxis(p, (size_t)(eq - p));
            step.value = strtof(eq + 1, &value_end);
            if (value_end == eq + 1 || value_end != at) {
                fprintf(stderr, "RR4_PAD_SCRIPT: bad value in \"%s\"\n", text);
                continue;
            }
        } else {
            step.mask = Rr4DiagButtonMask(p, (size_t)(at - p));
        }
        if (!step.mask && step.axis == RR4_PAD_AXIS_NONE) {
            fprintf(stderr, "RR4_PAD_SCRIPT: unknown button in \"%.*s\"\n", (int)(at - p), p);
            continue;
        }
        step.count = 4;
        step.period = 0;
        if (!Rr4DiagParseAnchor(at + 1, &p, &step.mode, &step.first) ||
            (*p == '+' && !Rr4DiagParseLong(p + 1, &p, 10, &step.count)) ||
            (*p == '%' && !Rr4DiagParseLong(p + 1, &p, 10, &step.period)) || *p) {
            fprintf(stderr, "RR4_PAD_SCRIPT: bad step \"%s\"\n", text);
            continue;
        }
        steps[count++] = step;
        ports_used[step.port] = 1;
    }
    return count;
}

int Rr4DiagParseRanges(const char *list, int ranges, Rr4VsyncRange *out, int max, const char *what) {
    const char *item;
    size_t length;
    int count = 0;

    while (count < max && NextItem(&list, &item, &length)) {
        char text[96];
        const char *p;
        Rr4VsyncRange r;

        CopyItem(text, sizeof(text), item, length);
        r.step = 1;
        if (!Rr4DiagParseLong(text, &p, 10, &r.first)) {
            fprintf(stderr, "%s: bad entry \"%s\"\n", what, text);
            continue;
        }
        r.last = r.first;
        if (ranges && ((*p == '-' && !Rr4DiagParseLong(p + 1, &p, 10, &r.last)) ||
                       (*p == '/' && !Rr4DiagParseLong(p + 1, &p, 10, &r.step)))) {
            fprintf(stderr, "%s: bad entry \"%s\"\n", what, text);
            continue;
        }
        if (*p) {
            fprintf(stderr, "%s: bad entry \"%s\"\n", what, text);
            continue;
        }
        if (r.step <= 0)
            r.step = 1;
        out[count++] = r;
    }
    return count;
}

/* "name[+offset][:size]" in place; 0 for a bad offset or size. */
static int SplitTarget(char *text, long *offset, int *size) {
    char *colon = strchr(text, ':'), *plus;
    long value;

    *size = 4;
    *offset = 0;
    if (colon) {
        *colon = '\0';
        if (!Rr4DiagParseLong(colon + 1, NULL, 10, &value) || (value != 1 && value != 2 && value != 4))
            return 0;
        *size = (int)value;
    }
    plus = strchr(text, '+');
    if (plus) {
        *plus = '\0';
        if (!Rr4DiagParseLong(plus + 1, NULL, 0, offset))
            return 0;
    }
    return *text != '\0';
}

int Rr4DiagParseWatch(const char *list, Rr4WatchSpec *out, int max) {
    const char *item;
    size_t length;
    int count = 0;

    while (count < max && NextItem(&list, &item, &length)) {
        Rr4WatchSpec *w = &out[count];
        char text[80];
        char *colon;

        CopyItem(text, sizeof(text), item, length);
        snprintf(w->name, sizeof(w->name), "%s", text);
        colon = strchr(text, ':');
        if (colon)
            *colon = '\0';
        snprintf(w->label, sizeof(w->label), "%s", text);
        if (!SplitTarget(w->name, &w->offset, &w->size)) {
            fprintf(stderr, "RR4_WATCH: bad item %s\n", w->label);
            continue;
        }
        count++;
    }
    return count;
}

int Rr4DiagParsePoke(const char *list, Rr4PokeSpec *out, int max) {
    const char *item;
    size_t length;
    int count = 0;

    while (count < max && NextItem(&list, &item, &length)) {
        Rr4PokeSpec *k = &out[count];
        char text[96], *eq, *at;
        const char *value;

        CopyItem(text, sizeof(text), item, length);
        eq = strchr(text, '=');
        at = strrchr(text, '@');
        if (!eq || !at || at < eq) {
            fprintf(stderr, "RR4_POKE: bad item %s\n", text);
            continue;
        }
        *at = '\0';
        if (!Rr4DiagParseAnchor(at + 1, NULL, &k->mode, &k->vsync)) {
            fprintf(stderr, "RR4_POKE: bad time in %s@%s\n", text, at + 1);
            continue;
        }
        value = eq + 1;
        k->from[0] = '\0';
        k->from_offset = 0;
        k->value = 0;
        if (!Rr4DiagParseLong(value, NULL, 0, &k->value)) {
            /* a global read when applied: name[+offset] */
            char *plus;
            snprintf(k->from, sizeof(k->from), "%s", value);
            plus = strchr(k->from, '+');
            if (plus) {
                *plus = '\0';
                if (!Rr4DiagParseLong(plus + 1, NULL, 0, &k->from_offset)) {
                    fprintf(stderr, "RR4_POKE: bad value %s\n", value);
                    continue;
                }
            }
            if (!k->from[0]) {
                fprintf(stderr, "RR4_POKE: bad value %s\n", value);
                continue;
            }
        }
        k->add = eq > text && eq[-1] == '+';
        eq[k->add ? -1 : 0] = '\0';
        snprintf(k->name, sizeof(k->name), "%s", text);
        if (!SplitTarget(k->name, &k->offset, &k->size)) {
            fprintf(stderr, "RR4_POKE: bad item %s\n", text);
            continue;
        }
        if (!strcmp(k->name, "rand"))
            k->add = 0; /* the BIOS rand() state: store only */
        count++;
    }
    return count;
}

unsigned Rr4DiagParseGraphics(const char *list) {
    static const struct {
        const char *name;
        unsigned bit;
    } kOptions[] = {
        {"geometry", RR4_GRAPHICS_GEOMETRY}, {"nodither", RR4_GRAPHICS_NODITHER},
        {"aa", RR4_GRAPHICS_AA},             {"filter", RR4_GRAPHICS_FILTER},
        {"interpolation", RR4_GRAPHICS_INTERPOLATION}, {"wide", RR4_GRAPHICS_WIDE},
    };
    const char *item;
    size_t length;
    unsigned bits = 0;

    while (NextItem(&list, &item, &length)) {
        int found = 0;
        while (length && (*item == ' ' || *item == '\t'))
            item++, length--;
        while (length && (item[length - 1] == ' ' || item[length - 1] == '\t'))
            length--;
        if (!length)
            continue;
        for (size_t i = 0; i < sizeof(kOptions) / sizeof(kOptions[0]); i++) {
            if (strlen(kOptions[i].name) == length && !strncmp(kOptions[i].name, item, length)) {
                bits |= kOptions[i].bit;
                found = 1;
            }
        }
        if (!found)
            fprintf(stderr, "RR4_GRAPHICS: unknown option \"%.*s\"\n", (int)length, item);
    }
    return bits;
}

int Rr4DiagParsePadTypes(const char *list, int kinds[2]) {
    static const struct {
        const char *name;
        int kind;
    } kKinds[] = {{"digital", PSYZ_CTRL_DIGITAL_PAD}, {"dualshock", PSYZ_CTRL_ANALOG_PAD},
                  {"negcon", PSYZ_CTRL_NEGCON}, {"jogcon", PSYZ_CTRL_JOGCON}};
    const char *item;
    size_t length;
    int ports = 0;

    while (ports < 2 && NextItem(&list, &item, &length)) {
        kinds[ports] = PSYZ_CTRL_DIGITAL_PAD;
        for (size_t i = 0;; i++) {
            if (i == sizeof(kKinds) / sizeof(kKinds[0])) {
                fprintf(stderr, "RR4_PAD_TYPE: unknown controller \"%.*s\"\n", (int)length, item);
                break;
            }
            if (strlen(kKinds[i].name) == length && !strncmp(kKinds[i].name, item, length)) {
                kinds[ports] = kKinds[i].kind;
                break;
            }
        }
        ports++;
    }
    return ports;
}
