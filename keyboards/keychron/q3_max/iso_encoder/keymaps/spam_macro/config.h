#pragma once

// Board overrides live in users/spam_macro/post_config.h, which QMK reads after every config.h.

// Spam macro settings live in QMK's keymap EEPROM datablock; the size must equal sizeof(spam_macro_config_t).
// Bump the version whenever that struct's layout changes: QMK then treats the stored block as invalid.
#define SPAM_MACRO_CONFIG_VERSION 1
#define EECONFIG_USER_DATA_SIZE 36
#define EECONFIG_USER_DATA_VERSION SPAM_MACRO_CONFIG_VERSION

// Pilot-mode keys by matrix position (see keyboard.json).
#define PILOT_POS_Q     {.col = 1, .row = 2}
#define PILOT_POS_E     {.col = 3, .row = 2}
#define PILOT_POS_DEL   {.col = 14, .row = 2}
#define PILOT_POS_END   {.col = 15, .row = 2}
#define PILOT_POS_PGDN  {.col = 16, .row = 2}
#define PILOT_POS_UP    {.col = 15, .row = 4}
#define PILOT_POS_LEFT  {.col = 14, .row = 5}
#define PILOT_POS_DOWN  {.col = 15, .row = 5}
#define PILOT_POS_RIGHT {.col = 16, .row = 5}
