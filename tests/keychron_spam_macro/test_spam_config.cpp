// Copyright 2026 MrKnights1
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#include <cstring>
#include "gtest/gtest.h"
#include "keycode.h"

extern "C" {
#include "keyboards/keychron/q3_max/iso_encoder/keymaps/spam_macro/spam_config.h"
}

namespace {
spam_macro_config_t defaults() {
    spam_macro_config_t cfg;
    spam_config_defaults(&cfg);
    return cfg;
}

spam_macro_config_t tuned() {
    spam_macro_config_t cfg = defaults();
    cfg.steer_hold_ms    = 300;
    cfg.brake_gap_jitter = 0;
    cfg.toggle_row       = 2;
    cfg.toggle_col       = 1;
    cfg.pilot_led_g      = 128;
    return cfg;
}

bool same(const spam_macro_config_t &a, const spam_macro_config_t &b) {
    return std::memcmp(&a, &b, sizeof(a)) == 0;
}
} // namespace

class SpamConfig : public testing::Test {
   protected:
    void SetUp() override {
        pilot_remap_clear();
    }
};

TEST_F(SpamConfig, defaults_are_kept) {
    spam_macro_config_t cfg = defaults();

    bool kept = spam_config_load(&cfg);

    EXPECT_TRUE(kept);
    EXPECT_TRUE(same(cfg, defaults()));
}

TEST_F(SpamConfig, current_config_is_kept_and_its_bindings_loaded) {
    spam_macro_config_t cfg            = tuned();
    cfg.pilot_binds[PILOT_BIND_Q]      = KC_P7;
    cfg.pilot_binds[PILOT_BIND_PGDN]   = KC_NO;
    spam_macro_config_t before         = cfg;

    bool kept = spam_config_load(&cfg);

    EXPECT_TRUE(kept);
    EXPECT_TRUE(same(cfg, before));
    EXPECT_EQ(pilot_remap_get_binding(PILOT_BIND_Q), KC_P7);
    EXPECT_EQ(pilot_remap_get_binding(PILOT_BIND_PGDN), KC_NO);
}

TEST_F(SpamConfig, out_of_range_timing_resets_to_defaults) {
    spam_macro_config_t cfg = tuned();
    cfg.steer_hold_ms       = 0;

    bool kept = spam_config_load(&cfg);

    EXPECT_FALSE(kept);
    EXPECT_TRUE(same(cfg, defaults()));
}

TEST_F(SpamConfig, out_of_range_toggle_resets_to_defaults) {
    spam_macro_config_t cfg = tuned();
    cfg.pilot_toggle_col    = 17;

    bool kept = spam_config_load(&cfg);

    EXPECT_FALSE(kept);
    EXPECT_TRUE(same(cfg, defaults()));
}

TEST_F(SpamConfig, erased_eeprom_resets_to_defaults) {
    spam_macro_config_t cfg;
    std::memset(&cfg, 0xFF, sizeof(cfg));

    bool kept = spam_config_load(&cfg);

    EXPECT_FALSE(kept);
    EXPECT_TRUE(same(cfg, defaults()));
}

TEST_F(SpamConfig, zeroed_block_resets_to_defaults) {
    spam_macro_config_t cfg;
    std::memset(&cfg, 0x00, sizeof(cfg)); // what QMK returns when the datablock's version stamp doesn't match

    bool kept = spam_config_load(&cfg);

    EXPECT_FALSE(kept);
    EXPECT_TRUE(same(cfg, defaults()));
}

TEST_F(SpamConfig, invalid_binding_resets_without_loading_any_binding) {
    pilot_remap_set_binding(PILOT_BIND_END, KC_P3);
    spam_macro_config_t cfg      = tuned();
    cfg.pilot_binds[PILOT_BIND_Q] = KC_P1;
    cfg.pilot_binds[PILOT_BIND_E] = KC_RIGHT_GUI + 1;

    bool kept = spam_config_load(&cfg);

    EXPECT_FALSE(kept);
    EXPECT_TRUE(same(cfg, defaults()));
    for (uint8_t slot = 0; slot < PILOT_BIND_COUNT; slot++) {
        EXPECT_EQ(pilot_remap_get_binding(slot), KC_TRNS);
    }
}
