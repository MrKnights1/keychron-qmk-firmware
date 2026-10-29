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

// Bump whenever the struct layout changes, so stale EEPROM bytes are migrated or reset instead of misread.
#define SPAM_MACRO_CONFIG_VERSION 1

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
    uint8_t  version;
} spam_macro_config_t;

typedef enum {
    SPAM_CONFIG_CURRENT,  // valid as read
    SPAM_CONFIG_MIGRATED, // pre-binding layout: settings kept, bindings cleared
    SPAM_CONFIG_RESET,    // unusable: replaced with defaults
} spam_config_result_t;

void spam_config_defaults(spam_macro_config_t *cfg);

// Validates a config read from EEPROM, migrating or resetting it in place, and loads its
// bindings into pilot_remap. Anything but SPAM_CONFIG_CURRENT must be written back.
spam_config_result_t spam_config_load(spam_macro_config_t *cfg);
