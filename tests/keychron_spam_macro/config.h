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

#pragma once

#include "test_common.h"

// Pilot keys inside the 4x10 test matrix: bindable slots on row 0, arrows on row 1.
#define PILOT_POS_Q     {.col = 0, .row = 0}
#define PILOT_POS_E     {.col = 1, .row = 0}
#define PILOT_POS_DEL   {.col = 2, .row = 0}
#define PILOT_POS_END   {.col = 3, .row = 0}
#define PILOT_POS_PGDN  {.col = 4, .row = 0}
#define PILOT_POS_UP    {.col = 0, .row = 1}
#define PILOT_POS_LEFT  {.col = 1, .row = 1}
#define PILOT_POS_DOWN  {.col = 2, .row = 1}
#define PILOT_POS_RIGHT {.col = 3, .row = 1}
