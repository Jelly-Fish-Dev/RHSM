#include "ui/screen_library.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "platform/log.h"
#include "platform/paths.h"
#include "ui/image.h"
#include "ui/theme.h"
#include "util/format.h"
#include "version.h"

/* layout */
#define LIST_Y      52
#define ROW_H       28
#define LIST_ROWS   7
#define PANEL_X     10
#define PANEL_Y     (LIST_Y + LIST_ROWS * ROW_H + 8)
#define PANEL_W     (SCREEN_W - 2 * PANEL_X)
#define PANEL_H     (FOOTER_Y - PANEL_Y - 6)
#define FILE_ROW_H  24
#define FILE_ROWS   6
#define FOOTER_Y    452
/* box art, right side of the bottom panel (Onion art is 2:3 portrait) */
#define ART_W       124
#define ART_H       (PANEL_H - 12)
#define ART_X       (PANEL_X + PANEL_W - 6 - ART_W)
#define ART_Y       (PANEL_Y + 6)
#define TEXT_RIGHT  (ART_X - 10) /* right edge of the file list */

/* ------------------------------------------------------------- state */

void screen_library_init(LibraryScreen *scr, Library *lib, const char *error)
{
    memset(scr, 0, sizeof *scr);
    scr->lib = *lib;
    memset(lib, 0, sizeof *lib);
    snprintf(scr->error, sizeof scr->error, "%s", error ? error : "");
    scr->art_for = -1;
}

void screen_library_free(LibraryScreen *scr)
{
    if (scr->art)
        SDL_FreeSurface(scr->art);
    scr->art = NULL;
    library_free(&scr->lib);
}

static void move_selection(LibraryScreen *scr, int delta)
{
    int n = scr->lib.game_count;
    if (n == 0)
        return;
    scr->sel += delta;
    if (scr->sel < 0)
        scr->sel = 0;
    if (scr->sel >= n)
        scr->sel = n - 1;
    if (scr->sel < scr->top)
        scr->top = scr->sel;
    if (scr->sel >= scr->top + LIST_ROWS)
        scr->top = scr->sel - LIST_ROWS + 1;
}

ScreenResult screen_library_handle(LibraryScreen *scr, Button btn)
{
    switch (btn) {
    case BTN_UP:    move_selection(scr, -1); break;
    case BTN_DOWN:  move_selection(scr, +1); break;
    case BTN_LEFT:
    case BTN_L1:    move_selection(scr, -LIST_ROWS); break;
    case BTN_RIGHT:
    case BTN_R1:    move_selection(scr, +LIST_ROWS); break;
    case BTN_MENU:  return SCREEN_EXIT;
    default: break;
    }
    return SCREEN_STAY;
}

/* Loads (once per selection) the selected game's Onion box art. */
static void update_art(LibraryScreen *scr)
{
    if (scr->art_for == scr->sel || scr->lib.game_count == 0)
        return;
    if (scr->art) {
        SDL_FreeSurface(scr->art);
        scr->art = NULL;
    }
    const LibGame *g = &scr->lib.games[scr->sel];
    char path[768];
    paths_boxart(path, sizeof path, g->system, g->rom_name);
    scr->art = image_load_fit(path, ART_W, ART_H);
    scr->art_for = scr->sel;
    LOG_DEBUG("art: %s -> %s", path, scr->art ? "loaded" : "none");
}

/* ------------------------------------------------------------- drawing */

static void render_header(const LibraryScreen *scr, Gfx *gfx)
{
    char line[96];
    gfx_text(gfx, gfx->font, "RHSM", 16, 10, C_WHITE, 0, ALIGN_LEFT);
    snprintf(line, sizeof line, "%d games  ·  %d files", scr->lib.game_count,
             scr->lib.file_count);
    gfx_text(gfx, gfx->font_small, line, SCREEN_W - 16, 16, C_DIM, 0, ALIGN_RIGHT);
    SDL_Rect rule = {16, LIST_Y - 6, SCREEN_W - 32, 1};
    gfx_fill(gfx, &rule, C_RULE);
}

static void render_list(const LibraryScreen *scr, Gfx *gfx)
{
    const Library *lib = &scr->lib;
    char line[64];

    for (int r = 0; r < LIST_ROWS && scr->top + r < lib->game_count; r++) {
        int i = scr->top + r;
        const LibGame *g = &lib->games[i];
        int y = LIST_Y + r * ROW_H;
        bool on = i == scr->sel;
        if (on) {
            SDL_Rect hl = {10, (Sint16)(y - 1), SCREEN_W - 20, ROW_H};
            gfx_fill(gfx, &hl, C_HIGHLIGHT);
        }
        snprintf(line, sizeof line, "%s   %d/%d", g->system, g->sram_count, g->state_count);
        gfx_text(gfx, gfx->font_small, line, SCREEN_W - 20, y + 2, on ? C_DARK : C_DIM, 0,
                 ALIGN_RIGHT);
        gfx_text(gfx, gfx->font_small, g->title, 20, y + 2, on ? C_DARK : C_WHITE, 440,
                 ALIGN_LEFT);
    }

    /* scroll position */
    if (lib->game_count > LIST_ROWS) {
        int track_h = LIST_ROWS * ROW_H;
        int bar_h = track_h * LIST_ROWS / lib->game_count;
        int bar_y = LIST_Y + track_h * scr->top / lib->game_count;
        SDL_Rect bar = {SCREEN_W - 6, (Sint16)bar_y, 3, (Uint16)(bar_h < 8 ? 8 : bar_h)};
        gfx_fill(gfx, &bar, C_SCROLLBAR);
    }
}

static void render_art(const LibraryScreen *scr, Gfx *gfx)
{
    SDL_Rect box = {ART_X, ART_Y, ART_W, ART_H};
    gfx_fill(gfx, &box, C_BG);

    if (scr->art)
        gfx_blit(gfx, scr->art, ART_X + (ART_W - scr->art->w) / 2,
                 ART_Y + (ART_H - scr->art->h) / 2);
    else
        gfx_text(gfx, gfx->font_small, "No art", ART_X + ART_W / 2, ART_Y + ART_H / 2 - 12,
                 C_DIM, ART_W - 8, ALIGN_CENTER);
}

static void render_details(LibraryScreen *scr, Gfx *gfx)
{
    const LibGame *g = &scr->lib.games[scr->sel];
    char line[64];

    SDL_Rect panel = {PANEL_X, PANEL_Y, PANEL_W, PANEL_H};
    gfx_fill(gfx, &panel, C_PANEL);
    gfx_text(gfx, gfx->font_small, g->rom_name, 20, PANEL_Y + 4, C_ACCENT, TEXT_RIGHT - 20,
             ALIGN_LEFT);

    update_art(scr);
    render_art(scr, gfx);

    for (int j = 0; j < g->file_count && j < FILE_ROWS; j++) {
        const LibFile *f = &g->files[j];
        int y = PANEL_Y + 32 + j * FILE_ROW_H;
        if (j == FILE_ROWS - 1 && g->file_count > FILE_ROWS) {
            snprintf(line, sizeof line, "+ %d more", g->file_count - (FILE_ROWS - 1));
            gfx_text(gfx, gfx->font_small, line, 20, y, C_DIM, 0, ALIGN_LEFT);
            break;
        }
        char slot[24], size[24], when[24];
        libfile_slot_label(f, slot, sizeof slot);
        format_size(f->size, size, sizeof size);
        format_time(f->mtime, when, sizeof when);
        gfx_text(gfx, gfx->font_small, slot, 20, y, C_WHITE, 100, ALIGN_LEFT);
        gfx_text(gfx, gfx->font_small, f->core, 124, y, C_DIM, 124, ALIGN_LEFT);
        gfx_text(gfx, gfx->font_small, size, 326, y, C_DIM, 0, ALIGN_RIGHT);
        gfx_text(gfx, gfx->font_small, when, TEXT_RIGHT, y, C_DIM, 0, ALIGN_RIGHT);
    }
}

static void render_footer(Gfx *gfx)
{
#ifdef DEBUG
    SDL_Rect badge = {SCREEN_W / 2 - 43, 12, 86, 26};
    gfx_fill(gfx, &badge, C_DEBUG);
    gfx_text(gfx, gfx->font_small, "DEBUG", SCREEN_W / 2, 13, C_WHITE, 0, ALIGN_CENTER);
#endif
    char line[96];
    snprintf(line, sizeof line, "UP/DOWN select  ·  L1/R1 page  ·  MENU exit  ·  v%s",
             RHSM_VERSION);
    gfx_text(gfx, gfx->font_small, line, SCREEN_W / 2, FOOTER_Y, C_DIM, SCREEN_W - 32,
             ALIGN_CENTER);
}

void screen_library_render(LibraryScreen *scr, Gfx *gfx)
{
    gfx_fill(gfx, NULL, C_BG);
    render_header(scr, gfx);

    if (scr->error[0] || scr->lib.game_count == 0) {
        gfx_text(gfx, gfx->font, scr->error[0] ? scr->error : "No saves found", SCREEN_W / 2,
                 200, C_DIM, SCREEN_W - 40, ALIGN_CENTER);
    } else {
        render_list(scr, gfx);
        render_details(scr, gfx);
    }

    render_footer(gfx);
    gfx_present(gfx);
}
