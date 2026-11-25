/* Schermo testo del C64 (40x25) con l'editor di schermo del KERNAL semplificato:
 * colori, inverso, cursore, righe logiche di 80 caratteri (necessarie per impaginare
 * correttamente i testi del gioco), scroll, cambio set di caratteri.
 */
(function (AV) {
  'use strict';
  const { toScreenCode } = AV.petscii;

  // tavolozza "Pepto" (valori di misura del VIC-II, di pubblico dominio)
  const PALETTE = ['#000000', '#ffffff', '#68372b', '#70a4b2', '#6f3d86', '#588d43', '#352879', '#b8c76f',
    '#6f4f25', '#433900', '#9a6759', '#444444', '#6c6c6c', '#9ad284', '#6c5eb5', '#959595'];
  const COLOR_CODES = { 0x90: 0, 0x05: 1, 0x1C: 2, 0x9F: 3, 0x9C: 4, 0x1E: 5, 0x1F: 6, 0x9E: 7,
    0x81: 8, 0x95: 9, 0x96: 10, 0x97: 11, 0x98: 12, 0x99: 13, 0x9A: 14, 0x9B: 15 };
  const W = 40, H = 25;

  class Screen {
    constructor() {
      this.chars = new Uint8Array(W * H);
      this.colors = new Uint8Array(W * H);
      this.link = new Uint8Array(H);         // 1 = la riga continua la precedente (riga logica)
      this.border = 14; this.bg = 6; this.color = 14;
      this.lower = false; this.rvs = false;
      this.x = 0; this.y = 0;
      this.dirty = true;
      this.clear();
    }

    clear() {
      this.chars.fill(0x20); this.colors.fill(this.color); this.link.fill(0);
      this.x = 0; this.y = 0; this.dirty = true;
    }

    lineStart(y) { while (y > 0 && this.link[y]) y--; return y; }
    lineEnd(y) { while (y < H - 1 && this.link[y + 1]) y++; return y; }

    scrollUp() {
      this.chars.copyWithin(0, W); this.colors.copyWithin(0, W);
      this.chars.fill(0x20, W * (H - 1)); this.colors.fill(this.color, W * (H - 1));
      this.link.copyWithin(0, 1); this.link[H - 1] = 0; this.link[0] = 0;
      this.dirty = true;
    }

    /** colonna logica corrente (0-79), come restituita da PLOT/POS */
    get col() { return (this.y - this.lineStart(this.y)) * W + this.x; }

    newRow() {
      if (this.y < H - 1) this.y++;
      else this.scrollUp();
    }

    put(code) {
      const i = this.y * W + this.x;
      this.chars[i] = code | (this.rvs ? 0x80 : 0);
      this.colors[i] = this.color;
      this.dirty = true;
      this.x++;
      if (this.x >= W) {
        this.x = 0;
        const start = this.lineStart(this.y);
        const growing = this.y === start;          // la riga logica passa da 40 a 80 caratteri
        this.newRow();
        if (growing) this.link[this.y] = 1;
        else this.link[this.y] = 0;
      }
    }

    cr() {
      this.y = this.lineEnd(this.y);
      this.x = 0;
      this.rvs = false;
      this.newRow();
      this.link[this.y] = 0;
    }

    /** CHROUT: un byte PETSCII sullo schermo */
    out(b) {
      if (b in COLOR_CODES) { this.color = COLOR_CODES[b]; return; }
      switch (b) {
        case 0x0D: case 0x8D: this.cr(); return;
        case 0x0E: this.lower = true; this.dirty = true; return;
        case 0x8E: this.lower = false; this.dirty = true; return;
        case 0x12: this.rvs = true; return;
        case 0x92: this.rvs = false; return;
        case 0x13: this.x = 0; this.y = 0; return;
        case 0x93: this.clear(); return;
        case 0x11: this.newRow(); return;
        case 0x91: if (this.y > 0) this.y--; return;
        case 0x1D:
          this.x++;
          if (this.x >= W) { this.x = 0; this.newRow(); }
          return;
        case 0x9D:
          if (this.x > 0) this.x--;
          else if (this.y > 0) { this.y--; this.x = W - 1; }
          return;
        case 0x14: {                                  // DEL
          if (this.x === 0 && this.y === 0) return;
          this.out(0x9D);
          const i = this.y * W + this.x;
          this.chars.copyWithin(i, i + 1, this.y * W + W); this.chars[this.y * W + W - 1] = 0x20;
          this.dirty = true;
          return;
        }
        case 0x94: {                                  // INST
          const i = this.y * W + this.x, e = this.y * W + W - 1;
          this.chars.copyWithin(i + 1, i, e); this.chars[i] = 0x20; this.dirty = true;
          return;
        }
      }
      const sc = toScreenCode(b);
      if (sc >= 0) this.put(sc);
    }

    print(bytes) { for (const b of bytes) this.out(b); }
  }

  /** disegna lo schermo su un canvas 384x272 (con bordo) */
  class Renderer {
    constructor(canvas) {
      this.canvas = canvas;
      canvas.width = 384; canvas.height = 272;
      this.ctx = canvas.getContext('2d');
      this.img = this.ctx.createImageData(384, 272);
      this.glyphs = AV.font.build();
      this.rgb = PALETTE.map(h => [parseInt(h.slice(1, 3), 16), parseInt(h.slice(3, 5), 16), parseInt(h.slice(5, 7), 16)]);
      this.blink = 0;
    }

    draw(scr, showCursor) {
      const d = this.img.data;
      const put = (x, y, c) => { const o = (y * 384 + x) * 4; d[o] = c[0]; d[o + 1] = c[1]; d[o + 2] = c[2]; d[o + 3] = 255; };
      const bc = this.rgb[scr.border & 15], bg = this.rgb[scr.bg & 15];
      for (let y = 0; y < 272; y++) for (let x = 0; x < 384; x++) {
        if (x < 32 || x >= 352 || y < 36 || y >= 236) put(x, y, bc);
      }
      const set = this.glyphs[scr.lower ? 1 : 0];
      const curOn = showCursor && (Math.floor(performance.now() / 333) & 1);
      for (let r = 0; r < 25; r++) for (let c = 0; c < 40; c++) {
        const i = r * 40 + c;
        let code = scr.chars[i];
        let inv = code & 0x80;
        if (curOn && r === scr.y && c === scr.x) inv ^= 0x80;
        const g = set[code & 0x7F];
        const fg = this.rgb[(curOn && r === scr.y && c === scr.x ? scr.color : scr.colors[i]) & 15];
        for (let yy = 0; yy < 8; yy++) {
          let bits = g[yy];
          if (inv) bits = ~bits & 0xFF;
          const py = 36 + r * 8 + yy;
          for (let xx = 0; xx < 8; xx++) put(32 + c * 8 + xx, py, (bits & (0x80 >> xx)) ? fg : bg);
        }
      }
      this.ctx.putImageData(this.img, 0, 0);
    }
  }

  AV.screen = { Screen, Renderer, PALETTE, COLOR_CODES };
})(globalThis.AV = globalThis.AV || {});
