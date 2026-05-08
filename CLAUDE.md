# Keychron Q3 Max ISO Encoder — Custom QMK Firmware

## Project Overview
- **What**: Forked Keychron QMK firmware with custom rapid-tap macro for GTA V driving and multiple bug fixes for the Q3 Max ISO encoder
- **Tech stack**: QMK firmware (C), ARM STM32F401, ChibiOS RTOS
- **Board**: Keychron Q3 Max ISO Encoder (Nordic layout, L-shaped Enter, extra key between LShift and Z)
- **Repo**: https://github.com/MrKnights1/keychron-qmk-firmware (private fork of Keychron/qmk_firmware)
- **Branch**: `wireless_playground` (Keychron's main branch for wireless keyboards, treat as main)

## Common Commands

```bash
# Compile firmware
cd /root/muud/keychron-qmk-firmware
qmk compile -kb keychron/q3_max/iso_encoder -km spam_macro

# Output: .build/keychron_q3max_custom.bin

# Init submodules (first time only)
git submodule update --init lib/chibios lib/chibios-contrib lib/lufa lib/printf

# Install python deps (first time only)
pip install -r requirements.txt

# Flash: Hold Escape while plugging USB, then flash .bin via QMK Toolbox
# Factory reset after flash: Fn + J + Z for 4 seconds
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
| `keyboards/keychron/q3_max/iso_encoder/keymaps/spam_macro/rules.mk` | Build config (VIA enabled) |
| `keyboards/keychron/q3_max/iso_encoder/info.json` | Layout definition (FIXED: ISO Enter position) |
| `keyboards/keychron/q3_max/iso_encoder/iso_encoder.c` | LED driver table + per-key RGB defaults |
| `keyboards/keychron/q3_max/iso_encoder/config.h` | CAPS_LOCK_INDEX and RGB config |
| `keyboards/keychron/q3_max/q3_max.c` | Board init (FIXED: uses keychron_common_init) |
| `keyboards/keychron/q3_max/config.h` | Board config (FIXED: added eeconfig_kb.h) |
| `keyboards/keychron/q3_max/rules.mk` | Build flags (FIXED: added debounce/snap/rgb enables) |
| `keyboards/keychron/common/eeconfig_kb.h` | EEPROM config (FIXED: undef before redefine) |
| `keyboards/keychron/q3_max/firmware/` | Pre-compiled .bin and stock backup |

## Bugs We Fixed in Keychron's Repo

Keychron's open-source repo has multiple issues for the Q3 Max ISO. These are all committed:

1. **info.json ISO Enter position** — `[2,13]` (Enter) was listed after row 3 instead of within row 2. This caused the LAYOUT macro to shift the entire home row by one position (A acted as CapsLock, S as A, D as S, etc.)

2. **CAPS_LOCK_INDEX** — Was 51 (A key's LED) instead of 50 (CapsLock's LED). Same off-by-one from the ISO Enter issue.

3. **rules.mk missing feature flags** — `DEBOUNCE_TYPE=custom`, `SNAP_CLICK_ENABLE=yes`, `KEYCHRON_RGB_ENABLE=yes` were missing. These are needed for v1.1.0 features (configurable debounce, snap action, per-key RGB). V3 Max had them but Q3 Max didn't.

4. **q3_max.c outdated board init** — Used old manual wireless/LED init instead of `keychron_common_init()` which handles everything. Updated to match V3 Max pattern.

5. **config.h missing include** — `eeconfig_kb.h` not included, needed for EEPROM-based config.

6. **eeconfig_kb.h double define** — `EECONFIG_KB_DATA_SIZE` was defined by both QMK core and Keychron's header. Added `#undef` before redefine.

7. **iso_encoder.c missing RGB defaults** — `default_per_key_led[]` and `default_region[]` arrays were missing, needed for per-key RGB and mixed RGB features.

## Build Environment Notes

- **Compiler**: `arm-none-eabi-gcc` — system version (14.2) works. Stock v1.1.0 was built with GCC 12. GCC 10 also works. Repo pre-built .bin used GCC 10.
- **dfu-suffix error**: Harmless — appears at end of compile, .bin is created before this error
- **Expected .bin size**: ~105-108KB (stock v1.1.0 is 104KB)
- The `wireless_playground` branch is Keychron's equivalent of `main` for wireless boards
- Upstream remote (`upstream`) points to Keychron/qmk_firmware for syncing

## Keymap Structure

- **Layer 0 (MAC_BASE)**: Stock Mac layout
- **Layer 1 (MAC_FN)**: Stock Mac Fn layer
- **Layer 2 (WIN_BASE)**: Stock Windows layout with SPAM_A/SPAM_S/SPAM_D on A/S/D positions
- **Layer 3 (WIN_FN)**: Stock Windows Fn layer with SPAM_TOGGLE on G position (Fn+G)

SPAM keys use `NEW_SAFE_RANGE` (Keychron's custom safe range, not QMK's `SAFE_RANGE`).

## Tuning

In `keymap.c`:
```c
#define SPAM_STEER_BASE   80   // min ms between steering taps (A/D)
#define SPAM_STEER_JITTER 90   // random range (total: 80-170ms)
#define SPAM_BRAKE_BASE   120  // min ms between brake taps (S)
#define SPAM_BRAKE_JITTER 100  // random range (total: 120-220ms)
```

## Recovery

Stock backup: `keyboards/keychron/q3_max/firmware/stock_v1.1.0_iso_encoder.bin`
Hold Escape + plug USB → QMK Toolbox → flash stock .bin

## Known Issue (TODO)

LED mapping may still need auditing — need to verify all LED indices match physical key positions across all rows, not just CapsLock. The same off-by-one pattern from the info.json fix may affect other LED-related features in Keychron Launcher.
