/**
 * Minimal stand-in for the browser DOM: only the element behaviour
 * configurator.js relies on. Unknown IDs throw so a typo can't pass silently.
 */
class FakeElement {
  constructor(tagName) {
    this.tagName = tagName;
    this.children = [];
    this.value = '';
    this.textContent = '';
    this.disabled = false;
    this.listeners = {};
  }

  appendChild(child) {
    this.children.push(child);
    return child;
  }

  addEventListener(type, fn) {
    this.listeners[type] = fn;
  }
}

// Like a real <select>: shows the first option until a value is chosen, and an unknown value selects nothing.
class FakeSelect extends FakeElement {
  constructor() {
    super('select');
    this.chosen = null;
  }

  get options() {
    return this.children.flatMap((c) => (c.tagName === 'optgroup' ? c.children : [c]));
  }

  get value() {
    const values = this.options.map((o) => String(o.value));
    if (this.chosen === null) return values[0] ?? '';
    return values.includes(this.chosen) ? this.chosen : '';
  }

  set value(v) {
    this.chosen = String(v);
  }
}

/** Installs globalThis.document with the given element IDs; returns the elements by ID. */
export function installFakeDom({ inputs, selects }) {
  const elements = {};
  for (const id of inputs) elements[id] = new FakeElement('input');
  for (const id of selects) elements[id] = new FakeSelect();

  globalThis.document = {
    getElementById: (id) => {
      if (!(id in elements)) throw new Error('fake DOM has no element #' + id);
      return elements[id];
    },
    createElement: (tagName) => (tagName === 'select' ? new FakeSelect() : new FakeElement(tagName)),
  };
  return elements;
}
