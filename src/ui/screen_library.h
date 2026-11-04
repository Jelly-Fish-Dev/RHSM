#ifndef RHSM_UI_SCREEN_LIBRARY_H
#define RHSM_UI_SCREEN_LIBRARY_H

#include <SDL/SDL.h>

#include "model/library.h"
#include "platform/input.h"
#include "ui/gfx.h"

/* The main screen: a strip of system tabs (All, GB, PS, SFC, ...), the game
 * list for the current tab, and a detail panel for the selected game (save
 * files + Onion box art). L2/R2 switch tabs. */

#define LIBRARY_MAX_TABS 48

typedef struct {
    char code[16]; /* Onion system folder name ("" for the All tab); the shown
                      name comes from ui/system_names.h */
    int count;     /* games in this tab */
} LibraryTab;

typedef struct {
    Library lib;      /* owned by the screen */
    char error[160];  /* shown instead of the list when non-empty */

    LibraryTab tabs[LIBRARY_MAX_TABS]; /* tabs[0] is "All", the rest sorted by label */
    int tab_count;
    int tab;          /* current tab */
    int *visible;     /* indexes into lib.games for the current tab */
    int visible_count;

    int sel;          /* selected row, an index into visible */
    int top;          /* first visible row */
    SDL_Surface *art; /* box art for art_for, already scaled to the art box */
    int art_for;      /* lib.games index the art was loaded for, -1 = none yet */
} LibraryScreen;

typedef enum { SCREEN_STAY, SCREEN_EXIT } ScreenResult;

/* Takes ownership of *lib (it is zeroed). error may be NULL or "". */
void screen_library_init(LibraryScreen *scr, Library *lib, const char *error);
void screen_library_free(LibraryScreen *scr);

ScreenResult screen_library_handle(LibraryScreen *scr, Button btn);
void screen_library_render(LibraryScreen *scr, Gfx *gfx);

#endif
