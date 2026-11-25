#!/usr/bin/env node
/* Test senza interfaccia: esegue il gioco nel motore JS simulando la tastiera
 * e stampa lo schermo come testo.
 *   node tools/selftest.js <gioco.tap> [tasti...]
 * Ogni argomento tasto e' un carattere o un nome: ENTER, F1, SPACE; "wait:N" esegue N unita'.
 */
'use strict';
const fs = require('fs');
const path = require('path');
const dir = path.join(__dirname, '..', 'web', 'js');
globalThis.performance = globalThis.performance || require('perf_hooks').performance;
for (const f of ['core/petscii.js', 'core/tap.js', 'core/pcode.js', 'core/basic.js', 'engine/font.js', 'engine/screen.js', 'engine/interp.js'])
  require(path.join(dir, f));
const AV = globalThis.AV;

const g = AV.tap.loadGameFromTap(new Uint8Array(fs.readFileSync(process.argv[2])));
const text = AV.basic.render(AV.pcode.decompile(g.mem));
const prog = AV.basic.parse(text);
const scr = new AV.screen.Screen();
let saved = null;
const host = {
  screen: scr,
  tapeWrite: (name, t) => { saved = { name, text: t }; return Promise.resolve(); },
  tapeRead: () => Promise.resolve(saved),
  readLine: () => Promise.resolve(''),
  onEnd: (r, e) => console.log('FINE:', r, e || ''),
};
const m = new AV.interp.Machine(prog, host);

function dump() {
  const rows = [];
  for (let y = 0; y < 25; y++) {
    let s = '';
    for (let x = 0; x < 40; x++) {
      const c = scr.chars[y * 40 + x] & 0x7F;
      if (c >= 1 && c <= 26) s += String.fromCharCode(96 + c);
      else if (c >= 0x41 && c <= 0x5A) s += String.fromCharCode(c);
      else if (c >= 0x20 && c < 0x40) s += String.fromCharCode(c);
      else if (c === 0) s += '@';
      else s += '#';
    }
    rows.push('|' + s + '|');
  }
  console.log(rows.join('\n'));
}

const KEYS = { ENTER: 13, F1: 133, SPACE: 32 };
async function main() {
  const steps = process.argv.slice(3);
  const runFor = async n => {
    let left = n;
    while (left > 0 && !m.halted) {
      const r = m.run(Math.min(left, 500));
      left -= 500;
      if (r === 'wait') await new Promise(res => setTimeout(res, 0));
    }
  };
  await runFor(20000);
  for (const k of steps) {
    if (k.startsWith('wait:')) { await runFor(+k.slice(5)); continue; }
    const code = KEYS[k] !== undefined ? KEYS[k] : k.charCodeAt(0);
    m.pushKey(code);
    await runFor(20000);
  }
  dump();
  console.log('pc riga:', m.code[m.pc] && m.code[m.pc].line, ' salvataggio:', saved && JSON.stringify(saved));
}
main().catch(e => { console.error(e); process.exit(1); });
