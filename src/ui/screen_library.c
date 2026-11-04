#include "ui/screen_library.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/log.h"
#include "platform/paths.h"
#include "ui/image.h"
#include "ui/theme.h"
#include "util/format.h"
#include "version.h"

/* layout */
#define TABS_Y      46
#define TABS_H      26
#define TAB_PAD     10 /* text padding inside a tab */
#define TAB_GAP     6
#define LIST_Y      (TABS_Y + TABS_H + 10)
#define ROW_H       28
#define LIST_ROWS   6
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

/* ------------------------------------------------------------- tabs */

static int cmp_tab(const void *a, const void *b)
{
    return strcmp(((const LibraryTab *)a)->name, ((const LibraryTab *)b)->name);
}

/* tabs[0] = All, then one tab per distinct system, sorted by name. */
static void build_tabs(LibraryScreen *scr)
{
    const Library *lib = &scr->lib;
    snprintf(scr->tabs[0].name, sizeof scr->tabs[0].name, "All");
    scr->tabs[0].count = lib->game_count;
    scr->tab_count = 1;

    for (int i = 0; i < lib->game_count; i++) {
        const char *sys = lib->games[i].system;
        int t = 1;
        while (t < scr->tab_count && strcmp(scr->tabs[t].name, sys) != 0)
            t++;
        if (t == scr->tab_count) {
            if (scr->tab_count == LIBRARY_MAX_TABS) {
                LOG_WARN("tabs: more than %d systems, %s only shown under All",
                         LIBRARY_MAX_TABS - 1, sys);
                continue;
            }
            snprintf(scr->tabs[t].name, sizeof scr->tabs[t].name, "%s", sys);
            scr->tabs[t].count = 0;
            scr->tab_count++;
        }
        scr->tabs[t].count++;
    }
    qsort(scr->tabs + 1, (size_t)(scr->tab_count - 1), sizeof scr->tabs[0], cmp_tab);
}

/* Switches to tab t: rebuilds the visible list and goes back to the top. */
static void apply_tab(LibraryScreen *scr, int t)
{
    scr->tab = t;
    scr->visible_count = 0;
    for (int i = 0; i < scr->lib.game_count; i++) {
        if (t == 0 || strcmp(scr->lib.games[i].system, scr->tabs[t].name) == 0)
            scr->visible[scr->visible_count++] = i;
    }
    scr->sel = 0;
    scr->top = 0;
    LOG_DEBUG("tab %s: %d games", scr->tabs[t].name, scr->visible_count);
}

/* lib.games index of the selected game, or -1 if the tab is empty. */
static int selected_game(const LibraryScreen *scr)
{
    return scr->visible_count ? scr->visible[scr->sel] : -1;
}

/* ------------------------------------------------------------- state */

void screen_library_init(LibraryScreen *scr, Library *lib, const char *error)
{
    memset(scr, 0, sizeof *scr);
    scr->lib = *lib;
    memset(lib, 0, sizeof *lib);
    snprintf(scr->error, sizeof scr->error, "%s", error ? error : "");
    scr->art_for = -1;

    scr->visible = calloc((size_t)(scr->lib.game_count ? scr->lib.game_count : 1),
                          sizeof *scr->visible);
    build_tabs(scr);
    apply_tab(scr, 0);
}

void screen_library_free(LibraryScreen *scr)
{
    if (scr->art)
        SDL_FreeSurface(scr->art);
    scr->art = NULL;
    free(scr->visible);
    scr->visible = NULL;
    library_free(&scr->lib);
}

static void move_selection(LibraryScreen *scr, int delta)
{
    int n = scr->visible_count;
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
    case BTN_L2:    apply_tab(scr, (scr->tab - 1 + scr->tab_count) % scr->tab_count); break;
    case BTN_R2:    apply_tab(scr, (scr->tab + 1) % scr->tab_count); break;
    case BTN_MENU:  return SCREEN_EXIT;
    default: break;
    }
    return SCREEN_STAY;
}

/* Loads (once per selection) the selected game's Onion box art. */
static void update_art(LibraryScreen *scr)
{
    int gi = selected_game(scr);
    if (gi < 0 || scr->art_for == gi)
        return;
    if (scr->art) {
        SDL_FreeSurface(scr->art);
        scr->art = NULL;
    }
    const LibGame *g = &scr->lib.games[gi];
    char path[768];
    paths_boxart(path, sizeof path, g->system, g->rom_name);
    scr->art = image_load_fit(path, ART_W, ART_H);
    scr->art_for = gi;
    LOG_DEBUG("art: %s -> %s", path, scr->art ? "loaded" : "none");
}

/* ------------------------------------------------------------- drawing */

static void render_header(const LibraryScreen *scr, Gfx *gfx)
{
    char line[96];
    gfx_text(gfx, gfx->font, "RHSM", 16, 10, C_WHITE, 0, ALIGN_LEFT);
    int title_w = gfx_text_width(gfx->font, "RHSM");
    gfx_text(gfx, gfx->font_small, "v" RHSM_VERSION, 16 + title_w + 8, 17, C_DIM, 0, ALIGN_LEFT);

    snprintf(line, sizeof line, "%d games  ·  %d files", scr->lib.game_count,
             scr->lib.file_count);
    gfx_text(gfx, gfx->font_small, line, SCREEN_W - 16, 16, C_DIM, 0, ALIGN_RIGHT);
}

/* Tab strip: "All 13  GB 2  MD 1 ...". Scrolls so the active tab is visible. */
static void render_tabs(const LibraryScreen *scr, Gfx *gfx)
{
    char label[LIBRARY_MAX_TABS][24];
    int width[LIBRARY_MAX_TABS] = {0}, start[LIBRARY_MAX_TABS] = {0};

    int x = 16;
    for (int t = 0; t < scr->tab_count; t++) {
        snprintf(label[t], sizeof label[t], "%s %d", scr->tabs[t].name, scr->tabs[t].count);
        width[t] = gfx_text_width(gfx->font_small, label[t]) + 2 * TAB_PAD;
        start[t] = x;
        x += width[t] + TAB_GAP;
    }

    /* shift left if the active tab would run off the right edge */
    int right_limit = SCREEN_W - 16;
    int active_end = start[scr->tab] + width[scr->tab];
    int offset = active_end > right_limit ? right_limit - active_end : 0;

    for (int t = 0; t < scr->tab_count; t++) {
        int tx = start[t] + offset;
        if (tx + width[t] < 0 || tx > SCREEN_W)
            continue;
        bool on = t == scr->tab;
        if (on) {
            SDL_Rect pill = {(Sint16)tx, TABS_Y, (Uint16)width[t], TABS_H};
            gfx_fill(gfx, &pill, C_HIGHLIGHT);
        }
        gfx_text(gfx, gfx->font_small, label[t], tx + TAB_PAD, TABS_Y + 2, on ? C_DARK : C_DIM, 0,
                 ALIGN_LEFT);
    }

    SDL_Rect rule = {16, LIST_Y - 6, SCREEN_W - 32, 1};
    gfx_fill(gfx, &rule, C_RULE);
}

static void render_list(const LibraryScreen *scr, Gfx *gfx)
{
    char line[64];

    for (int r = 0; r < LIST_ROWS && scr->top + r < scr->visible_count; r++) {
        int row = scr->top + r;
        const LibGame *g = &scr->lib.games[scr->visible[row]];
        int y = LIST_Y + r * ROW_H;
        bool on = row == scr->sel;
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
    if (scr->visible_count > LIST_ROWS) {
        int track_h = LIST_ROWS * ROW_H;
        int bar_h = track_h * LIST_ROWS / scr->visible_count;
        int bar_y = LIST_Y + track_h * scr->top / scr->visible_count;
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
    int gi = selected_game(scr);
    if (gi < 0)
        return;
    const LibGame *g = &scr->lib.games[gi];
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
    gfx_text(gfx, gfx->font_small, "UP/DOWN select  ·  L1/R1 page  ·  L2/R2 system  ·  MENU exit",
             SCREEN_W / 2, FOOTER_Y, C_DIM, SCREEN_W - 32, ALIGN_CENTER);
}

void screen_library_render(LibraryScreen *scr, Gfx *gfx)
{
    gfx_fill(gfx, NULL, C_BG);
    render_header(scr, gfx);

    if (scr->error[0] || scr->lib.game_count == 0) {
        SDL_Rect rule = {16, LIST_Y - 6, SCREEN_W - 32, 1};
        gfx_fill(gfx, &rule, C_RULE);
        gfx_text(gfx, gfx->font, scr->error[0] ? scr->error : "No saves found", SCREEN_W / 2,
                 200, C_DIM, SCREEN_W - 40, ALIGN_CENTER);
    } else {
        render_tabs(scr, gfx);
        render_list(scr, gfx);
        render_details(scr, gfx);
    }

    render_footer(gfx);
    gfx_present(gfx);
}
