#ifndef RHSM_UI_IMAGE_H
#define RHSM_UI_IMAGE_H

#include <SDL/SDL.h>

/* Loads an image (PNG/JPG via SDL_image) and scales it to fit inside
 * max_w x max_h, keeping its aspect ratio. Returns a 32-bit ARGB surface with
 * per-pixel alpha (blits with blending), or NULL if the file can't be loaded. */
SDL_Surface *image_load_fit(const char *path, int max_w, int max_h);

/* Onion's box art location: <sdcard>/Roms/<system>/Imgs/<rom_name>.png */
void image_boxart_path(char *out, size_t n, const char *system, const char *rom_name);

#endif
