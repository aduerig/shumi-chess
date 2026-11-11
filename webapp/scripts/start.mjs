import { spawn } from 'node:child_process';
import electron from 'electron';
import { build, root } from './build.mjs';

const args = process.argv.slice(2);
if (args.includes('--help') || args.includes('-h')) {
  console.log(`Shumi Chess desktop
  npm --prefix webapp start -- -d9 -t5000 -r2

  -d, --depth N       Maximum search depth (1..30)
  -t, --time N        Requested time per move in milliseconds
  -r, --rand N        Randomize the first N computer moves
  -wd/-bd N          Override white/black depth
  -wt/-bt N          Override white/black time
  -f/-wf/-bf MASK    Feature mask (decimal, hex, or binary; 0 = defaults)
  --fen "FEN"        Starting position
  --human            Start with two human players
  --debug            Build the native bridge in Debug configuration
  --asserts          Enable engine assertions (default)
  --no-asserts       Disable engine assertions

The build is incremental. No Python is used.`);
} else {
  try {
    build(args);
    const { parseOptions } = await import('../dist/desktop/options.js');
    parseOptions(args);
    const env = { ...process.env };
    delete env.ELECTRON_RUN_AS_NODE;
    const child = spawn(electron, ['.', ...args], { cwd: root, env, stdio: 'inherit' });
    child.on('error', error => { console.error(error.message); process.exitCode = 1; });
    child.on('exit', code => { process.exitCode = code ?? 0; });
    process.on('SIGINT', () => { child.kill(); });
  } catch (error) { console.error(error.message); process.exitCode = 1; }
}
