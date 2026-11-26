// Le illustrazioni dell'Espansione (originali del progetto), 128x128 con la tavolozza del C64.
// Ogni voce: name, keys (parole che la scelgono), title (didascalia), draw() -> Img.
"use strict";
const { Img, R } = require("./engine");
const P = require("./parts");
const S = 128;

const pics = [];
const pic = (name, keys, title, draw, bg = 0) => pics.push({ name, keys, title, draw, bg });

// ------------------------------------------------------------------ personaggi
pic("guerriero", "thoralf,beowulf,hotan,guerriero", "Il guerriero", () => {
  const img = new Img(S, S);
  P.sky(img, "dusk", 96); P.sun(img, 100, 70, 10);
  P.mountains(img, 100, [[20, 40, 34], [70, 30, 30], [118, 46, 30]], [0, 6, 4, 14]);
  P.ground(img, 100, [9, 8, 5]);
  img.put(P.bust({ cloth: R.steel, chain: true, armor: R.steel, cloak: R.red, hair: [9, 8, 7], hairStyle: "long", beard: "short", eyes: 14, weapon: "sword", scar: true }), 0, 0, 0);
  return img;
});

pic("drago", "drago", "Drago di fuoco", () => {
  const img = new Img(S, S);
  P.sky(img, "fire", 100);
  P.mountains(img, 104, [[16, 40, 30], [110, 52, 34]], [0, 9, 2]);
  P.ground(img, 104, [0, 9, 2]);
  const L = new Img(S, S);
  // ali
  L.poly([[60, 52], [8, 10], [18, 40], [6, 44], [22, 62], [10, 70], [40, 74]], [9, 2, 10], { k: 0.9 });
  L.poly([[70, 52], [124, 14], [112, 42], [124, 48], [104, 64], [118, 72], [86, 74]], [9, 2, 10], { flip: true });
  for (const [a, b] of [[8, 10], [18, 40], [22, 62]]) L.line(60, 52, a, b, 9);
  for (const [a, b] of [[124, 14], [112, 42], [104, 64]]) L.line(70, 52, a, b, 9);
  // coda e corpo
  L.curve([[56, 100], [20, 120], [10, 96]], 7, R.red, 2);
  L.ell(64, 88, 22, 18, R.red, { add: 0.1 });
  L.ell(64, 94, 12, 11, [8, 7, 7], { add: 0.2 });
  for (let y = 86; y < 104; y += 3) L.rect(58, y, 70, y, 8);
  L.cap(50, 100, 44, 118, 5, R.red); L.cap(78, 100, 84, 118, 5, R.red);
  L.ell(42, 120, 6, 3, R.red); L.ell(86, 120, 6, 3, R.red);
  // collo e testa
  L.curve([[64, 74], [58, 52], [66, 36]], 8, R.red, 6);
  L.ell(68, 30, 13, 9, R.red, { add: 0.1 });
  L.poly([[72, 26], [92, 30], [92, 36], [72, 38]], R.red);
  L.curve([[60, 24], [52, 12], [58, 4]], 2.5, R.bone, 1); L.curve([[70, 23], [70, 10], [78, 4]], 2.5, R.bone, 1);
  L.rect(66, 27, 69, 28, 7); L.set(68, 27, 0);
  L.rect(78, 36, 92, 36, 0); for (let x = 80; x < 92; x += 3) L.set(x, 37, 1);
  img.put(L, 0, 0, 0);
  // fiamma
  const F = new Img(S, S);
  F.poly([[92, 32], [128, 22], [124, 34], [128, 46], [92, 36]], [2, 8, 7, 1], { flip: true, k: 1 });
  F.ell(96, 34, 4, 3, 1);
  img.put(F, 0, 0, -1);
  return img;
});

pic("taverna", "taverna,gobbo", "Taverna del Gobbo", () => {
  const img = new Img(S, S);
  P.planks(img, 0, 80);
  for (let x = 0; x < S; x += 40) img.cap(x + 10, 0, x + 10, 80, 3, R.wood);
  P.torch(img, 30, 30); P.torch(img, 98, 30);
  // botti
  for (const x of [20, 108]) { img.ell(x, 66, 14, 16, R.wood, { add: 0.1 }); img.rect(x - 13, 58, x + 13, 59, 11); img.rect(x - 13, 73, x + 13, 74, 11); }
  // bancone
  img.poly([[0, 84], [128, 84], [128, 127], [0, 127]], R.wood, { vert: true, k: 0.7 });
  img.rect(0, 84, 127, 87, 8); img.rect(0, 88, 127, 88, 0);
  // boccale
  const L = new Img(S, S);
  L.poly([[48, 50], [80, 50], [78, 98], [50, 98]], [9, 8, 7, 1], { k: 1 });
  L.ell(64, 50, 17, 6, [12, 15, 1, 1], { add: 0.3 }); L.ell(58, 44, 9, 6, [12, 15, 1, 1], { add: 0.4 }); L.ell(70, 45, 8, 5, [12, 15, 1], { add: 0.4 });
  L.cap(84, 60, 92, 74, 3, R.gold); L.cap(92, 74, 82, 90, 3, R.gold);
  L.rect(48, 64, 80, 66, 9); L.rect(49, 86, 79, 88, 9);
  img.put(L, 0, 0, 0);
  return img;
});

pic("tempio", "tempio,lyss", "Tempio di LYSS", () => {
  const img = new Img(S, S);
  P.sky(img, "day", 100); P.sun(img, 64, 30, 9);
  P.ground(img, 104, [9, 8, 7]);
  // gradini
  for (let i = 0; i < 3; i++) img.rect(10 - i * 4, 100 + i * 5, 118 + i * 4, 104 + i * 5, i % 2 ? 15 : 12);
  // colonne
  for (let x = 20; x <= 108; x += 22) { img.poly([[x - 6, 52], [x + 6, 52], [x + 6, 100], [x - 6, 100]], [11, 12, 15, 1], { k: 1 }); for (let k = -4; k <= 4; k += 4) img.rect(x + k, 54, x + k, 98, 12); img.rect(x - 8, 48, x + 8, 52, 15); img.rect(x - 8, 98, x + 8, 100, 15); }
  // frontone
  img.poly([[6, 48], [122, 48], [122, 40], [64, 16], [6, 40]], [12, 15, 1], { vert: true, k: -0.6 });
  img.poly([[22, 40], [106, 40], [64, 24]], [8, 7]);
  img.ell(64, 34, 5, 5, [8, 7, 1], { add: 0.3 });
  img.line(6, 48, 122, 48, 11); img.line(6, 40, 64, 16, 11); img.line(64, 16, 122, 40, 11);
  return img;
});

pic("licantropo", "licantropo,lupi mannari,lupo", "Licantropo nero", () => {
  const img = new Img(S, S);
  P.sky(img, "night", 100); P.moon(img, 96, 28, 16);
  P.pine(img, 12, 104, 1.6); P.pine(img, 118, 104, 1.3);
  P.ground(img, 102, [0, 6, 11]);
  const L = new Img(S, S);
  const fur = [0, 11, 12, 15];
  L.cap(52, 92, 46, 124, 7, fur, 5); L.cap(76, 92, 82, 124, 7, fur, 5);
  L.ell(64, 74, 22, 24, fur, { add: 0 });
  L.cap(44, 60, 22, 80, 7, fur, 5); L.cap(22, 80, 18, 96, 5, fur, 4);
  L.cap(84, 60, 106, 80, 7, fur, 5); L.cap(106, 80, 110, 96, 5, fur, 4);
  for (const x of [14, 18, 22]) L.line(x + 2, 96, x, 104, 1);
  for (const x of [106, 110, 114]) L.line(x - 2, 96, x, 104, 1);
  L.ell(64, 40, 15, 13, fur);
  L.poly([[56, 40], [72, 40], [70, 58], [58, 58]], fur);
  L.poly([[50, 34], [46, 16], [58, 28]], fur); L.poly([[78, 34], [82, 16], [70, 28]], fur);
  L.rect(56, 37, 59, 38, 7); L.rect(69, 37, 72, 38, 7);
  L.rect(58, 56, 70, 57, 2); for (let x = 59; x < 70; x += 3) L.set(x, 55, 1);
  L.ell(64, 50, 3, 2, 0);
  img.put(L, 0, 0, 0);
  return img;
});

pic("maga", "rowena", "La maga", () => {
  const img = new Img(S, S);
  P.stoneWall(img, 0, 127, [0, 6, 11, 12]);
  P.glow(img, 96, 22, 26, [6, 4, 14]);
  img.put(P.bust({ cloth: R.purple, robe: [0, 6, 4], hair: [0, 2, 10], hairStyle: "long", eyes: 5, mouth: 10, circlet: [6, 14, 3], amulet: true, weapon: "staff", jaw: 17 }), 0, 0, 0);
  img.speck(10, 10, 50, 60, 3, 0.01, 4).speck(80, 40, 120, 90, 1, 0.006, 9);
  return img;
});

pic("avventuriero", "jasper,keilan,elrik,toscyro,tulsak,avventuriero", "L'avventuriero", () => {
  const img = new Img(S, S);
  P.sky(img, "day", 96); P.hills(img, 96, [0, 9, 5, 13], 3);
  P.tree(img, 18, 100, 1.2); P.tree(img, 112, 98, 1);
  P.ground(img, 98, [9, 5, 13]);
  img.put(P.human({ body: R.leather, sleeve: [0, 9, 8], legs: [0, 6, 14], cloak: R.dgreen, hair: [0, 9, 8], weapon: "sword", hy: 36 }), 0, 0, 0);
  return img;
});

pic("pellegrino", "pellegrino,anselmo", "Fratello Anselmo", () => {
  const img = new Img(S, S);
  P.sky(img, "dusk", 100); P.sun(img, 30, 80, 14);
  P.ground(img, 100, [9, 8, 7]);
  img.put(P.bust({ cloth: [0, 9, 8, 10], hood: [0, 9, 8], hair: [11, 12, 15, 1], beard: "long", eyes: 6, weapon: "staff" }), 0, 0, 0);
  return img;
});

pic("conte", "conte movas", "Il Conte Movas", () => {
  const img = new Img(S, S);
  P.stoneWall(img, 0, 127, [0, 0, 11, 2]);
  img.rect(40, 0, 88, 127, 2); for (let y = 0; y < 128; y += 4) img.rect(40, y, 88, y, 9);
  P.torch(img, 14, 40); P.torch(img, 114, 40);
  img.put(P.human({ body: [0, 11, 12], chain: false, pauldron: [0, 11, 12, 15], legs: [0, 0, 11], cloak: R.purple, hair: [0, 0, 11], crown: true, weapon: "sword", eyes: 2, shield: [0, 0, 11], shieldMark: 2 }), 0, 0, 0);
  return img;
});

pic("barbaro", "barbari", "Barbari", () => {
  const img = new Img(S, S);
  P.sky(img, "storm", 98); P.mountains(img, 100, [[30, 50, 40], [100, 40, 36]], [0, 11, 12, 15]);
  P.ground(img, 100, [0, 11, 12]);
  img.put(P.human({ bare: true, legs: [0, 9, 8], hairBack: [9, 8, 7], hair: [9, 8, 7], beard: [9, 8, 7], weapon: "axe", horns: R.bone, helm: [11, 12, 15], belt: true }), 0, 0, 0);
  return img;
});

pic("armigero", "armigeri,movas", "Armigeri di Movas", () => {
  const img = new Img(S, S);
  P.sky(img, "storm", 96);
  // castello sullo sfondo
  img.rect(80, 40, 120, 96, 11); for (let x = 80; x < 120; x += 8) img.rect(x, 34, x + 4, 40, 11); img.rect(96, 70, 104, 96, 0);
  P.ground(img, 96, [0, 9, 11]);
  img.put(P.human({ body: [0, 2, 10], chain: true, legs: [0, 11, 12], helm: R.steel, nasal: true, weapon: "spear", shield: [0, 2, 10], shieldMark: 0, pauldron: R.steel }), -6, 0, 0);
  return img;
});

pic("bandito", "bandito,briganti,predoni,mercenari,schiavisti", "Bandito", () => {
  const img = new Img(S, S);
  P.sky(img, "night", 96); P.moon(img, 24, 24, 10);
  P.pine(img, 100, 100, 1.8); P.pine(img, 120, 100, 1.4);
  P.ground(img, 98, [0, 9, 11]);
  img.put(P.human({ body: [0, 9, 8], legs: [0, 11, 12], hood: [0, 0, 11], cloak: [0, 0, 11], beard: [0, 0, 9], weapon: "scimitar", eyes: 7 }), 0, 0, 0);
  return img;
});

pic("vampiro", "vampir", "Vampiro maledetto", () => {
  const img = new Img(S, S);
  P.sky(img, "night", 110); P.moon(img, 64, 30, 22);
  img.rect(0, 70, 30, 127, 0); img.rect(98, 60, 127, 127, 0); img.rect(10, 60, 16, 70, 0); img.rect(104, 50, 110, 60, 0);
  const L = P.human({ skin: [11, 12, 15, 1], body: [0, 0, 11], legs: [0, 0, 11], cloak: [0, 2, 10], hair: [0, 0, 11], eyes: 2, mouth: 2, handL: [30, 70], belt: false });
  L.poly([[34, 50], [10, 40], [22, 100]], [0, 2, 10]); L.poly([[94, 50], [118, 40], [106, 100]], [0, 2, 10], { flip: true });
  L.set(61, 42, 1); L.set(67, 42, 1);
  img.put(L, 0, 0, 0);
  return img;
});

pic("orco", "orchi,orchetti,coboldi", "Orchi", () => {
  const img = new Img(S, S);
  P.sky(img, "swamp", 96); P.pine(img, 16, 100, 1.6); P.pine(img, 112, 100, 1.4);
  P.ground(img, 98, [0, 9, 5]);
  const L = P.human({ skin: [0, 9, 5, 13], bare: true, legs: [0, 9, 8], weapon: "axe", eyes: 7, brow: 0, mouth: 0, ears: true });
  L.set(60, 41, 1); L.set(68, 41, 1);
  img.put(L, 0, 0, 0);
  return img;
});

pic("troll", "troll", "Troll", () => {
  const img = new Img(S, S);
  P.stoneWall(img, 0, 127, [0, 9, 11, 12], 12, 22);
  img.ell(64, 70, 50, 60, 0);
  const L = P.human({ skin: [0, 11, 5, 13], bare: true, legs: [0, 11, 5], weapon: "club", eyes: 2, hr: 13, hy: 30, hairBack: [0, 9], belt: true });
  L.poly([[58, 34], [70, 34], [64, 44]], [0, 11, 5]);
  img.put(L, 0, 0, 0);
  return img;
});

pic("elfo", "elfi,folletti", "Elfi", () => {
  const img = new Img(S, S);
  P.sky(img, "day", 100);
  for (let x = 0; x < S; x += 22) P.tree(img, x + 8, 104, 1.3, [0, 5, 13, 7]);
  P.ground(img, 100, [9, 5, 13]);
  img.put(P.human({ body: [0, 5, 13], legs: [0, 9, 8], cloak: [0, 9, 5], hairBack: [8, 7, 1], hair: [8, 7, 1], ears: true, weapon: "bow", eyes: 5 }), 0, 0, 0);
  return img;
});

pic("nano", "nani,gnomi", "Nani", () => {
  const img = new Img(S, S);
  P.stoneWall(img, 0, 127, [0, 9, 11, 12], 10, 18);
  P.torch(img, 18, 40); P.torch(img, 110, 40);
  img.speck(4, 90, 30, 120, 7, 0.08, 5);
  img.put(P.human({ hy: 52, hr: 12, body: [0, 6, 14], legs: [0, 9, 8], helm: R.steel, nasal: true, beard: [2, 8, 7], longBeard: true, weapon: "axe", handL: [32, 82], handR: [90, 96], pauldron: R.steel }), 0, 0, 0);
  return img;
});

pic("tigroide", "tigroidi,uomini scimmia", "Tigroidi", () => {
  const img = new Img(S, S);
  P.sky(img, "dusk", 100);
  for (let x = 0; x < S; x += 18) img.cap(x + 4, 127, x + 8, 50, 2, [0, 9, 5]);
  P.ground(img, 104, [0, 9, 5]);
  const L = P.human({ skin: [9, 8, 7], bare: true, legs: [9, 8, 7], ears: true, eyes: 5, weapon: "spear", belt: true });
  for (let y = 50; y < 128; y += 7) for (let x = 40; x < 90; x += 9) if (L.get(x, y) >= 0) L.line(x, y, x + 3, y + 2, 0);
  for (let k = 0; k < 3; k++) L.line(56 + k * 6, 24, 58 + k * 6, 28, 0);
  img.put(L, 0, 0, 0);
  return img;
});

pic("minotauro", "minotauro", "Minotauro gigante", () => {
  const img = new Img(S, S);
  P.stoneWall(img, 0, 127, [0, 9, 8, 10], 12, 20);
  const L = P.human({ skin: [0, 9, 8, 10], bare: true, legs: [0, 9, 8], weapon: "axe", horns: R.bone, eyes: 2, hr: 13, hy: 32, belt: true });
  L.ell(64, 42, 9, 7, [9, 8, 10]); L.ell(60, 44, 1.5, 1.5, 0); L.ell(68, 44, 1.5, 1.5, 0);
  L.ell(64, 48, 3, 2, 7);
  img.put(L, 0, 0, 0);
  return img;
});

pic("demone", "demone,demoni", "Demone oscuro", () => {
  const img = new Img(S, S);
  P.sky(img, "fire", 128);
  img.speck(0, 80, 127, 127, 7, 0.02, 3);
  const L = new Img(S, S);
  L.poly([[52, 50], [4, 14], [12, 50], [2, 60], [30, 76]], [0, 0, 4], { k: 0.8 }); L.poly([[76, 50], [124, 14], [116, 50], [126, 60], [98, 76]], [0, 0, 4], { flip: true });
  L.put(P.human({ skin: [0, 2, 10], bare: true, legs: [0, 0, 2], horns: R.bone, glow: 7, eyes: 7, weapon: "spear", belt: false }), 0, 0, -1);
  img.put(L, 0, 0, 0);
  return img;
});

pic("gigante", "giganti", "Giganti", () => {
  const img = new Img(S, S);
  P.sky(img, "day", 110); P.mountains(img, 110, [[20, 30, 30], [108, 34, 30]], [6, 14, 3]);
  P.ground(img, 110, [9, 5, 13]);
  img.put(P.human({ body: [0, 9, 8], legs: [0, 9, 8], hair: [0, 9], beard: [0, 9, 8], weapon: "club", hy: 28, hr: 12, belt: true }), 0, 0, 0);
  for (const x of [10, 26, 102, 118]) P.pine(img, x, 124, 0.5);
  return img;
});

pic("centauro", "centauri", "Centauri", () => {
  const img = new Img(S, S);
  P.sky(img, "day", 96); P.hills(img, 98, [0, 9, 5, 13], 5);
  P.ground(img, 100, [9, 5, 13]);
  const L = new Img(S, S);
  const horse = [0, 9, 8, 10];
  L.ell(76, 82, 32, 14, horse, { add: 0.1 });
  for (const [x, d] of [[52, -4], [62, 3], [92, -3], [102, 4]]) { L.cap(x, 88, x + d, 110, 4, horse, 3); L.cap(x + d, 110, x + d, 122, 3, horse); L.rect(x + d - 3, 122, x + d + 3, 124, 0); }
  L.curve([[106, 76], [124, 80], [120, 104]], 3, [0, 0, 9], 1.5);
  L.put(P.human({ cx: 50, hy: 24, noLegs: true, bare: true, hair: [0, 9, 8], beard: [0, 9, 8], weapon: "bow", belt: false, handL: [24, 50], handR: [74, 56] }), 0, 0, -1);
  img.put(L, 0, 0, 0);
  return img;
});

// ------------------------------------------------------------------ creature
pic("golem", "golem", "Golem di pietra", () => {
  const img = new Img(S, S);
  P.sky(img, "storm", 100); P.ground(img, 100, [0, 11, 12]);
  const L = new Img(S, S), st = [0, 11, 12, 15];
  L.poly([[44, 52], [84, 52], [90, 96], [38, 96]], st, { k: 0.9 });
  L.poly([[50, 22], [78, 22], [80, 48], [48, 48]], st);
  L.rect(54, 32, 59, 35, 3); L.rect(69, 32, 74, 35, 3);
  for (const s of [-1, 1]) { L.poly([[64 + s * 20, 52], [64 + s * 36, 58], [64 + s * 40, 92], [64 + s * 28, 92]], st, { flip: s > 0 }); L.ell(64 + s * 34, 98, 9, 8, st); }
  L.poly([[42, 96], [60, 96], [58, 124], [40, 124]], st); L.poly([[68, 96], [86, 96], [88, 124], [70, 124]], st);
  for (const [a, b, c, d] of [[48, 60, 60, 74], [70, 64, 80, 82], [52, 26, 60, 30], [58, 84, 66, 94]]) L.line(a, b, c, d, 0);
  L.speck(36, 20, 92, 124, 5, 0.02, 8);
  img.put(L, 0, 0, 0);
  return img;
});

pic("tirannosauro", "tirannosauro", "Tirannosauro Atrox", () => {
  const img = new Img(S, S);
  P.sky(img, "dusk", 100); P.mountains(img, 100, [[100, 50, 30]], [0, 9, 2]);
  for (let x = 0; x < S; x += 26) P.tree(img, x + 6, 104, 0.9, [0, 9, 5]);
  P.ground(img, 102, [0, 9, 5]);
  const L = new Img(S, S), sk = [0, 9, 5, 13];
  L.curve([[60, 74], [100, 72], [126, 100]], 10, sk, 2);
  L.ell(56, 70, 24, 18, sk, { add: 0.1 });
  L.cap(54, 80, 66, 104, 9, sk, 6); L.cap(66, 104, 60, 122, 5, sk); L.rect(52, 121, 66, 124, 0);
  L.curve([[44, 60], [30, 44], [32, 28]], 10, sk, 9);
  L.ell(30, 26, 20, 12, sk, { add: 0.1 });
  L.poly([[10, 30], [42, 32], [44, 40], [12, 38]], sk);
  for (let x = 12; x < 40; x += 4) { L.set(x, 33, 1); L.set(x + 2, 37, 1); }
  L.rect(28, 21, 31, 22, 7); L.set(30, 21, 0);
  L.cap(42, 72, 34, 80, 2.5, sk); L.cap(34, 80, 32, 84, 1.5, 1);
  img.put(L, 0, 0, 0);
  return img;
});

pic("basilisco", "basilisco", "Basilisco", () => {
  const img = new Img(S, S);
  P.stoneWall(img, 0, 90, [0, 11, 12, 15]); P.ground(img, 90, [0, 11, 12]);
  // statua pietrificata
  img.put(P.human({ skin: [11, 12, 15], body: [11, 12, 15], legs: [11, 12, 15], weapon: "sword", eyes: 11, cx: 100, hy: 40, hr: 8 }), 0, 0, 0);
  const L = new Img(S, S), sk = [0, 9, 5, 7];
  L.curve([[90, 116], [60, 126], [20, 112]], 4, sk, 8);
  L.ell(40, 102, 24, 12, sk);
  for (const x of [26, 50]) { L.cap(x, 108, x - 6, 120, 3.5, sk); L.rect(x - 10, 119, x - 2, 121, 0); }
  L.curve([[28, 96], [16, 80], [26, 66]], 7, sk, 6);
  L.ell(30, 62, 13, 9, sk);
  L.poly([[22, 54], [28, 40], [34, 52], [40, 42], [42, 56]], [2, 10, 7]);
  L.ell(34, 60, 4, 3, 7); L.ell(34, 60, 1.5, 2.5, 0);
  L.line(40, 66, 50, 62, 2);
  img.put(L, 0, 0, 0);
  for (let k = 0; k < 6; k++) img.line(38, 60, 92 - k, 34 + k * 2, k % 2 ? 7 : 1);
  return img;
});

pic("idra", "idra", "Idra delle paludi", () => {
  const img = new Img(S, S);
  P.sky(img, "swamp", 90); img.vgrad(90, 127, [0, 9, 5], 0, 127, true);
  for (let y = 94; y < 128; y += 6) for (let x = (y * 7) % 13; x < S; x += 24) img.rect(x, y, x + 8, y, 13);
  const L = new Img(S, S), sk = [0, 6, 5, 13];
  L.ell(64, 104, 36, 18, sk);
  for (const [x, y, c] of [[20, 30, 36], [44, 18, 50], [64, 10, 64], [86, 18, 78], [108, 30, 92]]) {
    L.curve([[c, 96], [x + (c - x) * 0.2 + (x < 64 ? -10 : 10), 60], [x, y + 8]], 5, sk, 4);
    L.ell(x, y, 9, 7, sk);
    L.poly([[x - 6, y + 2], [x + (x < 64 ? -16 : 16), y + 6], [x - 4, y + 8]], sk);
    L.set(x - 2, y - 2, 7); L.set(x + 2, y - 2, 7);
  }
  img.put(L, 0, 0, 0);
  return img;
});

pic("spettro", "spettro,spettri,zombi", "Spettro del Re", () => {
  const img = new Img(S, S);
  P.stoneWall(img, 0, 127, [0, 0, 6, 11]);
  P.glow(img, 64, 60, 56, [0, 6, 6, 14]);
  const L = new Img(S, S), gh = [6, 14, 3, 1];
  L.poly([[40, 40], [88, 40], [104, 120], [90, 104], [78, 124], [64, 106], [50, 124], [38, 104], [24, 120]], gh, { k: 0.8 });
  L.cap(44, 52, 18, 76, 6, gh, 3); L.cap(84, 52, 110, 76, 6, gh, 3);
  L.ell(64, 34, 16, 18, gh, { add: 0.15 });
  L.ell(57, 34, 4, 5, 0); L.ell(71, 34, 4, 5, 0); L.set(57, 34, 3); L.set(71, 34, 3);
  L.ell(64, 46, 4, 5, 0);
  L.poly([[48, 20], [80, 20], [80, 12], [72, 16], [64, 6], [56, 16], [48, 12]], R.gold);
  img.put(L, 0, 0, 6);
  return img;
});

pic("ragno", "ragni,centipedi,caverne", "Ragni giganti", () => {
  const img = new Img(S, S);
  img.fill(0); P.glow(img, 64, 64, 64, [0, 0, 11]);
  for (let k = 0; k < 12; k++) { const a = k * Math.PI / 6; img.line(64, 50, 64 + Math.cos(a) * 90, 50 + Math.sin(a) * 90, 12); }
  for (let r = 12; r < 90; r += 14) for (let k = 0; k < 12; k++) { const a = k * Math.PI / 6, b = a + Math.PI / 6; img.line(64 + Math.cos(a) * r, 50 + Math.sin(a) * r, 64 + Math.cos(b) * r, 50 + Math.sin(b) * r, 11); }
  const L = new Img(S, S), sk = [0, 0, 4, 10];
  for (const s of [-1, 1]) for (let i = 0; i < 4; i++) {
    const kx = 64 + s * (26 + i * 6), ky = 62 - 20 + i * 8;
    L.cap(64 + s * 8, 70 + i * 3, kx, ky, 3, sk, 2.5); L.cap(kx, ky, 64 + s * (40 + i * 8), 100 + i * 7, 2.5, sk, 1.5);
  }
  L.ell(64, 92, 20, 22, sk); L.rect(56, 82, 72, 84, 2); L.rect(62, 86, 66, 100, 2);
  L.ell(64, 66, 12, 10, sk);
  for (const [x, y] of [[59, 62], [64, 60], [69, 62], [61, 66], [67, 66]]) L.set(x, y, 2);
  L.cap(60, 74, 58, 80, 1.5, 1); L.cap(68, 74, 70, 80, 1.5, 1);
  img.put(L, 0, 0, 0);
  return img;
});

pic("serpente", "serpenti,sauridi", "Serpenti", () => {
  const img = new Img(S, S);
  P.sky(img, "dusk", 96); img.vgrad(96, 127, [9, 8, 7], 0, 127, true);
  const L = new Img(S, S), sk = [0, 5, 13, 7];
  L.ell(64, 112, 40, 10, sk); L.ell(64, 104, 30, 8, sk, { add: 0.1 });
  L.curve([[70, 100], [100, 70], [62, 46]], 7, sk, 6);
  L.ell(58, 34, 22, 16, sk);
  for (let y = 24; y < 48; y += 4) L.line(46, y, 70, y, 5);
  L.ell(58, 44, 10, 8, sk); L.ell(54, 40, 2, 2, 7); L.ell(62, 40, 2, 2, 7); L.set(54, 40, 0); L.set(62, 40, 0);
  L.line(58, 52, 58, 58, 2); L.line(58, 58, 55, 61, 2); L.line(58, 58, 61, 61, 2);
  img.put(L, 0, 0, 0);
  return img;
});

pic("grifone", "grifoni,uomini falco,serpenti alati", "Grifoni", () => {
  const img = new Img(S, S);
  P.sky(img, "day", 110); P.mountains(img, 128, [[20, 40, 30], [110, 50, 30]], [0, 11, 12, 15]);
  const L = new Img(S, S), fe = [9, 8, 7, 1], li = [9, 8, 7];
  L.poly([[56, 54], [2, 20], [10, 40], [0, 52], [14, 58], [6, 70], [44, 72]], fe, { k: 0.9 });
  L.poly([[72, 54], [126, 20], [118, 40], [128, 52], [114, 58], [122, 70], [84, 72]], fe, { flip: true });
  L.ell(64, 78, 18, 20, li);
  L.cap(54, 92, 48, 116, 4, li); L.cap(74, 92, 80, 116, 4, li);
  L.ell(64, 48, 12, 12, [12, 15, 1]);
  L.poly([[58, 50], [70, 50], [64, 62]], [8, 7]);
  L.set(59, 45, 0); L.set(69, 45, 0);
  img.put(L, 0, 0, 0);
  return img;
});

pic("rospo", "rospi giganti", "Rospi giganti", () => {
  const img = new Img(S, S);
  P.sky(img, "swamp", 84); img.vgrad(84, 127, [0, 9, 5], 0, 127, true);
  for (const x of [10, 30, 100, 118]) img.cap(x, 127, x + 2, 70, 1.5, [9, 8]);
  const L = new Img(S, S), sk = [0, 9, 5, 13];
  L.ell(64, 88, 42, 28, sk);
  L.ell(64, 98, 26, 16, [8, 7, 7], { add: 0.2 });
  L.ell(42, 62, 10, 10, sk); L.ell(86, 62, 10, 10, sk);
  L.ell(42, 62, 5, 5, 7); L.ell(86, 62, 5, 5, 7); L.rect(39, 62, 45, 62, 0); L.rect(83, 62, 89, 62, 0);
  L.line(30, 84, 98, 84, 0);
  for (const s of [-1, 1]) L.ell(64 + s * 40, 112, 14, 8, sk);
  L.speck(30, 70, 100, 90, 0, 0.04, 6);
  img.put(L, 0, 0, 0);
  return img;
});

// ------------------------------------------------------------------ luoghi
pic("carovana", "carovana,caravanserraglio,deserto,nomadi", "La carovana", () => {
  const img = new Img(S, S);
  P.sky(img, "day", 80); P.sun(img, 100, 26, 12);
  img.vgrad(80, 127, [9, 8, 7], 0, 127, true);
  for (let x = 0; x < S; x++) { const h = 6 * Math.sin(x * 0.05) + 4; for (let y = 80 - h; y < 82; y++) img.tone(x, Math.floor(y), [8, 7], 0.4); }
  const camel = (x, y, s) => {
    const L = new Img(S, S), sk = [9, 8, 7];
    L.ell(x, y, 14 * s, 8 * s, sk); L.ell(x - 2 * s, y - 7 * s, 7 * s, 6 * s, sk);
    L.curve([[x + 12 * s, y - 2 * s], [x + 22 * s, y - 6 * s], [x + 20 * s, y - 18 * s]], 2.5 * s, sk, 2 * s);
    L.ell(x + 22 * s, y - 19 * s, 4 * s, 2.5 * s, sk);
    for (const d of [-9, -5, 6, 10]) L.cap(x + d * s, y + 5 * s, x + d * s, y + 20 * s, 1.8 * s, sk);
    L.rect(x - 8 * s, y - 14 * s, x + 2 * s, y - 10 * s, 2);
    img.put(L, 0, 0, 0);
  };
  camel(26, 92, 0.7); camel(56, 98, 0.85); camel(90, 104, 1.05);
  return img;
});

pic("citta", "babbakesch", "Babbakesch", () => {
  const img = new Img(S, S);
  P.sky(img, "dusk", 90); img.vgrad(90, 127, [9, 8, 7], 0, 127, true);
  const wall = [9, 8, 7, 1];
  for (const [x, w, h, d] of [[6, 24, 50, 1], [30, 30, 60, 1], [62, 30, 74, 1], [94, 28, 54, 1]]) {
    img.poly([[x, 100], [x + w, 100], [x + w, 100 - h], [x, 100 - h]], wall, { k: 0.8 });
    if (d) img.ell(x + w / 2, 100 - h, w / 2 - 2, w / 2.4, [6, 14, 3, 1], { add: 0.1 });
    img.rect(x + w / 2 - 2, 100 - h - w / 2.4 - 6, x + w / 2 + 1, 100 - h - w / 2.4, 7);
    for (let yy = 100 - h + 12; yy < 92; yy += 14) img.rect(x + w / 2 - 3, yy, x + w / 2 + 2, yy + 6, 0);
  }
  img.rect(0, 98, 127, 106, 8); for (let x = 0; x < S; x += 8) img.rect(x, 94, x + 4, 98, 8);
  img.rect(56, 98, 70, 112, 0); img.ell(63, 98, 7, 6, 0);
  return img;
});

pic("kho", "kho,boscimani,giungla", "Kho", () => {
  const img = new Img(S, S);
  P.sky(img, "day", 90); img.vgrad(90, 127, [0, 9, 5], 0, 127, true);
  for (let x = -6; x < S; x += 20) P.tree(img, x + 10, 98, 1.3, [0, 5, 13]);
  const hut = (x, y) => {
    for (const d of [-12, 12]) img.rect(x + d - 1, y, x + d + 1, y + 26, 9);
    img.rect(x - 16, y - 2, x + 16, y + 1, 8);
    img.poly([[x - 14, y - 2], [x + 14, y - 2], [x + 14, y - 18], [x - 14, y - 18]], R.wood, { k: 0.7 });
    img.poly([[x - 20, y - 16], [x + 20, y - 16], [x, y - 34]], [9, 8, 7], { k: 0.8 });
    img.rect(x - 4, y - 12, x + 4, y - 2, 0);
  };
  hut(34, 84); hut(92, 92);
  img.cap(70, 127, 74, 100, 1, 8); img.ell(72, 98, 4, 6, [2, 8, 7, 1], { add: 0.3 });
  return img;
});

pic("kebir", "kebir,pirati,nave", "Kebir", () => {
  const img = new Img(S, S);
  P.sky(img, "dusk", 70); P.sun(img, 28, 60, 12);
  for (const [x, h] of [[70, 40], [84, 30], [100, 50], [116, 34]]) { img.rect(x, 70 - h, x + 10, 70, 15); img.ell(x + 5, 70 - h, 6, 5, [6, 14, 3]); }
  img.rect(103, 6, 107, 22, 15);
  img.vgrad(70, 127, [0, 6, 14], 0, 127, true);
  for (let y = 74; y < 128; y += 5) for (let x = (y * 5) % 11; x < S; x += 18) img.rect(x, y, x + 6, y, 3);
  const L = new Img(S, S);
  L.poly([[14, 96], [100, 96], [88, 112], [24, 112]], R.wood, { vert: true, k: 0.6 });
  L.rect(54, 30, 56, 96, 9); L.rect(30, 50, 34, 96, 9);
  L.poly([[57, 34], [86, 40], [84, 86], [57, 88]], [11, 12, 15, 1], { flip: true }); L.poly([[35, 52], [52, 56], [52, 88], [35, 88]], [11, 12, 15, 1]);
  L.rect(66, 54, 74, 66, 2);
  L.poly([[56, 24], [70, 27], [56, 30]], 2);
  img.put(L, 0, 0, 0);
  return img;
});

pic("arena", "arena,campione", "Arena dei campioni", () => {
  const img = new Img(S, S);
  P.sky(img, "day", 60); P.sun(img, 100, 20, 9);
  for (let y = 26; y < 76; y++) for (let x = 0; x < S; x++) img.tone(x, y, [9, 8, 7, 1], 0.6 + 0.2 * Math.sin(x * 0.3));
  for (let x = 4; x < S; x += 14) { img.rect(x, 34, x + 8, 50, 0); img.ell(x + 4, 34, 4, 4, 0); img.rect(x, 58, x + 8, 72, 9); }
  img.speck(0, 52, 127, 56, 2, 0.3, 4).speck(0, 52, 127, 56, 14, 0.2, 7).speck(0, 52, 127, 56, 1, 0.2, 9);
  img.vgrad(76, 127, [8, 7, 7], 0, 127, true);
  img.put(P.human({ skin: R.dskin, bare: true, legs: [0, 9, 8], helm: R.gold, weapon: "scimitar", shield: R.gold, shieldMark: 2, hy: 46, hr: 9 }), 0, 0, 0);
  return img;
});

pic("armaiolo", "armaiolo,corazza,cotta,giaco,armatura", "L'Armaiolo", () => {
  const img = new Img(S, S);
  P.stoneWall(img, 0, 96, [0, 9, 11, 12]); P.planks(img, 96, 127);
  P.glow(img, 20, 70, 20, [2, 8, 7]);
  img.rect(4, 74, 36, 96, 11);
  const L = new Img(S, S), st = R.steel;
  L.rect(72, 30, 74, 118, 9); L.rect(60, 116, 86, 118, 9);
  L.ell(73, 24, 11, 10, st, { add: 0.1 }); L.rect(66, 24, 80, 26, 0); L.rect(72, 26, 74, 32, 11);
  L.poly([[54, 38], [92, 38], [88, 84], [58, 84]], st, { k: 0.9 });
  L.ell(54, 40, 9, 7, st); L.ell(92, 40, 9, 7, st);
  for (let y = 48; y < 82; y += 8) L.line(60, y, 86, y, 11);
  L.poly([[30, 104], [58, 104], [52, 112], [36, 112]], [0, 11, 12]);
  L.rect(40, 112, 48, 122, 11);
  L.cap(102, 60, 112, 100, 2, R.wood); L.poly([[100, 56], [118, 50], [116, 66]], st);
  img.put(L, 0, 0, 0);
  return img;
});

// ------------------------------------------------------------------ oggetti
pic("spada", "spada", "Spada magica", () => {
  const img = new Img(S, S);
  img.fill(0); P.glow(img, 64, 60, 62, [0, 6, 6, 14]);
  img.speck(0, 0, 127, 127, 3, 0.01, 5).speck(0, 0, 127, 127, 1, 0.006, 2);
  const L = new Img(S, S);
  L.poly([[58, 96], [70, 96], [70, 22], [64, 8], [58, 22]], [11, 12, 15, 1], { k: 1 });
  L.line(64, 14, 64, 94, 1);
  L.poly([[38, 96], [90, 96], [86, 104], [42, 104]], R.gold);
  L.ell(64, 100, 5, 5, [2, 10, 1], { add: 0.3 });
  L.cap(64, 104, 64, 118, 3.5, R.leather);
  L.ell(64, 122, 5, 5, R.gold);
  img.put(L, 0, 0, 0);
  for (const [x, y] of [[50, 30], [78, 50], [52, 70]]) { img.line(x - 3, y, x + 3, y, 1); img.line(x, y - 3, x, y + 3, 1); }
  return img;
});

pic("filtro", "filtro,guaritore,spezie", "Filtro magico", () => {
  const img = new Img(S, S);
  P.stoneWall(img, 0, 90, [0, 9, 11, 12]); P.planks(img, 90, 127);
  for (const [x, h, c] of [[16, 20, R.red], [108, 26, R.green]]) { img.ell(x, 92 - h / 2, 8, h / 2, c); img.rect(x - 2, 92 - h - 6, x + 2, 92 - h, 9); }
  const L = new Img(S, S);
  L.rect(56, 22, 72, 32, 9); L.rect(58, 32, 70, 50, [11, 12, 15][1]);
  L.ell(64, 80, 30, 28, [6, 4, 14, 3], { add: 0.1 });
  L.ell(64, 88, 24, 16, [6, 14, 3, 1], { add: 0.25 });
  L.ell(50, 66, 5, 8, 1);
  img.put(L, 0, 0, 0);
  img.speck(40, 10, 90, 40, 1, 0.02, 3);
  return img;
});

pic("teschio", "sei morto,morte,catacombe", "Le catacombe", () => {
  const img = new Img(S, S);
  P.stoneWall(img, 0, 127, [0, 0, 11, 12]);
  for (const x of [20, 108]) { img.rect(x - 4, 80, x + 4, 110, 15); img.ell(x, 74, 2, 4, [8, 7, 1], { add: 0.3 }); P.glow(img, x, 74, 0, [7]); }
  const L = new Img(S, S), bn = R.bone;
  L.ell(64, 56, 30, 30, bn, { add: 0.1 });
  L.poly([[44, 74], [84, 74], [80, 96], [48, 96]], bn);
  L.ell(52, 58, 8, 9, 0); L.ell(76, 58, 8, 9, 0);
  L.poly([[64, 66], [59, 78], [69, 78]], 0);
  for (let x = 50; x <= 78; x += 6) L.rect(x, 86, x + 3, 94, 1);
  L.rect(48, 84, 80, 85, 0); L.rect(48, 95, 80, 96, 0);
  L.cap(20, 116, 108, 124, 4, bn); L.cap(20, 124, 108, 114, 4, bn);
  img.put(L, 0, 0, 0);
  return img;
});

pic("corona", "eroe", "Sei un Eroe!", () => {
  const img = new Img(S, S);
  img.fill(6); for (let k = 0; k < 24; k++) { const a = k * Math.PI / 12; img.poly([[64, 64], [64 + Math.cos(a) * 100, 64 + Math.sin(a) * 100], [64 + Math.cos(a + 0.13) * 100, 64 + Math.sin(a + 0.13) * 100]], k % 2 ? 4 : 14); }
  P.glow(img, 64, 64, 40, [14, 3, 1]);
  const L = new Img(S, S);
  L.poly([[24, 90], [104, 90], [108, 42], [88, 62], [76, 30], [64, 56], [52, 30], [40, 62], [20, 42]], R.gold, { k: 0.9 });
  L.rect(24, 82, 104, 92, 8);
  for (const [x, y, c] of [[40, 86, 2], [64, 86, 14], [88, 86, 5], [64, 66, 2]]) L.ell(x, y, 4, 4, c === 2 ? [2, 10, 1] : c === 14 ? [6, 14, 1] : [5, 13, 1], { add: 0.3 });
  for (const [x, y] of [[20, 42], [52, 30], [76, 30], [108, 42]]) L.ell(x, y, 3, 3, [8, 7, 1]);
  img.put(L, 0, 0, 0);
  return img;
});

// ------------------------------------------------------------------ mezzibusti dei personaggi
// sostituiscono le figure intere: stesso nome, parole chiave e didascalia
const repic = (name, draw) => { const p = pics.find((x) => x.name === name); p.draw = draw; };
const scene = (bg, b) => () => { const img = new Img(S, S); bg(img); img.put(P.bust(b), 0, 0, 0); return img; };
const dusk = (img) => { P.sky(img, "dusk", 100); P.mountains(img, 104, [[16, 40, 30], [112, 46, 30]], [0, 6, 4, 14]); };
const night = (img) => { P.sky(img, "night", 110); P.moon(img, 104, 22, 12); };
const wall = (r) => (img) => P.stoneWall(img, 0, 127, r);
const forest = (img) => { P.sky(img, "day", 100); for (let x = 0; x < S; x += 26) P.tree(img, x + 8, 110, 1.2, [0, 5, 13]); };
const storm = (img) => { P.sky(img, "storm", 104); P.mountains(img, 108, [[24, 50, 40], [104, 40, 36]], [0, 11, 12, 15]); };

repic("avventuriero", scene((img) => { P.sky(img, "day", 100); P.hills(img, 104, [0, 9, 5, 13], 3); P.tree(img, 14, 112, 1.2); P.tree(img, 116, 110, 1); },
  { cloth: [0, 9, 8, 10], collar: 9, cloak: [0, 9, 5], hair: [0, 9, 8], eyes: 14, weapon: "sword", mustache: true }));
repic("conte", scene((img) => { P.stoneWall(img, 0, 127, [0, 0, 11, 2]); img.rect(36, 0, 92, 127, 2); for (let y = 0; y < 128; y += 4) img.rect(36, y, 92, y, 9); P.torch(img, 14, 44); P.torch(img, 114, 44); },
  { cloth: [0, 0, 11, 12], armor: [0, 11, 12], cloak: [0, 6, 4], hair: [0, 0, 11], skin: [11, 12, 15], crown: true, eyes: 2, angry: true, mouth: 0, scar: true, weapon: "sword" }));
repic("barbaro", scene(storm, { bare: true, skin: [9, 8, 10], hair: [9, 8, 7], hairStyle: "wild", beard: "short", helm: [11, 12, 15], horns: [11, 12, 15, 1], weapon: "axe", eyes: 6, angry: true, jaw: 21 }));
repic("armigero", scene((img) => { P.sky(img, "storm", 104); img.rect(0, 50, 127, 127, 11); for (let x = 0; x < 128; x += 10) img.rect(x, 44, x + 5, 50, 11); },
  { cloth: [0, 2, 10], chain: true, armor: [0, 11, 12, 15], helm: [11, 12, 15], nasal: true, eyes: 6, weapon: "spear", mouth: 0 }));
repic("bandito", scene((img) => { P.sky(img, "night", 110); P.moon(img, 22, 24, 10); P.pine(img, 110, 128, 2); },
  { cloth: [0, 9, 8], hood: [0, 0, 11], cloak: [0, 0, 11], hair: [0, 0, 9], beard: "short", eyes: 7, scar: true, angry: true, weapon: "sword" }));
repic("vampiro", scene((img) => { P.sky(img, "night", 128); P.moon(img, 64, 40, 34); },
  { cloth: [0, 0, 11], collar: 2, cloak: [0, 2, 10], skin: [11, 12, 15], hair: [0, 0, 11], glow: 2, fangs: true, mouth: 2, jaw: 17 }));
repic("elfo", scene(forest, { cloth: [0, 5, 13], collar: 7, cloak: [0, 9, 5], hair: [8, 7, 1], hairStyle: "long", ears: "pointed", eyes: 5, circlet: [5, 13, 1], weapon: "bow", jaw: 17 }));
repic("nano", scene((img) => { P.stoneWall(img, 0, 127, [0, 9, 11, 12], 10, 18); P.torch(img, 14, 40); P.torch(img, 114, 40); },
  { cloth: [0, 6, 14], armor: [11, 12, 15], helm: [11, 12, 15], nasal: true, hair: [2, 8, 7], beard: "long", bigNose: true, eyes: 6, jaw: 22, weapon: "axe" }));
repic("orco", scene((img) => { P.sky(img, "swamp", 110); P.pine(img, 14, 128, 1.8); P.pine(img, 114, 128, 1.6); },
  { skin: [0, 9, 5, 13], bare: true, ears: "pointed", tusks: true, glow: 7, angry: true, hair: [0, 0, 11], hairStyle: "wild", bigNose: true, weapon: "axe", jaw: 21, mouth: 0 }));
repic("troll", scene(wall([0, 9, 11, 12]), { skin: [0, 11, 5, 13], bare: true, bigNose: true, tusks: true, eyes: 2, angry: true, hair: [0, 9], hairStyle: "wild", jaw: 23, mouth: 0 }));
repic("tigroide", scene((img) => { P.sky(img, "dusk", 110); for (let x = 0; x < S; x += 18) img.cap(x + 4, 127, x + 8, 50, 2, [0, 9, 5]); }, {
  skin: [9, 8, 7], bare: true, ears: "pointed", eyes: 5, glow: 13, hair: [0, 9, 8], hairStyle: "wild", mouth: 0, fangs: true, weapon: "spear" }));
repic("demone", scene((img) => { P.sky(img, "fire", 128); img.speck(0, 80, 127, 127, 7, 0.02, 3); },
  { skin: [0, 2, 10], bare: true, horns: [11, 12, 15, 1], glow: 7, angry: true, fangs: true, ears: "pointed", mouth: 0, cloak: [0, 0, 4], weapon: "spear" }));
repic("minotauro", () => {
  const img = new Img(S, S);
  P.stoneWall(img, 0, 127, [0, 9, 8, 10], 12, 20);
  const L = P.bust({ skin: [0, 9, 8, 10], bare: true, horns: [12, 15, 1], hair: [0, 9], hairStyle: "wild", glow: 2, angry: true, jaw: 22, weapon: "axe" });
  L.ell(64, 66, 13, 9, [9, 8, 10]); L.ell(58, 68, 2, 2, 0); L.ell(70, 68, 2, 2, 0); L.curve([[60, 74], [64, 79], [68, 74]], 1, 7);
  img.put(L, 0, 0, 0);
  return img;
});
repic("gigante", scene((img) => { P.sky(img, "day", 110); P.mountains(img, 128, [[20, 40, 30], [108, 50, 30]], [6, 14, 3]); },
  { cloth: [0, 9, 8], hair: [0, 9], hairStyle: "wild", beard: "long", bigNose: true, eyes: 6, jaw: 23 }));
repic("arena", scene((img) => { P.sky(img, "day", 60); for (let y = 30; y < 128; y++) for (let x = 0; x < S; x++) img.tone(x, y, [9, 8, 7, 1], 0.6 + 0.2 * Math.sin(x * 0.3)); for (let x = 4; x < S; x += 14) img.rect(x, 40, x + 8, 60, 0); },
  { skin: [0, 9, 8, 10], bare: true, helm: [9, 8, 7, 1], eyes: 9, beard: "short", hair: [0, 0, 9], weapon: "sword", angry: true }));

pic("dama", "danzatrice,figlia,moglie,amante", "La dama rapita", scene((img) => { P.stoneWall(img, 0, 127, [0, 6, 4, 10]); img.rect(84, 14, 116, 60, 0); img.rect(99, 14, 101, 60, 9); img.rect(84, 36, 116, 37, 9); P.moon(img, 108, 26, 5); },
  { cloth: [0, 2, 10], collar: 7, hair: [0, 9, 8], hairStyle: "long", eyes: 5, mouth: 10, jaw: 16, amulet: true, circlet: [2, 10, 1] }));
pic("tesoro", "statua d'oro,perla nera,gemma rossa,pietra ircana,tesoro", "Il tesoro perduto", () => {
  const img = new Img(S, S);
  img.fill(0); P.glow(img, 64, 70, 60, [0, 9, 9, 8]);
  const L = new Img(S, S);
  L.poly([[22, 84], [106, 84], [100, 120], [28, 120]], [0, 9, 8], { k: 0.8 });
  L.rect(22, 84, 106, 88, 7); L.rect(60, 84, 68, 96, 7); L.rect(28, 104, 100, 106, 9);
  for (let i = 0; i < 9; i++) L.ell(30 + i * 9, 82 - (i % 3) * 3, 5, 4, [9, 8, 7, 1], { add: 0.2 });
  L.ell(48, 62, 10, 10, [0, 6, 4, 14], { add: 0.2 });
  L.poly([[64, 30], [82, 52], [64, 76], [46, 52]], [0, 2, 10, 1], { k: 1 }); L.line(64, 30, 64, 76, 10); L.line(46, 52, 82, 52, 2);
  L.ell(84, 66, 8, 8, [2, 10, 1], { add: 0.2 });
  img.put(L, 0, 0, 0);
  for (const [x, y] of [[40, 40], [92, 44], [70, 24]]) { img.line(x - 3, y, x + 3, y, 1); img.line(x, y - 3, x, y + 3, 1); }
  return img;
});
pic("pergamena", "pergamena", "La pergamena sacra", () => {
  const img = new Img(S, S);
  P.stoneWall(img, 0, 90, [0, 9, 11, 12]); P.planks(img, 90, 127);
  P.torch(img, 112, 40);
  const L = new Img(S, S), pa = [9, 8, 7, 1];
  L.poly([[24, 34], [104, 34], [104, 100], [24, 100]], pa, { k: 0.6, add: 0.3 });
  L.cap(20, 30, 108, 30, 6, pa); L.cap(20, 104, 108, 104, 6, pa);
  for (let y = 44; y < 92; y += 7) for (let x = 32; x < 96; x += 8) L.rect(x, y, x + 4 + ((x * y) % 3), y + 1, 9);
  L.ell(64, 96, 7, 6, [0, 2, 10], { add: 0.2 });
  img.put(L, 0, 0, 0);
  return img;
});

// ------------------------------------------------------------------ icone (16x16)
pic("icona_filtro", "", "Filtro magico", () => {
  const L = new Img(16, 16);
  L.rect(6, 1, 9, 2, 9); L.rect(6, 3, 9, 5, 15);
  L.ell(7.5, 10.5, 6, 5.5, [6, 14, 3, 1], { add: 0.2 });
  L.set(5, 8, 1); L.set(4, 9, 1);
  const o = new Img(16, 16); o.put(L, 0, 0, 0); return o;
});
pic("icona_pane", "", "Pane elfico", () => {
  const L = new Img(16, 16);
  L.ell(8, 9, 7, 5, [9, 8, 7], { add: 0.2 });
  for (const x of [5, 8, 11]) L.line(x - 1, 6, x + 1, 10, 9);
  L.rect(2, 13, 13, 14, 5); L.set(3, 12, 13); L.set(12, 12, 13);
  const o = new Img(16, 16); o.put(L, 0, 0, 0); return o;
});

module.exports = pics;
