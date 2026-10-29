/**
 * QMK keycodes offered as pilot-mode bindings.
 *
 * Values must match quantum/keycodes.h. The firmware's pilot_binding_valid()
 * accepts exactly this list, so isValidBinding() is a membership test.
 */

export const KC_NO = 0x0000;
export const KC_TRNS = 0x0001;

function run(start, names, group) {
  return names.map((name, i) => ({ code: start + i, name, group }));
}

const fKeys = (from, to) =>
  Array.from({ length: to - from + 1 }, (_, i) => 'F' + (from + i));

export const KEYCODES = [
  { code: KC_TRNS, name: 'Default (no remap)', group: 'Special' },
  { code: KC_NO, name: 'None (disable key)', group: 'Special' },

  ...run(0x04, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'.split(''), 'Letters'),
  ...run(0x1e, '1234567890'.split(''), 'Digits'),

  ...run(0x28, ['Enter', 'Escape', 'Backspace', 'Tab', 'Space'], 'Editing'),
  { code: 0x39, name: 'Caps Lock', group: 'Editing' },
  { code: 0x65, name: 'Menu', group: 'Editing' },

  ...run(
    0x2d,
    ['Minus', 'Equal', 'Left Bracket', 'Right Bracket', 'Backslash', 'Non-US Hash',
      'Semicolon', 'Quote', 'Grave', 'Comma', 'Period', 'Slash'],
    'Symbols',
  ),
  { code: 0x64, name: 'Non-US Backslash', group: 'Symbols' },

  ...run(0x3a, fKeys(1, 12), 'Function'),
  ...run(0x68, fKeys(13, 24), 'Function'),

  ...run(
    0x46,
    ['Print Screen', 'Scroll Lock', 'Pause', 'Insert', 'Home', 'Page Up',
      'Delete', 'End', 'Page Down', 'Right', 'Left', 'Down', 'Up'],
    'Navigation',
  ),

  ...run(
    0x53,
    ['Num Lock', 'Num /', 'Num *', 'Num -', 'Num +', 'Num Enter',
      'Num 1', 'Num 2', 'Num 3', 'Num 4', 'Num 5', 'Num 6', 'Num 7', 'Num 8', 'Num 9',
      'Num 0', 'Num .'],
    'Numpad',
  ),

  ...run(
    0xe0,
    ['Left Ctrl', 'Left Shift', 'Left Alt', 'Left Win', 'Right Ctrl', 'Right Shift', 'Right Alt', 'Right Win'],
    'Modifiers',
  ),

  ...run(0xa8, ['Mute', 'Volume Up', 'Volume Down', 'Next Track', 'Previous Track', 'Stop', 'Play/Pause'], 'Media'),

  ...run(0xd1, ['Mouse 1', 'Mouse 2', 'Mouse 3', 'Mouse 4', 'Mouse 5'], 'Mouse'),
  ...run(0xd9, ['Wheel Up', 'Wheel Down'], 'Mouse'),
];

const LISTED_CODES = new Set(KEYCODES.map((k) => k.code));

export function isValidBinding(code) {
  return LISTED_CODES.has(code);
}

/** @returns {Array<[string, Array<{code:number,name:string,group:string}>]>} groups in display order */
export function keycodeGroups() {
  const groups = new Map();
  for (const k of KEYCODES) {
    if (!groups.has(k.group)) groups.set(k.group, []);
    groups.get(k.group).push(k);
  }
  return [...groups.entries()];
}
