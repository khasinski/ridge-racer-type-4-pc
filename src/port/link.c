#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE /* setenv under -std=c11 */
#endif
#include "link.h"

#include <psyz.h>
#include <psyz/link.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef RR4_VERSION
#define RR4_VERSION "dev"
#endif

static int s_ready;
static char s_identity[96];
static int s_mode;
static unsigned s_port = RR4_LINK_DEFAULT_PORT;
static char s_address[RR4_LINK_ADDRESS_MAX];

static void SetEnvDefault(const char *name, const char *value) {
    if (getenv(name))
        return;
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

static const char *Override(void) {
    const char *spec = getenv("RR4_LINK");
    return spec && *spec ? spec : NULL;
}

void Rr4LinkPrepareEnv(void) {
    const char *headless = getenv("PSYZ_HEADLESS");
    if (!Override() || !headless || !*headless)
        return;
    SetEnvDefault("PSYZ_AUDIO_CLOCK", "wall");
    SetEnvDefault("PSYZ_HEADLESS_PACED", "1");
    if (strcmp(headless, "gpu"))
        fprintf(stderr, "RR4_LINK: PSYZ_HEADLESS=%s runs unpaced; use PSYZ_HEADLESS=gpu for a link\n",
                headless);
}

int Rr4LinkParse(const char *spec, int *mode, char *address, size_t size, unsigned *port) {
    const char *rest;
    char *end;
    *port = RR4_LINK_DEFAULT_PORT;
    if (size)
        address[0] = '\0';
    if (!strcmp(spec, "off")) {
        *mode = RR4_LINK_OFF;
        return 1;
    }
    if (!strncmp(spec, "host", 4) && (spec[4] == '\0' || spec[4] == ':')) {
        *mode = RR4_LINK_HOST;
        if (spec[4] == ':') {
            unsigned long v = strtoul(spec + 5, &end, 10);
            if (*end || v == 0 || v > 65535)
                return 0;
            *port = (unsigned)v;
        }
        return 1;
    }
    if (strncmp(spec, "join:", 5) || !spec[5])
        return 0;
    *mode = RR4_LINK_JOIN;
    rest = spec + 5;
    {
        const char *colon = NULL;
        size_t len;
        if (*rest == '[') { /* [v6 address]:port */
            const char *close = strchr(rest, ']');
            if (!close)
                return 0;
            len = (size_t)(close - rest - 1);
            if (len + 1 > size)
                return 0;
            memcpy(address, rest + 1, len);
            address[len] = '\0';
            colon = close[1] == ':' ? close + 1 : NULL;
            if (close[1] && !colon)
                return 0;
        } else {
            colon = strchr(rest, ':');
            if (colon && strchr(colon + 1, ':'))
                colon = NULL; /* a bare v6 address */
            len = colon ? (size_t)(colon - rest) : strlen(rest);
            if (!len || len + 1 > size)
                return 0;
            memcpy(address, rest, len);
            address[len] = '\0';
        }
        if (colon) {
            unsigned long v = strtoul(colon + 1, &end, 10);
            if (*end || v == 0 || v > 65535)
                return 0;
            *port = (unsigned)v;
        }
    }
    return address[0] != '\0';
}

static void Configure(void) {
    if (!s_ready)
        return;
    if (Psyz_LinkConfigure((PsyzLinkMode)s_mode, s_address, s_port, s_identity) != 0)
        Psyz_LinkConfigure(PSYZ_LINK_OFF, "", RR4_LINK_DEFAULT_PORT, s_identity);
}

static void FromPrefs(const Rr4Prefs *p) {
    s_mode = p->link_mode >= RR4_LINK_OFF && p->link_mode <= RR4_LINK_JOIN ? p->link_mode : RR4_LINK_OFF;
    s_port = p->link_port >= 1 && p->link_port <= 65535 ? p->link_port : RR4_LINK_DEFAULT_PORT;
    snprintf(s_address, sizeof(s_address), "%s", p->link_address);
    if (s_mode == RR4_LINK_JOIN && !s_address[0])
        s_mode = RR4_LINK_OFF; /* nowhere to go yet */
}

void Rr4LinkApply(const Rr4Prefs *prefs) {
    if (Override())
        return;
    FromPrefs(prefs);
    Configure();
}

void Rr4LinkStart(const Rr4Prefs *prefs, int ntsc, const char *release) {
    const char *spec = Override();
    const char *delay = getenv("RR4_LINK_DELAY");
    /* The game's lockstep needs the same code and field rate on both ends. */
    snprintf(s_identity, sizeof(s_identity), "Ridge Racer Type 4 PC %s, %s, %s video timing", RR4_VERSION,
             release && *release ? release : "PAL", ntsc ? "NTSC" : "PAL");
    s_ready = 1;
    if (delay && *delay)
        Psyz_LinkSetDelay((unsigned)strtoul(delay, NULL, 10));
    if (spec) {
        if (!Rr4LinkParse(spec, &s_mode, s_address, sizeof(s_address), &s_port)) {
            fprintf(stderr, "RR4_LINK: expected host[:port], join:address[:port] or off, not \"%s\"\n", spec);
            s_mode = RR4_LINK_OFF;
        }
    } else {
        FromPrefs(prefs);
    }
    Configure();
}

void Rr4LinkStatus(char *brief, size_t brief_size, char *text, size_t size) {
    unsigned rtt = 0;
    PsyzLinkState state = Psyz_LinkStatus(text, size);
    Psyz_LinkStats(&rtt, NULL, NULL);
    switch (state) {
    case PSYZ_LINK_CONNECTED:
        if (rtt)
            snprintf(brief, brief_size, "Connected, %.1f ms", rtt / 1000.0);
        else
            snprintf(brief, brief_size, "Connected");
        break;
    case PSYZ_LINK_WAITING:
        snprintf(brief, brief_size, "%s", s_mode == RR4_LINK_JOIN ? "Connecting" : "Waiting");
        break;
    case PSYZ_LINK_ERROR:
        snprintf(brief, brief_size, "Error");
        break;
    default:
        snprintf(brief, brief_size, "Off");
        break;
    }
    if (Override()) {
        size_t n = strlen(text);
        snprintf(text + n, size > n ? size - n : 0, " (RR4_LINK overrides this page)");
    }
}
