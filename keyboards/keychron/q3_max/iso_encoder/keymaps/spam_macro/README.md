# Keychron Q3 Max - GTA V Anti-Slide Steering Macro

Custom QMK firmware for the Keychron Q3 Max ISO Encoder that adds rapid-tap steering and braking for GTA V driving. Holding A/S/D rapidly taps instead of holding, reducing car sliding and oversteer.

## How It Works

When driving in GTA V, holding a steering key (A or D) makes the car turn at full lock, causing slides and oversteer. This firmware converts key holds into rapid taps with randomized human-like timing, giving smoother steering input.

### Macro Behavior

- **Spam mode OFF**: A, S, D work as completely normal keys (typing, menus, etc.)
- **Spam mode ON**: Holding A, S, or D rapidly taps instead of holding
  - Each tap has a randomized delay between presses (not a fixed interval)
  - The random seed changes every boot so the pattern is never the same
  - Each tap holds the key for 10ms to ensure the game registers it

### Key Assignments

| Key | Spam OFF | Spam ON |
|-----|----------|---------|
| A | Normal A | Rapid-tap A every 80-170ms (steering left) |
| D | Normal D | Rapid-tap D every 80-170ms (steering right) |
| S | Normal S | Rapid-tap S every 120-220ms (braking) |
| Fn + G | Nothing | Toggles spam mode on/off |

Braking (S) uses a slower interval than steering (A/D) because brake tapping needs to be gentler.

### Technical Details

The macro uses QMK's `process_record_user` to intercept A/S/D keypresses and `matrix_scan_user` as a timer loop to send repeated taps. Each tap generates a new random interval using `rand()` seeded with `timer_read32()` at boot. The `tap_code_delay()` function is used with a 10ms hold time to ensure games register each keypress.

## Features

- **Fn + G** toggles spam mode on/off
- **A/D** rapid-tap steering (80-170ms humanized intervals)
- **S** rapid-tap braking (120-220ms humanized intervals)
- All keys work normally when spam mode is off
- Randomized intervals seeded per boot to avoid detection
- Ghost keypress protection (spam state clears when toggling off)
- Includes all v1.1.0 features: configurable debounce, snap click, per-key RGB, mixed RGB

## Flashing Pre-Compiled Firmware

The easiest way. No build tools needed.

### Requirements

- [QMK Toolbox](https://github.com/qmk/qmk_toolbox/releases) (Windows/Mac)
- USB cable (data, not charge-only)

### Steps

1. **Download** `firmware/keychron_q3_max_iso_encoder_spam_macro.bin` from this repo
2. **Open** QMK Toolbox
3. **Enter bootloader mode**: Unplug keyboard, hold **Escape**, plug USB back in
4. QMK Toolbox should show **"DFU device connected"**
5. Click **Open**, select the `.bin` file
6. Click **Flash**
7. **Do not unplug** until it says flash complete
8. Unplug and replug the keyboard

### Factory Reset After Flashing

Hold **Fn + J + Z** for 4 seconds to clear saved settings (recommended after first flash).

## Building from Source

### Requirements

| Tool | Install | Tested Version |
|------|---------|----------------|
| Python 3 | System package manager | 3.13 |
| QMK CLI | `pip install qmk` | 1.2.0 |
| ARM toolchain | `apt install gcc-arm-none-eabi libnewlib-arm-none-eabi` (Linux) | 14.2.1 |
| Git | System package manager | 2.47+ |

#### Windows

Install [QMK MSYS](https://msys.qmk.fm/) which bundles Python, QMK CLI, and the ARM toolchain.

#### macOS

```bash
brew install qmk/qmk/qmk
brew install arm-none-eabi-gcc
```

#### Linux (Debian/Ubuntu)

```bash
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi python3-pip git
pip install qmk
```

### Build Steps

```bash
# 1. Clone our fork (includes all fixes and the custom keymap)
git clone -b wireless_playground https://github.com/MrKnights1/keychron-qmk-firmware.git
cd keychron-qmk-firmware

# 2. Initialize required submodules
git submodule update --init lib/chibios lib/chibios-contrib lib/lufa lib/printf

# 3. Install Python dependencies
pip install -r requirements.txt

# 4. Compile
qmk compile -kb keychron/q3_max/iso_encoder -km spam_macro
```

The compiled `.bin` will be at `.build/keychron_q3_max_iso_encoder_spam_macro.bin`.

### Syncing with Keychron Updates

When Keychron releases new firmware, sync the fork:

```bash
git remote add upstream https://github.com/Keychron/qmk_firmware.git
git fetch upstream
git merge upstream/wireless_playground
git push origin wireless_playground
```

> **Note**: The `dfu-suffix: not found` error at the end of compilation is harmless. The `.bin` file is created successfully before this error occurs.

### Verifying the Build

Compare your compiled `.bin` size with the stock firmware:

| File | Expected Size |
|------|---------------|
| Stock v1.1.0 | ~104 KB |
| Spam macro build | ~105-108 KB |

If the size is significantly different (e.g. 48KB), something went wrong with the board selection.

## Patches Explained

`patches/q3_max_v110_fixes.patch` fixes issues in Keychron's open-source repo that prevent the Q3 Max ISO from compiling correctly with v1.1.0 features:

| File | Fix | Why |
|------|-----|-----|
| `info.json` | Moved ISO Enter `[2,13]` to correct position in layout array | Without this, the home row (ASDFG) shifts by one key position |
| `info.json` | Updated `device_version` to `1.1.0` | Repo had `1.0.0` |
| `rules.mk` | Added `DEBOUNCE_TYPE=custom`, `SNAP_CLICK_ENABLE=yes`, `KEYCHRON_RGB_ENABLE=yes` | Keychron enabled these for v1.1.0 but didn't push the updated rules.mk |
| `q3_max.c` | Replaced old board init with `keychron_common_init()` | Old code didn't integrate with new common features (matches V3 Max pattern) |
| `config.h` | Added `#include "eeconfig_kb.h"` | Required for EEPROM-based config (debounce, snap click, RGB settings) |
| `eeconfig_kb.h` | Added `#undef` before `EECONFIG_KB_DATA_SIZE` redefinition | Prevents compiler error from double-define |
| `iso_encoder.c` | Added `default_per_key_led[]` and `default_region[]` arrays | Required by per-key RGB and mixed RGB features (88 LEDs) |

## Tuning the Intervals

Edit these values in `keymap/keymap.c` and recompile:

```c
#define SPAM_STEER_BASE   80   // min ms between steering taps
#define SPAM_STEER_JITTER 90   // random range added (total: 80-170ms)
#define SPAM_BRAKE_BASE   120  // min ms between brake taps
#define SPAM_BRAKE_JITTER 100  // random range added (total: 120-220ms)
```

### Tuning Guide

| Style | STEER_BASE | STEER_JITTER | Feel |
|-------|------------|--------------|------|
| Aggressive | 50 | 60 | Fast turns, still some sliding |
| Balanced | 80 | 90 | Good for most cars |
| Smooth | 120 | 100 | Gentle turns, slow response |
| Realistic | 150 | 150 | Closest to human tapping |

## Recovery / Restoring Stock Firmware

A stock backup is included at `firmware/stock/q3_max_iso_encoder_v1.1.0_2503261021.bin`.

1. Hold **Escape** while plugging in USB cable
2. Open QMK Toolbox
3. Load `firmware/stock/q3_max_iso_encoder_v1.1.0_2503261021.bin`
4. Click **Flash**

You can also download the latest stock firmware from the [Keychron Firmware Page](https://www.keychron.com/pages/firmware-and-json-files-of-the-keychron-qmk-keyboards).

> **You cannot brick your keyboard.** QMK Toolbox only writes to the application area, not the bootloader. Holding Escape while plugging in will always get you into bootloader mode for reflashing.

## File Structure

```
keychron-q3-max-spam-macro/
├── README.md
├── firmware/
│   ├── keychron_q3_max_iso_encoder_spam_macro.bin   # Pre-compiled, ready to flash
│   └── stock/
│       └── q3_max_iso_encoder_v1.1.0_2503261021.bin # Stock backup for recovery
├── keymap/
│   ├── keymap.c                                      # Custom keymap with spam macro
│   └── rules.mk                                      # Build config (VIA enabled)
└── patches/
    └── q3_max_v110_fixes.patch                       # Fixes for Keychron QMK repo
```

## License

This project uses code from QMK Firmware and Keychron, licensed under GPL-2.0.
