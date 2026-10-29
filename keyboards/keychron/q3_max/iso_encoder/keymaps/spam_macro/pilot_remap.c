#include "pilot_remap.h"
#include "keycodes.h"

typedef struct {
    keypos_t pos;     // physical key, so the remap and its indicator LED always agree
    uint16_t binding; // keycode sent while pilot mode is on
    uint16_t sent;    // keycode registered by the consumed press (KC_NO if it sent nothing)
    bool     held;    // press was consumed by the remap, so its release must be too
} pilot_key_t;

// Bindable slots first so enum pilot_bind_slot indexes them directly.
static pilot_key_t pilot_keys[] = {
    [PILOT_BIND_Q]    = {.pos = PILOT_POS_Q, .binding = KC_TRNS},
    [PILOT_BIND_E]    = {.pos = PILOT_POS_E, .binding = KC_TRNS},
    [PILOT_BIND_DEL]  = {.pos = PILOT_POS_DEL, .binding = KC_TRNS},
    [PILOT_BIND_END]  = {.pos = PILOT_POS_END, .binding = KC_TRNS},
    [PILOT_BIND_PGDN] = {.pos = PILOT_POS_PGDN, .binding = KC_TRNS},
    {.pos = PILOT_POS_UP, .binding = KC_P8},
    {.pos = PILOT_POS_LEFT, .binding = KC_P4},
    {.pos = PILOT_POS_DOWN, .binding = KC_P5},
    {.pos = PILOT_POS_RIGHT, .binding = KC_P6},
};

_Static_assert(sizeof(pilot_keys) / sizeof(pilot_keys[0]) == PILOT_KEY_COUNT, "pilot_keys must list every pilot key");

// Must accept exactly the codes offered in keychron-web/src/modules/keycodes.js.
bool pilot_binding_valid(uint16_t keycode) {
    return keycode == KC_NO || keycode == KC_TRNS
        || (keycode >= KC_A && keycode <= KC_APPLICATION)
        || (keycode >= KC_F13 && keycode <= KC_F24)
        || (keycode >= KC_AUDIO_MUTE && keycode <= KC_MEDIA_PLAY_PAUSE)
        || (keycode >= MS_BTN1 && keycode <= MS_BTN5)
        || (keycode >= MS_WHLU && keycode <= MS_WHLD)
        || (keycode >= KC_LEFT_CTRL && keycode <= KC_RIGHT_GUI);
}

bool pilot_remap_set_binding(uint8_t slot, uint16_t keycode) {
    if (slot >= PILOT_BIND_COUNT || !pilot_binding_valid(keycode)) return false;
    pilot_keys[slot].binding = keycode;
    return true;
}

uint16_t pilot_remap_get_binding(uint8_t slot) {
    if (slot >= PILOT_BIND_COUNT) return KC_NO;
    return pilot_keys[slot].binding;
}

bool pilot_remap_load(const uint16_t binds[PILOT_BIND_COUNT]) {
    for (uint8_t i = 0; i < PILOT_BIND_COUNT; i++) {
        if (!pilot_binding_valid(binds[i])) return false;
    }
    for (uint8_t i = 0; i < PILOT_BIND_COUNT; i++) {
        pilot_keys[i].binding = binds[i];
    }
    return true;
}

void pilot_remap_clear(void) {
    for (uint8_t i = 0; i < PILOT_BIND_COUNT; i++) {
        pilot_keys[i].binding = KC_TRNS;
    }
}

bool pilot_remap_via_get(uint8_t slot, uint8_t *value) {
    if (slot >= PILOT_BIND_COUNT) return false;
    value[0] = pilot_keys[slot].binding >> 8;
    value[1] = pilot_keys[slot].binding & 0xFF;
    return true;
}

bool pilot_remap_via_set(uint8_t slot, const uint8_t *value) {
    return pilot_remap_set_binding(slot, ((uint16_t)value[0] << 8) | value[1]);
}

uint8_t pilot_remap_lit_keys(keypos_t *out) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < PILOT_KEY_COUNT; i++) {
        if (pilot_keys[i].binding != KC_TRNS) out[count++] = pilot_keys[i].pos;
    }
    return count;
}

bool process_pilot_remap(keyrecord_t *record, bool pilot_enabled) {
    for (uint8_t i = 0; i < PILOT_KEY_COUNT; i++) {
        pilot_key_t *key = &pilot_keys[i];
        if (!KEYEQ(record->event.key, key->pos)) continue;

        if (record->event.pressed) {
            if (!pilot_enabled || key->binding == KC_TRNS) {
                key->held = false;
                return true;
            }
            // KC_NO swallows the key without sending anything.
            if (key->binding != KC_NO) register_code(key->binding);
            key->sent = key->binding;
            key->held = true;
            return false;
        }

        // Pair the release with how the press was handled, even if pilot mode or the binding changed since.
        if (!key->held) return true;
        if (key->sent != KC_NO) unregister_code(key->sent);
        key->sent = KC_NO;
        key->held = false;
        return false;
    }
    return true;
}

void pilot_remap_release_all(void) {
    for (uint8_t i = 0; i < PILOT_KEY_COUNT; i++) {
        // held stays set so the physical release is still swallowed.
        if (pilot_keys[i].sent != KC_NO) unregister_code(pilot_keys[i].sent);
        pilot_keys[i].sent = KC_NO;
    }
}
