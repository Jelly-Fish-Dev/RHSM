#ifndef RHSM_UI_SCREEN_LIBRARY_H
#define RHSM_UI_SCREEN_LIBRARY_H

#include <SDL/SDL.h>

#include "model/library.h"
#include "platform/input.h"
#include "ui/gfx.h"

/* The main screen: scrollable game list on top, detail panel for the selected
 * game (save files + Onion box art) at the bottom. */

typedef struct {
    Library lib;      /* owned by the screen */
    char error[160];  /* shown instead of the list when non-empty */
    int sel;          /* selected game */
    int top;          /* first visible row */
    SDL_Surface *art; /* box art for art_for, already scaled to the art box */
    int art_for;      /* game index the art was loaded for, -1 = none yet */
} LibraryScreen;

typedef enum { SCREEN_STAY, SCREEN_EXIT } ScreenResult;

/* Takes ownership of *lib (it is zeroed). error may be NULL or "". */
void screen_library_init(LibraryScreen *scr, Library *lib, const char *error);
void screen_library_free(LibraryScreen *scr);

ScreenResult screen_library_handle(LibraryScreen *scr, Button btn);
void screen_library_render(LibraryScreen *scr, Gfx *gfx);

#endif
