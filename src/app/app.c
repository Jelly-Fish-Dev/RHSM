#include "app/app.h"

#include <stdbool.h>

#include <SDL/SDL.h>

#include "core/library_loader.h"
#include "platform/input.h"
#include "platform/log.h"
#include "ui/gfx.h"
#include "ui/screen_library.h"
#include "ui/screen_message.h"

int app_run(void)
{
    Gfx gfx;
    if (!gfx_init(&gfx)) {
        gfx_quit(&gfx);
        return 1;
    }

    screen_message_show(&gfx, "Scanning saves...");

    Library lib = {0};
    char err[160] = "";
    if (!library_load(&lib, err, sizeof err))
        LOG_ERROR("%s", err);

    LibraryScreen screen;
    screen_library_init(&screen, &lib, err);

    bool running = true, dirty = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = false;
            } else if (ev.type == SDL_KEYDOWN) {
                Button btn = input_button_from_key(ev.key.keysym.sym);
                if (screen_library_handle(&screen, btn) == SCREEN_EXIT)
                    running = false;
                dirty = true;
            }
        }

        if (dirty && running) {
            screen_library_render(&screen, &gfx);
            dirty = false;
        }
        SDL_Delay(16);
    }

    LOG_INFO("exiting");
    screen_library_free(&screen);
    gfx_quit(&gfx);
    return 0;
}
