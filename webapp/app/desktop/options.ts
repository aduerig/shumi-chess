import type { Settings } from '../shared/types.js';

export interface Options { settings: Settings; fen: string; debug: boolean; asserts: boolean; }
export function parseOptions(args: string[]): Options {
  const values: Record<string, string> = {};
  let human = false, debug = false, asserts = true;
  const aliases: Record<string, string> = { d: 'depth', t: 'time', r: 'rand', f: 'feat', fen: 'fen', depth: 'depth', time: 'time', rand: 'rand', feat: 'feat', wd: 'wd', wt: 'wt', wf: 'wf', bd: 'bd', bt: 'bt', bf: 'bf' };
  for (let i = 0; i < args.length; i++) {
    const arg = args[i];
    if (arg === '--') continue;
    if (arg === '--human' || arg === '-human') { human = true; continue; }
    if (arg === '--debug') { debug = true; continue; }
    if (arg === '--asserts' || arg === '-asserts') { asserts = true; continue; }
    if (arg === '--no-asserts' || arg === '-no-asserts') { asserts = false; continue; }
    const match = arg.match(/^--?([a-z]+)(?:=(.*)|([0-9].*))?$/);
    if (!match || !aliases[match[1]]) throw new Error(`Unknown option: ${arg}`);
    const value = match[2] ?? match[3] ?? args[++i];
    if (value === undefined) throw new Error(`Missing value for ${arg}`);
    values[aliases[match[1]]] = value;
  }
  const integer = (key: string, fallback: number, min: number, max: number) => {
    if (values[key] === undefined) return fallback;
    const text = values[key];
    if (!/^(?:\d+|0x[\da-f]+|0b[01]+)$/i.test(text)) throw new Error(`${key} must be an integer.`);
    const value = Number(text);
    if (!Number.isSafeInteger(value) || value < min || value > max) throw new Error(`${key} must be between ${min} and ${max}.`);
    return value;
  };
  const depth = integer('depth', 9, 1, 30);
  const time = integer('time', 5000, 0, 3_600_000);
  // Preserve the legacy launcher convention: a zero mask selects core defaults.
  const features = integer('feat', 14, 0, 2_147_483_647) || 14;
  return {
    debug, asserts, fen: values.fen ?? '',
    settings: {
      white: { player: 'human', depth: integer('wd', depth, 1, 30), time: integer('wt', time, 0, 3_600_000), features: integer('wf', features, 0, 2_147_483_647) || 14 },
      black: { player: human ? 'human' : 'shumi', depth: integer('bd', depth, 1, 30), time: integer('bt', time, 0, 3_600_000), features: integer('bf', features, 0, 2_147_483_647) || 14 },
      random: integer('rand', 2, 0, 100), autoReset: false, swapColors: false,
    },
  };
}
