# Keychron Q3 Max ISO Encoder — Custom QMK Firmware

## Project Overview
- **What**: Keychron's QMK firmware with our rapid-tap macro for GTA V driving, pilot mode and a web configurator for the Q3 Max ISO encoder
- **Tech stack**: QMK firmware (C), ARM STM32F401, ChibiOS RTOS
- **Board**: Keychron Q3 Max ISO Encoder (Nordic layout, L-shaped Enter, extra key between LShift and Z)
- **Repo**: https://github.com/MrKnights1/keychron-qmk-firmware (private fork of Keychron/qmk_firmware)
- **Branch**: `wireless_playground` is our main. It is built on Keychron's `2025q3` branch (their main since late 2025), merged in rather than rebased.

## Common Commands

```bash
# Compile firmware
cd /root/muud/keychron-qmk-firmware
qmk compile -kb keychron/q3_max/iso_encoder -km spam_macro

# Output: .build/keychron_q3max_custom.bin

# Init submodules (first time, and after every Keychron merge; googletest is for the tests)
git submodule sync
git submodule update --init --recursive lib/chibios lib/chibios-contrib lib/lufa lib/printf lib/googletest

# Tests
make test:keychron_spam_macro         # firmware: pilot remap, EEPROM config, modes (tests/keychron_spam_macro/)
cd keychron-web && bun test           # web configurator (keychron-web/tests/)

# Check that nothing outside our folders differs from Keychron (before committing, after every merge)
keyboards/keychron/q3_max/iso_encoder/keymaps/spam_macro/check_upstream_clean.sh

# Install python deps (first time only)
pip install -r requirements.txt

# Flash: Hold Escape while plugging USB, then flash .bin via the web app or QMK Toolbox
# Every flash wipes saved settings (EEPROM lives in flash sector 1, which the .bin overwrites)
```

## What the Macro Does

Rapid-tap A/S/D keys when held (for smoother GTA V steering). Toggled with Fn+G.

- **Spam OFF**: A, S, D work as normal keys
- **Spam ON**: Holding A/D rapid-taps at 80-170ms (steering), S at 120-220ms (braking)
- Intervals are randomized per tap (humanized) and seeded per boot via `srand(timer_read32())`
- 10ms tap hold (`tap_code_delay`) ensures game registers each keypress
- Ghost keypress protection: spam state clears when toggling off

## Key Files

| File | Purpose |
|------|---------|
| `keyboards/keychron/q3_max/iso_encoder/keymaps/spam_macro/keymap.c` | Custom keymap with spam macro |
| `keyboards/keychron/q3_max/iso_encoder/keymaps/spam_macro/pilot_remap.c` | Pilot mode remap by physical position: fixed arrows→numpad, web-bindable Q/E/Del/End/PgDn (VIA value IDs 23–27) |
| `keyboards/keychron/q3_max/iso_encoder/keymaps/spam_macro/config.h` | EEPROM datablock size/version, pilot key positions (`PILOT_POS_*`) |
| `users/spam_macro/post_config.h` | Board overrides of Keychron's Q3 Max config (read after every config.h) |
| `keyboards/keychron/q3_max/iso_encoder/keymaps/spam_macro/spam_config.c` | EEPROM struct defaults and validation |
| `keyboards/keychron/q3_max/iso_encoder/keymaps/spam_macro/modes.c` | Spam/pilot mode rules: mutually exclusive, Windows layers only |
| `keyboards/keychron/q3_max/iso_encoder/keymaps/spam_macro/rules.mk` | Build config (VIA enabled, output name) |
| `keyboards/keychron/q3_max/iso_encoder/keymaps/spam_macro/check_upstream_clean.sh` | Fails if anything outside our folders differs from Keychron |
| `tests/keychron_spam_macro/` | QMK googletest suite for `pilot_remap.c`, `spam_config.c`, `modes.c` |
| `keychron-web/` | Web configurator + flasher (WebHID, bun) |
| `keyboards/keychron/q3_max/firmware/` | Pre-compiled .bin and stock backup |

## Working on Top of Keychron's Code

We never edit Keychron or QMK files. Everything of ours lives in `keymaps/spam_macro/`, `users/spam_macro/`, `tests/keychron_spam_macro/`, `keychron-web/`, `keyboards/keychron/q3_max/firmware/` and this file, so Keychron updates merge without conflicts. `check_upstream_clean.sh` enforces it.

**Updating to Keychron's latest:**
```bash
git fetch upstream                       # tracks upstream/2025q3
git checkout -b chore/update-keychron wireless_playground
git merge upstream/2025q3
git submodule sync && git submodule update --init --recursive lib/chibios lib/chibios-contrib lib/lufa lib/printf lib/googletest
# then: check_upstream_clean.sh, both test suites, compile, flash and try it
```
Land it with a regular merge (`git merge --no-ff`), not a squash, so the link to Keychron's history is kept.

**Board overrides** live in `users/spam_macro/post_config.h` (QMK picks `users/<keymap>/` by default and reads its `post_config.h` after every config.h). Not in keymap `config.h`: Keychron headers `#include "config.h"`, which finds the keymap's before the board's, so overrides there get redefined by the board:
- `SNLED27351_PHASE_CHANNEL` 12 channels: Keychron's 2025q3 config scans 9, which leaves Tab..Del (LEDs 33-47, driver 0 CB10-CB12) dark.
- `CAPS_LOCK_INDEX` 50: Keychron ships 51 (the A key's LED); Caps Lock `[3,0]` is LED 50 in `g_led_config`.
- `RGB_MATRIX_DEFAULT_*`: solid #07fff0 after a reset.

**ISO Enter**: Keychron's `keyboard.json` lists `[2,13]` (Enter) after row 3, so in `LAYOUT_iso_88` the Enter keycode comes after `KC_NUHS` at the end of row 3, and row 2 has 16 entries. Putting it back in row 2 shifts the whole home row by one key.

## Build Environment Notes

- **Compiler**: `arm-none-eabi-gcc` — system version (14.2) works
- **dfu-suffix error**: Harmless — appears at end of compile, .bin is created before this error
- **Expected .bin size**: ~115KB (app region is 224KB)
- Upstream remote (`upstream`) points to Keychron/qmk_firmware; Keychron's main branch is `2025q3`

## Keymap Structure

- **Layer 0 (MAC_BASE)**: Stock Mac layout
- **Layer 1 (MAC_FN)**: Stock Mac Fn layer
- **Layer 2 (WIN_BASE)**: Stock Windows layout; F23/F24 on the two top-right keys
- **Layer 3 (WIN_FN)**: Windows Fn layer; Fn+Win toggles the Win key lock

Spam and pilot handling run in `process_record_user`, before Keychron's handlers (`keychron_task.c` calls the user hook first, then `process_record_keychron`). Don't call `process_record_keychron_common` or `process_record_snap_click` from the keymap; Keychron's chain already does.

The spam and pilot toggles are matched by matrix position on WIN_FN (default G and H, configurable in the web app). Spam works on the plain `KC_A`/`KC_S`/`KC_D` keys.

**Pilot mode (Fn+H)**: arrows send NUM8/4/5/6; Q/E/Del/End/PgDn send whatever is bound in the web app (`KC_TRNS` = unchanged). Spam and pilot are mutually exclusive and both turn off on the Mac layers.

- Settings and bindings live in `spam_macro_config_t` (`spam_config.h`), stored in QMK's keymap EEPROM datablock. Changing its layout requires updating `EECONFIG_USER_DATA_SIZE` and bumping `SPAM_MACRO_CONFIG_VERSION` in keymap `config.h`; QMK then reports the stored block as invalid and the firmware writes defaults.
- Bindable keycodes are defined twice and must match: `pilot_binding_valid()` in `pilot_remap.c` and `KEYCODES` in `keychron-web/src/modules/keycodes.js` (the web test checks the firmware ranges).
- Changing VIA value IDs or their meaning requires bumping `SPAM_VIA_PROTOCOL_VERSION` in `keymap.c` and `PROTOCOL_VERSION` in `configurator.js`; the web app refuses to write on a mismatch.

## Tuning

Tuned live in the web configurator. Reset defaults are in `spam_config.h` (`SPAM_STEER_*`, `SPAM_BRAKE_*`).

## Recovery

Stock backup: `keyboards/keychron/q3_max/firmware/stock_v1.1.0_iso_encoder.bin`
Hold Escape + plug USB → QMK Toolbox → flash stock .bin

## Known Issue (TODO)

LED mapping may still need auditing — need to verify all LED indices match physical key positions across all rows, not just CapsLock.
