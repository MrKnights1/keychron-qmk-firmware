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

### Pilot Mode

**Fn + H** toggles pilot mode for GTA V flying. The arrows send Num 8/4/5/6, and the physical Q, E, Del, End and PgDn keys send whatever is bound to them in the web configurator ("Default" leaves the key unchanged). Remaps follow the physical key (positions in `config.h`), not the keycode, so the indicator LEDs always match.

- Spam and pilot mode are mutually exclusive: turning one on turns the other off.
- Both modes turn off when the OS switch selects the Mac layers, since their toggles only exist on the Windows Fn layer.

### Web Configurator Protocol

The web configurator talks to the keyboard over VIA custom channel `0x44`. Value ID 28 reports the protocol version (`SPAM_VIA_PROTOCOL_VERSION` in `keymap.c`); the web app refuses to send or save when it doesn't match, so bump it whenever value IDs or their meaning change. Settings are stored in QMK's keymap EEPROM datablock; every flash wipes them, so set them again in the web app and save.

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

1. **Download** `keyboards/keychron/q3_max/firmware/keychron_q3_max_iso_encoder_spam_macro.bin` from this repo
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
git submodule sync
git submodule update --init --recursive lib/chibios lib/chibios-contrib lib/lufa lib/printf

# 3. Install Python dependencies
pip install -r requirements.txt

# 4. Compile
qmk compile -kb keychron/q3_max/iso_encoder -km spam_macro
```

The compiled `.bin` will be at `.build/keychron_q3max_custom.bin`.

### Syncing with Keychron Updates

This firmware sits on Keychron's `2025q3` branch (their main). None of Keychron's files are edited: everything of ours lives in this keymap folder, `users/spam_macro/`, `tests/keychron_spam_macro/`, `keychron-web/` and `keyboards/keychron/q3_max/firmware/`. Keychron updates therefore merge without conflicts:

```bash
git remote add upstream https://github.com/Keychron/qmk_firmware.git   # once
git config --add remote.upstream.fetch +refs/heads/2025q3:refs/remotes/upstream/2025q3   # once
git fetch upstream
git checkout -b chore/update-keychron wireless_playground
git merge upstream/2025q3
git submodule sync && git submodule update --init --recursive lib/chibios lib/chibios-contrib lib/lufa lib/printf lib/googletest
keyboards/keychron/q3_max/iso_encoder/keymaps/spam_macro/check_upstream_clean.sh
```

Then run the tests, compile, flash and try it, and land it with `git merge --no-ff` (not a squash) so the link to Keychron's history is kept.

> **Note**: The `dfu-suffix: not found` error at the end of compilation is harmless. The `.bin` file is created successfully before this error occurs.

### Verifying the Build

Compare your compiled `.bin` size with the stock firmware:

| File | Expected Size |
|------|---------------|
| Stock v1.1.0 | ~104 KB |
| Spam macro build | ~115 KB |

If the size is significantly different (e.g. 48KB), something went wrong with the board selection.

## Testing

```bash
# Firmware: pilot remap, saved settings, mode switching (QMK googletest harness, needs the submodule once)
git submodule update --init lib/googletest
make test:keychron_spam_macro

# Web configurator (keychron-web/)
cd keychron-web && bun test
```

## Board Overrides

Fixes for Keychron's Q3 Max ISO code live in `users/spam_macro/post_config.h` (read after every `config.h`) instead of edits to Keychron's files:

| Setting | Value | Why |
|---------|-------|-----|
| `SNLED27351_PHASE_CHANNEL` | 12 channels | Keychron's config scans 9, which leaves the Tab to Del row dark |
| `CAPS_LOCK_INDEX` | 50 | Keychron ships 51, which lights the A key; Caps Lock `[3,0]` is LED 50 |
| `RGB_MATRIX_DEFAULT_*` | solid #07fff0 | Default backlight after a reset |

The layers in `keymap.c` follow Keychron's `keyboard.json` order, where ISO Enter `[2,13]` is listed after row 3: its keycode comes after `KC_NUHS`, at the end of the home row.

## Tuning the Intervals

Timings are tuned live in the web configurator and saved to EEPROM. The defaults used after a reset are in `spam_config.h`:

```c
#define SPAM_STEER_HOLD       200   // ms initial hold (full wheel turn)
#define SPAM_STEER_TAP_HOLD   80    // ms each tap is held down
#define SPAM_STEER_GAP_BASE   15    // ms gap between taps (key released)
#define SPAM_STEER_GAP_JITTER 15    // randomized gap range (total: 15-30ms)
```

## Recovery / Restoring Stock Firmware

A stock backup is included at `keyboards/keychron/q3_max/firmware/stock_v1.1.0_iso_encoder.bin`.

1. Hold **Escape** while plugging in USB cable
2. Open QMK Toolbox
3. Load `keyboards/keychron/q3_max/firmware/stock_v1.1.0_iso_encoder.bin`
4. Click **Flash**

You can also download the latest stock firmware from the [Keychron Firmware Page](https://www.keychron.com/pages/firmware-and-json-files-of-the-keychron-qmk-keyboards).

> **You cannot brick your keyboard.** QMK Toolbox only writes to the application area, not the bootloader. Holding Escape while plugging in will always get you into bootloader mode for reflashing.

## File Structure

```
keyboards/keychron/q3_max/
├── firmware/
│   ├── keychron_q3_max_iso_encoder_spam_macro.bin   # Pre-compiled, ready to flash
│   └── stock_v1.1.0_iso_encoder.bin                 # Stock backup for recovery
└── iso_encoder/keymaps/spam_macro/
    ├── keymap.c                                     # Layers, spam macro, VIA values, LED indicators
    ├── pilot_remap.c / spam_config.c / modes.c      # Pilot remap, saved settings, mode rules
    ├── config.h                                     # EEPROM size, pilot key positions
    ├── rules.mk                                     # Build config (VIA enabled, output name)
    └── check_upstream_clean.sh                      # Fails if Keychron's files were edited
users/spam_macro/post_config.h                      # Board overrides (LED scan, Caps Lock LED, default colour)
tests/keychron_spam_macro/                           # Firmware unit tests
keychron-web/                                        # Web configurator and flasher
```

## License

This project uses code from QMK Firmware and Keychron, licensed under GPL-2.0.
