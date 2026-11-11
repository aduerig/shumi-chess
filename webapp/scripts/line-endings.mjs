import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
const root = fileURLToPath(new URL('../', import.meta.url));
const skip = new Set(['node_modules', 'dist', 'build', 'data', 'artifacts']);
let failures = 0;
function visit(directory) {
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    if (skip.has(entry.name)) continue;
    const filename = path.join(directory, entry.name);
    if (entry.isDirectory()) { visit(filename); continue; }
    const original = fs.readFileSync(filename);
    if (original.includes(0)) continue;
    const text = original.toString('utf8');
    const normalized = text.replace(/\r?\n/g, '\r\n');
    if (text !== normalized) {
      if (process.argv.includes('--check')) { console.error(path.relative(root, filename)); failures++; }
      else fs.writeFileSync(filename, normalized, 'utf8');
    }
  }
}
visit(root);
if (failures) { console.error(`${failures} files need CRLF normalization.`); process.exitCode = 1; }
else console.log(process.argv.includes('--check') ? 'All source text uses CRLF.' : 'Source text normalized to CRLF.');
