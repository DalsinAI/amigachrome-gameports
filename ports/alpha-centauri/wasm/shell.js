// SPDX-License-Identifier: GPL-3.0-or-later
// Page glue for the WebAssembly Alpha Centauri frontend (alpha.js).
// - The player picks their own SMAC/SMAX install folder or files; they are
//   copied into the in-memory file system at /game (never uploaded anywhere)
//   and, if asked, kept in this browser's IndexedDB for the next visit.
// - File names resolve case-insensitively and "PROGDIR:" means /game, as the
//   Amiga build expects of its disk.
// - DOM input becomes Intuition messages for amiga/amiga_shim.cpp; the top
//   window's RGB frame is drawn on the canvas.
'use strict';
(function () {
  const $ = (id) => document.getElementById(id);
  const GAME = '/game';
  // Media and programs the Amiga build does not read. Original .wve movies
  // need the prepared .amov cache (native/prepare_startup_movie.py): their EA
  // TQI video has no decoder in the browser.
  const SKIP = /\.(wve|avi|exe|dll|ico|lnk|url)$/i;
  const MAX_FILE = 96 * 1024 * 1024, MAX_MOVIE = 640 * 1024 * 1024;
  const REQUIRED = ['openinga.pcx', 'ALPHC___.TTF', 'arialn.ttf'];
  const WANTED = ['alpha.txt', 'xopeninga.pcx', 'alphax.txt', 'texture.pcx', 'Units.pcx', 'console.pcx', 'Arial.ttf', 'opening menu.wav'];
  const DB_NAME = 'alpha-centauri-wasm', DB_STORE = 'files';

  const log = [], logTimes = [];
  let logDirty = false;
  function out(line, isErr) {
    log.push((isErr ? '! ' : '') + line);
    logTimes.push(performance.now());
    if (log.length > 20000) { log.splice(0, log.length - 20000); logTimes.splice(0, logTimes.length - 20000); }
    logDirty = true;
    if (isErr) console.warn(line);
  }
  setInterval(() => {
    if (!logDirty) return;
    logDirty = false;
    const pre = $('log');
    if (pre) { pre.textContent = log.slice(-300).join('\n'); pre.scrollTop = pre.scrollHeight; }
  }, 250);

  function status(text) { $('status').textContent = text; }

  // ---- presentation and input ----
  const canvas = $('screen');
  const ctx = canvas.getContext('2d', { alpha: false });
  let image = null;
  let running = false, modalOpen = false;
  let waiter = null;
  const events = [];
  function push(cls, code, x, y) {
    events.push({ cls, code, x: x | 0, y: y | 0 });
    if (waiter) { const w = waiter; waiter = null; w(1); }
  }
  const IDCMP_MOUSEBUTTONS = 0x8, IDCMP_CLOSEWINDOW = 0x200, IDCMP_RAWKEY = 0x400, IDCMP_VANILLAKEY = 0x200000;
  const RAW = { ArrowUp: 0x4c, ArrowDown: 0x4d, ArrowRight: 0x4e, ArrowLeft: 0x4f, Help: 0x5f,
    F1: 0x50, F2: 0x51, F3: 0x52, F4: 0x53, F5: 0x54, F6: 0x55, F7: 0x56, F8: 0x57, F9: 0x58, F10: 0x59 };
  const VANILLA = { Enter: 13, Escape: 27, Backspace: 8, Tab: 9, Delete: 127 };
  window.addEventListener('keydown', (e) => {
    if (!running || modalOpen) return;
    if (e.target && /^(INPUT|TEXTAREA|SELECT|BUTTON)$/.test(e.target.tagName) && e.target !== canvas) return;
    let handled = true;
    if (e.key in RAW) push(IDCMP_RAWKEY, RAW[e.key], 0, 0);
    else if (e.key in VANILLA) push(IDCMP_VANILLAKEY, VANILLA[e.key], 0, 0);
    else if (e.key.length === 1 && !e.altKey && !e.metaKey) {
      let c = e.key.charCodeAt(0);
      if (e.ctrlKey) { if (/[a-z]/i.test(e.key)) c &= 0x1f; else handled = false; }
      if (handled && c < 256) push(IDCMP_VANILLAKEY, c, 0, 0); else handled = false;
    } else handled = false;
    if (handled) e.preventDefault();
  });
  function canvasPoint(e) {
    const r = canvas.getBoundingClientRect();
    return [Math.floor((e.clientX - r.left) * canvas.width / r.width), Math.floor((e.clientY - r.top) * canvas.height / r.height)];
  }
  canvas.addEventListener('mousedown', (e) => {
    if (!running || modalOpen) return;
    const [x, y] = canvasPoint(e);
    push(IDCMP_MOUSEBUTTONS, e.button === 2 ? 0x69 : 0x68, x, y);
    canvas.focus();
    e.preventDefault();
  });
  canvas.addEventListener('mouseup', (e) => {
    if (!running || modalOpen) return;
    const [x, y] = canvasPoint(e);
    push(IDCMP_MOUSEBUTTONS, e.button === 2 ? 0xe9 : 0xe8, x, y);
  });
  canvas.addEventListener('contextmenu', (e) => e.preventDefault());
  $('close-window').addEventListener('click', () => { if (running && !modalOpen) push(IDCMP_CLOSEWINDOW, 0, 0, 0); canvas.focus(); });

  function present(rgb, w, h, title) {
    if (canvas.width !== w || canvas.height !== h || !image) {
      canvas.width = w; canvas.height = h;
      image = ctx.createImageData(w, h);
      const d = image.data;
      for (let i = 3; i < d.length; i += 4) d[i] = 255;
    }
    const d = image.data;
    for (let i = 0, j = 0, n = w * h; i < n; ++i, j += 3) { const k = i * 4; d[k] = rgb[j]; d[k + 1] = rgb[j + 1]; d[k + 2] = rgb[j + 2]; }
    ctx.putImageData(image, 0, 0);
    $('window-title').textContent = title || '';
  }

  // ---- file system helpers ----
  let FS = null;
  function ensureDir(path) {
    let cur = '';
    for (const part of path.split('/').filter(Boolean)) {
      cur += '/' + part;
      try { FS.mkdir(cur); } catch (e) { /* exists */ }
    }
  }
  function writeFile(rel, bytes) {
    rel = rel.replace(/^\/+/, '');
    const dir = rel.includes('/') ? rel.slice(0, rel.lastIndexOf('/')) : '';
    ensureDir(GAME + (dir ? '/' + dir : ''));
    FS.writeFile(GAME + '/' + rel, bytes);
  }
  function exists(name) {
    try { FS.lookupPath(GAME + '/' + name); return true; } catch (e) { return false; }
  }
  function listFiles(dir, prefix) {
    const outList = [];
    let names = [];
    try { names = FS.readdir(dir); } catch (e) { return outList; }
    for (const n of names) {
      if (n === '.' || n === '..') continue;
      const p = dir + '/' + n;
      const st = FS.stat(p);
      if (FS.isDir(st.mode)) outList.push(...listFiles(p, prefix + n + '/'));
      else outList.push({ name: prefix + n, size: st.size, mtime: st.mtime });
    }
    return outList;
  }
  function patchFS() {
    // Case-insensitive names, like the Amiga file system the native code expects.
    const lookupNode = FS.lookupNode;
    FS.lookupNode = function (parent, name) {
      try { return lookupNode.call(FS, parent, name); } catch (e) {
        if (parent && parent.contents && typeof parent.contents === 'object' && !ArrayBuffer.isView(parent.contents)) {
          const lower = String(name).toLowerCase();
          for (const k of Object.keys(parent.contents)) if (k.toLowerCase() === lower) return lookupNode.call(FS, parent, k);
        }
        throw e;
      }
    };
    // PROGDIR: is the program's directory: /game.
    const fix = (p) => typeof p === 'string' ? p.replace(/(^|\/)PROGDIR:/, (m, a) => a || GAME + '/') : p;
    for (const name of ['open', 'stat', 'lstat', 'mkdir', 'unlink', 'rename']) {
      const f = FS[name];
      FS[name] = function (p, ...rest) { return f.call(FS, fix(p), ...rest.map((r, i) => name === 'rename' && i === 0 ? fix(r) : r)); };
    }
  }

  // ---- IndexedDB copy of the player's files (optional) ----
  function db() {
    return new Promise((resolve, reject) => {
      const r = indexedDB.open(DB_NAME, 1);
      r.onupgradeneeded = () => r.result.createObjectStore(DB_STORE);
      r.onsuccess = () => resolve(r.result);
      r.onerror = () => reject(r.error);
    });
  }
  async function dbPut(entries) {
    const d = await db();
    await new Promise((resolve, reject) => {
      const t = d.transaction(DB_STORE, 'readwrite');
      for (const [k, v] of entries) t.objectStore(DB_STORE).put(v, k);
      t.oncomplete = resolve; t.onerror = () => reject(t.error);
    });
    d.close();
  }
  async function dbAll() {
    const d = await db();
    const result = await new Promise((resolve, reject) => {
      const t = d.transaction(DB_STORE, 'readonly');
      const s = t.objectStore(DB_STORE);
      const keys = s.getAllKeys(), vals = s.getAll();
      t.oncomplete = () => resolve(keys.result.map((k, i) => [k, vals.result[i]]));
      t.onerror = () => reject(t.error);
    });
    d.close();
    return result;
  }
  async function dbClear() {
    const d = await db();
    await new Promise((resolve) => { const t = d.transaction(DB_STORE, 'readwrite'); t.objectStore(DB_STORE).clear(); t.oncomplete = resolve; });
    d.close();
  }

  // ---- importing the player's files ----
  let imported = 0;
  async function importFiles(fileList, keep) {
    const files = Array.from(fileList);
    const entries = [];
    let skipped = 0, done = 0;
    for (const f of files) {
      let rel = f.webkitRelativePath || f.name;
      if (f.webkitRelativePath) rel = rel.split('/').slice(1).join('/') || f.name; // drop the chosen folder's own name
      if (/\.wve$/i.test(rel)) seenMovies.add(rel.split('/').pop().replace(/\.wve$/i, '').toLowerCase());
      if (SKIP.test(rel) || f.size > (/\.amov$/i.test(rel) ? MAX_MOVIE : MAX_FILE)) { ++skipped; continue; }
      const bytes = new Uint8Array(await f.arrayBuffer());
      writeFile(rel, bytes);
      if (keep) entries.push([rel, bytes]);
      if (++done % 50 === 0) status(`Reading your files... ${done}/${files.length}`);
    }
    imported += done;
    if (keep && entries.length) { try { await dbPut(entries); } catch (e) { out('Could not keep files in this browser: ' + e, true); } }
    const prepared = await prepareAudio();
    refreshSetup(`${done} file(s) added${skipped ? `, ${skipped} skipped (movies/programs not used here)` : ''}${prepared ? `, ${prepared} voice/music file(s) converted for playback` : ''}.`);
  }

  // ---- in-browser audio preparation ----
  // The native game reads voice-overs and music as PCM WAV
  // (tools/prepare_alpha_audio.py converts the original voices/*.mp3 and the
  // owner's music renders to PCM16 mono 22050 Hz with ffmpeg). Here the
  // browser's own decoder does the same once per visit: voices/NAME.mp3 ->
  // voices/NAME.wav, music/NAME.{mp3,ogg,flac,m4a,opus} -> music/NAME.wav.
  // Existing .wav files are used as they are.
  const seenMovies = new Set();
  const COMPRESSED = /^(voices|music)\/[^/]+\.(mp3|ogg|flac|m4a|opus)$/i;
  function wav16(samples, rate) {
    const n = samples.length, buf = new ArrayBuffer(44 + n * 2), v = new DataView(buf);
    const text = (o, t) => { for (let i = 0; i < t.length; ++i) v.setUint8(o + i, t.charCodeAt(i)); };
    text(0, 'RIFF'); v.setUint32(4, 36 + n * 2, true); text(8, 'WAVE'); text(12, 'fmt ');
    v.setUint32(16, 16, true); v.setUint16(20, 1, true); v.setUint16(22, 1, true); v.setUint32(24, rate, true);
    v.setUint32(28, rate * 2, true); v.setUint16(32, 2, true); v.setUint16(34, 16, true); text(36, 'data'); v.setUint32(40, n * 2, true);
    for (let i = 0; i < n; ++i) { const x = Math.max(-1, Math.min(1, samples[i])); v.setInt16(44 + i * 2, Math.round(x * 32767), true); }
    return new Uint8Array(buf);
  }
  async function toWav(bytes) {
    const ctx = new OfflineAudioContext(1, 1, 22050); // decodeAudioData resamples to 22050 Hz
    const audio = await ctx.decodeAudioData(bytes.slice().buffer);
    const mono = new Float32Array(audio.length);
    for (let c = 0; c < audio.numberOfChannels; ++c) { const d = audio.getChannelData(c); for (let i = 0; i < d.length; ++i) mono[i] += d[i] / audio.numberOfChannels; }
    return wav16(mono, audio.sampleRate);
  }
  async function prepareAudio() {
    if (!FS) return 0;
    const todo = listFiles(GAME, '').filter((f) => COMPRESSED.test(f.name) && !exists(f.name.replace(/\.[^.]+$/, '.wav')));
    let made = 0;
    for (const f of todo) {
      if (made % 10 === 0) status(`Converting voices and music for playback... ${made}/${todo.length}`);
      try {
        writeFile(f.name.replace(/\.[^.]+$/, '.wav'), await toWav(FS.readFile(GAME + '/' + f.name)));
        ++made;
        out(`AUDIO PREPARED ${f.name}`, false);
      } catch (e) { out(`Could not decode ${f.name}: ${e && e.message || e}`, true); }
    }
    return made;
  }
  function refreshSetup(note) {
    const missing = REQUIRED.filter((n) => !exists(n));
    const optional = WANTED.filter((n) => !exists(n));
    const all = FS ? listFiles(GAME, '') : [];
    $('file-count').textContent = String(all.length);
    $('missing').textContent = missing.length ? 'Missing (the title screen needs these): ' + missing.join(', ') : 'Title screen files found.';
    const movies = [...seenMovies].filter((m) => (m === 'opening' || m === 'openingx') && !exists(m + '.amov'));
    $('optional').textContent = (optional.length ? 'Not found (needed later for the map, Crossfire or sound): ' + optional.join(', ') + '. ' : '') +
      (movies.length ? `The intro movie (${movies.map((m) => m + '.wve').join(', ')}) needs its prepared .amov cache (native/prepare_startup_movie.py); the browser cannot decode EA TQI video, so V skips with a message.` : '');
    $('start').disabled = !FS || missing.length > 0 || running;
    status(note || (missing.length ? 'Choose your Alpha Centauri folder or files.' : 'Ready. Press Start.'));
    refreshSaves();
  }

  // ---- saves made by the game (AlphaPlay-NNN.SAV) ----
  const knownSaves = new Map();
  function refreshSaves() {
    if (!FS) return;
    const list = $('saves');
    const saves = listFiles(GAME, '').filter((f) => /\.sav$/i.test(f.name));
    list.innerHTML = '';
    for (const s of saves) {
      const li = document.createElement('li');
      const a = document.createElement('a');
      a.textContent = `${s.name} (${s.size} bytes)`;
      a.href = '#';
      a.addEventListener('click', (e) => {
        e.preventDefault();
        const blob = new Blob([FS.readFile(GAME + '/' + s.name)], { type: 'application/octet-stream' });
        const url = URL.createObjectURL(blob);
        const link = document.createElement('a'); link.href = url; link.download = s.name.split('/').pop(); link.click();
        setTimeout(() => URL.revokeObjectURL(url), 1000);
      });
      li.appendChild(a); list.appendChild(li);
    }
    $('saves-empty').hidden = saves.length > 0;
    // Keep new or changed saves in this browser when the player chose to.
    if ($('keep').checked) {
      const changed = saves.filter((s) => knownSaves.get(s.name) !== +s.mtime + ':' + s.size);
      if (changed.length) {
        for (const s of changed) knownSaves.set(s.name, +s.mtime + ':' + s.size);
        dbPut(changed.map((s) => [s.name, FS.readFile(GAME + '/' + s.name)])).catch((e) => out('Could not keep a save: ' + e, true));
      }
    }
  }
  setInterval(() => { if (running) refreshSaves(); }, 3000);

  // ---- the Amiga file requester (ASL) as a page dialog ----
  function askFile(title) {
    return new Promise((resolve) => {
      modalOpen = true;
      const dlg = $('requester');
      $('requester-title').textContent = title;
      const list = $('requester-list');
      list.innerHTML = '';
      const files = listFiles(GAME, '').filter((f) => /\.(sav|sc|scn|scen|mp)$/i.test(f.name) || /\.sav$/i.test(f.name));
      files.sort((a, b) => (/\.sav$/i.test(b.name) - /\.sav$/i.test(a.name)) || a.name.localeCompare(b.name));
      let choice = null;
      const finish = (name) => {
        dlg.close(); modalOpen = false; canvas.focus();
        $('requester-add').value = '';
        resolve(name);
      };
      for (const f of files) {
        const li = document.createElement('li');
        const b = document.createElement('button');
        b.type = 'button'; b.textContent = f.name;
        b.addEventListener('click', () => { choice = f.name; finish(choice); });
        b.addEventListener('dblclick', () => finish(f.name));
        li.appendChild(b); list.appendChild(li);
      }
      $('requester-none').hidden = files.length > 0;
      $('requester-add').onchange = async (e) => {
        const f = e.target.files[0];
        if (!f) return;
        writeFile(f.name, new Uint8Array(await f.arrayBuffer()));
        finish(f.name);
      };
      $('requester-cancel').onclick = () => finish(null);
      dlg.oncancel = (e) => { e.preventDefault(); finish(null); };
      dlg.showModal();
    });
  }

  // ---- "AI turn in progress": the engine yields every ~25 ms (wasm_runtime.cpp) ----
  // busyStats.gap: the longest stretch between two engine yields and the
  // yield points at both ends (sites numbered in wasm_runtime.cpp).
  const busyStats = { yields: 0, longest: 0, shownMs: 0, gap: { ms: 0, from: 0, to: 0 } };
  let busyShown = false;
  function busy(ms, gap, from, to) {
    ++busyStats.yields;
    if (ms > busyStats.longest) busyStats.longest = ms;
    if (gap > busyStats.gap.ms) busyStats.gap = { ms: Math.round(gap), from, to };
    if (ms < 300) return;
    if (!busyShown) { busyShown = true; $('busy').hidden = false; }
    busyStats.shownMs = ms;
    $('busy-time').textContent = (ms / 1000).toFixed(1) + ' s';
  }
  function idle() { if (busyShown) { busyShown = false; $('busy').hidden = true; } }

  // ---- sound level of everything the game sends to the speakers ----
  // Every audio node connected to the output also feeds an analyser, so the
  // page can show (and tests can check) that real, non-silent sound plays.
  const audioStats = { contexts: 0, state: 'none', peak: 0, loudMs: 0 };
  const taps = [];
  const connect = AudioNode.prototype.connect;
  AudioNode.prototype.connect = function (dest, ...rest) {
    const r = connect.call(this, dest, ...rest);
    try {
      if (dest instanceof AudioDestinationNode && !taps.some((t) => t.node === this)) {
        const a = this.context.createAnalyser();
        a.fftSize = 2048;
        connect.call(this, a);
        taps.push({ node: this, ctx: this.context, a, buf: new Float32Array(a.fftSize) });
        audioStats.contexts = new Set(taps.map((t) => t.ctx)).size;
      }
    } catch (e) { /* metering only */ }
    return r;
  };
  // SDL closes an audio device by disconnecting its ScriptProcessorNode but
  // leaves the node's onaudioprocess handler, which holds the freed device.
  // An audio block already queued then runs SDL's mixer on freed memory once
  // the page gets control back (seen as "divide by zero" right after the
  // world view replaces the title's audio device). A full disconnect also
  // drops the handler, and the analyser tap goes with the node.
  if (typeof ScriptProcessorNode !== 'undefined') {
    const disconnect = AudioNode.prototype.disconnect;
    ScriptProcessorNode.prototype.disconnect = function (...args) {
      if (!args.length) {
        this.onaudioprocess = null;
        const i = taps.findIndex((t) => t.node === this);
        if (i >= 0) taps.splice(i, 1);
      }
      return disconnect.apply(this, args);
    };
  }
  setInterval(() => {
    let peak = 0, state = 'none';
    for (const t of taps) {
      state = t.ctx.state;
      if (state !== 'running') continue;
      t.a.getFloatTimeDomainData(t.buf);
      for (const v of t.buf) { const m = Math.abs(v); if (m > peak) peak = m; }
    }
    audioStats.state = state; audioStats.peak = peak;
    if (peak > 0.002) audioStats.loudMs += 100;
    const el = $('sound');
    if (el) el.textContent = state === 'none' ? 'Sound: none yet' : state !== 'running' ? `Sound: ${state} (click the page)` : peak > 0.002 ? 'Sound: playing' : 'Sound: on, silent';
  }, 100);
  document.addEventListener('pointerdown', () => { for (const t of taps) if (t.ctx.state === 'suspended') t.ctx.resume(); });

  // ---- the emscripten module ----
  window.Module = {
    canvas,
    noInitialRun: true,
    print: (t) => out(t, false),
    printErr: (t) => out(t, true),
    alphaEvents: events,
    alphaLog: log,
    alphaLogTimes: logTimes,
    alphaPresent: present,
    alphaAskFile: askFile,
    alphaBusy: busy,
    alphaIdle: idle,
    alphaBusyStats: busyStats,
    alphaAudioStats: audioStats,
    alphaAudioEvents: [],
    alphaWaitEvent: (ms) => new Promise((resolve) => {
      if (events.length) { resolve(1); return; }
      const t = setTimeout(() => { if (waiter === done) waiter = null; resolve(0); }, ms);
      const done = (v) => { clearTimeout(t); resolve(v); };
      waiter = done;
    }),
    onRuntimeInitialized() {
      FS = Module.FS;
      ensureDir(GAME);
      patchFS();
      $('pick-folder').disabled = $('pick-files').disabled = false;
      dbAll().then((stored) => {
        if (stored.length) {
          for (const [k, v] of stored) writeFile(k, v);
          for (const [k] of stored) if (/\.sav$/i.test(k)) { try { const st = FS.stat(GAME + '/' + k); knownSaves.set(k, +st.mtime + ':' + st.size); } catch (e) { /* ignore */ } }
          prepareAudio().then((n) => refreshSetup(`${stored.length} file(s) restored from this browser${n ? `, ${n} voice/music file(s) converted` : ''}.`));
        } else refreshSetup();
      }).catch(() => refreshSetup());
    },
    onExit(code) {
      running = false;
      document.body.classList.remove('running');
      status(`The game exited (code ${code}). Reload the page to play again; your saves are listed below.`);
      refreshSaves();
    },
    onAbort(what) { running = false; status('The game stopped: ' + what); },
  };
  $('pick-folder').addEventListener('change', (e) => importFiles(e.target.files, $('keep').checked));
  $('pick-files').addEventListener('change', (e) => importFiles(e.target.files, $('keep').checked));
  $('forget').addEventListener('click', async () => { await dbClear(); status('Files kept in this browser were removed (the current page still has them until reload).'); });
  $('start').addEventListener('click', () => {
    if (running || !FS) return;
    running = true;
    document.body.classList.add('running');
    $('start').disabled = true;
    status('Running. Click the game to give it the keyboard.');
    FS.chdir(GAME);
    $('game').scrollIntoView();
    canvas.focus();
    try { Module.callMain([]); } catch (e) { if (!(e && e.name === 'ExitStatus')) { out(String(e), true); status('The game stopped: ' + e); } }
  });
})();
