/* Platform I/O only. Outcomes, payouts, reels, UI and PCM come from C++/Wasm. */
import { CONTROL } from './control-config.js';
const $ = selector => document.querySelector(selector);
const status = $('#status');
const canvas = $('#game');
const context = canvas.getContext('2d', { alpha: false });
const frame = context.createImageData(240, 135);
let module, running = false, busy = false, failed = false, database, storageInFlight = false, startupNeedsRetry = false;
let hiddenAt = document.hidden ? performance.now() : null, hiddenElapsed = 0;
let lastRecord = null, lastSnapshot = {}, commits = 0, lastStorageError = '';
const inputQueue = [];
let selectorReleaseFence = false;
function beginOwnership(selector) { storageInFlight = true; inputQueue.length = 0; if (selector) { selectorReleaseFence = true; consumeSelectorInputs(); } }
function consumeSelectorInputs() { blue?.consume(); black?.consume(); }
const sameRecord = (a,b) => { if (!a || !b) return !a && !b; const aa=new Uint8Array(a.bytes),bb=new Uint8Array(b.bytes);return aa.length===bb.length&&aa.every((v,i)=>v===bb[i])&&JSON.stringify(a.preferences)===JSON.stringify(b.preferences); };
const formatError = error => error?.message || String(error);

function openDatabase() {
  return new Promise((resolve, reject) => {
    const request = indexedDB.open('phoenix-link-cpp-demo-v1:' + new URL('.', import.meta.url).pathname, 1);
    request.onupgradeneeded = () => request.result.createObjectStore('records');
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => reject(request.error);
    request.onblocked = () => reject(new Error('Close other preview tabs, then reload.'));
  });
}

function readRecord() {
  return new Promise((resolve, reject) => {
    const transaction = database.transaction('records', 'readonly');
    const request = transaction.objectStore('records').get('current');
    let record;
    request.onsuccess = () => { record = request.result || null; };
    transaction.oncomplete = () => resolve(record);
    transaction.onerror = transaction.onabort = () => reject(transaction.error || new Error('Cannot read the saved game.'));
  });
}

async function persist(bytes, preferences, selector = false) {
  beginOwnership(selector);
  try {
  // Own a copy before Wasm resumes or its memory grows. Keep one exact record
  // per transaction; never replace a failed attempt with a new RNG outcome.
  const record = { bytes: bytes.slice(), preferences: JSON.parse(preferences), format: 1 };
  await new Promise((resolve, reject) => {
    let transaction;
    try { transaction = database.transaction('records', 'readwrite', { durability: 'strict' }); }
    catch (error) {
      if (!(error instanceof TypeError)) throw error;
      transaction = database.transaction('records', 'readwrite');
    }
    const store = transaction.objectStore('records');
    const currentRequest = store.get('current');
    let conflict = false;
    const same = (a, b) => {
      if (!a || !b) return !a && !b;
      const aa = new Uint8Array(a.bytes), bb = new Uint8Array(b.bytes);
      return aa.length === bb.length && aa.every((value, i) => value === bb[i]) &&
        JSON.stringify(a.preferences) === JSON.stringify(b.preferences);
    };
    currentRequest.onsuccess = () => {
      // Compare-and-swap inside the same write transaction. A second tab may
      // not silently replace a newer bank; an ambiguous retry is idempotent.
      const current = currentRequest.result || null;
      if (!same(current, lastRecord) && !same(current, record)) {
        conflict = true; transaction.abort(); return;
      }
      store.put(record, 'current');
    };
    transaction.oncomplete = resolve;
    transaction.onerror = transaction.onabort = () => reject(conflict ? new Error('Another tab updated this game. Reload before playing here.') : transaction.error || new Error('The game could not be saved.'));
  });
  const actual = await readRecord();
  const restored = actual?.bytes instanceof Uint8Array ? actual.bytes : new Uint8Array(actual?.bytes || []);
  if (restored.length !== bytes.length || !restored.every((value, i) => value === bytes[i]) ||
      JSON.stringify(actual.preferences) !== JSON.stringify(record.preferences)) {
    throw new Error('Saved-game readback did not match. Play remains paused.');
  }
  lastRecord = actual;
  commits++;
  return true;
  } finally { storageInFlight = false; }
}


async function verifyOwned(bytes, preferences) {
  beginOwnership(true);
  try {
    const expected={bytes:bytes.slice(),preferences:JSON.parse(preferences),format:1};
    const actual=await readRecord();
    if(!sameRecord(actual,lastRecord)||!sameRecord(actual,expected))throw new Error('The owned saved game changed. Reload before playing here.');
    lastRecord=actual;
    return true;
  } finally { storageInFlight=false; }
}

class BrowserAudio {
  constructor() { this.context = null; this.channels = []; this.masterValue = 192; this.magnification = 1; }
  async enable() {
    const AudioContextClass = window.AudioContext || window.webkitAudioContext;
    if (!AudioContextClass) return false;
    this.context ||= new AudioContextClass({ latencyHint: 'interactive', sampleRate: 44100 });
    if (!this.masterNode) {
      this.masterNode = this.context.createGain();
      this.masterNode.connect(this.context.destination);
      for (let i = 0; i < 8; i++) {
        const gain = this.context.createGain(); gain.connect(this.masterNode);
        this.channels.push({ gain, value: 255, queue: [] });
      }
    }
    await this.context.resume();
    this.master(this.masterValue, this.magnification);
    return this.context.state === 'running';
  }
  available() { return !!this.context && this.context.state !== 'closed'; }
  master(value, magnification, stereo = true) {
    this.masterValue = value; this.magnification = magnification;
    // Pinned M5 S16 mixer: squared master/channel values, mono duplication
    // (or the matching stereo multiplier), then the final mixed-output >>8.
    // Digital gain is 2*mag*master²*channel²/2^36; acoustic output still differs.
    if (this.masterNode) {
      this.masterNode.channelCountMode = 'explicit';
      this.masterNode.channelInterpretation = 'speakers';
      this.masterNode.channelCount = stereo ? 2 : 1;
      // WebAudio folds stereo to mono by averaging. Applying this mono2
      // factor to that average equals the M5 mono L+R sum at its native gain.
      this.masterNode.gain.value = 2 * magnification * value ** 2 * 255 ** 2 / 2 ** 36;
    }
  }
  gain(channel, value) {
    const lane = this.channels[channel]; if (!lane) return;
    lane.value = value; lane.gain.gain.value = (value / 255) ** 2;
  }
  queued(channel) {
    const lane = this.channels[channel]; if (!lane || !this.available()) return 0;
    lane.queue = lane.queue.filter(voice => !voice.ended && (voice.loop || voice.end > this.context.currentTime));
    return lane.queue.length;
  }
  play(samples, count, rate, stereo, repeat, channel, replace) {
    const lane = this.channels[channel];
    if (!lane || !this.available() || !count || !rate || !repeat) return false;
    if (replace) this.stop(channel);
    if (this.queued(channel) >= 2) return false;
    const channels = stereo ? 2 : 1;
    const length = count / channels;
    if (!Number.isInteger(length)) return false;
    const buffer = this.context.createBuffer(channels, length, rate);
    for (let c = 0; c < channels; c++) {
      const out = buffer.getChannelData(c);
      for (let i = 0; i < length; i++) out[i] = samples[i * channels + c] / 32768;
    }
    // WebAudio owns its PCM copy; C++ borrowed buffers may safely be reused.
    const source = this.context.createBufferSource(); source.buffer = buffer; source.connect(lane.gain);
    const loop = repeat === 0xffffffff;
    source.loop = loop || repeat > 1;
    const start = Math.max(this.context.currentTime + 0.003, lane.queue.at(-1)?.end || 0);
    const end = loop ? Infinity : start + buffer.duration * repeat;
    const voice = { source, start, end, loop, ended: false };
    source.onended = () => { voice.ended = true; source.disconnect(); };
    lane.queue.push(voice); source.start(start);
    if (!loop) source.stop(end);
    return true;
  }
  stop(channel) {
    if (channel < 0) { for (let i = 0; i < this.channels.length; i++) this.stop(i); return; }
    const lane = this.channels[channel]; if (!lane) return;
    for (const voice of lane.queue) {
      voice.ended = true;
      try { voice.source.stop(); } catch { /* Already ended. */ }
      voice.source.disconnect();
    }
    lane.queue = [];
  }
}
const audio = new BrowserAudio();

const bridge = {
  audio,
  heldButtons: 0,
  clock: () => (hiddenAt === null ? performance.now() : hiddenAt) - hiddenElapsed,
  persist,
  verifyOwned,
  ownedPreferences: () => lastRecord ? JSON.stringify(lastRecord.preferences) : null,
  storageFailure(error) {
    lastStorageError = formatError(error);
    failed = true;
    status.textContent = `${formatError(error)} Your current game is preserved; retry saving to continue.`;
    $('#retry').hidden = false;
  },
  present(buffer, pointer, width, height, rotation, brightness) {
    const pixels = new Uint16Array(buffer, pointer, width * height);
    for (let i = 0, j = 0; i < pixels.length; i++, j += 4) {
      const pixel = pixels[i];
      frame.data[j] = Math.round(((pixel >>> 11) & 31) * 255 / 31);
      frame.data[j + 1] = Math.round(((pixel >>> 5) & 63) * 255 / 63);
      frame.data[j + 2] = Math.round((pixel & 31) * 255 / 31);
      frame.data[j + 3] = 255;
    }
    context.putImageData(frame, 0, 0);
    canvas.style.transform = rotation === 3 ? 'rotate(180deg)' : '';
    canvas.style.filter = `brightness(${Math.max(0.25, brightness / 200)})`;
  }
};

async function call(name, result = null, types = [], args = []) {
  return module.ccall(name, result, types, args, { async: true });
}

function queueInput(code) {
  if (!running || failed || busy || storageInFlight || selectorReleaseFence || hiddenAt !== null) return;
  inputQueue.push(code);
}

function finishStartup() {
  if (lastRecord?.preferences) {
    const p = lastRecord.preferences;
    module._pl_preferences(!!p.spin, !!p.landings, p.volume, !!p.muted, !!p.credits, !!p.flipped, !!p.autoRefill, p.lastAddedMinor, p.brightness);
  }
  const info = JSON.parse(module.UTF8ToString(module._pl_info()));
  $('#version').textContent = info.version;
  status.textContent = info.accepted ? 'Saved on this browser. Play money only.' : 'Private unaccepted source review. Play money only; not a released build.';
  $('#cover').hidden = true;
  running = true;
  // Read-only diagnostics; no scripted-result, forced-feature or testEngine API.
  window.phoenixDemo = { snapshot: () => ({ ...lastSnapshot }), commits: () => commits, info: () => info, ready: () => running && !busy && !failed && !selectorReleaseFence };
  requestAnimationFrame(animationFrame);
}

async function animationFrame() {
  if (!running) return;
  if (!busy && hiddenAt === null) {
    busy = true;
    try {
      if (!failed) {
        await call('pl_frame', null, ['number'], [inputQueue.length ? inputQueue.shift() : -1]);
        lastSnapshot = JSON.parse(module.UTF8ToString(module._pl_snapshot()));
        selectorReleaseFence = !!module._pl_return_input_fenced();
        if (selectorReleaseFence) { inputQueue.length=0; consumeSelectorInputs(); }
      }
    } catch (error) {
      failed = true; status.textContent = `The preview stopped: ${formatError(error)}`;
    } finally { busy = false; }
  }
  requestAnimationFrame(animationFrame);
}

$('#start').addEventListener('click', async () => {
  $('#start').disabled = true;
  status.textContent = 'Loading game…';
  try {
    await audio.enable();
    database = await openDatabase();
    lastRecord = await readRecord();
    module = await window.PhoenixModule({ locateFile: file => new URL(file, import.meta.url).href });
    module.browserBridge = bridge;
    let bytes = null;
    if (lastRecord) {
      bytes = lastRecord.bytes instanceof Uint8Array ? lastRecord.bytes : new Uint8Array(lastRecord.bytes);
      if (!bytes.length || bytes.length > module._pl_checkpoint_capacity()) throw new Error('The stored game has an invalid size. It has been kept intact.');
    }
    const boot = bytes ? await call('pl_boot', 'number', ['array', 'number'], [bytes, bytes.length]) : await call('pl_boot', 'number', ['number', 'number'], [0, 0]);
    if (boot !== 1) {
      console.error('Browser startup diagnostics', module.UTF8ToString(module._pl_info()));
      if (boot === -2) { startupNeedsRetry = true; failed = true; $('#retry').hidden = false; status.textContent = `The initial game is waiting for a durable save. ${lastStorageError}`; return; }
      throw new Error(boot === -1 ? 'The stored game could not be validated. It has been kept intact.' : `The initial game could not be saved. ${lastStorageError}`);
    }
    finishStartup();
  } catch (error) {
    failed = true; status.textContent = formatError(error); $('#start').disabled = false;
  }
});

$('#retry').addEventListener('click', async () => {
  if (busy || !module) return;
  busy = true;
  try {
    if (await call('pl_retry_storage', 'number')) {
      failed = false; $('#retry').hidden = true; status.textContent = 'Game saved. Press blue to resume.';
      if (startupNeedsRetry) { startupNeedsRetry = false; finishStartup(); }
    }
  } catch (error) { bridge.storageFailure(error); }
  finally { busy = false; }
});

$('#download').addEventListener('click', () => {
  if (!lastRecord) return;
  const url = URL.createObjectURL(new Blob([lastRecord.bytes], { type: 'application/octet-stream' }));
  const link = document.createElement('a'); link.href = url; link.download = 'phoenix-browser-game.pck'; link.click();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
});

let chord = false;
function bindButton(element, short, long) {
  let down = false, held = false, consumed = false, timer = 0;
  const mask = element.id === 'blue' ? 1 : 2;
  const press = event => {
    if (event.repeat || down) return;
    if (!running || hiddenAt !== null) { event.preventDefault(); return; }
    event.preventDefault(); down = true; held = false; consumed = failed || busy || storageInFlight || selectorReleaseFence; bridge.heldButtons |= mask;
    if (bridge.heldButtons === 3) chord = true;
    if (event.pointerId !== undefined) element.setPointerCapture(event.pointerId);
    element.classList.add('pressed');
    if (consumed) return;
    const longWhenAvailable = () => {
      if (!down) return;
      held = true;
      if (chord || failed || selectorReleaseFence || consumed) { consumed=true; return; }
      if (busy || storageInFlight) { timer = setTimeout(longWhenAvailable, 16); return; }
      long();
    };
    timer = setTimeout(longWhenAvailable, CONTROL.holdMs);
  };
  const release = event => {
    if (!down) return;
    event.preventDefault(); down = false; bridge.heldButtons &= ~mask; clearTimeout(timer); element.classList.remove('pressed');
    if (!held && !chord && !consumed) short();
    if (!bridge.heldButtons) chord = false;
  };
  element.addEventListener('pointerdown', press); element.addEventListener('pointerup', release);
  element.addEventListener('keydown', event => { if (event.code === 'Space' || event.code === 'Enter') { event.stopPropagation(); press(event); } });
  element.addEventListener('keyup', event => { if (event.code === 'Space' || event.code === 'Enter') { event.stopPropagation(); release(event); } });
  const cancel = () => { down = false; bridge.heldButtons &= ~mask; clearTimeout(timer); element.classList.remove('pressed'); if (!bridge.heldButtons) chord = false; };
  element.addEventListener('pointercancel', cancel);
  const consume = () => { if (down) { consumed=true; held=true; clearTimeout(timer); } };
  return { press, release, cancel, consume };
}
const blue = bindButton($('#blue'), () => queueInput(0), () => queueInput(3));
const black = bindButton($('#black'), () => queueInput(1), () => queueInput(2));
window.addEventListener('keydown', event => {
  if (event.target.closest('input,textarea,a')) return;
  if (event.code === 'Space') blue.press(event);
  if (event.code === 'ArrowRight' || event.code === 'Enter') black.press(event);
  if (event.code === 'Escape') { event.preventDefault(); queueInput(5); }
});
window.addEventListener('keyup', event => {
  if (event.code === 'Space') blue.release(event);
  if (event.code === 'ArrowRight' || event.code === 'Enter') black.release(event);
});
document.addEventListener('visibilitychange', () => {
  if (document.hidden && hiddenAt === null) { blue.cancel(); black.cancel(); hiddenAt = performance.now(); audio.context?.suspend(); }
  else if (!document.hidden && hiddenAt !== null) { hiddenElapsed += performance.now() - hiddenAt; hiddenAt = null; audio.context?.resume(); }
});
window.addEventListener('blur', () => { blue.cancel(); black.cancel(); });

if ('serviceWorker' in navigator) navigator.serviceWorker.register('./sw.js').catch(() => {});
