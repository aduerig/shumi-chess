import { app, BrowserWindow, clipboard, ipcMain, screen } from 'electron';
import { mkdirSync, createWriteStream } from 'node:fs';
import { fileURLToPath } from 'node:url';
import path from 'node:path';
import { EngineClient } from './engine-client.js';
import { GameController } from './controller.js';
import { parseOptions } from './options.js';
import type { Action, AppEvent } from '../shared/types.js';

const root = fileURLToPath(new URL('../../', import.meta.url));
const data = path.join(root, 'data');
mkdirSync(data, { recursive: true });
app.setPath('userData', path.join(data, 'electron'));
app.setName('Shumi Chess');
let window: BrowserWindow | null = null;
let controller: GameController | null = null;
let closing = false;
let allowQuit = false;
const log = createWriteStream(path.join(data, 'session.log'), { flags: 'a' });

async function shutdown() {
  if (closing) return;
  closing = true;
  await controller?.close();
  log.end();
  allowQuit = true;
  app.quit();
}

app.on('before-quit', event => {
  if (!allowQuit) { event.preventDefault(); void shutdown(); }
});

void app.whenReady().then(async () => {
try {
  const options = parseOptions(process.argv.slice(2));
  const configuration = options.debug ? 'Debug' : 'Release';
  const executable = process.platform === 'win32'
    ? path.join(root, 'build', 'bin', configuration, 'shumi_bridge.exe')
    : path.join(root, 'build', 'bin', 'shumi_bridge');
  const engine = new EngineClient(executable, path.dirname(root));
  controller = new GameController(engine, options);
  const available = screen.getDisplayNearestPoint(screen.getCursorScreenPoint()).workArea;
  const width = Math.min(1280, available.width);
  const height = Math.min(930, available.height);
  window = new BrowserWindow({
    title: 'Shumi Chess', width, height,
    x: available.x + Math.floor((available.width - width) / 2),
    y: available.y + Math.floor((available.height - height) / 2),
    minWidth: Math.min(1000, available.width), minHeight: Math.min(760, available.height),
    backgroundColor: '#f5f3ed', autoHideMenuBar: true, show: false,
    webPreferences: {
      preload: fileURLToPath(new URL('preload.cjs', import.meta.url)),
      contextIsolation: true, nodeIntegration: false, sandbox: true,
    },
  });
  window.removeMenu();
  window.once('ready-to-show', () => { window?.show(); });
  window.webContents.setWindowOpenHandler(() => ({ action: 'deny' }));
  window.webContents.on('will-navigate', event => event.preventDefault());
  window.on('close', event => {
    if (!allowQuit) { event.preventDefault(); void shutdown(); }
  });
  const verifySender = (event: Electron.IpcMainInvokeEvent) => {
    if (!window || event.sender !== window.webContents || event.senderFrame !== window.webContents.mainFrame)
      throw new Error('Unknown application window.');
  };
  ipcMain.handle('shumi:state', event => { verifySender(event); return controller!.state; });
  ipcMain.handle('shumi:action', (event, action: Action) => { verifySender(event); return controller!.dispatch(action); });
  ipcMain.handle('shumi:copy', (event, kind: string) => {
    verifySender(event);
    if (kind !== 'fen' && kind !== 'pgn') throw new Error('Unknown export format.');
    const position = controller!.state.position;
    if (position) {
      clipboard.writeText(position[kind]);
      process.stdout.write(`${kind === 'fen' ? 'Current FEN: ' : ''}${position[kind]}\n`);
    }
  });
  ipcMain.handle('shumi:quit', event => { verifySender(event); void shutdown(); });
  controller.on('event', (event: AppEvent) => {
    if (event.type === 'log') {
      process.stderr.write(event.text);
      log.write(event.text.replace(/\x1b\[[0-9;]*[A-Za-z]/g, '').replace(/\r?\n/g, '\r\n'));
    }
    if (window && !window.isDestroyed()) window.webContents.send('shumi:event', event);
  });
  await window.loadFile(path.join(root, 'ui', 'index.html'));
  try { await controller.initialize(); }
  catch (error) {
    controller.state.busy = false;
    controller.state.error = error instanceof Error ? error.message : String(error);
    window.webContents.send('shumi:event', { type: 'state', state: controller.state });
  }
} catch (error) {
  console.error(error);
  await shutdown();
}
});
