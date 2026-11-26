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
  if (c === 'mod' && a.length === 3) return buildMod(a[0], a[1], a[2]);
  console.log(fs.readFileSync(__filename, 'utf8').split('\n').slice(1, 9).join('\n'));
  process.exit(1);
}

// ------------------------------------------------------------------ mod
// Una mod e' una cartella di file .bas scritti da noi, con direttive:
//   !NAME testo                 nome mostrato nel launcher
//   !HOOK riga GOSUB rigamod    prima di eseguire la riga originale chiama la routine della mod
//   !HOOK riga GOTO rigamod     sostituisce la riga originale saltando alla mod
// Il risultato (.avm) contiene solo codice nostro compilato + indirizzi di aggancio:
// il gioco originale resta intatto e viene verificato con un'impronta al caricamento.
function fnv1a(bytes) {
  let h = 0x811C9DC5;
  for (const b of bytes) { h ^= b; h = Math.imul(h, 0x01000193) >>> 0; }
  return h >>> 0;
}

function buildMod(tapPath, dir, outPath) {
  const g = load(tapPath);
  const ast = AV.pcode.decompile(g.mem);
  const files = fs.readdirSync(dir).filter(f => f.toLowerCase().endsWith('.bas')).sort();
  let src = '', name = path.basename(dir);
  const hooks = [];
  for (const f of files) {
    for (const raw of fs.readFileSync(path.join(dir, f), 'utf8').split(/\r?\n/)) {
      const line = raw.trim();
      if (line.startsWith('!')) {
        let m;
        if ((m = /^!NAME\s+(.+)$/i.exec(line))) name = m[1];
        else if ((m = /^!HOOK\s+(\d+)\s+(GOSUB|GOTO)\s+(\d+)$/i.exec(line))) hooks.push({ line: +m[1], kind: m[2].toLowerCase(), target: +m[3], file: f });
        else throw new Error(`${f}: direttiva sconosciuta: ${line}`);
        continue;
      }
      if (line.startsWith("'") || line === '') continue;     // commenti
      src += raw + '\n';
    }
  }
  const prog = AV.basic.parse(src);
  for (const l of prog.lines) if (ast.lineAddr.has(l.n)) throw new Error(`la riga ${l.n} esiste gia' nel gioco: usa le righe libere (4670-5990, 6150+)`);
  const origVar = new Map(ast.vars.map((n, i) => [n, i]));
  const newVars = [];
  const origArr = new Map();
  let maxSlot = 0;
  for (const k in ast.arrays) { const ar = ast.arrays[k]; if (ar.name) origArr.set(ar.name, { n: +k, nd: ar.nd }); maxSlot = Math.max(maxSlot, +k); }
  const newArr = new Map();
  const end = ast.hdr.arytab;
  const base = (end + 0xFF) & ~0xFF;
  const env = {
    base,
    varIndex(n) {
      if (origVar.has(n)) return origVar.get(n);
      let i = newVars.indexOf(n);
      if (i < 0) {
        if (!/^[MU]/.test(n)) console.log(`attenzione: variabile nuova ${n} non usa i prefissi M/U riservati alle mod`);
        newVars.push(n); i = newVars.length - 1;
      }
      return ast.vars.length + i;
    },
    arrInfo(n, nd) {
      if (origArr.has(n)) return origArr.get(n);
      if (!newArr.has(n)) newArr.set(n, { n: maxSlot + 1 + newArr.size, nd });
      return newArr.get(n);
    },
    lineAddr: n => ast.lineAddr.get(n),
  };
  const out = AV.compile.compile(prog, null, env);
  if (base + out.code.length > 0xFFF0) throw new Error('mod troppo grande per i 64 KB della VM');
  const lines = ['AVVMOD 1', `name ${name}`, `game ${fnv1a(g.mem.subarray(0x0801, g.end)).toString(16)}`, `base ${base}`];
  for (const v of newVars) lines.push(`var ${v}`);
  for (const h of hooks) {
    const at = ast.lineAddr.get(h.line), to = out.lines.get(h.target);
    if (at === undefined) throw new Error(`${h.file}: !HOOK su riga ${h.line} inesistente nel gioco`);
    if (to === undefined) throw new Error(`${h.file}: !HOOK verso riga ${h.target} inesistente nella mod`);
    lines.push(`hook ${at} ${h.kind} ${to}`);
  }
  lines.push(`code ${Buffer.from(out.code).toString('hex')}`);
  fs.writeFileSync(outPath, lines.join('\n') + '\n');
  console.log(`mod "${name}": ${out.code.length} byte di P-code a $${base.toString(16)}, ${newVars.length} variabili nuove, ${hooks.length} agganci -> ${outPath}`);
}

try { cmd(); } catch (e) { console.error('ERRORE:', e.message); process.exit(1); }
