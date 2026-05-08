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
#include "eeconfig_kb.h"
#include "eeconfig.h"
#include <stdlib.h>
#ifdef LK_WIRELESS_ENABLE
#    include "battery.h"
#endif

enum layers {
    MAC_BASE,
    MAC_FN,
    WIN_BASE,
    WIN_FN,
};

/* ── EEPROM-backed configuration ── */
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
} spam_macro_config_t;

_Static_assert(sizeof(spam_macro_config_t) == EECONFIG_SIZE_SPAM_MACRO,
               "spam_macro_config_t size mismatch with EECONFIG_SIZE_SPAM_MACRO");

#define DEFAULT_SPAM_TOGGLE_ROW   3  // G key [3,5]
#define DEFAULT_SPAM_TOGGLE_COL   5
#define DEFAULT_PILOT_TOGGLE_ROW  3  // H key [3,6]
#define DEFAULT_PILOT_TOGGLE_COL  6
#define DEFAULT_LED_R  255
#define DEFAULT_LED_G  0
#define DEFAULT_LED_B  0

static bool spam_enabled = false;
// uint16_t required — spam_value_ptr() returns uint16_t* for VIA protocol.
// VIA clamp ensures values fit in uint8_t for EEPROM save.
static uint16_t toggle_row = DEFAULT_SPAM_TOGGLE_ROW;
static uint16_t toggle_col = DEFAULT_SPAM_TOGGLE_COL;

static bool pilot_enabled = false;
static uint16_t pilot_toggle_row = DEFAULT_PILOT_TOGGLE_ROW;
static uint16_t pilot_toggle_col = DEFAULT_PILOT_TOGGLE_COL;

static uint16_t spam_led_r = DEFAULT_LED_R, spam_led_g = DEFAULT_LED_G, spam_led_b = DEFAULT_LED_B;
static uint16_t pilot_led_r = DEFAULT_LED_R, pilot_led_g = DEFAULT_LED_G, pilot_led_b = DEFAULT_LED_B;

static uint32_t flash_timer = 0;
static bool     flash_on    = false;
#define FLASH_DURATION_MS 300

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

/* ── EEPROM persistence ── */

void spam_macro_config_reset(void) {
    spam_macro_config_t cfg = {
        .steer_hold_ms     = SPAM_STEER_HOLD,
        .steer_tap_hold_ms = SPAM_STEER_TAP_HOLD,
        .steer_gap_base    = SPAM_STEER_GAP_BASE,
        .steer_gap_jitter  = SPAM_STEER_GAP_JITTER,
        .brake_hold_ms     = SPAM_BRAKE_HOLD,
        .brake_tap_hold_ms = SPAM_BRAKE_TAP_HOLD,
        .brake_gap_base    = SPAM_BRAKE_GAP_BASE,
        .brake_gap_jitter  = SPAM_BRAKE_GAP_JITTER,
        .toggle_row        = DEFAULT_SPAM_TOGGLE_ROW,
        .toggle_col        = DEFAULT_SPAM_TOGGLE_COL,
        .pilot_toggle_row  = DEFAULT_PILOT_TOGGLE_ROW,
        .pilot_toggle_col  = DEFAULT_PILOT_TOGGLE_COL,
        .spam_led_r = DEFAULT_LED_R, .spam_led_g = DEFAULT_LED_G, .spam_led_b = DEFAULT_LED_B,
        .pilot_led_r = DEFAULT_LED_R, .pilot_led_g = DEFAULT_LED_G, .pilot_led_b = DEFAULT_LED_B,
    };
    eeprom_update_block(&cfg, (uint8_t *)(EECONFIG_BASE_SPAM_MACRO), sizeof(cfg));
}

static bool spam_macro_config_valid(const spam_macro_config_t *cfg) {
    if (cfg->steer_hold_ms < 5 || cfg->steer_hold_ms > 2000) return false;
    if (cfg->steer_tap_hold_ms < 5 || cfg->steer_tap_hold_ms > 1000) return false;
    if (cfg->steer_gap_base < 5 || cfg->steer_gap_base > 2000) return false;
    if (cfg->steer_gap_jitter > 500) return false;
    if (cfg->brake_hold_ms < 5 || cfg->brake_hold_ms > 2000) return false;
    if (cfg->brake_tap_hold_ms < 5 || cfg->brake_tap_hold_ms > 1000) return false;
    if (cfg->brake_gap_base < 5 || cfg->brake_gap_base > 2000) return false;
    if (cfg->brake_gap_jitter > 500) return false;
    if (cfg->toggle_row > 5 || cfg->toggle_col > 16) return false;
    if (cfg->pilot_toggle_row > 5 || cfg->pilot_toggle_col > 16) return false;
    return true;
}

static void spam_macro_init(void) {
    spam_macro_config_t cfg;
    eeprom_read_block(&cfg, (uint8_t *)(EECONFIG_BASE_SPAM_MACRO), sizeof(cfg));

    if (!spam_macro_config_valid(&cfg)) {
        spam_macro_config_reset();
        eeprom_read_block(&cfg, (uint8_t *)(EECONFIG_BASE_SPAM_MACRO), sizeof(cfg));
    }

    // Load timing into spam_keys array
    spam_keys[SPAM_IDX_A].hold_ms     = cfg.steer_hold_ms;
    spam_keys[SPAM_IDX_A].tap_hold_ms = cfg.steer_tap_hold_ms;
    spam_keys[SPAM_IDX_A].gap_base    = cfg.steer_gap_base;
    spam_keys[SPAM_IDX_A].gap_jitter  = cfg.steer_gap_jitter;
    spam_keys[SPAM_IDX_D].hold_ms     = cfg.steer_hold_ms;
    spam_keys[SPAM_IDX_D].tap_hold_ms = cfg.steer_tap_hold_ms;
    spam_keys[SPAM_IDX_D].gap_base    = cfg.steer_gap_base;
    spam_keys[SPAM_IDX_D].gap_jitter  = cfg.steer_gap_jitter;
    spam_keys[SPAM_IDX_S].hold_ms     = cfg.brake_hold_ms;
    spam_keys[SPAM_IDX_S].tap_hold_ms = cfg.brake_tap_hold_ms;
    spam_keys[SPAM_IDX_S].gap_base    = cfg.brake_gap_base;
    spam_keys[SPAM_IDX_S].gap_jitter  = cfg.brake_gap_jitter;

    // Load toggle positions
    toggle_row       = cfg.toggle_row;
    toggle_col       = cfg.toggle_col;
    pilot_toggle_row = cfg.pilot_toggle_row;
    pilot_toggle_col = cfg.pilot_toggle_col;

    // Load LED colors
    spam_led_r  = cfg.spam_led_r;
    spam_led_g  = cfg.spam_led_g;
    spam_led_b  = cfg.spam_led_b;
    pilot_led_r = cfg.pilot_led_r;
    pilot_led_g = cfg.pilot_led_g;
    pilot_led_b = cfg.pilot_led_b;
}

// spam_enabled/pilot_enabled deliberately not persisted — always boot OFF
static void spam_macro_save(void) {
    spam_macro_config_t cfg = {
        .steer_hold_ms     = spam_keys[SPAM_IDX_A].hold_ms,
        .steer_tap_hold_ms = spam_keys[SPAM_IDX_A].tap_hold_ms,
        .steer_gap_base    = spam_keys[SPAM_IDX_A].gap_base,
        .steer_gap_jitter  = spam_keys[SPAM_IDX_A].gap_jitter,
        .brake_hold_ms     = spam_keys[SPAM_IDX_S].hold_ms,
        .brake_tap_hold_ms = spam_keys[SPAM_IDX_S].tap_hold_ms,
        .brake_gap_base    = spam_keys[SPAM_IDX_S].gap_base,
        .brake_gap_jitter  = spam_keys[SPAM_IDX_S].gap_jitter,
        .toggle_row        = (uint8_t)toggle_row,
        .toggle_col        = (uint8_t)toggle_col,
        .pilot_toggle_row  = (uint8_t)pilot_toggle_row,
        .pilot_toggle_col  = (uint8_t)pilot_toggle_col,
        .spam_led_r = (uint8_t)spam_led_r, .spam_led_g = (uint8_t)spam_led_g, .spam_led_b = (uint8_t)spam_led_b,
        .pilot_led_r = (uint8_t)pilot_led_r, .pilot_led_g = (uint8_t)pilot_led_g, .pilot_led_b = (uint8_t)pilot_led_b,
    };
    eeprom_update_block(&cfg, (uint8_t *)(EECONFIG_BASE_SPAM_MACRO), sizeof(cfg));
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
    spam_macro_init();
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
            flash_timer = timer_read32();
            flash_on    = spam_enabled;
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
            flash_timer = timer_read32();
            flash_on    = pilot_enabled;
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
    SPAM_VAL_BOOTLOADER,      // 21
    SPAM_VAL_BATTERY,         // 22
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

    // Bootloader entry: SET with magic 0xBEEF triggers DFU mode
    if (value_id == SPAM_VAL_BOOTLOADER) {
        if (*command_id == id_custom_get_value) {
            data[3] = 0;
            data[4] = 0;
        } else if (*command_id == id_custom_set_value) {
            uint16_t magic = ((uint16_t)data[3] << 8) | data[4];
            if (magic == 0xBEEF) {
                bootloader_jump();
            }
        }
        return;
    }

    // Battery level: GET returns [has_battery, percentage]
    if (value_id == SPAM_VAL_BATTERY) {
        if (*command_id == id_custom_get_value) {
#ifdef LK_WIRELESS_ENABLE
            data[3] = 1;  // has battery
            data[4] = battery_get_percentage();
#else
            data[3] = 0;  // no battery
            data[4] = 0;
#endif
        }
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
                spam_macro_save();
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
            spam_macro_save();
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
    // Whole-keyboard flash on mode toggle: green = ON, red = OFF
    if (flash_timer != 0) {
        if (timer_elapsed32(flash_timer) < FLASH_DURATION_MS) {
            uint8_t r = flash_on ? 0 : 255;
            uint8_t g = flash_on ? 255 : 0;
            for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
                rgb_matrix_set_color(i, r, g, 0);
            }
            return true;
        }
        flash_timer = 0;
    }

    if (host_keyboard_led_state().caps_lock) {
        rgb_matrix_set_color(50, 255, 0, 0);
    }
    if (keymap_config.no_gui && get_highest_layer(default_layer_state) == WIN_BASE) {
        rgb_matrix_set_color(78, 255, 0, 0);  // LWin
        rgb_matrix_set_color(82, 255, 0, 0);  // RWin
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
