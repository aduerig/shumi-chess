import { randomUUID } from 'node:crypto';
import { copyFile, mkdir, open, rename, rm } from 'node:fs/promises';
import path from 'node:path';
import { setTimeout as delay } from 'node:timers/promises';
import { fileURLToPath } from 'node:url';

export const FEN_FILE = fileURLToPath(new URL('../../../NNUE/FENs/From_webapp.txt', import.meta.url));

// A single append call can still fail after writing only part of a line. Build
// and flush the complete replacement beside the destination before publishing it.
// The lock also prevents two app instances from overwriting each other's update.
async function replaceFenFile(filename: string, line: string | null) {
  await mkdir(path.dirname(filename), { recursive: true });
  const lockPath = `${filename}.lock`;
  const lock = await open(lockPath, 'wx').catch((error: NodeJS.ErrnoException) => {
    if (error.code === 'EEXIST') throw new Error(`FEN file is locked: ${lockPath}. If no other app is writing, remove this stale lock file and retry.`);
    throw error;
  });
  const temporary = `${filename}.${randomUUID()}.tmp`;
  try {
    if (line !== null) {
      try { await copyFile(filename, temporary); }
      catch (error) { if ((error as NodeJS.ErrnoException).code !== 'ENOENT') throw error; }
    }
    const file = await open(temporary, 'a+');
    try {
      const { size } = await file.stat();
      if (size > 0) {
        const ending = Buffer.alloc(2);
        await file.read(ending, 0, 2, Math.max(0, size - 2));
        if (ending.toString() !== '\r\n') throw new Error('The existing FEN file does not end with a complete CRLF line.');
      }
      if (line !== null) await file.writeFile(line, 'utf8');
      await file.sync();
    } finally { await file.close(); }
    // Windows readers/indexers can briefly prevent replacement. Never fall back
    // to truncating the destination, which would expose an incomplete file.
    for (let attempt = 0; ; attempt++) {
      try { await rename(temporary, filename); break; }
      catch (error) {
        const code = (error as NodeJS.ErrnoException).code;
        if (process.platform !== 'win32' || !['EPERM', 'EACCES', 'EBUSY'].includes(code ?? '') || attempt >= 40) throw error;
        await delay(25);
      }
    }
  } finally {
    try { await rm(temporary, { force: true }); }
    finally {
      await lock.close();
      await rm(lockPath, { force: true });
    }
  }
}

export async function appendFen(filename: string, fen: string) {
  if (/[\r\n|]/.test(fen) || fen.trim().split(/\s+/).length !== 6) throw new Error('Cannot record an invalid FEN line.');
  await replaceFenFile(filename, `${fen} | *\r\n`);
}

export async function clearFenFile(filename: string) {
  await replaceFenFile(filename, null);
}
