#include "ui/image.h"

#include <stdio.h>
#include <sys/stat.h>

#include <SDL/SDL_image.h>

#include "platform/log.h"

#define AMASK 0xFF000000u
#define RMASK 0x00FF0000u
#define GMASK 0x0000FF00u
#define BMASK 0x000000FFu

/* Area-average scale of a 32-bit ARGB surface. Each destination pixel is the
 * mean of the source pixels it covers, so downscaled box art stays smooth. */
static SDL_Surface *scale_argb(SDL_Surface *src, int dw, int dh)
{
    SDL_Surface *dst = SDL_CreateRGBSurface(SDL_SWSURFACE, dw, dh, 32, RMASK, GMASK, BMASK, AMASK);
    if (!dst)
        return NULL;

    const int sw = src->w, sh = src->h;
    SDL_LockSurface(src);
    SDL_LockSurface(dst);
    for (int dy = 0; dy < dh; dy++) {
        int y0 = dy * sh / dh, y1 = (dy + 1) * sh / dh;
        if (y1 <= y0)
            y1 = y0 + 1;
        Uint32 *drow = (Uint32 *)((Uint8 *)dst->pixels + dy * dst->pitch);
        for (int dx = 0; dx < dw; dx++) {
            int x0 = dx * sw / dw, x1 = (dx + 1) * sw / dw;
            if (x1 <= x0)
                x1 = x0 + 1;
            Uint32 a = 0, r = 0, g = 0, b = 0, cnt = 0;
            for (int y = y0; y < y1; y++) {
                const Uint32 *srow = (const Uint32 *)((const Uint8 *)src->pixels + y * src->pitch);
                for (int x = x0; x < x1; x++) {
                    Uint32 p = srow[x];
                    a += p >> 24;
                    r += (p >> 16) & 0xFF;
                    g += (p >> 8) & 0xFF;
                    b += p & 0xFF;
                    cnt++;
                }
            }
            drow[dx] = ((a / cnt) << 24) | ((r / cnt) << 16) | ((g / cnt) << 8) | (b / cnt);
        }
    }
    SDL_UnlockSurface(dst);
    SDL_UnlockSurface(src);
    return dst;
}

SDL_Surface *image_load_fit(const char *path, int max_w, int max_h)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return NULL; /* no art for this game: not an error */

    SDL_Surface *raw = IMG_Load(path);
    if (!raw) {
        LOG_WARN("image: %s: %s", path, IMG_GetError());
        return NULL;
    }

    /* Normalise to ARGB8888. Copy alpha as-is rather than blending. */
    SDL_Surface *argb = SDL_CreateRGBSurface(SDL_SWSURFACE, raw->w, raw->h, 32,
                                             RMASK, GMASK, BMASK, AMASK);
    if (!argb) {
        SDL_FreeSurface(raw);
        return NULL;
    }
    if (!raw->format->Amask)
        SDL_FillRect(argb, NULL, AMASK); /* opaque images: start fully opaque */
    SDL_SetAlpha(raw, 0, SDL_ALPHA_OPAQUE);
    SDL_BlitSurface(raw, NULL, argb, NULL);
    SDL_FreeSurface(raw);

    /* fit inside max_w x max_h, keep aspect */
    int w = max_w, h = argb->h * max_w / argb->w;
    if (h > max_h) {
        h = max_h;
        w = argb->w * max_h / argb->h;
    }
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    SDL_Surface *out = scale_argb(argb, w, h);
    SDL_FreeSurface(argb);
    if (out)
        SDL_SetAlpha(out, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
    return out;
}
