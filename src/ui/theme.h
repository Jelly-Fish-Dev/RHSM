#ifndef RHSM_UI_THEME_H
#define RHSM_UI_THEME_H

#include <SDL/SDL.h>

/* Screen size, font and colours shared by all screens. */

#define SCREEN_W 640
#define SCREEN_H 480

#define FONT_PATH       "res/fonts/Exo2-Bold.ttf"
#define FONT_SIZE_BIG   52
#define FONT_SIZE       26
#define FONT_SIZE_SMALL 19

/* text */
static const SDL_Color C_WHITE  = {240, 240, 245, 0};
static const SDL_Color C_DIM    = {140, 145, 160, 0};
static const SDL_Color C_DARK   = {20, 22, 30, 0};
static const SDL_Color C_ACCENT = {255, 196, 60, 0};

/* fills */
static const SDL_Color C_BG        = {22, 24, 34, 0};
static const SDL_Color C_PANEL     = {32, 35, 48, 0};
static const SDL_Color C_RULE      = {60, 64, 84, 0};
static const SDL_Color C_SCROLLBAR = {90, 96, 120, 0};
static const SDL_Color C_HIGHLIGHT = {255, 196, 60, 0};
static const SDL_Color C_DEBUG     = {120, 30, 30, 0};

#endif
