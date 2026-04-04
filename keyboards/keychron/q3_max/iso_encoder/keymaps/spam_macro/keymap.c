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
#include "via.h"
#include <stdlib.h>

enum layers {
    MAC_BASE,
    MAC_FN,
    WIN_BASE,
    WIN_FN,
};

static bool spam_enabled = false;
static uint16_t toggle_row = 3;   // default: G key [3,5]
static uint16_t toggle_col = 5;

static bool pilot_enabled = false;
static uint16_t pilot_toggle_row = 3;  // default: H key [3,6]
static uint16_t pilot_toggle_col = 6;

// LED indicator colors (default red)
static uint16_t spam_led_r = 255, spam_led_g = 0, spam_led_b = 0;
static uint16_t pilot_led_r = 255, pilot_led_g = 0, pilot_led_b = 0;

enum spam_key_idx { SPAM_IDX_A, SPAM_IDX_D, SPAM_IDX_S };

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
    uint8_t  phase;        // 0 = initial hold, 1 = tap hold, 2 = tap gap
    uint16_t timer;
    uint16_t keycode;
    uint16_t hold_ms;
    uint16_t tap_hold_ms;
    uint16_t gap_base;
    uint16_t gap_jitter;
    uint16_t current_gap;  // pre-computed gap for current cycle
} spam_key_t;

static spam_key_t spam_keys[] = {
    { .active = false, .phase = 0, .timer = 0, .keycode = KC_A, .hold_ms = SPAM_STEER_HOLD, .tap_hold_ms = SPAM_STEER_TAP_HOLD, .gap_base = SPAM_STEER_GAP_BASE, .gap_jitter = SPAM_STEER_GAP_JITTER },
    { .active = false, .phase = 0, .timer = 0, .keycode = KC_D, .hold_ms = SPAM_STEER_HOLD, .tap_hold_ms = SPAM_STEER_TAP_HOLD, .gap_base = SPAM_STEER_GAP_BASE, .gap_jitter = SPAM_STEER_GAP_JITTER },
    { .active = false, .phase = 0, .timer = 0, .keycode = KC_S, .hold_ms = SPAM_BRAKE_HOLD, .tap_hold_ms = SPAM_BRAKE_TAP_HOLD, .gap_base = SPAM_BRAKE_GAP_BASE, .gap_jitter = SPAM_BRAKE_GAP_JITTER },
};

#define SPAM_KEY_COUNT (sizeof(spam_keys) / sizeof(spam_keys[0]))

static uint16_t randomized_gap(spam_key_t *key) {
    if (key->gap_jitter == 0) return key->gap_base;
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
        KC_ESC,   KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,    KC_MUTE,  KC_PSCR,  KC_F23,   KC_F24,
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,    KC_BSPC,  KC_INS,   KC_HOME,  KC_PGUP,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,   KC_ENT,   KC_DEL,   KC_END,   KC_PGDN,
        KC_CAPS,  KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,  KC_NUHS,
        KC_LSFT,  KC_NUBS,  KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,             KC_RSFT,            KC_UP,
        KC_LCTL,  KC_LWIN,  KC_LALT,                               KC_SPC,                                  KC_RALT,  KC_RWIN,  MO(WIN_FN),KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT),

    /* WIN_FN: Fn layer — spam toggle is position-based (configurable via web UI) */
    [WIN_FN] = LAYOUT_iso_88(
        _______,  KC_BRID,  KC_BRIU,  KC_TASK,  KC_FILE,  RGB_VAD,  RGB_VAI,  KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,  KC_VOLU,  RGB_TOG, _______, _______, RGB_TOG,
        _______,  BT_HST1,  BT_HST2,  BT_HST3,  P2P4G,    _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______, _______, _______, _______,
        RGB_TOG,  RGB_MOD,  RGB_VAI,  RGB_HUI,  RGB_SAI,  RGB_SPI,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______, _______, _______, _______,
        _______,  RGB_RMOD, RGB_VAD,  RGB_HUD,  RGB_SAD,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  BAT_LVL,  NK_TOGG,  _______,  _______,  _______,  _______,            _______,          _______,
        _______,  GU_TOGG,  _______,                                _______,                                _______,  _______,  _______,  _______, _______, _______, _______)
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

#ifdef SNAP_CLICK_ENABLE
extern bool process_record_snap_click(uint16_t keycode, keyrecord_t *record);
#endif

void keyboard_post_init_user(void) {
    srand(timer_read32());
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_keychron_common(keycode, record)) {
        return false;
    }

    // Position-based configurable toggle: Fn + configured key
    if (record->event.key.row == toggle_row
        && record->event.key.col == toggle_col
        && layer_state_is(WIN_FN)) {
        if (record->event.pressed) {
            spam_enabled = !spam_enabled;
            if (!spam_enabled) {
                for (uint8_t i = 0; i < SPAM_KEY_COUNT; i++) {
                    if (spam_keys[i].active) {
                        spam_key_release(&spam_keys[i]);
                    }
                }
            }
        }
        return false; // swallow both press and release
    }

    // Pilot mode toggle: Fn + configured key
    if (record->event.key.row == pilot_toggle_row
        && record->event.key.col == pilot_toggle_col
        && layer_state_is(WIN_FN)) {
        if (record->event.pressed) {
            pilot_enabled = !pilot_enabled;
            if (!pilot_enabled) {
                unregister_code(KC_P8);
                unregister_code(KC_P4);
                unregister_code(KC_P5);
                unregister_code(KC_P6);
            }
        }
        return false;
    }

    // Pilot mode: remap arrows to numpad for GTA V flying
    if (pilot_enabled) {
        uint16_t replacement = 0;
        switch (keycode) {
            case KC_UP:   replacement = KC_P8; break;
            case KC_LEFT: replacement = KC_P4; break;
            case KC_DOWN: replacement = KC_P5; break;
            case KC_RGHT: replacement = KC_P6; break;
        }
        if (replacement) {
            if (record->event.pressed) {
                register_code(replacement);
            } else {
                unregister_code(replacement);
            }
            return false;
        }
    }

    // When spam is enabled, intercept A/S/D for the spam macro.
    // When spam is off, they pass through to Snap Click and normal processing.
    if (spam_enabled) {
        for (uint8_t i = 0; i < SPAM_KEY_COUNT; i++) {
            if (keycode == spam_keys[i].keycode) {
                if (record->event.pressed) {
                    spam_key_press(&spam_keys[i]);
                } else {
                    spam_key_release(&spam_keys[i]);
                }
                return false;
            }
        }
    }

#ifdef SNAP_CLICK_ENABLE
    if (!process_record_snap_click(keycode, record)) {
        return false;
    }
#endif

    return true;
}

/* ── VIA custom value interface for web tuning ── */
#define SPAM_CHANNEL_ID  0x44

enum spam_value_id {
    SPAM_VAL_STEER_HOLD = 1,
    SPAM_VAL_STEER_TAP_HOLD,
    SPAM_VAL_STEER_GAP_BASE,
    SPAM_VAL_STEER_GAP_JITTER,
    SPAM_VAL_BRAKE_HOLD,
    SPAM_VAL_BRAKE_TAP_HOLD,
    SPAM_VAL_BRAKE_GAP_BASE,
    SPAM_VAL_BRAKE_GAP_JITTER,
    SPAM_VAL_TOGGLE_ROW,      // 9
    SPAM_VAL_TOGGLE_COL,      // 10
    SPAM_VAL_ENABLED,         // 11
    PILOT_VAL_TOGGLE_ROW,     // 12
    PILOT_VAL_TOGGLE_COL,     // 13
    PILOT_VAL_ENABLED,        // 14
    SPAM_LED_R,               // 15
    SPAM_LED_G,               // 16
    SPAM_LED_B,               // 17
    PILOT_LED_R,              // 18
    PILOT_LED_G,              // 19
    PILOT_LED_B,              // 20
};

static uint16_t *spam_value_ptr(uint8_t value_id) {
    switch (value_id) {
        case SPAM_VAL_STEER_HOLD:       return &spam_keys[SPAM_IDX_A].hold_ms;
        case SPAM_VAL_STEER_TAP_HOLD:   return &spam_keys[SPAM_IDX_A].tap_hold_ms;
        case SPAM_VAL_STEER_GAP_BASE:   return &spam_keys[SPAM_IDX_A].gap_base;
        case SPAM_VAL_STEER_GAP_JITTER: return &spam_keys[SPAM_IDX_A].gap_jitter;
        case SPAM_VAL_BRAKE_HOLD:       return &spam_keys[SPAM_IDX_S].hold_ms;
        case SPAM_VAL_BRAKE_TAP_HOLD:   return &spam_keys[SPAM_IDX_S].tap_hold_ms;
        case SPAM_VAL_BRAKE_GAP_BASE:   return &spam_keys[SPAM_IDX_S].gap_base;
        case SPAM_VAL_BRAKE_GAP_JITTER: return &spam_keys[SPAM_IDX_S].gap_jitter;
        case SPAM_VAL_TOGGLE_ROW:       return &toggle_row;
        case SPAM_VAL_TOGGLE_COL:       return &toggle_col;
        case PILOT_VAL_TOGGLE_ROW:      return &pilot_toggle_row;
        case PILOT_VAL_TOGGLE_COL:      return &pilot_toggle_col;
        case SPAM_LED_R:                return &spam_led_r;
        case SPAM_LED_G:                return &spam_led_g;
        case SPAM_LED_B:                return &spam_led_b;
        case PILOT_LED_R:               return &pilot_led_r;
        case PILOT_LED_G:               return &pilot_led_g;
        case PILOT_LED_B:               return &pilot_led_b;
        default: return NULL;
    }
}

static void spam_sync_steer(void) {
    // A and D share the same steering values
    spam_keys[SPAM_IDX_D].hold_ms     = spam_keys[SPAM_IDX_A].hold_ms;
    spam_keys[SPAM_IDX_D].tap_hold_ms = spam_keys[SPAM_IDX_A].tap_hold_ms;
    spam_keys[SPAM_IDX_D].gap_base    = spam_keys[SPAM_IDX_A].gap_base;
    spam_keys[SPAM_IDX_D].gap_jitter  = spam_keys[SPAM_IDX_A].gap_jitter;
}

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    uint8_t *command_id = &data[0];
    uint8_t  channel_id = data[1];
    uint8_t  value_id   = data[2];

    if (channel_id != SPAM_CHANNEL_ID) {
        *command_id = id_unhandled;
        return;
    }

    // Handle bool values separately (not uint16_t)
    if (value_id == SPAM_VAL_ENABLED || value_id == PILOT_VAL_ENABLED) {
        bool *flag = (value_id == SPAM_VAL_ENABLED) ? &spam_enabled : &pilot_enabled;
        switch (*command_id) {
            case id_custom_get_value:
                data[3] = 0;
                data[4] = *flag ? 1 : 0;
                break;
            case id_custom_set_value:
                *flag = data[4] != 0;
                if (value_id == SPAM_VAL_ENABLED && !spam_enabled) {
                    for (uint8_t i = 0; i < SPAM_KEY_COUNT; i++) {
                        if (spam_keys[i].active) spam_key_release(&spam_keys[i]);
                    }
                }
                if (value_id == PILOT_VAL_ENABLED && !pilot_enabled) {
                    unregister_code(KC_P8);
                    unregister_code(KC_P4);
                    unregister_code(KC_P5);
                    unregister_code(KC_P6);
                }
                break;
            case id_custom_save:
                break;
        }
        return;
    }

    uint16_t *ptr = spam_value_ptr(value_id);
    if (!ptr) {
        *command_id = id_unhandled;
        return;
    }

    switch (*command_id) {
        case id_custom_get_value:
            data[3] = (*ptr >> 8) & 0xFF;
            data[4] = *ptr & 0xFF;
            break;
        case id_custom_set_value: {
            uint16_t val = ((uint16_t)data[3] << 8) | data[4];
            // Clamp to sane ranges
            if (value_id == SPAM_VAL_STEER_GAP_JITTER || value_id == SPAM_VAL_BRAKE_GAP_JITTER) {
                if (val == 0) val = 1;
                if (val > 500) val = 500;
            } else if (value_id == SPAM_VAL_STEER_TAP_HOLD || value_id == SPAM_VAL_BRAKE_TAP_HOLD) {
                if (val < 5) val = 5;
                if (val > 1000) val = 1000;
            } else if (value_id == SPAM_VAL_TOGGLE_ROW || value_id == PILOT_VAL_TOGGLE_ROW) {
                if (val > 5) val = 5;
            } else if (value_id == SPAM_VAL_TOGGLE_COL || value_id == PILOT_VAL_TOGGLE_COL) {
                if (val > 16) val = 16;
            } else if (value_id >= SPAM_LED_R && value_id <= PILOT_LED_B) {
                if (val > 255) val = 255;
            } else {
                if (val < 5) val = 5;
                if (val > 2000) val = 2000;
            }
            *ptr = val;
            if (value_id >= SPAM_VAL_STEER_HOLD && value_id <= SPAM_VAL_STEER_GAP_JITTER) {
                spam_sync_steer();
            }
            break;
        }
        case id_custom_save:
            // Values are RAM-only; no EEPROM save needed
            break;
    }
}

void matrix_scan_user(void) {
    if (!spam_enabled) return;
    for (uint8_t i = 0; i < SPAM_KEY_COUNT; i++) {
        spam_key_t *key = &spam_keys[i];
        if (!key->active) continue;

        switch (key->phase) {
            case 0: // Initial hold: key held down, wheels turning to full lock
                if (timer_elapsed(key->timer) > key->hold_ms) {
                    unregister_code(key->keycode);
                    key->phase       = 2;
                    key->timer       = timer_read();
                    key->current_gap = randomized_gap(key);
                }
                break;

            case 1: // Tap hold: key is pressed down during a tap
                if (timer_elapsed(key->timer) > key->tap_hold_ms) {
                    unregister_code(key->keycode);
                    key->phase       = 2;
                    key->timer       = timer_read();
                    key->current_gap = randomized_gap(key);
                }
                break;

            case 2: // Gap: key is released briefly between taps
                if (timer_elapsed(key->timer) > key->current_gap) {
                    register_code(key->keycode);
                    key->phase = 1;
                    key->timer = timer_read();
                }
                break;
        }
    }
}

#ifdef RGB_MATRIX_ENABLE
bool rgb_matrix_indicators_user(void) {
    if (host_keyboard_led_state().caps_lock) {
        rgb_matrix_set_color(50, 255, 0, 0);
    }
    if (spam_enabled) {
        uint8_t r = spam_led_r, g = spam_led_g, b = spam_led_b;
        rgb_matrix_set_color(51, r, g, b);  // A
        rgb_matrix_set_color(52, r, g, b);  // S
        rgb_matrix_set_color(53, r, g, b);  // D
    }
    if (pilot_enabled) {
        uint8_t r = pilot_led_r, g = pilot_led_g, b = pilot_led_b;
        rgb_matrix_set_color(76, r, g, b);  // Up
        rgb_matrix_set_color(85, r, g, b);  // Left
        rgb_matrix_set_color(86, r, g, b);  // Down
        rgb_matrix_set_color(87, r, g, b);  // Right
    }
    return true;
}
#endif
