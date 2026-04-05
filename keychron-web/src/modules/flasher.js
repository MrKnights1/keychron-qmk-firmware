/**
 * Firmware flash helper — enters bootloader via WebHID,
 * attempts DFU flash via WebUSB, falls back to QMK Toolbox instructions.
 */

import { setValue, isConnected } from './hid.js';
import { stopPolling } from './configurator.js';

/* ── VIA bootloader command ── */
const VAL_BOOTLOADER = 21;
const BOOTLOADER_MAGIC = 0xBEEF;

/* ── DFU constants ── */
const DFU_DNLOAD    = 0x01;
const DFU_GETSTATUS = 0x03;
const DFU_CLRSTATUS = 0x04;
const DFU_ABORT     = 0x06;

const STATE_IDLE        = 2;
const STATE_DNLOAD_IDLE = 5;
const STATE_ERROR       = 10;

const DFUSE_SET_ADDRESS = 0x21;
const DFUSE_ERASE       = 0x41;

const STM32_VID  = 0x0483;
const STM32_PID  = 0xDF11;
const FLASH_BASE = 0x08000000;
const PAGE_SIZE  = 2048;

const SECTORS = [
  { addr: 0x08000000, size: 0x4000  },
  { addr: 0x08004000, size: 0x4000  },
  { addr: 0x08008000, size: 0x4000  },
  { addr: 0x0800C000, size: 0x4000  },
  { addr: 0x08010000, size: 0x10000 },
  { addr: 0x08020000, size: 0x20000 },
];

/* ── DOM helpers ── */
const $ = (id) => document.getElementById(id);
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

let selectedFile = null;
let inBootloaderMode = false;

function log(msg) {
  const el = $('flash-log');
  if (!el) return;
  el.classList.remove('hidden');
  el.textContent += msg + '\n';
  el.scrollTop = el.scrollHeight;
}

function clearLog() {
  const el = $('flash-log');
  if (el) { el.textContent = ''; el.classList.add('hidden'); }
}

function setProgress(pct) {
  const bar = $('flash-progress-bar');
  const wrap = $('flash-progress');
  if (wrap) wrap.classList.remove('hidden');
  if (bar) bar.style.width = pct + '%';
}

function hideProgress() {
  const wrap = $('flash-progress');
  if (wrap) wrap.classList.add('hidden');
}

/* ── DFU protocol ── */

async function dfuGetStatus(dev) {
  const r = await dev.controlTransferIn({
    requestType: 'class', recipient: 'interface',
    request: DFU_GETSTATUS, value: 0, index: 0,
  }, 6);
  const d = new Uint8Array(r.data.buffer);
  return { status: d[0], pollTimeout: d[1] | (d[2] << 8) | (d[3] << 16), state: d[4] };
}

async function dfuClearStatus(dev) {
  await dev.controlTransferOut({
    requestType: 'class', recipient: 'interface',
    request: DFU_CLRSTATUS, value: 0, index: 0,
  });
}

async function dfuAbort(dev) {
  await dev.controlTransferOut({
    requestType: 'class', recipient: 'interface',
    request: DFU_ABORT, value: 0, index: 0,
  });
}

async function dfuDnload(dev, blockNum, data) {
  await dev.controlTransferOut({
    requestType: 'class', recipient: 'interface',
    request: DFU_DNLOAD, value: blockNum, index: 0,
  }, data);
}

async function waitForState(dev, target) {
  for (let i = 0; i < 100; i++) {
    const s = await dfuGetStatus(dev);
    if (s.state === target) return s;
    if (s.state === STATE_ERROR) { await dfuClearStatus(dev); throw new Error('DFU error'); }
    if (s.pollTimeout > 0) await sleep(s.pollTimeout);
  }
  throw new Error('DFU state timeout');
}

async function dfuseCmd(dev, cmd, addr) {
  const d = new Uint8Array(5);
  d[0] = cmd; d[1] = addr & 0xFF; d[2] = (addr >> 8) & 0xFF; d[3] = (addr >> 16) & 0xFF; d[4] = (addr >> 24) & 0xFF;
  await dfuDnload(dev, 0, d);
  await waitForState(dev, STATE_DNLOAD_IDLE);
}

/* ── Flash flow ── */

async function enterBootloader() {
  stopPolling();
  log('Sending bootloader command...');
  try { await setValue(VAL_BOOTLOADER, BOOTLOADER_MAGIC); } catch (_) {}
  log('Keyboard entering DFU mode — waiting 3 seconds...');
  await sleep(3000);
}

async function flashFirmware(firmware) {
  clearLog();
  hideProgress();

  const size = firmware.byteLength;
  log('Firmware: ' + (size / 1024).toFixed(1) + ' KB');

  if (size < 10240 || size > 262144) {
    throw new Error('Invalid firmware size (' + size + ' bytes)');
  }

  // Validate STM32 firmware header: initial SP in SRAM, reset vector in flash
  const hdr = new DataView(firmware);
  const sp = hdr.getUint32(0, true);
  const rv = hdr.getUint32(4, true);
  if ((sp & 0xFFF00000) !== 0x20000000 || (rv & 0xFFF00000) !== 0x08000000) {
    throw new Error('File does not appear to be valid STM32 firmware');
  }

  if (isConnected()) await enterBootloader();

  log('Select "STM32 BOOTLOADER" in the browser popup...');
  let dev;
  try {
    dev = await navigator.usb.requestDevice({ filters: [{ vendorId: STM32_VID, productId: STM32_PID }] });
  } catch (e) {
    throw new Error('No DFU device selected. If it does not appear, use QMK Toolbox instead.');
  }

  await dev.open();
  await dev.selectConfiguration(1);
  await dev.claimInterface(0);
  log('Connected: ' + (dev.productName || 'STM32 DFU'));

  try {
    let s = await dfuGetStatus(dev);
    if (s.state === STATE_ERROR) { await dfuClearStatus(dev); s = await dfuGetStatus(dev); }
    if (s.state !== STATE_IDLE) { await dfuAbort(dev); await waitForState(dev, STATE_IDLE); }

    const sectors = SECTORS.filter((sec) => sec.addr < FLASH_BASE + size);
    log('Erasing ' + sectors.length + ' sectors...');
    for (let i = 0; i < sectors.length; i++) {
      await dfuseCmd(dev, DFUSE_ERASE, sectors[i].addr);
      setProgress(((i + 1) / sectors.length) * 30);
    }

    const pages = Math.ceil(size / PAGE_SIZE);
    log('Writing ' + pages + ' pages...');
    await dfuseCmd(dev, DFUSE_SET_ADDRESS, FLASH_BASE);
    for (let p = 0; p < pages; p++) {
      await dfuDnload(dev, p + 2, firmware.slice(p * PAGE_SIZE, Math.min((p + 1) * PAGE_SIZE, size)));
      await waitForState(dev, STATE_DNLOAD_IDLE);
      setProgress(30 + ((p + 1) / pages) * 65);
    }

    log('Booting new firmware...');
    setProgress(98);
    await dfuseCmd(dev, DFUSE_SET_ADDRESS, FLASH_BASE);
    await dfuDnload(dev, 0, new ArrayBuffer(0));
    try { await dfuGetStatus(dev); } catch (_) {}
    setProgress(100);
    log('Flash complete! Keyboard will reboot.');
    log('Click CONNECT to reconnect.');
    const connectBtn = $('connect-btn');
    if (connectBtn) connectBtn.disabled = false;
  } finally {
    try { await dev.close(); } catch (_) {}
  }
}

/* ── UI wiring ── */

export function initFlasher() {
  const bootloaderBtn = $('flash-bootloader-btn');
  const exitBootBtn = $('flash-exit-btn');
  const flashBtn = $('flash-firmware-btn');
  const fileInput = $('flash-file-input');

  // Disable exit/flash until bootloader is entered
  if (exitBootBtn) exitBootBtn.disabled = true;

  if (fileInput) {
    fileInput.addEventListener('change', () => {
      const file = fileInput.files[0];
      selectedFile = file || null;
      const info = $('flash-file-info');
      if (file) {
        if ($('flash-file-name')) $('flash-file-name').textContent = file.name;
        if ($('flash-file-size')) $('flash-file-size').textContent = (file.size / 1024).toFixed(1) + ' KB';
        if (info) info.classList.remove('hidden');
        if (flashBtn) flashBtn.disabled = !inBootloaderMode;
      } else {
        if (info) info.classList.add('hidden');
        if (flashBtn) flashBtn.disabled = true;
      }
    });
  }

  if (bootloaderBtn) {
    bootloaderBtn.addEventListener('click', async () => {
      clearLog();
      if (!isConnected()) {
        log('ERROR: Not connected. Click CONNECT first.');
        log('Or hold ESC while plugging USB to enter bootloader manually.');
        return;
      }
      bootloaderBtn.disabled = true;
      await enterBootloader();
      inBootloaderMode = true;
      log('Keyboard is in DFU bootloader mode.');
      log('Use QMK Toolbox to flash, or select a .bin file above and click FLASH.');
      bootloaderBtn.disabled = false;
      if (exitBootBtn) exitBootBtn.disabled = false;
      if (flashBtn && selectedFile) flashBtn.disabled = false;
    });
  }

  if (exitBootBtn) {
    exitBootBtn.addEventListener('click', async () => {
      clearLog();
      log('Select "STM32 BOOTLOADER" in the browser popup...');
      let dev;
      try {
        dev = await navigator.usb.requestDevice({ filters: [{ vendorId: STM32_VID, productId: STM32_PID }] });
        await dev.open();
        await dev.selectConfiguration(1);
        await dev.claimInterface(0);
      } catch (_) {
        log('No DFU device selected. Keyboard may not be in bootloader mode.');
        return;
      }
      try {
        let s = await dfuGetStatus(dev);
        if (s.state === STATE_ERROR) { await dfuClearStatus(dev); s = await dfuGetStatus(dev); }
        if (s.state !== STATE_IDLE) { await dfuAbort(dev); await waitForState(dev, STATE_IDLE); }
        await dfuseCmd(dev, DFUSE_SET_ADDRESS, FLASH_BASE);
        await dfuDnload(dev, 0, new ArrayBuffer(0));
        try { await dfuGetStatus(dev); } catch (_) {}
        inBootloaderMode = false;
        log('Keyboard is rebooting with existing firmware.');
        log('Click CONNECT to reconnect.');
        const connectBtn = $('connect-btn');
        if (connectBtn) connectBtn.disabled = false;
        if (flashBtn) flashBtn.disabled = true;
        if (exitBootBtn) exitBootBtn.disabled = true;
      } catch (err) {
        log('ERROR: ' + err.message);
      } finally {
        try { await dev.close(); } catch (_) {}
      }
    });
  }

  if (flashBtn) {
    flashBtn.addEventListener('click', async () => {
      if (!selectedFile) return;
      flashBtn.disabled = true;
      if (bootloaderBtn) bootloaderBtn.disabled = true;
      try {
        await flashFirmware(await selectedFile.arrayBuffer());
      } catch (err) {
        log('ERROR: ' + err.message);
        log('');
        log('The keyboard is NOT bricked.');
        log('Hold ESC + plug USB to re-enter bootloader, then use QMK Toolbox.');
        hideProgress();
      } finally {
        inBootloaderMode = false;
        flashBtn.disabled = true;
        if (exitBootBtn) exitBootBtn.disabled = true;
        if (bootloaderBtn) bootloaderBtn.disabled = false;
      }
    });
  }

  const header = $('flash-driver-toggle');
  const body = $('flash-driver-body');
  if (header && body) {
    header.addEventListener('click', () => {
      header.classList.toggle('open');
      body.classList.toggle('open');
    });
  }
}
