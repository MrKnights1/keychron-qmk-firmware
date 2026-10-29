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

#include "keyboard_report_util.hpp"
#include "keycode.h"
#include "test_common.hpp"
#include "test_fixture.hpp"
#include "test_keymap_key.hpp"

extern "C" {
#include "keyboards/keychron/q3_max/iso_encoder/keymaps/spam_macro/pilot_remap.h"
}

using testing::_;

namespace {
bool pilot_enabled = false;

const keypos_t POS_Q     = PILOT_POS_Q;
const keypos_t POS_E     = PILOT_POS_E;
const keypos_t POS_DEL   = PILOT_POS_DEL;
const keypos_t POS_END   = PILOT_POS_END;
const keypos_t POS_PGDN  = PILOT_POS_PGDN;
const keypos_t POS_UP    = PILOT_POS_UP;
const keypos_t POS_LEFT  = PILOT_POS_LEFT;
const keypos_t POS_DOWN  = PILOT_POS_DOWN;
const keypos_t POS_RIGHT = PILOT_POS_RIGHT;

KeymapKey key_at(keypos_t pos, uint16_t keycode) {
    return KeymapKey(0, pos.col, pos.row, keycode);
}

bool contains(const keypos_t *positions, uint8_t count, keypos_t pos) {
    for (uint8_t i = 0; i < count; i++) {
        if (KEYEQ(positions[i], pos)) return true;
    }
    return false;
}
} // namespace

extern "C" bool process_record_user(uint16_t keycode, keyrecord_t* record) {
    return process_pilot_remap(record, pilot_enabled);
}

class PilotRemap : public TestFixture {
   public:
    void SetUp() override {
        pilot_enabled = false;
        pilot_remap_clear();
        pilot_remap_release_all();
    }
};

TEST_F(PilotRemap, bound_key_sends_original_keycode_when_pilot_off) {
    TestDriver driver;
    KeymapKey  key_q = key_at(POS_Q, KC_Q);
    set_keymap({key_q});
    pilot_remap_set_binding(PILOT_BIND_Q, KC_P7);

    EXPECT_REPORT(driver, (KC_Q));
    key_q.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_EMPTY_REPORT(driver);
    key_q.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);
}

TEST_F(PilotRemap, unbound_key_sends_original_keycode_when_pilot_on) {
    TestDriver driver;
    KeymapKey  key_e = key_at(POS_E, KC_E);
    set_keymap({key_e});
    pilot_enabled = true;

    EXPECT_REPORT(driver, (KC_E));
    key_e.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_EMPTY_REPORT(driver);
    key_e.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);
}

TEST_F(PilotRemap, bound_key_sends_binding_when_pilot_on) {
    TestDriver driver;
    KeymapKey  key_q = key_at(POS_Q, KC_Q);
    set_keymap({key_q});
    pilot_remap_set_binding(PILOT_BIND_Q, KC_P7);
    pilot_enabled = true;

    EXPECT_REPORT(driver, (KC_P7));
    key_q.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_EMPTY_REPORT(driver);
    key_q.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);
}

TEST_F(PilotRemap, every_bindable_key_sends_its_binding_when_pilot_on) {
    TestDriver driver;
    KeymapKey  key_e    = key_at(POS_E, KC_E);
    KeymapKey  key_del  = key_at(POS_DEL, KC_DEL);
    KeymapKey  key_end  = key_at(POS_END, KC_END);
    KeymapKey  key_pgdn = key_at(POS_PGDN, KC_PGDN);
    set_keymap({key_e, key_del, key_end, key_pgdn});
    pilot_remap_set_binding(PILOT_BIND_E, KC_P9);
    pilot_remap_set_binding(PILOT_BIND_DEL, KC_F13);
    pilot_remap_set_binding(PILOT_BIND_END, KC_LSFT);
    pilot_remap_set_binding(PILOT_BIND_PGDN, KC_SPC);
    pilot_enabled = true;

    testing::InSequence seq;
    EXPECT_REPORT(driver, (KC_P9));
    EXPECT_EMPTY_REPORT(driver);
    EXPECT_REPORT(driver, (KC_F13));
    EXPECT_EMPTY_REPORT(driver);
    EXPECT_REPORT(driver, (KC_LSFT));
    EXPECT_EMPTY_REPORT(driver);
    EXPECT_REPORT(driver, (KC_SPC));
    EXPECT_EMPTY_REPORT(driver);
    tap_keys(key_e, key_del, key_end, key_pgdn);
    VERIFY_AND_CLEAR(driver);
}

TEST_F(PilotRemap, arrow_sends_numpad_when_pilot_on) {
    TestDriver driver;
    KeymapKey  key_up = key_at(POS_UP, KC_UP);
    set_keymap({key_up});
    pilot_enabled = true;

    EXPECT_REPORT(driver, (KC_P8));
    key_up.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_EMPTY_REPORT(driver);
    key_up.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);
}

TEST_F(PilotRemap, arrow_sends_arrow_when_pilot_off) {
    TestDriver driver;
    KeymapKey  key_left = key_at(POS_LEFT, KC_LEFT);
    set_keymap({key_left});

    testing::InSequence seq;
    EXPECT_REPORT(driver, (KC_LEFT));
    EXPECT_EMPTY_REPORT(driver);
    tap_key(key_left);
    VERIFY_AND_CLEAR(driver);
}

TEST_F(PilotRemap, key_bound_to_none_sends_nothing_when_pilot_on) {
    TestDriver driver;
    KeymapKey  key_end = key_at(POS_END, KC_END);
    set_keymap({key_end});
    pilot_remap_set_binding(PILOT_BIND_END, KC_NO);
    pilot_enabled = true;

    EXPECT_NO_REPORT(driver);
    tap_key(key_end);
    VERIFY_AND_CLEAR(driver);
}

TEST_F(PilotRemap, same_keycode_at_another_position_is_not_remapped) {
    TestDriver driver;
    KeymapKey  moved_q(0, 5, 3, KC_Q);
    set_keymap({moved_q});
    pilot_remap_set_binding(PILOT_BIND_Q, KC_P7);
    pilot_enabled = true;

    testing::InSequence seq;
    EXPECT_REPORT(driver, (KC_Q));
    EXPECT_EMPTY_REPORT(driver);
    tap_key(moved_q);
    VERIFY_AND_CLEAR(driver);
}

TEST_F(PilotRemap, pilot_key_release_keeps_other_key_with_same_keycode_held) {
    TestDriver driver;
    KeymapKey  pilot_del = key_at(POS_DEL, KC_DEL);
    KeymapKey  other_del(0, 5, 3, KC_DEL);
    set_keymap({pilot_del, other_del});
    pilot_remap_set_binding(PILOT_BIND_DEL, KC_P1);
    pilot_enabled = true;
    EXPECT_REPORT(driver, (KC_P1));
    pilot_del.press();
    run_one_scan_loop();
    EXPECT_REPORT(driver, (KC_P1, KC_DEL));
    other_del.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_REPORT(driver, (KC_DEL));
    pilot_del.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_EMPTY_REPORT(driver);
    other_del.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);
}

TEST_F(PilotRemap, release_all_releases_held_binding_and_swallows_later_release) {
    TestDriver driver;
    KeymapKey  key_q = key_at(POS_Q, KC_Q);
    set_keymap({key_q});
    pilot_remap_set_binding(PILOT_BIND_Q, KC_P7);
    pilot_enabled = true;
    EXPECT_REPORT(driver, (KC_P7));
    key_q.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);

    EXPECT_EMPTY_REPORT(driver);
    pilot_enabled = false;
    pilot_remap_release_all();
    VERIFY_AND_CLEAR(driver);

    EXPECT_NO_REPORT(driver);
    key_q.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);
}

TEST_F(PilotRemap, release_unregisters_originally_sent_key_after_binding_changes) {
    TestDriver driver;
    KeymapKey  key_pgdn = key_at(POS_PGDN, KC_PGDN);
    set_keymap({key_pgdn});
    pilot_remap_set_binding(PILOT_BIND_PGDN, KC_P3);
    pilot_enabled = true;
    EXPECT_REPORT(driver, (KC_P3));
    key_pgdn.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);
    pilot_remap_set_binding(PILOT_BIND_PGDN, KC_P1);

    EXPECT_EMPTY_REPORT(driver);
    key_pgdn.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);
}

TEST_F(PilotRemap, key_pressed_before_pilot_on_releases_original_keycode) {
    TestDriver driver;
    KeymapKey  key_q = key_at(POS_Q, KC_Q);
    set_keymap({key_q});
    pilot_remap_set_binding(PILOT_BIND_Q, KC_P7);
    EXPECT_REPORT(driver, (KC_Q));
    key_q.press();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);
    pilot_enabled = true;

    EXPECT_EMPTY_REPORT(driver);
    key_q.release();
    run_one_scan_loop();
    VERIFY_AND_CLEAR(driver);
}

TEST_F(PilotRemap, set_binding_rejects_keycode_above_basic_range_and_keeps_previous) {
    pilot_remap_set_binding(PILOT_BIND_Q, KC_P7);

    bool accepted = pilot_remap_set_binding(PILOT_BIND_Q, KC_RIGHT_GUI + 1);

    EXPECT_FALSE(accepted);
    EXPECT_KEYCODE_EQ(pilot_remap_get_binding(PILOT_BIND_Q), KC_P7);
}

TEST_F(PilotRemap, set_binding_rejects_modded_keycode) {
    bool accepted = pilot_remap_set_binding(PILOT_BIND_E, LSFT(KC_A));

    EXPECT_FALSE(accepted);
    EXPECT_KEYCODE_EQ(pilot_remap_get_binding(PILOT_BIND_E), KC_TRNS);
}

TEST_F(PilotRemap, set_binding_rejects_slot_out_of_range) {
    bool accepted = pilot_remap_set_binding(PILOT_BIND_COUNT, KC_A);

    EXPECT_FALSE(accepted);
}

TEST_F(PilotRemap, binding_valid_accepts_both_ends_of_every_offered_range) {
    const uint16_t accepted[] = {
        KC_NO, KC_TRNS, 0x04, 0x65, 0x68, 0x73, 0xA8, 0xAE, 0xD1, 0xD5, 0xD9, 0xDA, 0xE0, 0xE7,
    };
    for (uint16_t code : accepted) {
        EXPECT_TRUE(pilot_binding_valid(code)) << "code 0x" << std::hex << code;
    }
}

TEST_F(PilotRemap, binding_valid_rejects_codes_just_outside_offered_ranges) {
    const uint16_t rejected[] = {
        0x02, 0x03, 0x66, 0x67, 0x74, 0xA7, 0xAF, 0xD0, 0xD6, 0xD8, 0xDB, 0xDF, 0xE8,
    };
    for (uint16_t code : rejected) {
        EXPECT_FALSE(pilot_binding_valid(code)) << "code 0x" << std::hex << code;
    }
}

TEST_F(PilotRemap, get_binding_returns_none_for_slot_out_of_range) {
    uint16_t binding = pilot_remap_get_binding(PILOT_BIND_COUNT);

    EXPECT_KEYCODE_EQ(binding, KC_NO);
}

TEST_F(PilotRemap, load_applies_every_binding) {
    const uint16_t binds[PILOT_BIND_COUNT] = {KC_P7, KC_P9, KC_NO, KC_TRNS, KC_F13};

    bool loaded = pilot_remap_load(binds);

    EXPECT_TRUE(loaded);
    for (uint8_t slot = 0; slot < PILOT_BIND_COUNT; slot++) {
        EXPECT_KEYCODE_EQ(pilot_remap_get_binding(slot), binds[slot]);
    }
}

TEST_F(PilotRemap, load_applies_nothing_when_one_binding_is_invalid) {
    pilot_remap_set_binding(PILOT_BIND_PGDN, KC_P3);
    const uint16_t binds[PILOT_BIND_COUNT] = {KC_P7, KC_P9, LSFT(KC_A), KC_P1, KC_P2};

    bool loaded = pilot_remap_load(binds);

    EXPECT_FALSE(loaded);
    EXPECT_KEYCODE_EQ(pilot_remap_get_binding(PILOT_BIND_Q), KC_TRNS);
    EXPECT_KEYCODE_EQ(pilot_remap_get_binding(PILOT_BIND_PGDN), KC_P3);
}

TEST_F(PilotRemap, clear_unbinds_every_slot) {
    pilot_remap_set_binding(PILOT_BIND_Q, KC_P7);
    pilot_remap_set_binding(PILOT_BIND_END, KC_NO);

    pilot_remap_clear();

    for (uint8_t slot = 0; slot < PILOT_BIND_COUNT; slot++) {
        EXPECT_KEYCODE_EQ(pilot_remap_get_binding(slot), KC_TRNS);
    }
}

TEST_F(PilotRemap, via_get_writes_binding_big_endian) {
    pilot_remap_set_binding(PILOT_BIND_E, KC_P7);
    uint8_t value[2] = {0xAA, 0xAA};

    bool handled = pilot_remap_via_get(PILOT_BIND_E, value);

    EXPECT_TRUE(handled);
    EXPECT_EQ(value[0], 0x00);
    EXPECT_EQ(value[1], KC_P7);
}

TEST_F(PilotRemap, via_get_rejects_slot_out_of_range) {
    uint8_t value[2] = {0xAA, 0xAA};

    bool handled = pilot_remap_via_get(PILOT_BIND_COUNT, value);

    EXPECT_FALSE(handled);
    EXPECT_EQ(value[0], 0xAA);
}

TEST_F(PilotRemap, via_set_reads_binding_big_endian) {
    const uint8_t value[2] = {0x00, KC_P7};

    bool handled = pilot_remap_via_set(PILOT_BIND_DEL, value);

    EXPECT_TRUE(handled);
    EXPECT_KEYCODE_EQ(pilot_remap_get_binding(PILOT_BIND_DEL), KC_P7);
}

TEST_F(PilotRemap, via_set_rejects_invalid_code_and_keeps_previous) {
    pilot_remap_set_binding(PILOT_BIND_DEL, KC_P1);
    const uint8_t little_endian_p7[2] = {KC_P7, 0x00};

    bool handled = pilot_remap_via_set(PILOT_BIND_DEL, little_endian_p7);

    EXPECT_FALSE(handled);
    EXPECT_KEYCODE_EQ(pilot_remap_get_binding(PILOT_BIND_DEL), KC_P1);
}

TEST_F(PilotRemap, lit_keys_are_only_the_arrows_when_nothing_is_bound) {
    keypos_t lit[PILOT_KEY_COUNT];

    uint8_t count = pilot_remap_lit_keys(lit);

    EXPECT_EQ(count, 4);
    EXPECT_TRUE(contains(lit, count, POS_UP));
    EXPECT_TRUE(contains(lit, count, POS_LEFT));
    EXPECT_TRUE(contains(lit, count, POS_DOWN));
    EXPECT_TRUE(contains(lit, count, POS_RIGHT));
}

TEST_F(PilotRemap, lit_keys_include_slots_bound_to_a_key_or_none) {
    pilot_remap_set_binding(PILOT_BIND_E, KC_NO);
    pilot_remap_set_binding(PILOT_BIND_PGDN, KC_P1);
    keypos_t lit[PILOT_KEY_COUNT];

    uint8_t count = pilot_remap_lit_keys(lit);

    EXPECT_EQ(count, 6);
    EXPECT_TRUE(contains(lit, count, POS_E));
    EXPECT_TRUE(contains(lit, count, POS_PGDN));
    EXPECT_FALSE(contains(lit, count, POS_Q));
}
