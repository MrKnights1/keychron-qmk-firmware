#include <stddef.h>
#include <string.h>
#include "spam_config.h"
#include "eeconfig_spam_macro.h"
#include "keycodes.h"

_Static_assert(sizeof(spam_macro_config_t) == EECONFIG_SIZE_SPAM_MACRO,
               "spam_macro_config_t size mismatch with EECONFIG_SIZE_SPAM_MACRO");

// Migration relies on the pre-binding (26-byte, unversioned) layout being an unchanged prefix.
_Static_assert(offsetof(spam_macro_config_t, pilot_binds) == 26,
               "settings fields must keep their pre-binding EEPROM offsets");

void spam_config_defaults(spam_macro_config_t *cfg) {
    *cfg = (spam_macro_config_t){
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
        .version           = SPAM_MACRO_CONFIG_VERSION,
    };
    for (uint8_t i = 0; i < PILOT_BIND_COUNT; i++) {
        cfg->pilot_binds[i] = KC_TRNS;
    }
}

static bool settings_valid(const spam_macro_config_t *cfg) {
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

static spam_config_result_t reset(spam_macro_config_t *cfg) {
    spam_config_defaults(cfg);
    pilot_remap_clear();
    return SPAM_CONFIG_RESET;
}

spam_config_result_t spam_config_load(spam_macro_config_t *cfg) {
    if (!settings_valid(cfg)) return reset(cfg);

    if (cfg->version != SPAM_MACRO_CONFIG_VERSION) {
        // Bytes past the settings prefix are whatever followed the old layout in EEPROM.
        for (uint8_t i = 0; i < PILOT_BIND_COUNT; i++) {
            cfg->pilot_binds[i] = KC_TRNS;
        }
        cfg->version = SPAM_MACRO_CONFIG_VERSION;
        pilot_remap_clear();
        return SPAM_CONFIG_MIGRATED;
    }

    // Copy out of the packed struct: pilot_remap_load needs aligned uint16_t.
    uint16_t binds[PILOT_BIND_COUNT];
    memcpy(binds, cfg->pilot_binds, sizeof(binds));
    if (!pilot_remap_load(binds)) return reset(cfg);
    return SPAM_CONFIG_CURRENT;
}
