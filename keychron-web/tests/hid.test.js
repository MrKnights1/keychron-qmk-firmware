import { afterEach, describe, expect, test } from 'bun:test';
import { FirmwareRejectedError, disconnect, getValue, setValue } from '../src/modules/hid.js';
import { connectTo, fakeDevice } from './helpers/fake-device.js';

const CMD_GET = 0x08;
const CMD_SET = 0x07;
const CHANNEL = 0x44;

afterEach(() => {
  disconnect();
});

describe('setValue', () => {
  test('sends_value_id_and_big_endian_value_on_custom_channel', async () => {
    const device = fakeDevice((req) => req);
    await connectTo(device);

    await setValue(24, 0x0061);

    expect(device.sent).toEqual([[CMD_SET, CHANNEL, 24, 0x00, 0x61]]);
  });

  test('throws_firmware_rejected_when_keyboard_answers_unhandled', async () => {
    const device = fakeDevice((req) => {
      req[0] = 0xff;
      return req;
    });
    await connectTo(device);

    const result = setValue(24, 0x0100);

    await expect(result).rejects.toThrow('Firmware rejected value 24');
  });
});

describe('getValue', () => {
  test('returns_16_bit_value_from_response', async () => {
    const device = fakeDevice((req) => {
      req[3] = 0x00;
      req[4] = 0xd1;
      return req;
    });
    await connectTo(device);

    const value = await getValue(25);

    expect(value).toBe(0xd1);
    expect(device.sent[0].slice(0, 3)).toEqual([CMD_GET, CHANNEL, 25]);
  });

  test('throws_firmware_rejected_when_keyboard_answers_unhandled', async () => {
    const device = fakeDevice((req) => {
      req[0] = 0xff;
      return req;
    });
    await connectTo(device);

    const result = getValue(28);

    await expect(result).rejects.toBeInstanceOf(FirmwareRejectedError);
  });

  test('returns_null_when_keyboard_does_not_answer', async () => {
    await connectTo(fakeDevice(() => null));

    const value = await getValue(23);

    expect(value).toBeNull();
  });
});
