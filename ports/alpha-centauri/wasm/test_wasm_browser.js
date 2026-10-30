#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Browser check of a build_wasm.sh output with Playwright's Chromium.
//
//   node test_wasm_browser.js BUILD_DIR SHOT_DIR [FIXTURE_DIR]
//
// 1. Serves BUILD_DIR on localhost, opens index.html and waits for the
//    program to load and the file prompt to show (no game files given).
// 2. With FIXTURE_DIR (make_fixture.py: synthetic files, no original data),
//    adds those files, presses Start and drives the native frontend: title
//    screen, Alien Crossfire profile (X), New game (N) through its screens,
//    the engine game in the map window, End Turn (E), save (V), quit (Q)
//    back to the title, and exit (Q).
// Fails on any console error or page error. Screenshots go to SHOT_DIR.
'use strict';
const { chromium } = require('playwright');
const http = require('http');
const fs = require('fs');
const path = require('path');

const [buildDir, shotDir, fixtureDir] = process.argv.slice(2);
if (!buildDir || !shotDir) { console.error('usage: test_wasm_browser.js BUILD_DIR SHOT_DIR [FIXTURE_DIR]'); process.exit(2); }
fs.mkdirSync(shotDir, { recursive: true });
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
  page.on('pageerror', (e) => errors.push('pageerror: ' + e.message));
  const results = [];
  const step = (name, detail) => { results.push({ name, detail }); console.log(`ok ${results.length} ${name}${detail ? ': ' + detail : ''}`); };
  const logText = () => page.evaluate(() => Module.alphaLog.join('\n'));
  const waitLog = async (re, timeout = 60000) => {
    await page.waitForFunction((src) => new RegExp(src).test(Module.alphaLog.join('\n')), re.source, { timeout });
    const m = (await logText()).match(re);
    return m ? m[0] : '';
  };
  const shot = async (name) => { const p = path.join(shotDir, name); await page.screenshot({ path: p, fullPage: false }); return p; };
  const canvasShot = async (name) => { const p = path.join(shotDir, name); await page.locator('#screen').screenshot({ path: p }); return p; };
  const key = async (k) => { await page.locator('#screen').focus(); await page.keyboard.press(k); };
  try {
    await page.goto(url);
    await page.waitForFunction(() => document.getElementById('status').textContent.includes('Choose your Alpha Centauri folder'), null, { timeout: 60000 });
    step('page loaded, program initialised, file prompt shown', await page.textContent('#status'));
    step('start disabled without files', String(await page.isDisabled('#start')));
    await shot('01-file-prompt.png');
    if (fixtureDir) {
      const files = fs.readdirSync(fixtureDir).map((f) => path.join(fixtureDir, f));
      await page.setInputFiles('#pick-files', files);
      await page.waitForFunction(() => document.getElementById('status').textContent.includes('file(s) added'), null, { timeout: 60000 });
      step('files added', await page.textContent('#status'));
      await page.click('#start');
      step('title screen', await waitLog(/FRONTEND FONTS[^\n]*/));
      await waitLog(/FRONTEND variant=base[^\n]*/);
      await page.waitForTimeout(300);
      await canvasShot('02-title.png');
      await key('x');
      step('Alien Crossfire profile', await waitLog(/FRONTEND variant=expansion[^\n]*/));
      await key('n');
      step('new game screens', await waitLog(/FRONTEND NEWGAME open/));
      await page.waitForTimeout(300);
      await canvasShot('03-new-game.png');
      for (let i = 0; i < 12; ++i) {
        if (/FRONTEND NEWGAME start=/.test(await logText())) break;
        await key('Enter');
        await page.waitForTimeout(250);
      }
      step('new game started', await waitLog(/FRONTEND NEWGAME start=\w+/));
      step('map window drawn', await waitLog(/DRAW selected=[^\n]*/, 120000));
      await page.waitForTimeout(500);
      await canvasShot('04-map.png');
      const before = (await logText()).split('\n').length;
      await key('e');
      await page.waitForFunction((n) => Module.alphaLog.length > n && /DRAW selected=/.test(Module.alphaLog.slice(n).join('\n')), before, { timeout: 180000 });
      step('End Turn', (await logText()).split('\n').slice(before).filter((l) => !l.startsWith('DRAW')).slice(0, 3).join(' | '));
      await page.waitForTimeout(500);
      await canvasShot('05-after-end-turn.png');
      await key('v');
      await page.waitForFunction(() => /AlphaPlay-\d+\.SAV/.test(document.getElementById('saves').textContent), null, { timeout: 30000 });
      step('save written', await page.textContent('#saves'));
      await key('q');
      await page.waitForTimeout(300);
      if (!/FRONTEND MAP RC/.test(await logText())) await key('q'); // first Q may warn about unsaved turns
      step('back to title', await waitLog(/FRONTEND MAP RC \d+[^\n]*/));
      // Play a saved game: the Amiga file requester is a page dialog.
      const draws = (await logText()).match(/DRAW selected=/g).length;
      await key('p');
      await page.waitForSelector('#requester[open]', { timeout: 30000 });
      await shot('06-requester.png');
      await page.click('#requester-list button:text-matches("AlphaPlay-001\\\\.SAV", "i")');
      await page.waitForFunction((n) => (Module.alphaLog.join('\n').match(/DRAW selected=/g) || []).length > n, draws, { timeout: 120000 });
      step('saved game reopened', (await logText()).split('\n').filter((l) => /Turn \d+, mission year/.test(l) || /^PLAY /.test(l)).slice(-1)[0] || 'map drawn');
      await page.waitForTimeout(500);
      await canvasShot('07-saved-game.png');
      await key('q');
      await page.waitForFunction(() => (Module.alphaLog.join('\n').match(/FRONTEND MAP RC/g) || []).length >= 2, null, { timeout: 30000 });
      step('back to title again', 'FRONTEND MAP RC');
      await key('q');
      step('exit', await waitLog(/FRONTEND EXIT RC \d+/));
      await shot('08-exited.png');
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
  console.log(`PASS ${results.length} steps, no console errors (${browser.version ? 'chromium' : ''})`);
})();
