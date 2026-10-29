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


#include "gtest/gtest.h"

extern "C" {
#include "keyboards/keychron/q3_max/iso_encoder/keymaps/spam_macro/modes.h"
}

namespace {
const modes_t OFF        = {.spam = false, .pilot = false};
const modes_t SPAM_ONLY  = {.spam = true, .pilot = false};
const modes_t PILOT_ONLY = {.spam = false, .pilot = true};
} // namespace

TEST(Modes, turning_spam_on_turns_pilot_off) {
    modes_t next = modes_set_spam(PILOT_ONLY, true);

    EXPECT_TRUE(next.spam);
    EXPECT_FALSE(next.pilot);
}

TEST(Modes, turning_pilot_on_turns_spam_off) {
    modes_t next = modes_set_pilot(SPAM_ONLY, true);

    EXPECT_FALSE(next.spam);
    EXPECT_TRUE(next.pilot);
}

TEST(Modes, turning_spam_off_leaves_pilot_on) {
    modes_t next = modes_set_spam(PILOT_ONLY, false);

    EXPECT_FALSE(next.spam);
    EXPECT_TRUE(next.pilot);
}

TEST(Modes, turning_pilot_off_leaves_spam_on) {
    modes_t next = modes_set_pilot(SPAM_ONLY, false);

    EXPECT_TRUE(next.spam);
    EXPECT_FALSE(next.pilot);
}

TEST(Modes, turning_spam_on_from_off_enables_only_spam) {
    modes_t next = modes_set_spam(OFF, true);

    EXPECT_TRUE(next.spam);
    EXPECT_FALSE(next.pilot);
}

TEST(Modes, leaving_windows_turns_both_off) {
    EXPECT_FALSE(modes_on_default_layer(SPAM_ONLY, false).spam);
    EXPECT_FALSE(modes_on_default_layer(PILOT_ONLY, false).pilot);
}

TEST(Modes, windows_layer_keeps_modes) {
    modes_t next = modes_on_default_layer(PILOT_ONLY, true);

    EXPECT_FALSE(next.spam);
    EXPECT_TRUE(next.pilot);
}
