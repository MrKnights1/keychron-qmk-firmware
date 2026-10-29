#pragma once

// Board overrides for the Q3 Max ISO. QMK reads this after every keyboard and keymap config.h
// (keymap config.h is too early: Keychron headers #include "config.h" and pick it up first).

// Keychron's config scans 9 driver channels, but Tab..Del (LEDs 33-47) sit on CB10-CB12 of driver 0.
#undef SNLED27351_PHASE_CHANNEL
#define SNLED27351_PHASE_CHANNEL SNLED27351_SCAN_PHASE_12_CHANNEL

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
