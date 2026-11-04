#ifndef RHSM_SCAN_CORES_H
#define RHSM_SCAN_CORES_H

/* Maps a RetroArch core's save folder name (its library_name, e.g. "Snes9x",
 * "PicoDrive") to the Onion Roms/<SYSTEM> folders it can run.
 * Returns a comma-separated list, most likely system first, or NULL if unknown. */
const char *cores_systems_for(const char *core);

#endif
