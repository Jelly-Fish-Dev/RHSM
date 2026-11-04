#include "scan/cores.h"

#include <string.h>

/* Folder names are RetroArch core library names as Onion's RetroArch writes
 * them under Saves/CurrentProfile/{saves,states}/. Systems are Onion's
 * Roms/<SYSTEM> folder names. Will move to spec/cores.json (shared with the
 * desktop companion) in a later milestone. */
static const struct {
    const char *core;
    const char *systems;
} CORES[] = {
    /* Nintendo */
    {"FCEUmm", "FC,FDS"},
    {"Nestopia", "FC,FDS"},
    {"QuickNES", "FC"},
    {"Mesen", "FC,FDS"},
    {"Snes9x", "SFC"},
    {"Snes9x 2002", "SFC"},
    {"Snes9x 2005", "SFC"},
    {"Snes9x 2005 Plus", "SFC"},
    {"Supafaust", "SFC"},
    {"Mednafen Supafaust", "SFC"},
    {"Gambatte", "GB,GBC"},
    {"Gearboy", "GB,GBC"},
    {"TGB Dual", "GB,GBC"},
    {"SameBoy", "GB,GBC"},
    {"mGBA", "GBA,GB,GBC"},
    {"gpSP", "GBA"},
    {"VBA-M", "GBA"},
    {"VBA Next", "GBA"},
    {"Beetle VB", "VB"},
    {"PokeMini", "POKE"},
    /* Sega */
    {"Genesis Plus GX", "MD,MS,GG,SEGACD,SG1000"},
    {"PicoDrive", "MD,MS,SEGACD,THIRTYTWOX"},
    {"SMS Plus GX", "MS,GG"},
    {"Gearsystem", "MS,GG,SG1000"},
    /* NEC */
    {"Beetle PCE Fast", "PCE,PCECD"},
    {"Beetle PCE", "PCE,PCECD"},
    {"Beetle SuperGrafx", "SGFX,PCE"},
    /* Sony */
    {"PCSX-ReARMed", "PS"},
    {"Beetle PSX", "PS"},
    {"DuckStation", "PS"},
    /* Arcade */
    {"FinalBurn Neo", "ARCADE,NEOGEO,CPS1,CPS2,CPS3"},
    {"FB Alpha 2012", "ARCADE,NEOGEO"},
    {"MAME 2000", "ARCADE"},
    {"MAME 2003", "ARCADE"},
    {"MAME 2003-Plus", "ARCADE"},
    /* Atari */
    {"Stella 2014", "ATARI"},
    {"Stella", "ATARI"},
    {"ProSystem", "SEVENTYEIGHTHUNDRED"},
    {"Handy", "LYNX"},
    {"Beetle Lynx", "LYNX"},
    {"Atari800", "FIFTYTWOHUNDRED,EIGHTHUNDRED"},
    /* Other handhelds / computers */
    {"Beetle NeoPop", "NGP"},
    {"RACE", "NGP"},
    {"Beetle WonderSwan", "WS"},
    {"blueMSX", "MSX"},
    {"fMSX", "MSX"},
    {"Gearcoleco", "COLECO"},
    {"VICE x64", "COMMODORE"},
    {"PUAE", "AMIGA"},
    {"PrBoom", "DOOM"},
};

const char *cores_systems_for(const char *core)
{
    for (size_t i = 0; i < sizeof CORES / sizeof CORES[0]; i++) {
        if (strcmp(CORES[i].core, core) == 0)
            return CORES[i].systems;
    }
    return NULL;
}
