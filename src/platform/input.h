#ifndef RHSM_PLATFORM_INPUT_H
#define RHSM_PLATFORM_INPUT_H

#include <SDL/SDL.h>

/* Miyoo Mini buttons. OnionOS delivers them as SDL 1.2 keyboard events
 * (see Onion's src/common/utils/keymap_sw.h). */
typedef enum {
    BTN_NONE = -1,
    BTN_UP = 0,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_A,
    BTN_B,
    BTN_X,
    BTN_Y,
    BTN_L1,
    BTN_R1,
    BTN_L2,
    BTN_R2,
    BTN_SELECT,
    BTN_START,
    BTN_MENU,
    BTN_POWER,
    BTN_COUNT
} Button;

Button input_button_from_key(SDLKey key);
const char *input_button_name(Button btn);

#endif
