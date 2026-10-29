import { connect } from '../../src/modules/hid.js';

/**
 * Stands in for the WebHID device. `respond` builds the keyboard's reply from
 * the request, or returns null to stay silent (the request then times out).
 */
export function fakeDevice(respond) {
  const listeners = {};
  const sent = [];
  return {
    sent,
    productName: 'Fake Q3 Max',
    open: async () => {},
    close: () => {},
    addEventListener: (type, fn) => {
      listeners[type] = fn;
    },
    sendReport: async (_reportId, buf) => {
      sent.push(Array.from(buf.slice(0, 5)));
      const resp = respond(Uint8Array.from(buf));
      if (resp === null) return;
      queueMicrotask(() => listeners.inputreport({ data: new DataView(resp.buffer) }));
    },
  };
}

export async function connectTo(device) {
  Object.defineProperty(globalThis.navigator, 'hid', {
    value: { requestDevice: async () => [device] },
    configurable: true,
  });
  await connect({});
}
