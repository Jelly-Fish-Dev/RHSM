#ifndef RHSM_UI_GFX_H
#define RHSM_UI_GFX_H

#include <stdbool.h>

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

/* Display, fonts and drawing primitives. Screens draw into gfx->screen and
 * call gfx_present() when a frame is done. */

typedef struct {
    SDL_Surface *video;  /* real display surface */
    SDL_Surface *screen; /* off-screen buffer screens draw into */
    TTF_Font *font_big;
    TTF_Font *font;
    TTF_Font *font_small;
} Gfx;

typedef enum { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT } TextAlign;

/* Opens the 640x480 display and loads the fonts. On failure, call gfx_quit(). */
bool gfx_init(Gfx *gfx);
void gfx_quit(Gfx *gfx);

void gfx_present(Gfx *gfx);

/* Fills rect (NULL = whole screen) with a colour. */
void gfx_fill(Gfx *gfx, const SDL_Rect *rect, SDL_Color color);

/* Draws a surface at x,y. */
void gfx_blit(Gfx *gfx, SDL_Surface *surf, int x, int y);

int gfx_text_width(TTF_Font *font, const char *text);

/* Draws text anchored at x (left / centre / right edge). If max_w > 0 and the
 * text is wider, it is cut on a UTF-8 boundary and ends with "...". */
void gfx_text(Gfx *gfx, TTF_Font *font, const char *text, int x, int y, SDL_Color color,
              int max_w, TextAlign align);

#endif
