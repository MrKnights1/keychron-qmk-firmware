#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "action.h"

/*
 * Pilot mode key remapping: arrows always send numpad 8/4/5/6, and the
 * slots below send a user-chosen keycode. A slot bound to KC_TRNS passes
 * the original key through. Keys are matched by physical position
 * (PILOT_POS_* in config.h), not by keycode.
 */
enum pilot_bind_slot {
    PILOT_BIND_Q,
    PILOT_BIND_E,
    PILOT_BIND_DEL,
    PILOT_BIND_END,
    PILOT_BIND_PGDN,
    PILOT_BIND_COUNT,
};

// Bindable slots plus the four arrows.
#define PILOT_KEY_COUNT (PILOT_BIND_COUNT + 4)

bool     pilot_binding_valid(uint16_t keycode);
bool     pilot_remap_set_binding(uint8_t slot, uint16_t keycode);
uint16_t pilot_remap_get_binding(uint8_t slot);

// Applies every binding, or none of them if any is invalid.
bool pilot_remap_load(const uint16_t binds[PILOT_BIND_COUNT]);

// Unbinds every slot (KC_TRNS).
void pilot_remap_clear(void);

// VIA custom-value payload: value[0..1] is the binding, big-endian.
bool pilot_remap_via_get(uint8_t slot, uint8_t *value);
bool pilot_remap_via_set(uint8_t slot, const uint8_t *value);

// Writes the positions to light in pilot mode (arrows and bound slots) to out[PILOT_KEY_COUNT]; returns how many.
uint8_t pilot_remap_lit_keys(keypos_t *out);

// Returns false when the event was consumed by the remap.
bool process_pilot_remap(keyrecord_t *record, bool pilot_enabled);

// Releases every key the remap is currently holding down.
void pilot_remap_release_all(void);
