#!/usr/bin/env node
/* Genera native/src/font_data.h dal font originale definito in web/js/engine/font.js */
'use strict';
const fs = require('fs');
const path = require('path');
require(path.join(__dirname, '..', 'web', 'js', 'engine', 'font.js'));
const [upper, lower] = globalThis.AV.font.build();
const fmt = set => set.map(g => '  {' + g.map(b => '0x' + b.toString(16).padStart(2, '0')).join(',') + '}').join(',\n');
const out = `// GENERATO da tools/gen_font.js - non modificare a mano.
// Font 8x8 originale del progetto (non e' la ROM Commodore).
#pragma once
#include <cstdint>
static const uint8_t FONT_UPPER[128][8] = {
${fmt(upper)}
};
static const uint8_t FONT_LOWER[128][8] = {
${fmt(lower)}
};
`;
fs.writeFileSync(path.join(__dirname, '..', 'native', 'src', 'font_data.h'), out);
console.log('scritto native/src/font_data.h');
