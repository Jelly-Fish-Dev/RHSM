#ifndef RHSM_UI_SYSTEM_NAMES_H
#define RHSM_UI_SYSTEM_NAMES_H

#include <string.h>

/* Display names for Onion's Roms/<SYSTEM> folder codes.
 *   name  - the full name
 *   short - compact name used where space is tight (the tabs). Leave it ""
 *           to use the full name there too.
 * Edit the name columns to rename a system. Codes not listed here are shown
 * as the code itself. */
static const struct {
    const char *code;  /* Onion folder name (as stored in the index) */
    const char *name;  /* full name */
    const char *short_name; /* "" = same as name */
} SYSTEM_NAMES[] = {
    /* code                  name                              short */
    /* Nintendo */
    {"FC",                  "Nintendo Entertainment System",  "NES"},
    {"FDS",                 "Famicom Disk System",            "FDS"},
    {"SFC",                 "Super Nintendo",                 "SNES"},
    {"GB",                  "Game Boy",                       "GB"},
    {"GBC",                 "Game Boy Color",                 "GBC"},
    {"GBA",                 "Game Boy Advance",               "GBA"},
    {"VB",                  "Virtual Boy",                    ""},
    {"POKE",                "Pokemon Mini",                   ""},
    /* Sega */
    {"MD",                  "Mega Drive",                     ""},
    {"MS",                  "Master System",                  ""},
    {"GG",                  "Game Gear",                      ""},
    {"SEGACD",              "Mega-CD",                        ""},
    {"THIRTYTWOX",          "Sega 32X",                       "32X"},
    {"SG1000",              "SG-1000",                        ""},
    /* NEC */
    {"PCE",                 "PC Engine",                      ""},
    {"PCECD",               "PC Engine CD",                   "PCE CD"},
    {"SGFX",                "SuperGrafx",                     ""},
    /* Sony */
    {"PS",                  "PlayStation",                    "PS1"},
    /* Arcade */
    {"ARCADE",              "Arcade",                         ""},
    {"NEOGEO",              "Neo Geo",                        ""},
    {"CPS1",                "Capcom CPS-1",                   "CPS-1"},
    {"CPS2",                "Capcom CPS-2",                   "CPS-2"},
    {"CPS3",                "Capcom CPS-3",                   "CPS-3"},
    /* Atari */
    {"ATARI",               "Atari 2600",                     "2600"},
    {"FIFTYTWOHUNDRED",     "Atari 5200",                     "5200"},
    {"SEVENTYEIGHTHUNDRED", "Atari 7800",                     "7800"},
    {"EIGHTHUNDRED",        "Atari 800",                      ""},
    {"LYNX",                "Atari Lynx",                     "Lynx"},
    /* Other */
    {"NGP",                 "Neo Geo Pocket",                 "NGP"},
    {"WS",                  "WonderSwan",                     ""},
    {"MSX",                 "MSX",                            ""},
    {"COLECO",              "ColecoVision",                   ""},
    {"COMMODORE",           "Commodore 64",                   "C64"},
    {"AMIGA",               "Amiga",                          ""},
    {"DOOM",                "Doom",                           ""},
    {"PICO",                "PICO-8",                         ""},
    {"SCUMMVM",             "ScummVM",                        ""},
    {"OTHER",               "Other",                          ""},
};

/* "SFC" -> "Super Nintendo"; unknown codes come back unchanged. */
static inline const char *system_display_name(const char *code)
{
    for (size_t i = 0; i < sizeof SYSTEM_NAMES / sizeof SYSTEM_NAMES[0]; i++) {
        if (strcmp(SYSTEM_NAMES[i].code, code) == 0)
            return SYSTEM_NAMES[i].name;
    }
    return code;
}

/* "SFC" -> "SNES"; falls back to the full name when the short name is blank,
 * and to the code itself for unknown systems. */
static inline const char *system_short_name(const char *code)
{
    for (size_t i = 0; i < sizeof SYSTEM_NAMES / sizeof SYSTEM_NAMES[0]; i++) {
        if (strcmp(SYSTEM_NAMES[i].code, code) == 0) {
            const char *s = SYSTEM_NAMES[i].short_name;
            return (s && s[0]) ? s : SYSTEM_NAMES[i].name;
        }
    }
    return code;
}

#endif
