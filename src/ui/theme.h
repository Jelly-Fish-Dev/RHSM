#ifndef RHSM_UI_THEME_H
#define RHSM_UI_THEME_H

#include "mui/theme.h"

/* RHSM's colours on top of the shared Miyoo-UI theme (screen size, font,
 * C_WHITE / C_DIM / C_BG / C_PANEL / C_HIGHLIGHT ...). */

static const SDL_Color C_SCROLLBAR  = {90, 96, 120, 0};
static const SDL_Color C_TAB_BORDER = {72, 76, 96, 0}; /* faint | between tabs */
static const SDL_Color C_DEBUG      = {120, 30, 30, 0};

#endif
