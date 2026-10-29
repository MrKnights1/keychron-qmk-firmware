import { describe, expect, test } from 'bun:test';
import {
  KEYCODES,
  KC_NO,
  KC_TRNS,
  isValidBinding,
  keycodeGroups,
} from '../src/modules/keycodes.js';

// Inclusive ranges accepted by pilot_binding_valid() in keymaps/spam_macro/pilot_remap.c.
const FIRMWARE_RANGES = [
  [0x00, 0x01],
  [0x04, 0x65],
  [0x68, 0x73],
  [0xa8, 0xae],
  [0xd1, 0xd5],
  [0xd9, 0xda],
  [0xe0, 0xe7],
];

function toRanges(codes) {
  const ranges = [];
  for (const code of [...codes].sort((a, b) => a - b)) {
    const last = ranges[ranges.length - 1];
    if (last && code === last[1] + 1) last[1] = code;
    else ranges.push([code, code]);
  }
  return ranges;
}

describe('KEYCODES', () => {
  test('has_no_duplicate_codes', () => {
    const codes = KEYCODES.map((k) => k.code);

    expect(new Set(codes).size).toBe(codes.length);
  });

  test('has_no_duplicate_names', () => {
    const names = KEYCODES.map((k) => k.name);

    expect(new Set(names).size).toBe(names.length);
  });

  test('covers_exactly_the_codes_the_firmware_accepts', () => {
    const ranges = toRanges(KEYCODES.map((k) => k.code));

    expect(ranges).toEqual(FIRMWARE_RANGES);
  });

  test('lists_default_passthrough_first', () => {
    expect(KEYCODES[0].code).toBe(KC_TRNS);
  });
});

describe('isValidBinding', () => {
  test.each([
    ['KC_NO', KC_NO],
    ['KC_TRNS', KC_TRNS],
    ['KC_A', 0x04],
    ['KC_APPLICATION', 0x65],
    ['KC_F13', 0x68],
    ['KC_RIGHT_GUI', 0xe7],
  ])('accepts_%s', (_, code) => {
    expect(isValidBinding(code)).toBe(true);
  });

  test.each([
    ['reserved_0x02', 0x02],
    ['unlisted_power_0x66', 0x66],
    ['unlisted_0x74', 0x74],
    ['above_basic_range_0xE8', 0xe8],
    ['modded_keycode_0x0100', 0x0100],
    ['negative', -1],
    ['fraction', 1.5],
    ['undefined', undefined],
    ['string', '4'],
  ])('rejects_%s', (_, code) => {
    expect(isValidBinding(code)).toBe(false);
  });
});

describe('keycodeGroups', () => {
  test('keeps_every_keycode_exactly_once', () => {
    const grouped = keycodeGroups().flatMap(([, entries]) => entries);

    expect(grouped).toHaveLength(KEYCODES.length);
  });

  test('starts_with_special_group', () => {
    const [firstGroup] = keycodeGroups()[0];

    expect(firstGroup).toBe('Special');
  });
});
