/**
 * Spam-macro and pilot-mode configurator.
 *
 * Reads/writes timing parameters, toggle keys, and LED colours
 * from the keyboard's custom VIA channel.
 */

import {
  getValue,
  setValue,
  saveEeprom,
  isBusy,
  setBusy,
  isConnected,
  FirmwareRejectedError,
} from './hid.js';
import { keycodeGroups, isValidBinding } from './keycodes.js';

/* ── Value IDs (must match firmware) ── */
const VAL = {
  'steer-hold': 1,
  'steer-tap': 2,
  'steer-gap': 3,
  'steer-jitter': 4,
  'brake-hold': 5,
  'brake-tap': 6,
  'brake-gap': 7,
  'brake-jitter': 8,
};

const VAL_TOGGLE_ROW = 9;
const VAL_TOGGLE_COL = 10;
const VAL_ENABLED = 11;
const VAL_PILOT_TOGGLE_ROW = 12;
const VAL_PILOT_TOGGLE_COL = 13;
const VAL_PILOT_ENABLED = 14;
const VAL_SPAM_LED_R = 15;
const VAL_SPAM_LED_G = 16;
const VAL_SPAM_LED_B = 17;
const VAL_PILOT_LED_R = 18;
const VAL_PILOT_LED_G = 19;
const VAL_PILOT_LED_B = 20;

const VAL_PILOT_BIND = {
  'bind-q': 23,
  'bind-e': 24,
  'bind-del': 25,
  'bind-end': 26,
  'bind-pgdn': 27,
};

// Must equal SPAM_VIA_PROTOCOL_VERSION in keymap.c.
const VAL_PROTOCOL = 28;
const PROTOCOL_VERSION = 1;

/* ── Toggle-key lookup table ── */
const TOGGLE_KEYS = [
  { name: 'G', row: 3, col: 5 },
  { name: 'A', row: 3, col: 1 },
  { name: 'B', row: 4, col: 6 },
  { name: 'C', row: 4, col: 4 },
  { name: 'D', row: 3, col: 3 },
  { name: 'E', row: 2, col: 3 },
  { name: 'F', row: 3, col: 4 },
  { name: 'H', row: 3, col: 6 },
  { name: 'I', row: 2, col: 8 },
  { name: 'J', row: 3, col: 7 },
  { name: 'K', row: 3, col: 8 },
  { name: 'L', row: 3, col: 9 },
  { name: 'M', row: 4, col: 8 },
  { name: 'N', row: 4, col: 7 },
  { name: 'O', row: 2, col: 9 },
  { name: 'P', row: 2, col: 10 },
  { name: 'Q', row: 2, col: 1 },
  { name: 'R', row: 2, col: 4 },
  { name: 'S', row: 3, col: 2 },
  { name: 'T', row: 2, col: 5 },
  { name: 'U', row: 2, col: 7 },
  { name: 'V', row: 4, col: 5 },
  { name: 'W', row: 2, col: 2 },
  { name: 'X', row: 4, col: 3 },
  { name: 'Y', row: 2, col: 6 },
  { name: 'Z', row: 4, col: 2 },
];

/* ── Helpers ── */
const $ = (id) => document.getElementById(id);

function hexToRgb(hex) {
  const r = parseInt(hex.slice(1, 3), 16);
  const g = parseInt(hex.slice(3, 5), 16);
  const b = parseInt(hex.slice(5, 7), 16);
  return { r, g, b };
}

function rgbToHex(r, g, b) {
  return (
    '#' +
    [r, g, b].map((v) => v.toString(16).padStart(2, '0')).join('')
  );
}

function populateBindingSelect(select) {
  for (const [group, entries] of keycodeGroups()) {
    const optgroup = document.createElement('optgroup');
    optgroup.label = group;
    for (const k of entries) {
      const opt = document.createElement('option');
      opt.value = k.code;
      opt.textContent = k.name;
      optgroup.appendChild(opt);
    }
    select.appendChild(optgroup);
  }
}

/** @returns {boolean} false when the list doesn't offer the code; the select is left untouched */
function selectBinding(select, code) {
  if (!isValidBinding(code)) return false;
  select.value = String(code);
  return true;
}

function bindingProblem(id, code) {
  const key = id.slice('bind-'.length).toUpperCase();
  if (code === null) return key + ' binding';
  return key + ' binding 0x' + code.toString(16).toUpperCase().padStart(4, '0');
}

/* ── Status / busy helpers (delegated to app.js via callbacks) ── */
let _setStatus = () => {};
let _setBusyUI = () => {};
let _updateLed = () => {};

/* ── Polling ── */
let pollInterval = null;

async function pollEnabled() {
  if (isBusy() || !isConnected()) return;
  try {
    const spam = await getValue(VAL_ENABLED);
    if (spam !== null) _updateLed('spam-led', spam !== 0);
    const pilot = await getValue(VAL_PILOT_ENABLED);
    if (pilot !== null) _updateLed('pilot-led', pilot !== 0);
  } catch (_) {
    /* ignore polling errors */
  }
}

export function startPolling() {
  stopPolling();
  pollInterval = setInterval(pollEnabled, 500);
}

export function stopPolling() {
  clearInterval(pollInterval);
  pollInterval = null;
}

/* ── Write lock: Send/Save only after a complete read from matching firmware ── */
let lockReason = 'NOT READ YET';

export function canWrite() {
  return lockReason === null;
}

/** @returns {Promise<string|null>} why writes must stay locked, or null when the firmware speaks our protocol */
async function protocolProblem() {
  let version;
  try {
    version = await getValue(VAL_PROTOCOL);
  } catch (err) {
    if (err instanceof FirmwareRejectedError) return 'FIRMWARE OUTDATED — FLASH LATEST';
    throw err;
  }
  if (version === null) return 'NO RESPONSE — PRESS READ';
  if (version !== PROTOCOL_VERSION) {
    return 'FIRMWARE v' + version + ', APP EXPECTS v' + PROTOCOL_VERSION;
  }
  return null;
}

/** @returns {Promise<boolean>} false when the position timed out or isn't in TOGGLE_KEYS */
async function readToggle(rowId, colId, selectId) {
  const row = await getValue(rowId);
  const col = await getValue(colId);
  const idx = TOGGLE_KEYS.findIndex((k) => k.row === row && k.col === col);
  if (idx < 0) return false;
  $(selectId).value = idx;
  $('v-' + selectId).textContent = TOGGLE_KEYS[idx].name;
  return true;
}

/** @returns {Promise<boolean>} false when any channel timed out */
async function readColor(rId, gId, bId, inputId) {
  const r = await getValue(rId);
  const g = await getValue(gId);
  const b = await getValue(bId);
  if (r === null || g === null || b === null) return false;
  const hex = rgbToHex(r, g, b);
  $(inputId).value = hex;
  $(inputId + '-hex').textContent = hex;
  return true;
}

/* ── Read all values from keyboard ── */
export async function readAll() {
  if (isBusy()) return;
  setBusy(true);
  lockReason = 'READING';
  _setBusyUI(true);
  try {
    const versionProblem = await protocolProblem();
    if (versionProblem) {
      lockReason = versionProblem;
      _setStatus(versionProblem, 'disconnected');
      return;
    }

    // A value that timed out or that the UI can't show would be overwritten by the next write.
    const unread = [];

    for (const [key, vid] of Object.entries(VAL)) {
      const val = await getValue(vid);
      if (val !== null) {
        $(key).value = val;
        $('v-' + key).textContent = $(key).value;
      } else {
        unread.push(key);
      }
    }

    const spamOn = await getValue(VAL_ENABLED);
    if (spamOn !== null) _updateLed('spam-led', spamOn !== 0);
    const pilotOn = await getValue(VAL_PILOT_ENABLED);
    if (pilotOn !== null) _updateLed('pilot-led', pilotOn !== 0);

    if (!(await readToggle(VAL_TOGGLE_ROW, VAL_TOGGLE_COL, 'toggle-key'))) {
      unread.push('spam toggle');
    }
    if (!(await readToggle(VAL_PILOT_TOGGLE_ROW, VAL_PILOT_TOGGLE_COL, 'pilot-toggle-key'))) {
      unread.push('pilot toggle');
    }
    if (!(await readColor(VAL_SPAM_LED_R, VAL_SPAM_LED_G, VAL_SPAM_LED_B, 'spam-color'))) {
      unread.push('spam colour');
    }
    if (!(await readColor(VAL_PILOT_LED_R, VAL_PILOT_LED_G, VAL_PILOT_LED_B, 'pilot-color'))) {
      unread.push('pilot colour');
    }

    for (const [id, vid] of Object.entries(VAL_PILOT_BIND)) {
      const code = await getValue(vid);
      if (code === null || !selectBinding($(id), code)) unread.push(bindingProblem(id, code));
    }

    if (unread.length > 0) {
      lockReason = 'READ INCOMPLETE (' + unread.join(', ') + ') — PRESS READ';
      _setStatus(lockReason, 'disconnected');
    } else {
      lockReason = null;
      _setStatus('SYNCED', 'connected', 1500);
    }
  } catch (err) {
    console.error('readAll error:', err);
    lockReason = 'READ ERROR — PRESS READ';
    _setStatus('READ ERROR', 'disconnected');
  } finally {
    setBusy(false);
    _setBusyUI(false);
  }
}

/* ── Send all values to keyboard (RAM only) ── */
async function sendAllValues() {
  for (const [key, vid] of Object.entries(VAL)) {
    await setValue(vid, parseInt($(key).value, 10));
  }

  const tk = TOGGLE_KEYS[$('toggle-key').value];
  await setValue(VAL_TOGGLE_ROW, tk.row);
  await setValue(VAL_TOGGLE_COL, tk.col);

  const pk = TOGGLE_KEYS[$('pilot-toggle-key').value];
  await setValue(VAL_PILOT_TOGGLE_ROW, pk.row);
  await setValue(VAL_PILOT_TOGGLE_COL, pk.col);

  const sc = hexToRgb($('spam-color').value);
  await setValue(VAL_SPAM_LED_R, sc.r);
  await setValue(VAL_SPAM_LED_G, sc.g);
  await setValue(VAL_SPAM_LED_B, sc.b);

  const pc = hexToRgb($('pilot-color').value);
  await setValue(VAL_PILOT_LED_R, pc.r);
  await setValue(VAL_PILOT_LED_G, pc.g);
  await setValue(VAL_PILOT_LED_B, pc.b);

  for (const [id, vid] of Object.entries(VAL_PILOT_BIND)) {
    await setValue(vid, parseInt($(id).value, 10));
  }
}

export async function sendAll() {
  if (isBusy()) return;
  if (!canWrite()) {
    _setStatus(lockReason, 'disconnected');
    return;
  }
  setBusy(true);
  _setBusyUI(true);
  try {
    await sendAllValues();
    _setStatus('SENT', 'connected', 1500);
  } catch (err) {
    console.error('sendAll error:', err);
    _setStatus('SEND ERROR', 'disconnected');
  } finally {
    setBusy(false);
    _setBusyUI(false);
  }
}

/* ── Send + persist to EEPROM ── */
export async function saveToEeprom() {
  if (isBusy()) return;
  if (!canWrite()) {
    _setStatus(lockReason, 'disconnected');
    return;
  }
  setBusy(true);
  _setBusyUI(true);
  try {
    await sendAllValues();
    await saveEeprom();
    _setStatus('SAVED TO EEPROM', 'connected', 2000);
  } catch (err) {
    console.error('saveToEeprom error:', err);
    _setStatus('SAVE ERROR', 'disconnected');
  } finally {
    setBusy(false);
    _setBusyUI(false);
  }
}

/* ── Initialise configurator UI bindings ── */
export function initConfigurator({ setStatus, setBusyUI, updateLed }) {
  _setStatus = setStatus;
  _setBusyUI = setBusyUI;
  _updateLed = updateLed;

  // Populate toggle-key dropdowns
  ['toggle-key', 'pilot-toggle-key'].forEach((selId) => {
    TOGGLE_KEYS.forEach((k, i) => {
      const opt = document.createElement('option');
      opt.value = i;
      opt.textContent = k.name;
      $(selId).appendChild(opt);
    });
  });

  Object.keys(VAL_PILOT_BIND).forEach((id) => populateBindingSelect($(id)));

  // Default pilot toggle to H
  const hIdx = TOGGLE_KEYS.findIndex((k) => k.name === 'H');
  if (hIdx >= 0) $('pilot-toggle-key').value = hIdx;

  // Dropdown change handlers
  $('toggle-key').addEventListener('change', () => {
    $('v-toggle-key').textContent =
      TOGGLE_KEYS[$('toggle-key').value].name;
  });
  $('pilot-toggle-key').addEventListener('change', () => {
    $('v-pilot-toggle-key').textContent =
      TOGGLE_KEYS[$('pilot-toggle-key').value].name;
  });

  // Color picker handlers
  $('spam-color').addEventListener('input', () => {
    $('spam-color-hex').textContent = $('spam-color').value;
  });
  $('pilot-color').addEventListener('input', () => {
    $('pilot-color-hex').textContent = $('pilot-color').value;
  });

  // Slider value displays
  Object.keys(VAL).forEach((key) => {
    const slider = $(key);
    const display = $('v-' + key);
    slider.addEventListener('input', () => {
      display.textContent = slider.value;
    });
  });
}
