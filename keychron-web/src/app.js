/**
 * Entry point — tab switching, connection button, status bar,
 * and module initialisation.
 */

import { connect, isConnected } from './modules/hid.js';
import {
  initConfigurator,
  readAll,
  sendAll,
  saveToEeprom,
  startPolling,
  stopPolling,
} from './modules/configurator.js';
import { initFlasher } from './modules/flasher.js';

/* ── DOM helpers ── */
const $ = (id) => document.getElementById(id);

let statusTimeout = null;

/* ── Status bar ── */
function setStatus(msg, className, revertMs) {
  clearTimeout(statusTimeout);
  $('status').textContent = msg;
  if (className) $('status').className = className;
  if (revertMs && isConnected()) {
    statusTimeout = setTimeout(() => {
      if (isConnected()) $('status').textContent = 'ONLINE';
    }, revertMs);
  }
}

/* ── Busy state (disables action buttons) ── */
function setBusyUI(isBusy) {
  $('read-btn').disabled = isBusy;
  $('send-btn').disabled = isBusy;
  $('save-btn').disabled = isBusy;
}

/* ── LED indicator ── */
function updateLed(id, on) {
  const el = $(id);
  if (!el) return;
  el.classList.toggle('on', on);
}

/* ── Disconnect handler ── */
function onDisconnect() {
  stopPolling();
  $('connect-btn').disabled = false;
  $('leds').classList.add('hidden');
  $('dashboard').classList.add('hidden');
  $('welcome').classList.remove('hidden');
  $('device-name').textContent = 'Not connected';
  setBusyUI(false);
  setStatus('OFFLINE', 'disconnected');
}

/* ── Tab switching ── */
function initTabs() {
  document.querySelectorAll('.tab').forEach((tab) => {
    tab.addEventListener('click', () => {
      document.querySelectorAll('.tab').forEach((t) => t.classList.remove('active'));
      document.querySelectorAll('.tab-content').forEach((c) => c.classList.remove('active'));
      tab.classList.add('active');
      document.getElementById('tab-' + tab.dataset.tab).classList.add('active');
    });
  });
}

/* ── Connect button ── */
function initConnectButton() {
  // WebHID check
  if (!navigator.hid) {
    $('connect-btn').disabled = true;
    setStatus('NO WEBHID', 'disconnected');
    $('welcome').querySelector('p').textContent =
      'WebHID is not available. Use Chrome or Edge.';
    return;
  }

  $('connect-btn').addEventListener('click', async () => {
    try {
      const device = await connect({
        onConnect: (dev) => {
          $('device-name').textContent = dev.productName || 'Keychron Q3 Max';
          setStatus('ONLINE', 'connected');
          $('connect-btn').disabled = true;
          $('welcome').classList.add('hidden');
          $('dashboard').classList.remove('hidden');
          $('leds').classList.remove('hidden');
        },
        onDisconnect,
      });

      if (!device) return; // user cancelled picker

      await readAll();
      startPolling();
    } catch (err) {
      console.error('connect error:', err);
      $('connect-btn').disabled = false;
      setStatus('ERROR', 'disconnected');
    }
  });
}

/* ── Action buttons ── */
function initActionButtons() {
  $('read-btn').addEventListener('click', () => {
    if (isConnected()) readAll();
  });
  $('send-btn').addEventListener('click', () => {
    if (isConnected()) sendAll();
  });
  $('save-btn').addEventListener('click', () => {
    if (isConnected()) saveToEeprom();
  });
}

/* ── Bootstrap ── */
function init() {
  initTabs();
  initConnectButton();

  initConfigurator({
    setStatus,
    setBusyUI,
    updateLed,
  });

  initActionButtons();
  initFlasher();
}

init();
