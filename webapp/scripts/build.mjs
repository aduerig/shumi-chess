import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

export const root = fileURLToPath(new URL('../', import.meta.url));
export function run(command, args) {
  const result = spawnSync(command, args, { cwd: root, stdio: 'inherit', windowsHide: true });
  if (result.error) throw result.error;
  if (result.status !== 0) throw new Error(`${command} exited with status ${result.status}`);
}
export function build(args = []) {
  const configuration = args.includes('--debug') ? 'Debug' : 'Release';
  const asserts = args.includes('--no-asserts') || args.includes('-no-asserts') ? 'OFF' : 'ON';
  run(process.execPath, ['node_modules/typescript/bin/tsc', '-p', 'tsconfig.json']);
  run('cmake', ['-S', 'native', '-B', 'build', `-DSHUMI_ASSERTS=${asserts}`, ...(process.platform === 'win32' ? [] : [`-DCMAKE_BUILD_TYPE=${configuration}`])]);
  run('cmake', ['--build', 'build', '--config', configuration, '--target', 'shumi_bridge', '--parallel', '4']);
}
if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try { build(process.argv.slice(2)); } catch (error) { console.error(error.message); process.exitCode = 1; }
}
