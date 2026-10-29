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
  // Large media the Amiga build does not read (it uses prepared .amov caches).
  const SKIP = /\.(wve|avi|exe|dll|ico|lnk|url)$/i;
  const REQUIRED = ['openinga.pcx', 'ALPHC___.TTF', 'arialn.ttf'];
  const WANTED = ['alpha.txt', 'xopeninga.pcx', 'alphax.txt', 'texture.pcx', 'Units.pcx', 'console.pcx', 'Arial.ttf', 'opening menu.wav'];
  const DB_NAME = 'alpha-centauri-wasm', DB_STORE = 'files';

  const log = [];
  let logDirty = false;
  function out(line, isErr) {
    log.push((isErr ? '! ' : '') + line);
    if (log.length > 4000) log.splice(0, log.length - 4000);
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
      if (SKIP.test(rel) || f.size > 96 * 1024 * 1024) { ++skipped; continue; }
      const bytes = new Uint8Array(await f.arrayBuffer());
      writeFile(rel, bytes);
      if (keep) entries.push([rel, bytes]);
      if (++done % 50 === 0) status(`Reading your files... ${done}/${files.length}`);
    }
    imported += done;
    if (keep && entries.length) { try { await dbPut(entries); } catch (e) { out('Could not keep files in this browser: ' + e, true); } }
    refreshSetup(`${done} file(s) added${skipped ? `, ${skipped} skipped (movies/programs not used here)` : ''}.`);
  }
  function refreshSetup(note) {
    const missing = REQUIRED.filter((n) => !exists(n));
    const optional = WANTED.filter((n) => !exists(n));
    const all = FS ? listFiles(GAME, '') : [];
    $('file-count').textContent = String(all.length);
    $('missing').textContent = missing.length ? 'Missing (the title screen needs these): ' + missing.join(', ') : 'Title screen files found.';
    $('optional').textContent = optional.length ? 'Not found (needed later for the map, Crossfire or sound): ' + optional.join(', ') : '';
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

  // ---- the emscripten module ----
  window.Module = {
    canvas,
    noInitialRun: true,
    print: (t) => out(t, false),
    printErr: (t) => out(t, true),
    alphaEvents: events,
    alphaLog: log,
    alphaPresent: present,
    alphaAskFile: askFile,
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
          refreshSetup(`${stored.length} file(s) restored from this browser.`);
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
    canvas.focus();
    try { Module.callMain([]); } catch (e) { if (!(e && e.name === 'ExitStatus')) { out(String(e), true); status('The game stopped: ' + e); } }
  });
})();
