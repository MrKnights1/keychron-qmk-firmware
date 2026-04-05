/**
 * WebHID protocol layer for Keychron Q3 Max custom firmware.
 *
 * Exposes low-level transport (sendReport, getValue, setValue) and
 * connection lifecycle (connect, disconnect, onInputReport).
 */

/* ── Protocol constants ── */
const VIA_USAGE_PAGE = 0xff60;
const VIA_USAGE = 0x61;
const REPORT_LEN = 32;
const CMD_GET = 0x08;
const CMD_SET = 0x07;
const CMD_SAVE = 0x09;
const CHANNEL = 0x44;

/* ── Module state ── */
let device = null;
let busy = false;
let pending = null;
let sendQueue = Promise.resolve();

/* ── Public accessors ── */
export function getDevice() {
  return device;
}

export function isBusy() {
  return busy;
}

export function setBusy(val) {
  busy = val;
}

export function isConnected() {
  return device !== null;
}

/* ── Input-report handler (called by WebHID) ── */
function onInputReport(e) {
  if (!pending) return;
  const resp = new Uint8Array(e.data.buffer);
  if (
    resp[0] === pending.cmd &&
    resp[1] === pending.channel &&
    resp[2] === pending.valueId
  ) {
    const p = pending;
    pending = null;
    p.resolve(resp);
  }
}

/* ── Low-level transport (serialized via queue) ── */
function sendReportImpl(cmd, channel, valueId, extraBytes) {
  if (!device) return Promise.resolve(null);
  const buf = new Uint8Array(REPORT_LEN);
  buf[0] = cmd;
  buf[1] = channel;
  buf[2] = valueId;
  if (extraBytes) {
    buf[3] = extraBytes[0];
    buf[4] = extraBytes[1];
  }
  return new Promise((resolve, reject) => {
    const timeout = setTimeout(() => {
      pending = null;
      resolve(null);
    }, 1000);
    pending = {
      cmd,
      channel,
      valueId,
      resolve: (resp) => {
        clearTimeout(timeout);
        resolve(resp);
      },
    };
    device.sendReport(0x00, buf).catch((err) => {
      clearTimeout(timeout);
      pending = null;
      reject(err);
    });
  });
}

export function sendReport(cmd, channel, valueId, extraBytes) {
  const next = sendQueue.then(() => sendReportImpl(cmd, channel, valueId, extraBytes));
  sendQueue = next.catch(() => {});
  return next;
}

export async function getValue(valueId) {
  const resp = await sendReport(CMD_GET, CHANNEL, valueId);
  if (!resp || resp[0] !== CMD_GET) return null;
  return (resp[3] << 8) | resp[4];
}

export async function setValue(valueId, val) {
  const resp = await sendReport(
    CMD_SET,
    CHANNEL,
    valueId,
    [(val >> 8) & 0xff, val & 0xff],
  );
  if (!resp) throw new Error('Timeout setting value ' + valueId);
  if (resp[0] === 0xff) throw new Error('Firmware rejected value ' + valueId);
}

export async function saveEeprom() {
  const resp = await sendReport(CMD_SAVE, CHANNEL, 1);
  if (!resp) throw new Error('Timeout saving to EEPROM');
  if (resp[0] === 0xff) throw new Error('Firmware rejected save');
}

/* ── Connection lifecycle ── */

/**
 * @param {Object} callbacks
 * @param {Function} callbacks.onConnect  - called with device after open
 * @param {Function} callbacks.onDisconnect - called when device disconnects
 */
export async function connect(callbacks) {
  const devices = await navigator.hid.requestDevice({
    filters: [{ usagePage: VIA_USAGE_PAGE, usage: VIA_USAGE }],
  });
  if (!devices.length) return null;

  device = devices[0];
  await device.open();

  device.addEventListener('inputreport', onInputReport);
  device.addEventListener('disconnect', () => {
    device = null;
    pending = null;
    busy = false;
    if (callbacks.onDisconnect) callbacks.onDisconnect();
  });

  if (callbacks.onConnect) callbacks.onConnect(device);
  return device;
}

export function disconnect() {
  if (device) {
    device.close();
    device = null;
  }
  pending = null;
  busy = false;
}

/* ── Re-export constants needed by other modules ── */
export { CMD_GET, CMD_SET, CMD_SAVE, CHANNEL };
