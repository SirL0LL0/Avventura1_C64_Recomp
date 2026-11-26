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

// ------------------------------------------------------------------ mezzobusto
// Ritratto da vicino: testa grande con lineamenti veri. Opzioni:
// skin, cloth (scala abiti), armor (scala metallo: spallacci), chain, robe, cloak, hood,
// hair (scala), hairStyle: short|long|bald|wild, beard: none|short|long, mustache, brow,
// eyes (colore iride), glow, helm, nasal, horns, crown, circlet, ears: pointed, tusks, fangs,
// bigNose, scar, jaw (larghezza), weapon (arma che spunta da una spalla), collar (colore)
function bust(o = {}) {
  const L = new Img(S, S);
  const skin = o.skin || [9, 8, 10], cx = 64, hy = o.hy || 46;
  const hw = o.jaw || 19;                                        // mezza larghezza del viso
  const hair = o.hair, st = o.hairStyle || "short";
  // capelli lunghi dietro la testa
  if (hair && (st === "long" || st === "wild")) {
    L.poly([[cx - hw - 4, hy - 14], [cx + hw + 4, hy - 14], [cx + hw + 12, hy + 46], [cx + hw - 2, hy + 52], [cx - hw + 2, hy + 52], [cx - hw - 12, hy + 46]], hair, { k: 0.7 });
    for (let k = -2; k <= 2; k++) L.curve([[cx + k * 9, hy - 8], [cx + k * 13, hy + 20], [cx + k * 12, hy + 50]], 0.6, hair[0]);
  }
  if (o.cloak) L.poly([[0, S], [6, 100], [30, 86], [98, 86], [122, 100], [128, S]], o.cloak, { k: 0.6 });
  if (o.weapon === "sword") { L.poly([[96, 128], [102, 128], [104, 40], [100, 30], [96, 40]], [11, 12, 15, 1], { k: 1 }); L.rect(90, 92, 110, 95, 7); }
  if (o.weapon === "staff") { L.cap(26, 128, 22, 30, 2.2, [0, 9, 8]); L.ell(22, 26, 6, 6, [6, 14, 3, 1], { add: 0.3 }); }
  if (o.weapon === "axe") { L.cap(100, 128, 104, 44, 2, [0, 9, 8]); L.poly([[104, 42], [122, 32], [124, 60], [104, 54]], [11, 12, 15, 1], { k: 1 }); }
  if (o.weapon === "spear") { L.cap(104, 128, 108, 24, 1.6, [0, 9, 8]); L.poly([[108, 8], [113, 26], [103, 26]], [11, 12, 15, 1]); }
  if (o.weapon === "bow") { L.curve([[106, 20], [128, 70], [106, 124]], 1.8, [0, 9, 8]); L.line(106, 20, 106, 124, 15); }
  // spalle e petto
  const cloth = o.cloth || [0, 9, 8, 10];
  L.poly([[4, S], [10, 104], [26, 92], [50, 86], [78, 86], [102, 92], [118, 104], [124, S]], o.bare ? skin : cloth, { k: 0.75, add: 0.15 });
  if (o.bare) { L.curve([[46, 100], [64, 106], [82, 100]], 0.7, skin[0]); L.line(64, 104, 64, 128, skin[0]); L.curve([[38, 118], [50, 122], [60, 114]], 0.6, skin[0]); L.curve([[90, 118], [78, 122], [68, 114]], 0.6, skin[0]); }
  if (o.chain) for (let y = 94; y < S; y += 3) for (let x = 14; x < 116; x += 3) if (L.get(x, y) >= 0) L.set(x + ((y / 3) & 1), y, 11);
  if (o.robe) { L.poly([[50, 86], [78, 86], [70, 128], [58, 128]], o.robe, { k: 0.5 }); L.line(64, 88, 64, 128, o.robe[0]); }
  if (o.collar != null) L.poly([[46, 86], [82, 86], [76, 96], [64, 92], [52, 96]], o.collar === 7 ? [9, 8, 7, 1] : [o.collar]);
  if (o.armor) {
    L.ell(24, 100, 20, 13, o.armor, { add: -0.1 }); L.ell(104, 100, 20, 13, o.armor, { add: -0.2 });
    L.rect(8, 100, 40, 101, o.armor[0]); L.rect(88, 100, 120, 101, o.armor[0]);
    for (const x of [14, 24, 34, 94, 104, 114]) L.set(x, 94, 1);
    L.poly([[40, 100], [88, 100], [84, 128], [44, 128]], o.armor, { k: 0.8 });
    L.line(64, 100, 64, 128, o.armor[0]);
  }
  if (o.amulet) { L.curve([[50, 88], [64, 104], [78, 88]], 0.6, 7); L.ell(64, 106, 3.5, 3.5, [2, 10, 1], { add: 0.3 }); }
  // collo
  L.poly([[cx - 9, hy + 18], [cx + 9, hy + 18], [cx + 12, hy + 42], [cx - 12, hy + 42]], skin, { k: 0.6, add: -0.05 });
  L.line(cx - 6, hy + 30, cx + 6, hy + 30, skin[0]);
  // cappuccio dietro
  if (o.hood) L.poly([[cx - hw - 12, hy + 40], [cx - hw - 10, hy - 14], [cx, hy - 34], [cx + hw + 10, hy - 14], [cx + hw + 12, hy + 40]], o.hood, { k: 0.7 });
  // orecchie
  if (!o.hood) {
    if (o.ears === "pointed") { L.poly([[cx - hw + 2, hy - 2], [cx - hw - 12, hy - 14], [cx - hw + 2, hy + 10]], skin); L.poly([[cx + hw - 2, hy - 2], [cx + hw + 12, hy - 14], [cx + hw - 2, hy + 10]], skin, { flip: true }); }
    else { L.ell(cx - hw, hy + 3, 4, 6, skin); L.ell(cx + hw, hy + 3, 4, 6, skin, { add: -0.2 }); }
  }
  // testa: cranio + mascella
  L.ell(cx, hy - 4, hw + 1, hw + 3, skin, { add: 0.25, k: 0.85 });
  L.poly([[cx - hw, hy], [cx + hw, hy], [cx + hw - 3, hy + 14], [cx + 7, hy + 23], [cx - 7, hy + 23], [cx - hw + 3, hy + 14]], skin, { k: 0.8, add: 0.2 });
  // ombra sotto gli zigomi e del mento
  L.line(cx + hw - 4, hy + 6, cx + 8, hy + 21, skin[0]);
  L.line(cx - 6, hy + 24, cx + 6, hy + 24, skin[0]);
  // occhi
  const ey = hy + 2, iris = o.eyes == null ? 6 : o.eyes;
  for (const s of [-1, 1]) {
    const ex = cx + s * 8;
    L.rect(ex - 4, ey - 1, ex + 3, ey + 1, o.glow ? o.glow : 15);
    if (!o.glow) { L.rect(ex - 1, ey - 1, ex + 1, ey + 1, iris); L.set(ex, ey, 0); L.set(ex - 1, ey - 1, 1); }
    L.rect(ex - 4, ey - 2, ex + 3, ey - 2, 0);                // palpebra
    L.set(ex - 5, ey - 1, 0); L.set(ex + 4, ey - 1, 0);
    L.rect(ex - 3, ey + 2, ex + 2, ey + 2, skin[0]);
    const bc = o.brow == null ? (hair ? hair[0] : skin[0]) : o.brow;   // sopracciglia
    L.line(ex - 5, ey - 5 + (o.angry ? (s < 0 ? -1 : 1) * -1 : 0), ex + 4, ey - 5 + (o.angry ? (s < 0 ? 1 : -1) * -1 : 0), bc, 1);
    L.line(ex - 5, ey - 4, ex + 4, ey - 4, bc, 1);
  }
  // naso
  const nw = o.bigNose ? 5 : 2;
  L.line(cx + 1, ey - 2, cx + 2, ey + 9, skin[0]);
  L.line(cx - 1, ey, cx - 1, ey + 8, skin[3] != null ? skin[3] : skin[2]);
  if (o.bigNose) L.ell(cx, ey + 10, nw + 1, 4, skin, { add: 0.1 });
  L.rect(cx - nw, ey + 11, cx - nw + 1, ey + 11, skin[0]); L.rect(cx + nw - 1, ey + 11, cx + nw, ey + 11, skin[0]);
  // bocca
  const my = hy + 17;
  L.line(cx - 6, my, cx + 6, my, o.mouth == null ? 2 : o.mouth);
  L.set(cx - 7, my - 1, skin[0]); L.set(cx + 7, my - 1, skin[0]);
  L.line(cx - 4, my + 2, cx + 4, my + 2, skin[3] != null ? skin[3] : skin[2]);
  if (o.fangs) { L.set(cx - 4, my + 1, 1); L.set(cx + 4, my + 1, 1); L.set(cx - 4, my + 2, 1); L.set(cx + 4, my + 2, 1); }
  if (o.tusks) { L.poly([[cx - 8, my + 1], [cx - 5, my + 1], [cx - 7, my - 7]], [12, 15, 1]); L.poly([[cx + 5, my + 1], [cx + 8, my + 1], [cx + 7, my - 7]], [12, 15, 1]); }
  if (o.scar) L.line(cx + 6, ey - 8, cx + 12, ey + 10, 2);
  // barba e baffi
  if (o.beard && o.beard !== "none") {
    const by = o.beard === "long" ? hy + 52 : hy + 32;
    L.poly([[cx - hw + 1, hy + 6], [cx - 8, my - 3], [cx + 8, my - 3], [cx + hw - 1, hy + 6], [cx + hw - 3, hy + 22], [cx + 8, by], [cx - 8, by], [cx - hw + 3, hy + 22]], hair || [0, 9, 8], { k: 0.7 });
    for (let k = -3; k <= 3; k++) L.line(cx + k * 4, my + 3, cx + k * 3, by - 2, (hair || [0, 9])[0]);
    L.line(cx - 5, my + 1, cx + 5, my + 1, 0);
  }
  if (o.mustache || (o.beard && o.beard !== "none")) { L.poly([[cx - 9, my + 2], [cx - 2, my - 3], [cx + 2, my - 3], [cx + 9, my + 2], [cx + 2, my - 1], [cx - 2, my - 1]], hair || [0, 9, 8]); }
  // capelli sopra
  if (hair && st !== "bald" && !o.helm && !o.hood) {
    L.poly([[cx - hw - 3, hy + 4], [cx - hw - 2, hy - 16], [cx - 8, hy - 28], [cx + 10, hy - 27], [cx + hw + 3, hy - 14], [cx + hw + 3, hy + 4], [cx + hw - 2, hy - 8], [cx + 6, hy - 14], [cx - 4, hy - 10], [cx - hw + 2, hy - 6]], hair, { k: 0.8, add: 0.1 });
    for (let k = 0; k < 5; k++) L.curve([[cx - 12 + k * 6, hy - 26], [cx - 16 + k * 7, hy - 18], [cx - 20 + k * 9, hy - 8]], 0.5, hair[0]);
    L.curve([[cx - 10, hy - 24], [cx - 14, hy - 16], [cx - 16, hy - 8]], 0.6, hair[hair.length - 1]);
    if (st === "wild") for (let k = 0; k < 7; k++) L.poly([[cx - hw - 4 + k * 7, hy - 20], [cx - hw - 6 + k * 7, hy - 32], [cx - hw + 2 + k * 7, hy - 22]], hair);
  }
  // copricapi
  if (o.hood) { L.poly([[cx - hw - 4, hy + 6], [cx - hw - 4, hy - 14], [cx, hy - 28], [cx + hw + 4, hy - 14], [cx + hw + 4, hy + 6], [cx + hw - 2, hy - 8], [cx, hy - 16], [cx - hw + 2, hy - 8]], o.hood, { k: 0.8, add: 0.1 }); L.rect(cx - hw + 2, hy - 8, cx + hw - 2, hy - 6, o.hood[0]); }
  if (o.helm) {
    const H = new Img(S, S); H.ell(cx, hy - 8, hw + 4, 22, o.helm, { add: 0.2 });
    for (let y = 0; y < hy - 5; y++) for (let x = 0; x < S; x++) L.set(x, y, H.get(x, y));
    L.rect(cx - hw - 4, hy - 10, cx + hw + 4, hy - 6, o.helm[1]); L.rect(cx - hw - 4, hy - 6, cx + hw + 4, hy - 6, o.helm[0]);
    for (let x = cx - hw; x <= cx + hw; x += 6) L.set(x, hy - 8, 1);
    if (o.nasal) L.poly([[cx - 2, hy - 6], [cx + 2, hy - 6], [cx + 1, ey + 8], [cx - 1, ey + 8]], o.helm);
    L.poly([[cx - hw - 4, hy - 6], [cx - hw + 2, hy - 6], [cx - hw + 2, hy + 16], [cx - hw - 3, hy + 12]], o.helm);
    L.poly([[cx + hw + 4, hy - 6], [cx + hw - 2, hy - 6], [cx + hw - 2, hy + 16], [cx + hw + 3, hy + 12]], o.helm, { flip: true });
  }
  if (o.horns) { L.curve([[cx - hw, hy - 16], [cx - hw - 20, hy - 18], [cx - hw - 14, hy - 40]], 4, o.horns, 1); L.curve([[cx + hw, hy - 16], [cx + hw + 20, hy - 18], [cx + hw + 14, hy - 40]], 4, o.horns, 1); }
  if (o.crown) { L.poly([[cx - 18, hy - 18], [cx + 18, hy - 18], [cx + 19, hy - 32], [cx + 10, hy - 24], [cx, hy - 36], [cx - 10, hy - 24], [cx - 19, hy - 32]], [9, 8, 7, 1], { k: 0.9 }); L.ell(cx, hy - 22, 2, 2, [2, 10, 1]); L.set(cx - 19, hy - 33, 1); L.set(cx + 19, hy - 33, 1); L.set(cx, hy - 37, 1); }
  if (o.circlet) { L.curve([[cx - hw - 1, hy - 10], [cx, hy - 16], [cx + hw + 1, hy - 10]], 0.8, 7); L.ell(cx, hy - 14, 2.2, 2.2, o.circlet); }
  return L;
}

// cornice di testo / oggetto su sfondo scuro con alone
function glow(img, x, y, r, ramp) { for (let k = r; k > 0; k -= 2) img.ell(x, y, k, k, ramp[Math.min(ramp.length - 1, Math.floor((1 - k / r) * ramp.length))]); return img; }

module.exports = { bust, S, sky, moon, sun, ground, mountains, hills, tree, pine, stoneWall, planks, torch, human, weapon, glow };
