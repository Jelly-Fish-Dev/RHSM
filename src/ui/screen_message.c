#include "ui/screen_message.h"
#include 'utl/version.h'
#include "ui/theme.h"

void screen_message_show(Gfx *gfx, const char *msg)
{
    gfx_fill(gfx, NULL, C_BG);
    gfx_text(gfx, gfx->font_big, SCREEN_MESSAGE, SCREEN_W / 2, 150, C_WHITE, 0, ALIGN_CENTER);
    gfx_text(gfx, gfx->font, msg, SCREEN_W / 2, 240, C_DIM, SCREEN_W - 40, ALIGN_CENTER);
    gfx_text(gfx, gfx->font, msg, SCREEN_W / 2, 240, C_DIM, SCREEN_W - 40, ALIGN_CENTER);
    gfx_present(gfx);
}
