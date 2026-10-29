import { afterEach, beforeEach, describe, expect, test } from 'bun:test';
import { disconnect } from '../src/modules/hid.js';
import {
  canWrite,
  initConfigurator,
  readAll,
  saveToEeprom,
  sendAll,
} from '../src/modules/configurator.js';
import { KEYCODES, keycodeGroups } from '../src/modules/keycodes.js';
import { connectTo, fakeDevice } from './helpers/fake-device.js';
import { installFakeDom } from './helpers/fake-dom.js';

const CMD_GET = 0x08;
const CMD_SET = 0x07;
const CMD_SAVE = 0x09;
const CHANNEL = 0x44;
const ID_UNHANDLED = 0xff;

const SLIDERS = [
  'steer-hold', 'steer-tap', 'steer-gap', 'steer-jitter',
  'brake-hold', 'brake-tap', 'brake-gap', 'brake-jitter',
];

// What the current firmware reports, by VIA value ID.
const FIRMWARE_VALUES = {
  1: 200, 2: 80, 3: 15, 4: 15, 5: 150, 6: 60, 7: 20, 8: 15,
  9: 3, 10: 5, 11: 0, 12: 3, 13: 6, 14: 0,
  15: 255, 16: 0, 17: 0, 18: 0, 19: 128, 20: 255,
  23: 0x5f, 24: 0x0001, 25: 0x0001, 26: 0x0000, 27: 0x0001,
  28: 1,
};

/** Answers like the firmware: unknown IDs get id_unhandled, IDs in `silent` never answer. */
function firmware(values, silent = []) {
  return fakeDevice((req) => {
    const [cmd, , id] = req;
    if (silent.includes(id)) return null;
    if (cmd === CMD_SAVE) return req;
    if (!(id in values)) {
      req[0] = ID_UNHANDLED;
      return req;
    }
    if (cmd === CMD_GET) {
      req[3] = values[id] >> 8;
      req[4] = values[id] & 0xff;
    }
    return req;
  });
}

function writesSent(device) {
  return device.sent.filter(([cmd]) => cmd === CMD_SET || cmd === CMD_SAVE);
}

let dom;
let status;

beforeEach(() => {
  dom = installFakeDom({
    inputs: [
      ...SLIDERS,
      ...SLIDERS.map((id) => 'v-' + id),
      'v-toggle-key', 'v-pilot-toggle-key',
      'spam-color', 'spam-color-hex', 'pilot-color', 'pilot-color-hex',
    ],
    selects: ['toggle-key', 'pilot-toggle-key', 'bind-q', 'bind-e', 'bind-del', 'bind-end', 'bind-pgdn'],
  });
  status = null;
  initConfigurator({
    setStatus: (msg) => {
      status = msg;
    },
    setBusyUI: () => {},
    updateLed: () => {},
  });
});

afterEach(() => {
  disconnect();
});

describe('initConfigurator', () => {
  test('fills_binding_selects_with_every_keycode_grouped', () => {
    const select = dom['bind-q'];

    expect(select.children.map((g) => g.label)).toEqual(keycodeGroups().map(([group]) => group));
    expect(select.options).toHaveLength(KEYCODES.length);
  });
});

describe('readAll', () => {
  test('locks_writes_when_firmware_does_not_know_the_protocol_value', async () => {
    const { 28: _, ...oldFirmware } = FIRMWARE_VALUES;
    const device = firmware(oldFirmware);
    await connectTo(device);

    await readAll();
    await saveToEeprom();

    expect(status).toBe('FIRMWARE OUTDATED — FLASH LATEST');
    expect(canWrite()).toBe(false);
    expect(writesSent(device)).toEqual([]);
  });

  test('locks_writes_when_protocol_version_differs', async () => {
    const device = firmware({ ...FIRMWARE_VALUES, 28: 2 });
    await connectTo(device);

    await readAll();
    await sendAll();

    expect(status).toBe('FIRMWARE v2, APP EXPECTS v1');
    expect(writesSent(device)).toEqual([]);
  });

  test('locks_writes_when_a_binding_read_times_out', async () => {
    const device = firmware(FIRMWARE_VALUES, [23]);
    await connectTo(device);

    await readAll();
    await saveToEeprom();

    expect(status).toBe('READ INCOMPLETE (Q binding) — PRESS READ');
    expect(writesSent(device)).toEqual([]);
  });

  test('locks_writes_and_adds_no_option_for_unlisted_binding', async () => {
    const device = firmware({ ...FIRMWARE_VALUES, 23: 0x0066 });
    await connectTo(device);

    await readAll();
    await saveToEeprom();

    expect(status).toBe('READ INCOMPLETE (Q binding 0x0066) — PRESS READ');
    expect(dom['bind-q'].options).toHaveLength(KEYCODES.length);
    expect(writesSent(device)).toEqual([]);
  });

  test('shows_every_value_read_from_keyboard', async () => {
    await connectTo(firmware(FIRMWARE_VALUES));

    await readAll();

    expect(status).toBe('SYNCED');
    expect(canWrite()).toBe(true);
    expect(dom['steer-hold'].value).toBe(200);
    expect(dom['v-toggle-key'].textContent).toBe('G');
    expect(dom['v-pilot-toggle-key'].textContent).toBe('H');
    expect(dom['pilot-color'].value).toBe('#0080ff');
    expect(dom['bind-q'].value).toBe(String(0x5f));
    expect(dom['bind-end'].value).toBe('0');
  });
});

describe('saveToEeprom', () => {
  test('writes_every_value_read_then_saves', async () => {
    const device = firmware(FIRMWARE_VALUES);
    await connectTo(device);
    await readAll();

    await saveToEeprom();

    const expectedSets = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 15, 16, 17, 18, 19, 20, 23, 24, 25, 26, 27]
      .map((id) => [CMD_SET, CHANNEL, id, FIRMWARE_VALUES[id] >> 8, FIRMWARE_VALUES[id] & 0xff]);
    expect(writesSent(device)).toEqual([...expectedSets, [CMD_SAVE, CHANNEL, 1, 0, 0]]);
    expect(status).toBe('SAVED TO EEPROM');
  });
});
