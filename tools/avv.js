#!/usr/bin/env node
/* CLI per decompilare, verificare e ricompilare Avventura 1 usando lo stesso core del launcher.
 *
 *   node tools/avv.js decompile <gioco.tap> <out.bas>      P-code -> listato BASIC
 *   node tools/avv.js verify    <gioco.tap>                decompila, riparsa, ricompila e confronta
 *   node tools/avv.js compile   <gioco.tap> <in.bas> <out.prg>   BASIC (anche modificato) -> PRG per C64
 *
 * Il .tap e' sempre la copia dell'utente: serve per il runtime originale.
 */
'use strict';
const fs = require('fs');
const path = require('path');
const core = path.join(__dirname, '..', 'web', 'js', 'core');
for (const f of ['petscii.js', 'tap.js', 'pcode.js', 'basic.js', 'compile.js']) require(path.join(core, f));
const AV = globalThis.AV;

function load(tapPath) {
  const g = AV.tap.loadGameFromTap(new Uint8Array(fs.readFileSync(tapPath)));
  return g;
}

const HEADER = [
  '0 REM AVVENTURA 1 (FANTASY) - LISTATO DECOMPILATO DAL P-CODE',
  '1 REM GENERATO DALLA COPIA DELL\'UTENTE - NON DISTRIBUIRE',
];

function decompileText(g) {
  const ast = AV.pcode.decompile(g.mem);
  return { ast, text: AV.basic.render(ast) };
}

function cmd() {
  const [, , c, ...a] = process.argv;
  if (c === 'decompile' && a.length === 2) {
    const g = load(a[0]);
    const { ast, text } = decompileText(g);
    fs.writeFileSync(a[1], text);
    console.log(`scritto ${a[1]}: ${ast.lines.length} righe, ${ast.data.length} voci DATA`);
    return;
  }
  if (c === 'verify' && a.length === 1) {
    const g = load(a[0]);
    const { text } = decompileText(g);
    const ast2 = AV.basic.parse(text);
    const text2 = AV.basic.render(ast2);
    if (text2 !== text) {
      const l1 = text.split('\n'), l2 = text2.split('\n');
      const k = l1.findIndex((l, i) => l !== l2[i]);
      console.log('ERRORE: render(parse(testo)) diverso dal testo alla riga', k);
      console.log(' originale:', l1[k]); console.log(' riparsato:', l2[k]);
      process.exit(1);
    }
    const out = AV.compile.compile(ast2, g.mem.subarray(0x0801));
    const orig = g.mem.subarray(0x0801, g.end);
    let diff = -1;
    const n = Math.max(orig.length, out.bytes.length);
    for (let i = 0; i < n; i++) if (orig[i] !== out.bytes[i]) { diff = i; break; }
    console.log(`testo: ${text.split('\n').length - 1} righe; P-code ricompilato: ${out.sizes.pcode} byte, ${out.sizes.vars} variabili`);
    if (diff < 0 && orig.length === out.bytes.length) {
      console.log('VERIFICA OK: TAP -> BASIC -> P-code identico all\'originale (' + orig.length + ' byte)');
      return;
    }
    console.log(`VERIFICA FALLITA: primo byte diverso a $${(0x0801 + diff).toString(16)} (lunghezze ${orig.length}/${out.bytes.length})`);
    for (let i = Math.max(0, diff - 4); i < diff + 12; i++)
      console.log(`  $${(0x0801 + i).toString(16)}  orig ${(orig[i] ?? -1).toString(16).padStart(2, '0')}  nuovo ${(out.bytes[i] ?? -1).toString(16).padStart(2, '0')}`);
    process.exit(1);
  }
  if (c === 'compile' && a.length === 3) {
    const g = load(a[0]);
    const ast = AV.basic.parse(fs.readFileSync(a[1], 'utf8'));
    const out = AV.compile.compile(ast, g.mem.subarray(0x0801));
    fs.writeFileSync(a[2], AV.compile.toPrg(out));
    console.log(`scritto ${a[2]}: $0801-$${(0x0801 + out.bytes.length - 1).toString(16)}, avvio con RUN`);
    return;
  }
  console.log(fs.readFileSync(__filename, 'utf8').split('\n').slice(1, 9).join('\n'));
  process.exit(1);
}

try { cmd(); } catch (e) { console.error('ERRORE:', e.message); process.exit(1); }
