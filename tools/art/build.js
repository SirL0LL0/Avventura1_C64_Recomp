// Genera le illustrazioni dell'Espansione.
//   node tools/art/build.js [anteprima.png] [solo,questi,nomi]
// Scrive native/src/art_data.h (pixel a colori indicizzati, un esadecimale per pixel)
// e, se richiesto, un foglio d'anteprima PNG ingrandito x3.
"use strict";
const fs = require("fs");
const path = require("path");
const zlib = require("zlib");
const { PALETTE } = require("./engine");
const PICS = require("./pictures");

const out = path.join(__dirname, "../../native/src/art_data.h");
const preview = process.argv[2];
const only = process.argv[3] ? process.argv[3].split(",") : null;

const list = PICS.filter((p) => !only || only.includes(p.name));
const imgs = list.map((p) => {
  const img = p.draw();
  for (let i = 0; i < img.p.length; i++) if (img.p[i] < 0) img.p[i] = p.bg || 0;
  return img;
});

if (!only) {
  let h = "// Generato da tools/art/build.js: non modificare a mano.\n#pragma once\n#include <cstdint>\n\n";
  h += "struct ArtDef { const char* name; const char* keys; const char* title; int w, h; const char* px; };\n\n";
  h += "static const ArtDef ART_DEFS[] = {\n";
  list.forEach((p, k) => {
    const img = imgs[k];
    let hex = "";
    for (const v of img.p) hex += v.toString(16);
    h += `  {"${p.name}", "${p.keys}", "${p.title.replace(/"/g, '\\"')}", ${img.w}, ${img.h},\n`;
    for (let i = 0; i < hex.length; i += 128) h += `   "${hex.slice(i, i + 128)}"\n`;
    h += "  },\n";
  });
  h += "};\nstatic const int ART_COUNT = sizeof(ART_DEFS) / sizeof(ART_DEFS[0]);\n";
  fs.writeFileSync(out, h);
  console.log(`${list.length} immagini -> ${out} (${(h.length / 1024).toFixed(0)} KB)`);
}

if (preview) {
  const SC = 3, COLS = Math.min(6, imgs.length), cell = 128 * SC + 8;
  const rows = Math.ceil(imgs.length / COLS);
  const W = COLS * cell, H = rows * cell;
  const raw = Buffer.alloc((W * 3 + 1) * H, 0x20);
  for (let y = 0; y < H; y++) raw[y * (W * 3 + 1)] = 0;
  imgs.forEach((img, k) => {
    const ox = (k % COLS) * cell + 4, oy = Math.floor(k / COLS) * cell + 4;
    const sc = Math.floor(128 * SC / Math.max(img.w, img.h));
    for (let y = 0; y < img.h * sc; y++) for (let x = 0; x < img.w * sc; x++) {
      const c = PALETTE[img.p[Math.floor(y / sc) * img.w + Math.floor(x / sc)]];
      const o = (oy + y) * (W * 3 + 1) + 1 + (ox + x) * 3;
      raw[o] = c >> 16; raw[o + 1] = (c >> 8) & 255; raw[o + 2] = c & 255;
    }
  });
  const crcT = []; for (let n = 0; n < 256; n++) { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; crcT[n] = c >>> 0; }
  const crc = (b) => { let c = 0xffffffff; for (const x of b) c = crcT[(c ^ x) & 255] ^ (c >>> 8); return (c ^ 0xffffffff) >>> 0; };
  const chunk = (t, d) => { const l = Buffer.alloc(4); l.writeUInt32BE(d.length); const td = Buffer.concat([Buffer.from(t), d]); const c = Buffer.alloc(4); c.writeUInt32BE(crc(td)); return Buffer.concat([l, td, c]); };
  const ihdr = Buffer.alloc(13); ihdr.writeUInt32BE(W, 0); ihdr.writeUInt32BE(H, 4); ihdr[8] = 8; ihdr[9] = 2;
  fs.writeFileSync(preview, Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk("IHDR", ihdr), chunk("IDAT", zlib.deflateSync(raw)), chunk("IEND", Buffer.alloc(0))]));
  console.log("anteprima -> " + preview + "  (" + list.map((p) => p.name).join(" ") + ")");
}
