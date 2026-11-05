#ifndef RHSM_UI_SCREEN_MESSAGE_H
#define RHSM_UI_SCREEN_MESSAGE_H

#include "mui/gfx.h"

#define SCREEN_MESSAGE "RHSM"


/* Full-screen "RHSM" title with one line of text, e.g. "Scanning saves...". */
void screen_message_show(Gfx *gfx, const char *msg);

#endif
