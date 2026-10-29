#pragma once

#include <stdint.h>
#include "pilot_remap.h"

/*
 * Phase 1 (initial hold): key held down for HOLD_MS so wheels reach full lock
 * Phase 2 (tap cycle):    hold for TAP_HOLD_MS, release for GAP_MS, repeat
 */
#define SPAM_STEER_HOLD       200   // ms initial hold (full wheel turn)
#define SPAM_STEER_TAP_HOLD   80    // ms each tap is held down
#define SPAM_STEER_GAP_BASE   15    // ms gap between taps (key released)
#define SPAM_STEER_GAP_JITTER 15    // randomized gap range (total: 15-30ms)

#define SPAM_BRAKE_HOLD       150   // ms initial hold
#define SPAM_BRAKE_TAP_HOLD   60    // ms each brake tap held down
#define SPAM_BRAKE_GAP_BASE   20    // ms gap between brake taps
#define SPAM_BRAKE_GAP_JITTER 15    // randomized gap range (total: 20-35ms)

#define DEFAULT_SPAM_TOGGLE_ROW   3  // G key [3,5]
#define DEFAULT_SPAM_TOGGLE_COL   5
#define DEFAULT_PILOT_TOGGLE_ROW  3  // H key [3,6]
#define DEFAULT_PILOT_TOGGLE_COL  6
#define DEFAULT_LED_R  255
#define DEFAULT_LED_G  0
#define DEFAULT_LED_B  0

// Stored in QMK's keymap EEPROM datablock; changing the layout requires bumping SPAM_MACRO_CONFIG_VERSION in config.h.
typedef struct __attribute__((__packed__)) {
    uint16_t steer_hold_ms;
    uint16_t steer_tap_hold_ms;
    uint16_t steer_gap_base;
    uint16_t steer_gap_jitter;
    uint16_t brake_hold_ms;
    uint16_t brake_tap_hold_ms;
    uint16_t brake_gap_base;
    uint16_t brake_gap_jitter;
    uint8_t  toggle_row;
    uint8_t  toggle_col;
    uint8_t  pilot_toggle_row;
    uint8_t  pilot_toggle_col;
    uint8_t  spam_led_r, spam_led_g, spam_led_b;
    uint8_t  pilot_led_r, pilot_led_g, pilot_led_b;
    uint16_t pilot_binds[PILOT_BIND_COUNT];
} spam_macro_config_t;

void spam_config_defaults(spam_macro_config_t *cfg);

// Validates a config read from EEPROM and loads its bindings into pilot_remap. Returns false when
// it was unusable and has been replaced with defaults in place, which must then be written back.
bool spam_config_load(spam_macro_config_t *cfg);
