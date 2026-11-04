#include "platform/input.h"

Button input_button_from_key(SDLKey key)
{
    switch (key) {
    case SDLK_UP:        return BTN_UP;
    case SDLK_DOWN:      return BTN_DOWN;
    case SDLK_LEFT:      return BTN_LEFT;
    case SDLK_RIGHT:     return BTN_RIGHT;
    case SDLK_SPACE:     return BTN_A;
    case SDLK_LCTRL:     return BTN_B;
    case SDLK_LSHIFT:    return BTN_X;
    case SDLK_LALT:      return BTN_Y;
    case SDLK_e:         return BTN_L1;
    case SDLK_t:         return BTN_R1;
    case SDLK_TAB:       return BTN_L2;
    case SDLK_BACKSPACE: return BTN_R2;
    case SDLK_RCTRL:     return BTN_SELECT;
    case SDLK_RETURN:    return BTN_START;
    case SDLK_ESCAPE:    return BTN_MENU;
    case SDLK_POWER:     return BTN_POWER;
    default:             return BTN_NONE;
    }
}

const char *input_button_name(Button btn)
{
    static const char *names[BTN_COUNT] = {
        "UP", "DOWN", "LEFT", "RIGHT", "A", "B", "X", "Y",
        "L1", "R1", "L2", "R2", "SELECT", "START", "MENU", "POWER",
    };
    if (btn < 0 || btn >= BTN_COUNT)
        return "?";
    return names[btn];
}
