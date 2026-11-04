/* RHSM – Retro Handheld Save Manager
 * Milestone 2: scan RetroArch saves/states, group them by game in the SQLite
 * index, and show the list on screen. `rhsm --dump` prints the same data to
 * stdout without touching the display (useful over SSH). */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#include "db/index.h"
#include "platform/input.h"
#include "platform/log.h"
#include "platform/paths.h"
#include "scan/scanner.h"
#include "ui/image.h"

#ifndef RHSM_VERSION
#define RHSM_VERSION "dev"
#endif

#define SCREEN_W 640
#define SCREEN_H 480
#define FONT_PATH "res/fonts/Exo2-Bold.ttf"

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

typedef struct {
    SDL_Surface *video;  /* real display surface */
    SDL_Surface *screen; /* off-screen buffer we draw into */
    TTF_Font *font_big;
    TTF_Font *font;
    TTF_Font *font_small;
} App;

typedef struct {
    Library lib;
    char error[160];
    int sel; /* selected game */
    int top; /* first visible row */
    SDL_Surface *art; /* box art for art_for, already scaled to the art box */
    int art_for;      /* game index the art was loaded for, -1 = none yet */
} View;

/* ------------------------------------------------------------- helpers */

static Uint32 rgb(SDL_Surface *s, Uint8 r, Uint8 g, Uint8 b)
{
    return SDL_MapRGB(s->format, r, g, b);
}

static void format_size(long long bytes, char *out, size_t n)
{
    if (bytes < 1024)
        snprintf(out, n, "%lld B", bytes);
    else if (bytes < 1024 * 1024)
        snprintf(out, n, "%lld KB", (bytes + 1023) / 1024);
    else
        snprintf(out, n, "%.1f MB", (double)bytes / (1024.0 * 1024.0));
}

static void format_time(long long t, char *out, size_t n)
{
    time_t tt = (time_t)t;
    struct tm tm;
    localtime_r(&tt, &tm);
    strftime(out, n, "%y-%m-%d %H:%M", &tm);
}

static void format_slot(const LibFile *f, char *out, size_t n)
{
    if (strcmp(f->kind, "state") != 0)
        snprintf(out, n, "%s", strcmp(f->kind, "sram") == 0 ? "SRAM" : "FILE");
    else if (f->slot == SLOT_AUTO)
        snprintf(out, n, "STATE auto");
    else
        snprintf(out, n, "STATE %d", f->slot);
}

/* Prints the whole library. Used by --dump (stdout) and the debug log. */
static void print_library(FILE *out, const Library *lib)
{
    fprintf(out, "Library: %d games, %d files  (db %s)\n", lib->game_count, lib->file_count,
            lib->db_path);
    for (int i = 0; i < lib->game_count; i++) {
        const LibGame *g = &lib->games[i];
        char art[768];
        struct stat st;
        image_boxart_path(art, sizeof art, g->system, g->rom_name);
        fprintf(out, "[%s] %s  -- %d sram, %d states  (%s)  art: %s\n", g->system, g->title,
                g->sram_count, g->state_count, g->rom_name, stat(art, &st) == 0 ? "yes" : "no");
        for (int j = 0; j < g->file_count; j++) {
            const LibFile *f = &g->files[j];
            char slot[24], size[24], when[24];
            format_slot(f, slot, sizeof slot);
            format_size(f->size, size, sizeof size);
            format_time(f->mtime, when, sizeof when);
            fprintf(out, "    %-11s %-18s %8s  %s%s\n", slot, f->core, size, when,
                    f->has_thumb ? "  [thumb]" : "");
        }
    }
}

/* ------------------------------------------------------------- loading */

/* Called once the library is in memory. Kept separate and non-inlined so gdb
 * can hook it: scripts/rhsm.gdb prints every game and file from here. */
static __attribute__((noinline)) void on_library_loaded(const Library *lib)
{
    LOG_INFO("library: %d games, %d files", lib->game_count, lib->file_count);
#ifdef DEBUG
    print_library(stderr, lib);
#endif
}

/* scan -> store in SQLite -> load back. Returns false and fills err on failure. */
static bool load_library(Library *lib, char *err, size_t err_n)
{
    char profile[512], roms[512], data[512], db_path[512];
    paths_sd(profile, sizeof profile, PATH_PROFILE);
    paths_sd(roms, sizeof roms, PATH_ROMS);
    paths_sd(data, sizeof data, PATH_DATA);
    paths_sd(db_path, sizeof db_path, PATH_DB);

    if (paths_mkdirs(data) != 0) {
        snprintf(err, err_n, "Cannot create %s", data);
        return false;
    }

    ScanList scan = {0};
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    scan_saves(profile, roms, &scan);

    sqlite3 *db = NULL;
    if (db_open(db_path, &db) != 0) {
        snprintf(err, err_n, "Cannot open database %s", db_path);
        scan_list_free(&scan);
        return false;
    }
    bool ok = db_store_scan(db, &scan) == 0 && db_load_library(db, lib) == 0;
    db_close(db);
    scan_list_free(&scan);

    if (!ok) {
        snprintf(err, err_n, "Database error (see log.txt)");
        return false;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    LOG_INFO("load_library took %ld ms",
             (long)((t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_nsec - t0.tv_nsec) / 1000000));
    on_library_loaded(lib);
    return true;
}

/* ------------------------------------------------------------- drawing */

static int text_width(TTF_Font *font, const char *text)
{
    int w = 0, h = 0;
    TTF_SizeUTF8(font, text, &w, &h);
    return w;
}

/* Draws text; if wider than max_w (>0), cuts it on a UTF-8 boundary and adds "...". */
static void draw_text(SDL_Surface *dst, TTF_Font *font, const char *text, int x, int y,
                      SDL_Color color, int max_w, int align /* 0 left, 1 center, 2 right */)
{
    char buf[320];
    snprintf(buf, sizeof buf, "%s", text);

    if (max_w > 0 && text_width(font, buf) > max_w) {
        size_t len = strlen(buf);
        while (len > 0) {
            do {
                len--;
            } while (len > 0 && ((unsigned char)buf[len] & 0xC0) == 0x80);
            buf[len] = '\0';
            char tmp[330];
            snprintf(tmp, sizeof tmp, "%s...", buf);
            if (text_width(font, tmp) <= max_w) {
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
    int px = align == 1 ? x - surf->w / 2 : align == 2 ? x - surf->w : x;
    SDL_Rect pos = {(Sint16)px, (Sint16)y, 0, 0};
    SDL_BlitSurface(surf, NULL, dst, &pos);
    SDL_FreeSurface(surf);
}

static const SDL_Color C_WHITE = {240, 240, 245, 0};
static const SDL_Color C_DIM = {140, 145, 160, 0};
static const SDL_Color C_DARK = {20, 22, 30, 0};
static const SDL_Color C_ACCENT = {255, 196, 60, 0};

static void present(App *app)
{
    SDL_BlitSurface(app->screen, NULL, app->video, NULL);
    SDL_Flip(app->video);
}

static void render_message(App *app, const char *msg)
{
    SDL_Surface *s = app->screen;
    SDL_FillRect(s, NULL, rgb(s, 22, 24, 34));
    draw_text(s, app->font_big, "RHSM", SCREEN_W / 2, 150, C_WHITE, 0, 1);
    draw_text(s, app->font, msg, SCREEN_W / 2, 240, C_DIM, SCREEN_W - 40, 1);
    present(app);
}

/* Loads (once per selection) the selected game's Onion box art. */
static void update_art(View *v)
{
    if (v->art_for == v->sel || v->lib.game_count == 0)
        return;
    if (v->art) {
        SDL_FreeSurface(v->art);
        v->art = NULL;
    }
    const LibGame *g = &v->lib.games[v->sel];
    char path[768];
    image_boxart_path(path, sizeof path, g->system, g->rom_name);
    v->art = image_load_fit(path, ART_W, ART_H);
    v->art_for = v->sel;
    LOG_DEBUG("art: %s -> %s", path, v->art ? "loaded" : "none");
}

static void render_art(App *app, const View *v)
{
    SDL_Surface *s = app->screen;
    SDL_Rect box = {ART_X, ART_Y, ART_W, ART_H};
    SDL_FillRect(s, &box, rgb(s, 22, 24, 34));

    if (v->art) {
        SDL_Rect pos = {(Sint16)(ART_X + (ART_W - v->art->w) / 2),
                        (Sint16)(ART_Y + (ART_H - v->art->h) / 2), 0, 0};
        SDL_BlitSurface(v->art, NULL, s, &pos);
    } else {
        draw_text(s, app->font_small, "No art", ART_X + ART_W / 2, ART_Y + ART_H / 2 - 12,
                  C_DIM, ART_W - 8, 1);
    }
}

static void render(App *app, View *v)
{
    SDL_Surface *s = app->screen;
    const Library *lib = &v->lib;
    char line[160];

    SDL_FillRect(s, NULL, rgb(s, 22, 24, 34));

    /* header */
    draw_text(s, app->font, "RHSM", 16, 10, C_WHITE, 0, 0);
    snprintf(line, sizeof line, "%d games  ·  %d files", lib->game_count, lib->file_count);
    draw_text(s, app->font_small, line, SCREEN_W - 16, 16, C_DIM, 0, 2);
    SDL_Rect rule = {16, LIST_Y - 6, SCREEN_W - 32, 1};
    SDL_FillRect(s, &rule, rgb(s, 60, 64, 84));

    if (v->error[0] || lib->game_count == 0) {
        draw_text(s, app->font, v->error[0] ? v->error : "No saves found", SCREEN_W / 2, 200,
                  C_DIM, SCREEN_W - 40, 1);
    } else {
        /* game list */
        for (int r = 0; r < LIST_ROWS && v->top + r < lib->game_count; r++) {
            int i = v->top + r;
            const LibGame *g = &lib->games[i];
            int y = LIST_Y + r * ROW_H;
            bool on = i == v->sel;
            if (on) {
                SDL_Rect hl = {10, (Sint16)(y - 1), SCREEN_W - 20, ROW_H};
                SDL_FillRect(s, &hl, rgb(s, 255, 196, 60));
            }
            snprintf(line, sizeof line, "%s   %d/%d", g->system, g->sram_count, g->state_count);
            draw_text(s, app->font_small, line, SCREEN_W - 20, y + 2, on ? C_DARK : C_DIM, 0, 2);
            draw_text(s, app->font_small, g->title, 20, y + 2, on ? C_DARK : C_WHITE, 440, 0);
        }

        /* scroll position */
        if (lib->game_count > LIST_ROWS) {
            int track_h = LIST_ROWS * ROW_H;
            int bar_h = track_h * LIST_ROWS / lib->game_count;
            int bar_y = LIST_Y + track_h * v->top / lib->game_count;
            SDL_Rect bar = {SCREEN_W - 6, (Sint16)bar_y, 3, (Uint16)(bar_h < 8 ? 8 : bar_h)};
            SDL_FillRect(s, &bar, rgb(s, 90, 96, 120));
        }

        /* detail panel for the selected game */
        const LibGame *g = &lib->games[v->sel];
        SDL_Rect panel = {PANEL_X, PANEL_Y, PANEL_W, PANEL_H};
        SDL_FillRect(s, &panel, rgb(s, 32, 35, 48));
        draw_text(s, app->font_small, g->rom_name, 20, PANEL_Y + 4, C_ACCENT, TEXT_RIGHT - 20, 0);

        update_art(v);
        render_art(app, v);

        for (int j = 0; j < g->file_count && j < FILE_ROWS; j++) {
            const LibFile *f = &g->files[j];
            int y = PANEL_Y + 32 + j * FILE_ROW_H;
            if (j == FILE_ROWS - 1 && g->file_count > FILE_ROWS) {
                snprintf(line, sizeof line, "+ %d more", g->file_count - (FILE_ROWS - 1));
                draw_text(s, app->font_small, line, 20, y, C_DIM, 0, 0);
                break;
            }
            char slot[24], size[24], when[24];
            format_slot(f, slot, sizeof slot);
            format_size(f->size, size, sizeof size);
            format_time(f->mtime, when, sizeof when);
            draw_text(s, app->font_small, slot, 20, y, C_WHITE, 100, 0);
            draw_text(s, app->font_small, f->core, 124, y, C_DIM, 124, 0);
            draw_text(s, app->font_small, size, 326, y, C_DIM, 0, 2);
            draw_text(s, app->font_small, when, TEXT_RIGHT, y, C_DIM, 0, 2);
        }
    }

    /* footer */
#ifdef DEBUG
    SDL_Rect badge = {SCREEN_W / 2 - 43, 12, 86, 26};
    SDL_FillRect(s, &badge, rgb(s, 120, 30, 30));
    draw_text(s, app->font_small, "DEBUG", SCREEN_W / 2, 13, C_WHITE, 0, 1);
#endif
    snprintf(line, sizeof line, "UP/DOWN select  ·  L1/R1 page  ·  MENU exit  ·  v%s",
             RHSM_VERSION);
    draw_text(s, app->font_small, line, SCREEN_W / 2, FOOTER_Y, C_DIM, SCREEN_W - 32, 1);

    present(app);
}

/* ------------------------------------------------------------- app */

static void move_selection(View *v, int delta)
{
    int n = v->lib.game_count;
    if (n == 0)
        return;
    v->sel += delta;
    if (v->sel < 0)
        v->sel = 0;
    if (v->sel >= n)
        v->sel = n - 1;
    if (v->sel < v->top)
        v->top = v->sel;
    if (v->sel >= v->top + LIST_ROWS)
        v->top = v->sel - LIST_ROWS + 1;
}

static bool app_init(App *app)
{
    memset(app, 0, sizeof *app);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    SDL_ShowCursor(SDL_DISABLE);
    SDL_EnableKeyRepeat(300, 60);

    /* Same pattern as Onion's own apps: draw into a buffer, blit to the video surface. */
    app->video = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 32, SDL_HWSURFACE);
    if (!app->video) {
        LOG_ERROR("SDL_SetVideoMode failed: %s", SDL_GetError());
        return false;
    }
    app->screen = SDL_CreateRGBSurface(SDL_HWSURFACE, SCREEN_W, SCREEN_H, 32, 0, 0, 0, 0);
    if (!app->screen) {
        LOG_ERROR("SDL_CreateRGBSurface failed: %s", SDL_GetError());
        return false;
    }

    if (TTF_Init() != 0) {
        LOG_ERROR("TTF_Init failed: %s", TTF_GetError());
        return false;
    }
    app->font_big = TTF_OpenFont(FONT_PATH, 52);
    app->font = TTF_OpenFont(FONT_PATH, 26);
    app->font_small = TTF_OpenFont(FONT_PATH, 19);
    if (!app->font_big || !app->font || !app->font_small) {
        LOG_ERROR("TTF_OpenFont(%s) failed: %s", FONT_PATH, TTF_GetError());
        return false;
    }

    LOG_INFO("video %dx%d %dbpp", app->video->w, app->video->h, app->video->format->BitsPerPixel);
    return true;
}

static void app_quit(App *app)
{
    if (app->font_small) TTF_CloseFont(app->font_small);
    if (app->font) TTF_CloseFont(app->font);
    if (app->font_big) TTF_CloseFont(app->font_big);
    if (TTF_WasInit()) TTF_Quit();
    if (app->screen) SDL_FreeSurface(app->screen);
    SDL_Quit();
}

static int run_dump(void)
{
    Library lib;
    char err[160] = "";
    if (!load_library(&lib, err, sizeof err)) {
        fprintf(stderr, "%s\n", err);
        return 1;
    }
    print_library(stdout, &lib);
    library_free(&lib);
    return 0;
}

int main(int argc, char **argv)
{
    LOG_INFO("RHSM %s starting (sdcard %s)", RHSM_VERSION, paths_sdcard());

    if (argc > 1 && strcmp(argv[1], "--dump") == 0)
        return run_dump();

    App app;
    if (!app_init(&app)) {
        app_quit(&app);
        return 1;
    }

    View view = {0};
    view.art_for = -1;
    render_message(&app, "Scanning saves...");
    if (!load_library(&view.lib, view.error, sizeof view.error))
        LOG_ERROR("%s", view.error);

    bool running = true, dirty = true;

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = false;
            } else if (ev.type == SDL_KEYDOWN) {
                dirty = true;
                switch (input_button_from_key(ev.key.keysym.sym)) {
                case BTN_UP:    move_selection(&view, -1); break;
                case BTN_DOWN:  move_selection(&view, +1); break;
                case BTN_LEFT:
                case BTN_L1:    move_selection(&view, -LIST_ROWS); break;
                case BTN_RIGHT:
                case BTN_R1:    move_selection(&view, +LIST_ROWS); break;
                case BTN_MENU:  running = false; break;
                default: break;
                }
            }
        }

        if (dirty) {
            render(&app, &view);
            dirty = false;
        }
        SDL_Delay(16);
    }

    LOG_INFO("exiting");
    if (view.art)
        SDL_FreeSurface(view.art);
    library_free(&view.lib);
    app_quit(&app);
    return 0;
}
