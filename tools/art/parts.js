// Componenti riutilizzabili delle illustrazioni: sfondi, paesaggi, figure umane, armi.
"use strict";
const { Img, R } = require("./engine");

const S = 128;

// ------------------------------------------------------------------ sfondi
function sky(img, kind, horizon = 84) {
  if (kind === "day") img.vgrad(0, horizon, [6, 14, 3]);
  else if (kind === "dusk") img.vgrad(0, horizon, [6, 4, 10, 8, 7]);
  else if (kind === "fire") img.vgrad(0, horizon, [0, 9, 2, 8]);
  else if (kind === "storm") img.vgrad(0, horizon, [0, 11, 12]);
  else if (kind === "swamp") img.vgrad(0, horizon, [0, 9, 11, 5]);
  else {                                   // notte
    img.vgrad(0, horizon, [0, 0, 6, 6, 14]);
    img.speck(0, 0, S - 1, horizon - 20, 15, 0.012, 7).speck(0, 0, S - 1, horizon - 30, 1, 0.005, 3);
  }
  return img;
}
function moon(img, x, y, r) { img.ell(x, y, r, r, [12, 15, 1, 1], { add: 0.25 }); img.ell(x + r * 0.3, y + r * 0.2, r * 0.25, r * 0.2, 12); return img; }
function sun(img, x, y, r) { img.ell(x, y, r + 3, r + 3, 8); img.ell(x, y, r, r, [8, 7, 1], { add: 0.4 }); return img; }

function ground(img, y0, ramp, seed = 1) {
  img.vgrad(y0, S - 1, ramp, 0, S - 1, true);
  img.speck(0, y0 + 2, S - 1, S - 1, ramp[0], 0.04, seed);
  return img;
}

function mountains(img, base, peaks, ramp) {
  for (const [x, h, w] of peaks) img.poly([[x - w, base], [x, base - h], [x + w, base]], ramp, { k: 0.8, add: 0.15 });
  for (const [x, h, w] of peaks) if (h > 30) img.poly([[x - w * 0.22, base - h * 0.78], [x, base - h], [x + w * 0.22, base - h * 0.78]], 1);
  return img;
}

function hills(img, base, ramp, seed = 1) {
  for (let x = 0; x < S; x++) {
    const h = 8 + 6 * Math.sin(x * 0.07 + seed) + 4 * Math.sin(x * 0.19 + seed * 2);
    for (let y = Math.floor(base - h); y <= base; y++) img.tone(x, y, ramp, 1 - (y - base + h) / (h + 8));
  }
  return img;
}

function tree(img, x, y, s = 1, ramp = R.green) {
  img.cap(x, y, x, y - 18 * s, 2.5 * s, R.wood);
  img.ell(x - 6 * s, y - 20 * s, 9 * s, 8 * s, ramp);
  img.ell(x + 6 * s, y - 22 * s, 9 * s, 8 * s, ramp);
  img.ell(x, y - 30 * s, 10 * s, 9 * s, ramp, { add: 0.1 });
  return img;
}

function pine(img, x, y, s = 1, ramp = R.dgreen) {
  img.rect(x - 1, y - 4 * s, x + 1, y, 9);
  for (let i = 0; i < 3; i++) img.poly([[x - (12 - i * 3) * s, y - (4 + i * 9) * s], [x, y - (20 + i * 9) * s], [x + (12 - i * 3) * s, y - (4 + i * 9) * s]], ramp, { k: 0.7 });
  return img;
}

// muro di pietra a blocchi
function stoneWall(img, y0, y1, ramp = [0, 11, 12, 15], bh = 9, bw = 16, seed = 2) {
  for (let y = y0; y <= y1; y++) for (let x = 0; x < S; x++) img.tone(x, y, ramp, 0.42 + 0.12 * Math.sin(x * 0.9 + y * 1.7));
  let row = 0;
  for (let y = y0; y <= y1; y += bh, row++) {
    img.rect(0, y, S - 1, y, ramp[0]);
    img.rect(0, y + 1, S - 1, y + 1, ramp[2]);
    for (let x = (row % 2) * (bw / 2); x < S; x += bw) { img.rect(x, y, x, Math.min(y1, y + bh - 1), ramp[0]); img.rect(x + 1, y + 1, x + 1, Math.min(y1, y + bh - 1), ramp[2]); }
  }
  img.speck(0, y0, S - 1, y1, ramp[0], 0.03, seed);
  return img;
}

function planks(img, y0, y1) {
  for (let y = y0; y <= y1; y++) for (let x = 0; x < S; x++) img.tone(x, y, R.wood, 0.55 + 0.3 * Math.sin(y * 0.6 + Math.floor(x / 14) * 2));
  for (let x = 0; x < S; x += 14) img.rect(x, y0, x, y1, 0);
  return img;
}

function torch(img, x, y) {
  img.rect(x - 1, y, x + 1, y + 10, 9);
  img.ell(x, y - 3, 3, 5, [2, 8, 7, 1], { add: 0.3 });
  img.ell(x, y - 2, 1.5, 2, 1);
  return img;
}

// ------------------------------------------------------------------ figure umane
// o: skin, body, legs, cloak, hair, beard, helm, hood, horns, ears, eyes, weapon, shield, bare, robe, crown
function human(o = {}) {
  const L = new Img(S, S);
  const cx = o.cx || 64, hy = o.hy || 34, hr = o.hr || 11;
  const skin = o.skin || R.skin, body = o.body || R.red, legs = o.legs || R.wood;
  // mantello dietro
  if (o.cloak) L.poly([[cx - 20, hy + 14], [cx + 20, hy + 14], [cx + 34, S], [cx - 34, S]], o.cloak, { k: 0.8 });
  // gambe o veste
  if (o.noLegs) {} else if (o.robe) L.poly([[cx - 15, 88], [cx + 15, 88], [cx + 26, S], [cx - 26, S]], o.robe, { k: 0.8 });
  else { L.cap(cx - 8, 90, cx - 11, S + 4, 6.5, legs, 5.5); L.cap(cx + 8, 90, cx + 11, S + 4, 6.5, legs, 5.5); }
  // busto
  const bodyR = o.bare ? skin : body;
  L.poly([[cx - 19, hy + 15], [cx + 19, hy + 15], [cx + 14, 92], [cx - 14, 92]], bodyR, { k: 0.85 });
  if (o.bare) { L.ell(cx - 7, hy + 25, 6, 4, skin, { add: 0.1 }); L.ell(cx + 7, hy + 25, 6, 4, skin); L.line(cx, hy + 22, cx, hy + 40, skin[0]); }
  if (o.belt !== false) { L.rect(cx - 14, 84, cx + 14, 88, o.beltC || 9); L.rect(cx - 2, 84, cx + 2, 88, 7); }
  if (o.chain) for (let y = hy + 18; y < 84; y += 3) for (let x = cx - 16; x < cx + 16; x += 3) L.set(x + ((y / 3) & 1), y, 11);
  // braccia: a sinistra (destra del personaggio) alzato con l'arma, a destra lungo il fianco
  const arm = o.bare ? skin : o.sleeve || body;
  const handL = o.handL || [cx - 30, hy + 30], handR = o.handR || [cx + 26, hy + 52];
  L.cap(cx - 19, hy + 18, cx - 26, hy + 40, 6, arm, 5);
  L.cap(cx - 26, hy + 40, handL[0], handL[1], 5, arm, 4.5);
  L.cap(cx + 19, hy + 18, cx + 25, hy + 38, 6, arm, 5);
  L.cap(cx + 25, hy + 38, handR[0], handR[1], 5, arm, 4.5);
  if (o.pauldron) { L.ell(cx - 19, hy + 17, 9, 6, o.pauldron); L.ell(cx + 19, hy + 17, 9, 6, o.pauldron); }
  // collo e testa
  L.cap(cx, hy + 6, cx, hy + 15, 5, skin);
  if (o.hood) L.ell(cx, hy - 1, hr + 5, hr + 6, o.hood);
  if (o.hairBack) L.poly([[cx - hr - 2, hy - 4], [cx + hr + 2, hy - 4], [cx + hr + 5, hy + 22], [cx - hr - 5, hy + 22]], o.hairBack, { k: 0.6 });
  L.ell(cx, hy, hr, hr + 1.5, skin, { add: o.faceAdd == null ? 0.3 : o.faceAdd, k: 0.85 });
  if (o.ears) { L.poly([[cx - hr + 1, hy - 1], [cx - hr - 7, hy - 8], [cx - hr + 1, hy + 4]], skin); L.poly([[cx + hr - 1, hy - 1], [cx + hr + 7, hy - 8], [cx + hr - 1, hy + 4]], skin, { flip: true }); }
  if (o.hair) { L.ell(cx, hy - 7, hr + 1, hr * 0.55, o.hair); L.poly([[cx - hr - 1, hy - 7], [cx - hr + 3, hy - 7], [cx - hr + 2, hy + 6], [cx - hr - 1, hy + 4]], o.hair); L.poly([[cx + hr + 1, hy - 7], [cx + hr - 3, hy - 7], [cx + hr - 2, hy + 6], [cx + hr + 1, hy + 4]], o.hair); }
  // viso
  const eye = o.eyes == null ? 0 : o.eyes;
  if (o.hood) L.poly([[cx - hr, hy - 4], [cx + hr, hy - 4], [cx + hr, hy - 13], [cx - hr, hy - 13]], o.hood);
  L.rect(cx - 6, hy - 2, cx - 3, hy - 2, o.brow == null ? skin[0] : o.brow);
  L.rect(cx + 3, hy - 2, cx + 6, hy - 2, o.brow == null ? skin[0] : o.brow);
  L.rect(cx - 5, hy, cx - 4, hy, eye); L.rect(cx + 4, hy, cx + 5, hy, eye);
  if (o.glow) { L.set(cx - 5, hy, o.glow); L.set(cx + 5, hy, o.glow); }
  L.rect(cx, hy + 1, cx + 1, hy + 4, skin[0]);
  L.rect(cx - 3, hy + 7, cx + 3, hy + 7, o.mouth == null ? skin[0] : o.mouth);
  if (o.beard) L.poly([[cx - hr + 1, hy + 3], [cx + hr - 1, hy + 3], [cx + 5, hy + (o.longBeard ? 26 : 16)], [cx - 5, hy + (o.longBeard ? 26 : 16)]], o.beard, { k: 0.6 });
  if (o.beard) L.rect(cx - 3, hy + 7, cx + 3, hy + 7, 0);
  if (o.helm) {
    L.ell(cx, hy - 4, hr + 2, hr - 1, o.helm, { add: 0.15 });
    L.rect(cx - hr - 2, hy - 4, cx + hr + 2, hy - 2, o.helm[1]);
    if (o.nasal) L.rect(cx - 1, hy - 3, cx + 1, hy + 3, o.helm[1]);
  }
  if (o.horns) { L.curve([[cx - hr, hy - 8], [cx - hr - 14, hy - 12], [cx - hr - 8, hy - 26]], 3, o.horns, 1); L.curve([[cx + hr, hy - 8], [cx + hr + 14, hy - 12], [cx + hr + 8, hy - 26]], 3, o.horns, 1); }
  if (o.crown) { L.poly([[cx - 10, hy - 9], [cx + 10, hy - 9], [cx + 10, hy - 17], [cx + 5, hy - 12], [cx, hy - 19], [cx - 5, hy - 12], [cx - 10, hy - 17]], R.gold); L.set(cx, hy - 13, 2); }
  // arma e scudo
  if (o.weapon) weapon(L, o.weapon, handL[0], handL[1]);
  if (o.shield) { L.ell(handR[0] - 2, handR[1] - 6, 14, 17, o.shield, { add: 0.1 }); L.ell(handR[0] - 2, handR[1] - 6, 3, 3, R.gold); if (o.shieldMark) L.rect(handR[0] - 3, handR[1] - 19, handR[0] - 1, handR[1] + 7, o.shieldMark); }
  // mani
  L.ell(handL[0], handL[1], 4, 4, skin);
  if (!o.shield) L.ell(handR[0], handR[1], 4, 4, skin);
  return L;
}

function weapon(L, kind, x, y) {
  if (kind === "sword") {
    L.poly([[x - 2, y - 3], [x + 2, y - 3], [x + 2, y - 44], [x, y - 50], [x - 2, y - 44]], R.steel, { k: 1 });
    L.rect(x - 8, y - 5, x + 8, y - 3, 7); L.rect(x - 1, y + 3, x + 1, y + 8, 9); L.ell(x, y + 9, 2, 2, 7);
  } else if (kind === "axe") {
    L.cap(x, y + 12, x + 2, y - 40, 1.5, R.wood);
    L.poly([[x + 2, y - 38], [x + 16, y - 46], [x + 18, y - 26], [x + 2, y - 30]], R.steel, { k: 1 });
  } else if (kind === "staff") {
    L.cap(x, y + 30, x + 1, y - 44, 1.6, R.wood);
    L.ell(x + 1, y - 48, 5, 5, [6, 14, 3, 1], { add: 0.3 });
  } else if (kind === "spear") {
    L.cap(x, y + 30, x + 3, y - 46, 1.3, R.wood);
    L.poly([[x + 3, y - 60], [x + 7, y - 46], [x - 1, y - 46]], R.steel);
  } else if (kind === "scimitar") {
    L.curve([[x, y - 3], [x - 4, y - 30], [x + 12, y - 46]], 2.5, R.steel, 1);
    L.rect(x - 6, y - 4, x + 6, y - 2, 7);
  } else if (kind === "club") {
    L.cap(x, y + 6, x - 4, y - 36, 2.5, R.wood, 6);
    L.speck(x - 10, y - 42, x + 2, y - 26, 0, 0.12, 3);
  } else if (kind === "bow") {
    L.curve([[x + 2, y - 34], [x - 14, y], [x + 2, y + 34]], 1.6, R.wood);
    L.line(x + 2, y - 34, x + 2, y + 34, 15);
  }
}

// cornice di testo / oggetto su sfondo scuro con alone
function glow(img, x, y, r, ramp) { for (let k = r; k > 0; k -= 2) img.ell(x, y, k, k, ramp[Math.min(ramp.length - 1, Math.floor((1 - k / r) * ramp.length))]); return img; }

module.exports = { S, sky, moon, sun, ground, mountains, hills, tree, pine, stoneWall, planks, torch, human, weapon, glow };
