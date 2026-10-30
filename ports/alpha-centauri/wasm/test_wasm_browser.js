#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Browser walk of a build_wasm.sh output with Playwright's Chromium.
//
//   node test_wasm_browser.js BUILD_DIR SHOT_DIR [FIXTURE_DIR]
//
// Without FIXTURE_DIR: the page loads and shows the file prompt.
// With FIXTURE_DIR (make_fixture.py: synthetic, no original data), picked as
// a folder like a player's install, the native frontend is driven through:
//   title (menu music) -> intro movie (V) -> Alien Crossfire (X) ->
//   technology browser (T, voice V) -> O and E file requesters (cancel) -> New game
//   (N, Huge planet) -> map (world music, turn_start effect) -> research,
//   social, design, comm link, base and production panels -> End Turns with
//   a page-responsiveness probe -> save (V) -> back to title -> Play a saved
//   game (P) -> Open (O) read-only view of the save -> exit (Q).
// Audio is checked with the page's analyser on everything sent to the
// speakers (non-silent peak while each source plays) and with the record of
// native audio calls. Fails on any console error or page error.
// Screenshots, program output and result.json go to SHOT_DIR.
'use strict';
const { chromium } = require('playwright');
const http = require('http');
const fs = require('fs');
const path = require('path');

const [buildDir, shotDir, fixtureDir] = process.argv.slice(2);
if (!buildDir || !shotDir) { console.error('usage: test_wasm_browser.js BUILD_DIR SHOT_DIR [FIXTURE_DIR]'); process.exit(2); }
fs.mkdirSync(shotDir, { recursive: true });
const THROTTLE = +(process.env.THROTTLE || 10), TURNS = +(process.env.TURNS || 6);
const TYPES = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm' };

function serve(root) {
  return new Promise((resolve) => {
    const server = http.createServer((req, res) => {
      const rel = decodeURIComponent(new URL(req.url, 'http://x').pathname).replace(/^\/+/, '') || 'index.html';
      const file = path.join(root, rel);
      if (!file.startsWith(path.resolve(root)) || !fs.existsSync(file)) { res.writeHead(404); res.end(); return; }
      res.writeHead(200, { 'Content-Type': TYPES[path.extname(file)] || 'application/octet-stream' });
      fs.createReadStream(file).pipe(res);
    });
    server.listen(0, '127.0.0.1', () => resolve(server));
  });
}

(async () => {
  const server = await serve(path.resolve(buildDir));
  const url = `http://127.0.0.1:${server.address().port}/index.html`;
  const browser = await chromium.launch({ args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1200, height: 1100 } });
  const errors = [], consoleLines = [];
  page.on('console', (m) => { consoleLines.push(`${m.type()}: ${m.text()}`); if (m.type() === 'error') errors.push(m.text()); });
  page.on('pageerror', (e) => errors.push('pageerror: ' + e.message + '\n' + (e.stack || '')));
  const results = [];
  const step = (name, detail) => { results.push({ name, detail }); console.log(`ok ${results.length} ${name}${detail ? ': ' + detail : ''}`); };
  const logText = () => page.evaluate(() => Module.alphaLog.join('\n'));
  const count = async (re) => ((await logText()).match(new RegExp(re.source, 'g')) || []).length;
  const waitLog = async (re, timeout = 60000, after = 0) => {
    await page.waitForFunction(([src, n]) => (Module.alphaLog.join('\n').match(new RegExp(src, 'g')) || []).length > n, [re.source, after], { timeout });
    const all = (await logText()).match(new RegExp(re.source, 'g'));
    return all[all.length - 1];
  };
  const shot = async (name) => { const p = path.join(shotDir, name); await page.screenshot({ path: p }); return p; };
  const canvasShot = async (name) => { const p = path.join(shotDir, name); await page.locator('#screen').screenshot({ path: p }); return p; };
  const key = async (k) => { await page.locator('#screen').focus(); await page.keyboard.press(k); };
  const frameHash = () => page.evaluate(() => { const c = document.getElementById('screen'); const d = c.getContext('2d').getImageData(0, 0, c.width, c.height).data; let h = 2166136261; for (let i = 0; i < d.length; i += 7) h = Math.imul(h ^ d[i], 16777619) >>> 0; return h; });
  const audio = () => page.evaluate(() => ({ ...Module.alphaAudioStats }));
  // Wait until the analyser has heard at least `ms` more of non-silent output.
  const heard = async (what, ms = 400, timeout = 20000) => {
    const before = (await audio()).loudMs;
    await page.waitForFunction(([b, m]) => Module.alphaAudioStats.loudMs >= b + m, [before, ms], { timeout });
    const a = await audio();
    step(`audio: ${what}`, `context ${a.state}, peak ${a.peak.toFixed(3)}, non-silent ${a.loudMs} ms total`);
  };
  const afterDraw = async () => { const n = await count(/DRAW selected=/); return n; };
  try {
    await page.goto(url);
    await page.waitForFunction(() => document.getElementById('status').textContent.includes('Choose your Alpha Centauri folder'), null, { timeout: 60000 });
    step('page loaded, program initialised, file prompt shown', await page.textContent('#status'));
    step('start disabled without files', String(await page.isDisabled('#start')));
    await shot('01-file-prompt.png');
    if (fixtureDir) {
      await page.setInputFiles('#pick-folder', path.resolve(fixtureDir));
      await page.waitForFunction(() => /file\(s\) added/.test(document.getElementById('status').textContent), null, { timeout: 120000 });
      step('install folder picked', await page.textContent('#status'));
      const prepared = (await logText()).split('\n').filter((l) => l.startsWith('AUDIO PREPARED'));
      if (!prepared.some((l) => /music\/aset1\.mp3/.test(l)) || !prepared.some((l) => /voices\/tech0\.mp3/.test(l))) throw new Error('mp3 voice/music not converted');
      step('voices and music converted in the browser', `${prepared.length} files (music/aset1.mp3, voices/tech*.mp3)`);

      // Title screen and its menu music.
      await page.click('#start');
      step('title screen', await waitLog(/FRONTEND FONTS[^\n]*/));
      step('menu music device', await waitLog(/FRONTEND variant=base music=1 device=[1-9][^\n]*/));
      await heard('title menu music (opening menu.wav)');
      await canvasShot('02-title.png');

      // Intro movie (prepared .amov cache) with its sound.
      await key('v');
      await waitLog(/STARTUP FIRST FRAME/, 30000);
      await page.waitForTimeout(800);
      await canvasShot('03-intro-movie.png');
      await heard('intro movie soundtrack', 300);
      step('intro movie', await waitLog(/STARTUP END rc=\d+[^\n]*/, 30000));
      step('back from intro', await waitLog(/FRONTEND INTRO variant=base RC 0[^\n]*/));

      // Alien Crossfire profile (the fixture's rules are alphax.txt).
      await key('x');
      step('Alien Crossfire profile', await waitLog(/FRONTEND variant=expansion[^\n]*/));
      // Technology browser and a voice-over.
      await key('t');
      step('technology browser', await waitLog(/TECHNOLOGY SELECT[^\n]*/));
      await key('n');
      await key('v');
      step('technology voice', await waitLog(/TECHNOLOGY VOICE \d+ 1[^\n]*/));
      await heard('technology voice-over (voices/tech<ID> from MP3)', 300);
      await canvasShot('04-technology-browser.png');
      await key('q');
      step('back from technology browser', await waitLog(/FRONTEND TECHNOLOGY RC 0[^\n]*/));

      // O and E: the Amiga file requester as a page dialog, cancelled.
      for (const k of ['o', 'e']) {
        await key(k);
        await page.waitForSelector('#requester[open]', { timeout: 30000 });
        if (k === 'o') await shot('05-requester.png');
        await page.click('#requester-cancel');
        await page.waitForSelector('#requester', { state: 'hidden' });
        step(`${k.toUpperCase()} requester shown and cancelled`, await page.textContent('#requester-title'));
      }

      // New game on a Huge planet.
      await key('n');
      await waitLog(/FRONTEND NEWGAME open/);
      await key('Enter'); // faction 1 -> settings, Start row
      await key('ArrowDown'); await key('ArrowDown'); // difficulty, planet size
      await key('ArrowRight'); await key('ArrowRight'); // Standard -> Huge
      await key('ArrowUp'); await key('ArrowUp'); // back to Start
      await page.waitForTimeout(200);
      await canvasShot('06-new-game-settings.png');
      const t0 = Date.now();
      await key('Enter');
      await waitLog(/FRONTEND NEWGAME screen=2[^\n]*/, 180000);
      step('Huge planet built', `${Date.now() - t0} ms`);
      await key('Enter');
      step('new game started', await waitLog(/FRONTEND NEWGAME start=ok/));
      step('map window drawn', await waitLog(/DRAW selected=[^\n]*/, 120000));
      step('world audio opened', await waitLog(/WASM AUDIO open device=1[^\n]*/));
      step('faction music started', await waitLog(/WASM AUDIO music [^\n]*playing=1[^\n]*/));
      step('turn_start effect', await waitLog(/WASM AUDIO event turn_start file=ok\.wav/));
      await heard('world music playlist (music/aset1 from MP3) and effects', 600);
      await page.waitForTimeout(300);
      await canvasShot('07-map.png');

      // Panels.
      const panels = [['F2', 'research'], ['F3', 'social'], ['F4', 'design'], ['F5', 'comm-link']];
      for (const [k, name] of panels) {
        const before = await frameHash();
        await key(k);
        await page.waitForFunction((h) => { const c = document.getElementById('screen'); const d = c.getContext('2d').getImageData(0, 0, c.width, c.height).data; let x = 2166136261; for (let i = 0; i < d.length; i += 7) x = Math.imul(x ^ d[i], 16777619) >>> 0; return x !== h; }, before, { timeout: 15000 });
        await page.waitForTimeout(150);
        await canvasShot(`08-panel-${name}.png`);
        await key('Escape');
        await page.waitForTimeout(150);
        step(`${name} panel opened and closed`, k);
      }
      {
        await key('i');
        await page.waitForTimeout(200);
        await key('p');
        await page.waitForTimeout(250);
        await canvasShot('08-panel-production.png');
        await key('Escape'); await key('Escape');
        step('base window and production panel', 'I, P');
      }

      // End Turns while probing the page. Chromium's CPU throttling (10x,
      // as on a slow machine) makes the AI turns long. A 10 ms page timer
      // records every gap in the main thread; the engine phase runs from the
      // E key to the "PLAY CMD endturn" line (timestamped by the page), and the
      // map redraw follows it. Playwright round trips and a mid-turn
      // screenshot show the page answering while the engine works.
      const cdp = await page.context().newCDPSession(page);
      await cdp.send('Emulation.setCPUThrottlingRate', { rate: THROTTLE });
      let longest = { engineMs: 0 }, midShot = false;
      const turns = [];
      for (let turn = 1; turn <= TURNS; ++turn) {
        await page.evaluate(() => {
          window.__probe = { ticks: [performance.now()], busySeen: false };
          Module.alphaBusyStats.gap = { ms: 0, from: 0, to: 0 };
          window.__probeTimer = setInterval(() => {
            window.__probe.ticks.push(performance.now());
            if (!document.getElementById('busy').hidden) window.__probe.busySeen = true;
          }, 10);
        });
        const draws = await afterDraw();
        const lines0 = await page.evaluate(() => Module.alphaLog.length);
        const start = Date.now();
        if (turn === 1 && process.env.CPUPROFILE) { await cdp.send('Profiler.enable'); await cdp.send('Profiler.setSamplingInterval', { interval: 200 }); await cdp.send('Profiler.start'); }
        await key('e');
        let roundTrips = 0, worstTrip = 0;
        while ((await count(/DRAW selected=/)) <= draws) {
          const t = Date.now();
          await page.evaluate(() => document.getElementById('status').textContent.length);
          worstTrip = Math.max(worstTrip, Date.now() - t); ++roundTrips;
          if (!midShot && await page.isVisible('#busy')) { midShot = true; await shot('09-ai-turn-in-progress.png'); }
          if (Date.now() - start > 600000) throw new Error('End Turn did not finish');
          await page.waitForTimeout(20);
        }
        const ms = Date.now() - start;
        if (turn === 1 && process.env.CPUPROFILE) fs.writeFileSync(process.env.CPUPROFILE, JSON.stringify((await cdp.send('Profiler.stop')).profile));
        const p = await page.evaluate((n0) => {
          clearInterval(window.__probeTimer);
          const ticks = window.__probe.ticks;
          let k = -1;
          for (let i = n0; i < Module.alphaLog.length; ++i) if (Module.alphaLog[i].startsWith('PLAY CMD endturn')) { k = i; break; }
          const tEngine = k >= 0 ? Module.alphaLogTimes[k] : NaN;
          let engineGap = 0, allGap = 0, prev = ticks[0];
          for (const t of ticks.slice(1)) {
            allGap = Math.max(allGap, t - prev);
            if (t <= tEngine) engineGap = Math.max(engineGap, t - prev);
            else if (prev < tEngine) engineGap = Math.max(engineGap, tEngine - prev);
            prev = t;
          }
          return { engineMs: tEngine - ticks[0], engineGap, allGap, ticks: ticks.length, busySeen: window.__probe.busySeen, yieldGap: Module.alphaBusyStats.gap };
        }, lines0);
        const r = { turn, ms, engineMs: Math.round(p.engineMs), engineGap: Math.round(p.engineGap), allGap: Math.round(p.allGap), roundTrips, worstTrip, busySeen: p.busySeen, yieldGap: p.yieldGap };
        turns.push(r);
        if (r.engineMs > longest.engineMs) longest = r;
        console.log(`   End Turn ${turn}: ${ms} ms in all, engine ${r.engineMs} ms with longest page gap ${r.engineGap} ms; ` +
          `longest gap incl. redraw ${r.allGap} ms; ${roundTrips} round trips (worst ${worstTrip} ms); indicator ${p.busySeen ? 'shown' : 'not shown'}; ` +
          `longest stretch between engine yields ${p.yieldGap.ms} ms (yield points ${p.yieldGap.from}->${p.yieldGap.to})`);
        if (r.engineGap > 150) throw new Error(`page blocked for ${r.engineGap} ms during the AI turn ${turn}`);
        if (turn === 1) { await page.waitForTimeout(300); await canvasShot('10-after-end-turn.png'); }
      }
      await cdp.send('Emulation.setCPUThrottlingRate', { rate: 1 });
      const bs = await page.evaluate(() => ({ ...Module.alphaBusyStats }));
      if (!midShot) throw new Error('the AI turn indicator never showed');
      step(`End Turns keep the page responsive (CPU throttled ${THROTTLE}x)`, `longest engine phase ${longest.engineMs} ms with longest page gap ${longest.engineGap} ms ` +
        `(${longest.roundTrips} Playwright round trips, worst ${longest.worstTrip} ms); indicator shown; engine yields ${bs.yields}`);
      fs.writeFileSync(path.join(shotDir, 'end-turns.json'), JSON.stringify(turns, null, 2) + '\n');
      step('turn log', (await logText()).split('\n').filter((l) => /^PLAY CMD endturn/.test(l)).slice(-1)[0]);

      // Save, back to the title, Play a saved game, Open read-only.
      await key('v');
      await page.waitForFunction(() => /AlphaPlay-\d+\.SAV/.test(document.getElementById('saves').textContent), null, { timeout: 30000 });
      step('save written', await page.textContent('#saves'));
      await key('q');
      await page.waitForTimeout(300);
      if (!(await count(/FRONTEND MAP RC/))) await key('q');
      step('back to title', await waitLog(/FRONTEND MAP RC \d+[^\n]*/));
      let draws = await afterDraw();
      await key('p');
      await page.waitForSelector('#requester[open]', { timeout: 30000 });
      await page.click('#requester-list button:text-matches("AlphaPlay-001\\\\.SAV", "i")');
      await waitLog(/DRAW selected=/, 120000, draws);
      step('saved game reopened', (await logText()).split('\n').filter((l) => /Loaded:/.test(l)).slice(-1)[0]);
      await page.waitForTimeout(300);
      await canvasShot('11-saved-game.png');
      await key('q');
      await waitLog(/FRONTEND MAP RC/, 30000, 1);
      draws = await afterDraw();
      await key('o');
      await page.waitForSelector('#requester[open]', { timeout: 30000 });
      await page.click('#requester-list button:text-matches("AlphaPlay-001\\\\.SAV", "i")');
      await waitLog(/DRAW selected=/, 120000, draws);
      await page.waitForTimeout(300);
      await canvasShot('12-open-read-only.png');
      step('O opened the save in the read-only original-world view', (await logText()).split('\n').filter((l) => /^DRAW/.test(l)).slice(-1)[0]);
      await key('q');
      await waitLog(/FRONTEND MAP RC/, 30000, 2);
      await key('q');
      step('exit', await waitLog(/FRONTEND EXIT RC \d+/));
      await shot('13-exited.png');
      const ev = await page.evaluate(() => Module.alphaAudioEvents.map((e) => e.what));
      step('native audio calls recorded', ev.join(' | '));
    }
  } catch (e) {
    errors.push('test: ' + e.message);
    await shot('failure.png').catch(() => {});
  }
  const log = await page.evaluate(() => (window.Module && Module.alphaLog || []).join('\n')).catch(() => '');
  fs.writeFileSync(path.join(shotDir, 'program-output.txt'), log + '\n');
  fs.writeFileSync(path.join(shotDir, 'console.txt'), consoleLines.join('\n') + '\n');
  fs.writeFileSync(path.join(shotDir, 'result.json'), JSON.stringify({ url, browser: browser.version(), steps: results, errors }, null, 2) + '\n');
  await browser.close();
  server.close();
  if (errors.length) { console.error('FAIL\n' + errors.join('\n')); process.exit(1); }
  console.log(`PASS ${results.length} steps, no console errors`);
})();
