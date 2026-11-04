#include "ui/gfx.h"

#include <stdio.h>
#include <string.h>

#include "platform/log.h"
#include "ui/theme.h"

bool gfx_init(Gfx *gfx)
{
    memset(gfx, 0, sizeof *gfx);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    SDL_ShowCursor(SDL_DISABLE);
    SDL_EnableKeyRepeat(300, 60);

    /* Same pattern as Onion's own apps: draw into a buffer, blit to the video surface. */
    gfx->video = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 32, SDL_HWSURFACE);
    if (!gfx->video) {
        LOG_ERROR("SDL_SetVideoMode failed: %s", SDL_GetError());
        return false;
    }
    gfx->screen = SDL_CreateRGBSurface(SDL_HWSURFACE, SCREEN_W, SCREEN_H, 32, 0, 0, 0, 0);
    if (!gfx->screen) {
        LOG_ERROR("SDL_CreateRGBSurface failed: %s", SDL_GetError());
        return false;
    }

    if (TTF_Init() != 0) {
        LOG_ERROR("TTF_Init failed: %s", TTF_GetError());
        return false;
    }
    gfx->font_big = TTF_OpenFont(FONT_PATH, FONT_SIZE_BIG);
    gfx->font = TTF_OpenFont(FONT_PATH, FONT_SIZE);
    gfx->font_small = TTF_OpenFont(FONT_PATH, FONT_SIZE_SMALL);
    if (!gfx->font_big || !gfx->font || !gfx->font_small) {
        LOG_ERROR("TTF_OpenFont(%s) failed: %s", FONT_PATH, TTF_GetError());
        return false;
    }

    LOG_INFO("video %dx%d %dbpp", gfx->video->w, gfx->video->h,
             gfx->video->format->BitsPerPixel);
    return true;
}

void gfx_quit(Gfx *gfx)
{
    if (gfx->font_small) TTF_CloseFont(gfx->font_small);
    if (gfx->font) TTF_CloseFont(gfx->font);
    if (gfx->font_big) TTF_CloseFont(gfx->font_big);
    if (TTF_WasInit()) TTF_Quit();
    if (gfx->screen) SDL_FreeSurface(gfx->screen);
    SDL_Quit();
    memset(gfx, 0, sizeof *gfx);
}

void gfx_present(Gfx *gfx)
{
    SDL_BlitSurface(gfx->screen, NULL, gfx->video, NULL);
    SDL_Flip(gfx->video);
}

void gfx_fill(Gfx *gfx, const SDL_Rect *rect, SDL_Color c)
{
    SDL_Rect r;
    if (rect)
        r = *rect; /* SDL_FillRect may clip the rect it is given */
    SDL_FillRect(gfx->screen, rect ? &r : NULL, SDL_MapRGB(gfx->screen->format, c.r, c.g, c.b));
}

void gfx_blit(Gfx *gfx, SDL_Surface *surf, int x, int y)
{
    SDL_Rect pos = {(Sint16)x, (Sint16)y, 0, 0};
    SDL_BlitSurface(surf, NULL, gfx->screen, &pos);
}

int gfx_text_width(TTF_Font *font, const char *text)
{
    int w = 0, h = 0;
    TTF_SizeUTF8(font, text, &w, &h);
    return w;
}

void gfx_text(Gfx *gfx, TTF_Font *font, const char *text, int x, int y, SDL_Color color,
              int max_w, TextAlign align)
{
    char buf[320];
    snprintf(buf, sizeof buf, "%s", text);

    if (max_w > 0 && gfx_text_width(font, buf) > max_w) {
        size_t len = strlen(buf);
        while (len > 0) {
            do {
                len--;
            } while (len > 0 && ((unsigned char)buf[len] & 0xC0) == 0x80);
            buf[len] = '\0';
            char tmp[330];
            snprintf(tmp, sizeof tmp, "%s...", buf);
            if (gfx_text_width(font, tmp) <= max_w) {
                memcpy(buf, tmp, strlen(tmp) + 1); /* fits: len shrank by >= 1 */
                break;
            }
        }
    }
    if (!buf[0])
        return;

    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, buf, color);
    if (!surf) {
        LOG_WARN("TTF_RenderUTF8_Blended failed: %s", TTF_GetError());
        return;
    }
    int px = align == ALIGN_CENTER ? x - surf->w / 2 : align == ALIGN_RIGHT ? x - surf->w : x;
    gfx_blit(gfx, surf, px, y);
    SDL_FreeSurface(surf);
}
