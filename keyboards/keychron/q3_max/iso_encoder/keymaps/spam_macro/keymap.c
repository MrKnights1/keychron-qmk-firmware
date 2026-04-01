/* Copyright 2023 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H
#include "keychron_common.h"
#include <stdlib.h>

enum layers {
    MAC_BASE,
    MAC_FN,
    WIN_BASE,
    WIN_FN,
};

enum custom_keycodes {
    SPAM_A = NEW_SAFE_RANGE,
    SPAM_D,
    SPAM_S,
    SPAM_TOGGLE,
};

static bool spam_enabled = false;

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

typedef struct {
    bool     active;
    uint8_t  phase;       // 0 = initial hold, 1 = tap hold, 2 = tap gap
    uint16_t timer;
    uint16_t keycode;
    uint16_t hold_ms;
    uint16_t tap_hold_ms;
    uint16_t gap_base;
    uint16_t gap_jitter;
} spam_key_t;

static spam_key_t spam_keys[] = {
    { .active = false, .phase = 0, .timer = 0, .keycode = KC_A, .hold_ms = SPAM_STEER_HOLD, .tap_hold_ms = SPAM_STEER_TAP_HOLD, .gap_base = SPAM_STEER_GAP_BASE, .gap_jitter = SPAM_STEER_GAP_JITTER },
    { .active = false, .phase = 0, .timer = 0, .keycode = KC_D, .hold_ms = SPAM_STEER_HOLD, .tap_hold_ms = SPAM_STEER_TAP_HOLD, .gap_base = SPAM_STEER_GAP_BASE, .gap_jitter = SPAM_STEER_GAP_JITTER },
    { .active = false, .phase = 0, .timer = 0, .keycode = KC_S, .hold_ms = SPAM_BRAKE_HOLD, .tap_hold_ms = SPAM_BRAKE_TAP_HOLD, .gap_base = SPAM_BRAKE_GAP_BASE, .gap_jitter = SPAM_BRAKE_GAP_JITTER },
};

#define SPAM_KEY_COUNT (sizeof(spam_keys) / sizeof(spam_keys[0]))

static uint16_t randomized_gap(spam_key_t *key) {
    return key->gap_base + (rand() % key->gap_jitter);
}

static void spam_key_press(spam_key_t *key) {
    if (spam_enabled) {
        register_code(key->keycode);   // hold key down to start turning
        key->active = true;
        key->phase  = 0;               // initial hold phase
        key->timer  = timer_read();
    } else {
        register_code(key->keycode);
    }
}

static void spam_key_release(spam_key_t *key) {
    key->active = false;
    key->phase  = 0;
    unregister_code(key->keycode);
}

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [MAC_BASE] = LAYOUT_iso_88(
        KC_ESC,   KC_BRID,  KC_BRIU,  KC_MCTRL, KC_LNPAD, RGB_VAD,  RGB_VAI,  KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,  KC_VOLU,   KC_MUTE,  KC_SNAP,  KC_SIRI,  RGB_MOD,
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,    KC_BSPC,  KC_INS,   KC_HOME,  KC_PGUP,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,   KC_ENT,   KC_DEL,   KC_END,   KC_PGDN,
        KC_CAPS,  KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,  KC_NUHS,
        KC_LSFT,  KC_NUBS,  KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,             KC_RSFT,            KC_UP,
        KC_LCTL,  KC_LOPTN, KC_LCMMD,                               KC_SPC,                                 KC_RCMMD, KC_ROPTN, MO(MAC_FN),KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT),

    [MAC_FN] = LAYOUT_iso_88(
        _______,  KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,    RGB_TOG,  _______,   _______, RGB_TOG,
        _______,  BT_HST1,  BT_HST2,  BT_HST3,  P2P4G,    _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,   _______,  _______,   _______, _______,
        RGB_TOG,  RGB_MOD,  RGB_VAI,  RGB_HUI,  RGB_SAI,  RGB_SPI,  _______,  _______,  _______,  _______,  _______,  _______,  _______,   _______,  _______,   _______, _______,
        _______,  RGB_RMOD, RGB_VAD,  RGB_HUD,  RGB_SAD,  RGB_SPD,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  BAT_LVL,  NK_TOGG,  _______,  _______,  _______,  _______,             _______,             _______,
        _______,  _______,  _______,                                _______,                                _______,  _______,  _______,   _______,  _______,   _______, _______),

    [WIN_BASE] = LAYOUT_iso_88(
        KC_ESC,   KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,    KC_MUTE,  KC_PSCR,  KC_CTANA, RGB_MOD,
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,    KC_BSPC,  KC_INS,   KC_HOME,  KC_PGUP,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,   KC_ENT,   KC_DEL,   KC_END,   KC_PGDN,
        KC_CAPS,  SPAM_A,   SPAM_S,   SPAM_D,   KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,  KC_NUHS,
        KC_LSFT,  KC_NUBS,  KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,             KC_RSFT,            KC_UP,
        KC_LCTL,  KC_LWIN,  KC_LALT,                               KC_SPC,                                  KC_RALT,  KC_RWIN,  MO(WIN_FN),KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT),

    /* WIN_FN: Fn layer — Fn+G toggles spam mode */
    [WIN_FN] = LAYOUT_iso_88(
        _______,  KC_BRID,  KC_BRIU,  KC_TASK,  KC_FILE,  RGB_VAD,  RGB_VAI,  KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,  KC_VOLU,  RGB_TOG, _______, _______, RGB_TOG,
        _______,  BT_HST1,  BT_HST2,  BT_HST3,  P2P4G,    _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______, _______, _______, _______,
        RGB_TOG,  RGB_MOD,  RGB_VAI,  RGB_HUI,  RGB_SAI,  RGB_SPI,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______, _______, _______, _______,
        _______,  RGB_RMOD, RGB_VAD,  RGB_HUD,  RGB_SAD,  SPAM_TOGGLE, _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  BAT_LVL,  NK_TOGG,  _______,  _______,  _______,  _______,            _______,          _______,
        _______,  _______,  _______,                                _______,                                _______,  _______,  _______,  _______, _______, _______, _______)
};

// clang-format on
#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [MAC_BASE] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [MAC_FN]   = {ENCODER_CCW_CW(RGB_VAD, RGB_VAI)},
    [WIN_BASE] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [WIN_FN]   = {ENCODER_CCW_CW(RGB_VAD, RGB_VAI)},
};
#endif // ENCODER_MAP_ENABLE

void keyboard_post_init_user(void) {
    srand(timer_read32());
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_keychron_common(keycode, record)) {
        return false;
    }

    switch (keycode) {
        case SPAM_TOGGLE:
            if (record->event.pressed) {
                spam_enabled = !spam_enabled;
                if (!spam_enabled) {
                    for (uint8_t i = 0; i < SPAM_KEY_COUNT; i++) {
                        spam_keys[i].active = false;
                    }
                }
            }
            return false;

        case SPAM_A:
            if (record->event.pressed) {
                spam_key_press(&spam_keys[0]);
            } else {
                spam_key_release(&spam_keys[0]);
            }
            return false;

        case SPAM_D:
            if (record->event.pressed) {
                spam_key_press(&spam_keys[1]);
            } else {
                spam_key_release(&spam_keys[1]);
            }
            return false;

        case SPAM_S:
            if (record->event.pressed) {
                spam_key_press(&spam_keys[2]);
            } else {
                spam_key_release(&spam_keys[2]);
            }
            return false;
    }
    return true;
}

void matrix_scan_user(void) {
    if (!spam_enabled) return;
    for (uint8_t i = 0; i < SPAM_KEY_COUNT; i++) {
        spam_key_t *key = &spam_keys[i];
        if (!key->active) continue;

        switch (key->phase) {
            case 0: // Initial hold: key held down, wheels turning to full lock
                if (timer_elapsed(key->timer) > key->hold_ms) {
                    unregister_code(key->keycode);  // brief release
                    key->phase = 2;                 // go to gap
                    key->timer = timer_read();
                }
                break;

            case 1: // Tap hold: key is pressed down during a tap
                if (timer_elapsed(key->timer) > key->tap_hold_ms) {
                    unregister_code(key->keycode);  // release
                    key->phase = 2;                 // go to gap
                    key->timer = timer_read();
                }
                break;

            case 2: // Gap: key is released briefly between taps
                if (timer_elapsed(key->timer) > randomized_gap(key)) {
                    register_code(key->keycode);    // press again
                    key->phase = 1;                 // go to tap hold
                    key->timer = timer_read();
                }
                break;
        }
    }
}
