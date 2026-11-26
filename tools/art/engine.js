// Piccolo motore di disegno per le illustrazioni dell'Espansione.
// Disegna su una bitmap a colori indicizzati con la tavolozza del C64 (16 colori):
// forme piene, volumi ombreggiati con retinatura ordinata (stile C64), contorni.
"use strict";

const PALETTE = [0x000000, 0xffffff, 0x68372b, 0x70a4b2, 0x6f3d86, 0x588d43, 0x352879, 0xb8c76f,
  0x6f4f25, 0x433900, 0x9a6759, 0x444444, 0x6c6c6c, 0x9ad284, 0x6c5eb5, 0x959595];

// nomi dei colori
const C = { k: 0, w: 1, r: 2, c: 3, p: 4, g: 5, b: 6, y: 7, o: 8, n: 9, lr: 10, dg: 11, gr: 12, lg: 13, lb: 14, lgr: 15 };

// scale di tono (dal piu' scuro al piu' chiaro)
const R = {
  grey: [0, 11, 12, 15, 1], stone: [0, 11, 12, 15], steel: [11, 12, 15, 1], skin: [9, 8, 10, 15],
  dskin: [0, 9, 8, 10], red: [0, 9, 2, 10], green: [0, 9, 5, 13], dgreen: [0, 11, 5, 13],
  blue: [0, 6, 14, 3], purple: [0, 6, 4, 10], gold: [9, 8, 7, 1], wood: [0, 9, 8],
  fire: [2, 8, 7, 1], night: [0, 6, 14], bone: [11, 12, 15, 1], leather: [0, 9, 8, 10],
  cyan: [6, 14, 3, 1], sand: [9, 8, 7], lgreen: [5, 13, 7],
};

// matrice di Bayer 4x4 per la retinatura
const BAYER = [0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5].map((v) => (v + 0.5) / 16);

class Img {
  constructor(w, h, fill = -1) { this.w = w; this.h = h; this.p = new Int8Array(w * h).fill(fill); }
  get(x, y) { return x < 0 || y < 0 || x >= this.w || y >= this.h ? -1 : this.p[y * this.w + x]; }
  set(x, y, c) { x |= 0; y |= 0; if (c >= 0 && x >= 0 && y >= 0 && x < this.w && y < this.h) this.p[y * this.w + x] = c; }
  // tono t in [0,1] su una scala, retinato in base alla posizione
  tone(x, y, ramp, t) {
    t = Math.max(0, Math.min(0.9999, t)) * (ramp.length - 1);
    const i = Math.floor(t), f = t - i;
    const c = f > BAYER[(y & 3) * 4 + (x & 3)] ? ramp[Math.min(i + 1, ramp.length - 1)] : ramp[i];
    this.set(x, y, c);
  }
  fill(c) { this.p.fill(c); return this; }
  rect(x0, y0, x1, y1, c) { for (let y = y0; y <= y1; y++) for (let x = x0; x <= x1; x++) this.set(x, y, c); return this; }
  // sfumatura verticale tra y0 e y1 su una scala
  vgrad(y0, y1, ramp, x0 = 0, x1 = this.w - 1, rev = false) {
    for (let y = y0; y <= y1; y++) for (let x = x0; x <= x1; x++) {
      const t = (y - y0) / Math.max(1, y1 - y0);
      this.tone(x, y, ramp, rev ? 1 - t : t);
    }
    return this;
  }
  // ellisse piena; se ramp e' un array viene ombreggiata come una sfera (luce in alto a sinistra)
  ell(cx, cy, rx, ry, c, opt = {}) {
    const L = norm(opt.light || [-0.55, -0.6, 0.6]);
    for (let y = Math.floor(cy - ry); y <= Math.ceil(cy + ry); y++)
      for (let x = Math.floor(cx - rx); x <= Math.ceil(cx + rx); x++) {
        const dx = (x + 0.5 - cx) / rx, dy = (y + 0.5 - cy) / ry, d = dx * dx + dy * dy;
        if (d > 1) continue;
        if (typeof c === "number") { this.set(x, y, c); continue; }
        const z = Math.sqrt(1 - d);
        const t = Math.max(0, dx * L[0] + dy * L[1] + z * L[2]) * (opt.k || 1) + (opt.add || 0);
        this.tone(x, y, c, t);
      }
    return this;
  }
  // poligono pieno; con una scala viene ombreggiato da sinistra (chiaro) a destra (scuro)
  poly(pts, c, opt = {}) {
    let y0 = Infinity, y1 = -Infinity;
    for (const [, y] of pts) { y0 = Math.min(y0, y); y1 = Math.max(y1, y); }
    for (let y = Math.floor(y0); y <= Math.ceil(y1); y++) {
      const xs = [], yy = y + 0.5;
      for (let i = 0; i < pts.length; i++) {
        const [ax, ay] = pts[i], [bx, by] = pts[(i + 1) % pts.length];
        if ((ay <= yy && by > yy) || (by <= yy && ay > yy)) xs.push(ax + (yy - ay) * (bx - ax) / (by - ay));
      }
      xs.sort((a, b) => a - b);
      for (let k = 0; k + 1 < xs.length; k += 2) {
        const xa = Math.round(xs[k]), xb = Math.round(xs[k + 1]) - 1;
        for (let x = xa; x <= xb; x++) {
          if (typeof c === "number") this.set(x, y, c);
          else {
            const u = xb > xa ? (x - xa) / (xb - xa) : 0.5;
            const v = (y - y0) / Math.max(1, y1 - y0);
            const t = opt.vert ? 1 - v * (opt.k || 1) : (opt.flip ? u : 1 - u) * (opt.k || 0.9) + (opt.add || 0.1) - (opt.vfade || 0) * v;
            this.tone(x, y, c, t);
          }
        }
      }
    }
    return this;
  }
  // capsula (segmento spesso, arrotondato) ombreggiata come un cilindro: braccia, gambe, colli, code
  cap(x1, y1, x2, y2, r1, c, r2 = r1, opt = {}) {
    const L = norm(opt.light || [-0.55, -0.6, 0.6]);
    const dx = x2 - x1, dy = y2 - y1, len2 = dx * dx + dy * dy || 1;
    const rm = Math.max(r1, r2);
    for (let y = Math.floor(Math.min(y1, y2) - rm); y <= Math.ceil(Math.max(y1, y2) + rm); y++)
      for (let x = Math.floor(Math.min(x1, x2) - rm); x <= Math.ceil(Math.max(x1, x2) + rm); x++) {
        const px = x + 0.5, py = y + 0.5;
        let t = ((px - x1) * dx + (py - y1) * dy) / len2;
        t = Math.max(0, Math.min(1, t));
        const qx = x1 + t * dx, qy = y1 + t * dy, r = r1 + (r2 - r1) * t;
        const ox = (px - qx) / r, oy = (py - qy) / r, d = ox * ox + oy * oy;
        if (d > 1) continue;
        if (typeof c === "number") { this.set(x, y, c); continue; }
        const z = Math.sqrt(1 - d);
        this.tone(x, y, c, Math.max(0, ox * L[0] + oy * L[1] + z * L[2]) * (opt.k || 1) + (opt.add || 0));
      }
    return this;
  }
  // linea spessa senza ombreggiatura
  line(x1, y1, x2, y2, c, w = 1) {
    const n = Math.ceil(Math.max(Math.abs(x2 - x1), Math.abs(y2 - y1))) + 1;
    for (let i = 0; i <= n; i++) {
      const x = x1 + (x2 - x1) * i / n, y = y1 + (y2 - y1) * i / n;
      if (w <= 1) this.set(Math.round(x), Math.round(y), c);
      else this.ell(x, y, w / 2, w / 2, c);
    }
    return this;
  }
  // curva (quadratica) spessa: capelli, corna, code
  curve(pts, r1, c, r2 = r1) {
    const [a, b, d] = pts, N = 14;
    let px = a[0], py = a[1];
    for (let i = 1; i <= N; i++) {
      const t = i / N, u = 1 - t;
      const x = u * u * a[0] + 2 * u * t * b[0] + t * t * d[0], y = u * u * a[1] + 2 * u * t * b[1] + t * t * d[1];
      this.cap(px, py, x, y, r1 + (r2 - r1) * (i - 1) / N, c, r1 + (r2 - r1) * i / N);
      px = x; py = y;
    }
    return this;
  }
  // puntinatura casuale (ma ripetibile) in un rettangolo: erba, pietra, stelle
  speck(x0, y0, x1, y1, c, dens, seed = 1) {
    let s = seed * 9301 + 49297;
    const rnd = () => ((s = (s * 9301 + 49297) % 233280) / 233280);
    const n = Math.round((x1 - x0 + 1) * (y1 - y0 + 1) * dens);
    for (let i = 0; i < n; i++) this.set(x0 + Math.floor(rnd() * (x1 - x0 + 1)), y0 + Math.floor(rnd() * (y1 - y0 + 1)), c);
    return this;
  }
  // incolla un livello; con outline >= 0 aggiunge un contorno attorno alle sue forme
  put(layer, dx = 0, dy = 0, outline = 0) {
    if (outline >= 0)
      for (let y = -1; y <= layer.h; y++) for (let x = -1; x <= layer.w; x++) {
        if (layer.get(x, y) >= 0) continue;
        if (layer.get(x - 1, y) >= 0 || layer.get(x + 1, y) >= 0 || layer.get(x, y - 1) >= 0 || layer.get(x, y + 1) >= 0) this.set(x + dx, y + dy, outline);
      }
    for (let y = 0; y < layer.h; y++) for (let x = 0; x < layer.w; x++) { const c = layer.get(x, y); if (c >= 0) this.set(x + dx, y + dy, c); }
    return this;
  }
  // copia specchiata orizzontalmente
  mirror() { const o = new Img(this.w, this.h); for (let y = 0; y < this.h; y++) for (let x = 0; x < this.w; x++) o.p[y * this.w + x] = this.get(this.w - 1 - x, y); return o; }
}

function norm(v) { const l = Math.hypot(...v); return v.map((x) => x / l); }

module.exports = { Img, PALETTE, C, R, BAYER };
