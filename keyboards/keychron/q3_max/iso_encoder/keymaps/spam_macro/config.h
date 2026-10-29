#pragma once

// Board overrides. Keychron headers #include "config.h", which resolves to this file before the board's
// config.h is read, so pull the board's in first; the redefinitions below then win in either order.
#include "../../config.h"

// Keychron ships 51 (the A key); Caps Lock [3,0] is LED 50 in g_led_config.
#undef CAPS_LOCK_INDEX
#define CAPS_LOCK_INDEX 50

// Default backlight after a reset: solid #07fff0.
#undef RGB_MATRIX_DEFAULT_MODE
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_SOLID_COLOR
#undef RGB_MATRIX_DEFAULT_HUE
#define RGB_MATRIX_DEFAULT_HUE 125
#undef RGB_MATRIX_DEFAULT_SAT
#define RGB_MATRIX_DEFAULT_SAT 249
#undef RGB_MATRIX_DEFAULT_VAL
#define RGB_MATRIX_DEFAULT_VAL 255

// Spam macro settings live in QMK's keymap EEPROM datablock; must equal sizeof(spam_macro_config_t).
#define EECONFIG_USER_DATA_SIZE 37

// Pilot-mode keys by matrix position (see keyboard.json).
#define PILOT_POS_Q     {.col = 1, .row = 2}
#define PILOT_POS_E     {.col = 3, .row = 2}
#define PILOT_POS_DEL   {.col = 14, .row = 2}
#define PILOT_POS_END   {.col = 15, .row = 2}
#define PILOT_POS_PGDN  {.col = 16, .row = 2}
#define PILOT_POS_UP    {.col = 15, .row = 4}
#define PILOT_POS_LEFT  {.col = 14, .row = 5}
#define PILOT_POS_DOWN  {.col = 15, .row = 5}
#define PILOT_POS_RIGHT {.col = 16, .row = 5}
