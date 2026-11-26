// Controlla che le righe stampate dalla mod non superino 39 colonne
// (una riga di 40 caratteri seguita da a capo produce una riga vuota in piu').
// Uso: node tools/checklen.js mods/espansione
const fs = require("fs");
const dir = process.argv[2] || "mods/espansione";
let bad = 0;
for (const f of fs.readdirSync(dir).filter((x) => x.endsWith(".bas"))) {
  fs.readFileSync(dir + "/" + f, "utf8").split(/\r?\n/).forEach((l, i) => {
    const re = /PRINT ((?:"[^"]*"|[^:"])*)/g;
    let m;
    while ((m = re.exec(l))) {
      const s = m[1];
      if (/;\s*$/.test(s)) continue;               // continua sulla stessa riga
      const lits = s.match(/"[^"]*"/g) || [];
      const v = lits.map((x) => x.slice(1, -1)
        .replace(/\{(\d+) rght\}/g, (a, n) => " ".repeat(+n))
        .replace(/\{(\d+) (down|up)\}/g, "")
        .replace(/\{[^}]*\}/g, (c) => (c === "{rght}" || c === "{sspc}" ? " " : ""))).join("");
      const rest = s.replace(/"[^"]*"/g, "");
      const vars = (rest.match(/[A-Z][A-Z0-9]*\$?/g) || []);
      // stima: numeri 6 colonne, stringhe 12
      const est = v.length + vars.reduce((a, x) => a + (x.endsWith("$") ? 12 : 6), 0);
      if (est >= 40) { bad++; console.log(`${f}:${i + 1} [${v.length}${vars.length ? "+" + vars.join(",") : ""}] ${v}`); }
    }
  });
}
process.exit(bad ? 1 : 0);
