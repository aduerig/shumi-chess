// Exercise the actual renderer, preload, controller, and native engine without
// opening a visible window or touching the user's clipboard/session data.
import { app, BrowserWindow, ipcMain } from 'electron';
import assert from 'node:assert/strict';
import { mkdirSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { EngineClient } from '../dist/desktop/engine-client.js';
import { GameController } from '../dist/desktop/controller.js';
import { parseOptions } from '../dist/desktop/options.js';
import { START_FEN } from '../dist/shared/types.js';

const root = fileURLToPath(new URL('../', import.meta.url));
const artifacts = path.join(root, 'artifacts');
mkdirSync(artifacts, { recursive: true });
app.setPath('userData', path.join(artifacts, 'renderer-profile'));
// Software rendering makes offscreen layout checks/captures independent of GPU state.
app.disableHardwareAcceleration();
let window;
let controller;
const errors = [];
const copies = [];
const deadline = setTimeout(() => { console.error('Renderer test timed out.'); app.exit(1); }, 30000);

const run = script => window.webContents.executeJavaScript(script);
async function until(script) {
  const end = Date.now() + 5000;
  while (!await run(script)) {
    if (Date.now() > end) throw new Error(`Renderer condition timed out: ${script}; size ${await run('[innerWidth, innerHeight]')}`);
    await new Promise(resolve => setTimeout(resolve, 20));
  }
}
const click = id => run(`document.getElementById(${JSON.stringify(id)}).click()`);
const square = name => run(`document.querySelector('[data-square="${name}"]').click()`);
async function capture(name) {
  if (process.env.SHUMI_CAPTURE_UI !== '1') return;
  await run('new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve)))');
  writeFileSync(path.join(artifacts, `${name}.png`), (await window.webContents.capturePage()).toPNG());
}

void app.whenReady().then(async () => {
  let failed = false;
  try {
    const executable = path.join(root, 'build', 'bin', ...(process.platform === 'win32' ? ['Release', 'shumi_bridge.exe'] : ['shumi_bridge']));
    const engine = new EngineClient(executable, path.dirname(root));
    controller = new GameController(engine, parseOptions(['--human', '-r0', '-d2', '-t100']));
    window = new BrowserWindow({
      width: 1280, height: 930, show: false,
      webPreferences: { preload: path.join(root, 'dist', 'desktop', 'preload.cjs'), contextIsolation: true, nodeIntegration: false, sandbox: true, backgroundThrottling: false, offscreen: true },
    });
    window.webContents.on('console-message', event => {
      if (event.level === 'error') errors.push(event.message);
    });
    ipcMain.handle('shumi:state', () => controller.state);
    ipcMain.handle('shumi:action', (_event, action) => controller.dispatch(action));
    ipcMain.handle('shumi:copy', (_event, kind) => copies.push(controller.state.position[kind]));
    ipcMain.handle('shumi:quit', () => {});
    controller.on('event', event => window.webContents.send('shumi:event', event));
    await controller.initialize();
    await window.loadFile(path.join(root, 'ui', 'index.html'));
    await until('document.querySelectorAll(".square").length === 64');
    await until('[...document.querySelectorAll(".square .piece")].every(image => image.complete && image.naturalWidth > 0)');
    assert.equal(await run('new Set([...document.querySelectorAll(".square .piece")].map(image => image.src)).size'), 12);
    assert.equal(await run('document.getElementById("color-mode").value'), 'keep');

    for (const [width, height] of [[1000, 650], [1000, 760], [1280, 930], [1600, 1000]]) {
      window.setContentSize(width, height);
      // Windows display scaling can round a requested content size by one DIP.
      await until(`Math.abs(innerWidth - ${width}) <= 2 && Math.abs(innerHeight - ${height}) <= 2`);
      const layout = await run(`(() => {
        const board = document.getElementById('board').getBoundingClientRect();
        const inside = (a, b) => a.left >= b.left - 0.5 && a.top >= b.top - 0.5 && a.right <= b.right + 0.5 && a.bottom <= b.bottom + 0.5;
        const squares = [...document.querySelectorAll('.square')];
        return {
          squareCount: squares.length,
          pieceCount: document.querySelectorAll('.square .piece').length,
          clipped: squares.filter(cell => {
            const bounds = cell.getBoundingClientRect();
            const piece = cell.querySelector('.piece');
            return !inside(bounds, board) || Math.abs(bounds.width - bounds.height) > 0.5 || (piece && !inside(piece.getBoundingClientRect(), bounds));
          }).map(cell => cell.dataset.square),
          horizontalOverflow: document.documentElement.scrollWidth > innerWidth,
          boardVisible: board.bottom <= innerHeight,
          controlsVisible: document.querySelector('.board-actions').getBoundingClientRect().bottom <= innerHeight,
          resignVisible: inside(document.getElementById('resign').getBoundingClientRect(), document.querySelector('.play-area').getBoundingClientRect()),
        };
      })()`);
      assert.equal(layout.squareCount, 64);
      assert.equal(layout.pieceCount, 32);
      assert.deepEqual(layout.clipped, [], `Clipped squares at ${width}x${height}`);
      assert.equal(layout.horizontalOverflow, false);
      assert.equal(layout.boardVisible, true, `Board must fit the window at ${width}x${height}`);
      assert.equal(layout.controlsVisible, true, `Play controls must fit the window at ${width}x${height}`);
      assert.equal(layout.resignVisible, true, `Resign must fit beside the play controls at ${width}x${height}`);
      console.log(`PASS board layout ${width}x${height}: 64 square cells, 32 unclipped pieces`);
      await capture(`desktop-${width}x${height}`);
    }
    window.setContentSize(1280, 930);
    await until('Math.abs(innerWidth - 1280) <= 2 && Math.abs(innerHeight - 930) <= 2');

    await square('e2');
    assert.deepEqual(await run('[...document.querySelectorAll(".square.legal")].map(x => x.dataset.square).sort()'), ['e3', 'e4']);
    await square('e4');
    await until('document.getElementById("moves").textContent.includes("e4")');
    assert.equal(controller.state.position.moves[0].uci, 'e2e4');
    window.setContentSize(1000, 650);
    await until('Math.abs(innerWidth - 1000) <= 2 && Math.abs(innerHeight - 650) <= 2');
    assert.equal(await run('document.querySelector(".board-actions").getBoundingClientRect().bottom <= innerHeight'), true, 'Play controls still fit after the game starts');
    await capture('playing-1000x650');
    window.setContentSize(1280, 930);
    await until('Math.abs(innerWidth - 1280) <= 2 && Math.abs(innerHeight - 930) <= 2');
    await click('flip');
    await until('document.querySelector(".square").dataset.square === "h1"');
    await click('copy-fen');
    await until('document.getElementById("fen").value.includes(" b ")');
    assert.equal(copies.at(-1), controller.state.position.fen);
    console.log('PASS click-to-move, legal markers, move history, flip, and FEN export');

    const beforeResign = controller.state.position.fen;
    await click('resign');
    await until('document.getElementById("resignation").open');
    assert.equal(await run('document.getElementById("resignation-title").textContent'), 'Resign as Black?');
    assert.equal(await run('document.activeElement.id'), 'cancel-resign');
    await capture('resign-confirmation');
    await click('cancel-resign');
    await until('!document.getElementById("resignation").open');
    assert.equal(controller.state.position.result, '*');
    assert.equal(controller.state.position.fen, beforeResign);
    await click('resign');
    await until('document.getElementById("resignation").open');
    await click('confirm-resign');
    await until('document.getElementById("status").textContent === "Black resigned"');
    assert.equal(await run('document.getElementById("game-title").textContent'), 'White wins.');
    assert.equal(await run('document.getElementById("resign").disabled'), true);
    assert.equal(controller.state.position.fen, beforeResign);
    assert.equal(controller.state.match.players.a.wins, 1);
    await click('copy-pgn');
    await until('window.shumi.state().then(state => state.position.result === "1-0")');
    assert.match(copies.at(-1), /\[Result "1-0"\]/);
    assert(copies.at(-1).endsWith('1-0'));
    console.log('PASS resignation confirmation/cancel, result message, match score, and PGN export');

    await run(`document.getElementById('fen').value = '4k3/P7/8/8/8/8/8/4K3 w - - 0 1'`);
    await click('load-fen');
    await until('document.querySelectorAll(".square .piece").length === 3');
    await square('a7');
    await square('a8');
    await until('document.getElementById("promotion").open');
    await until('[...document.querySelectorAll("#promotion-options .piece")].every(image => image.complete && image.naturalWidth > 0)');
    assert.equal(await run('document.querySelectorAll("#promotion-options button").length'), 4);
    await capture('promotion');
    await run(`document.querySelector('#promotion-options button[value="a7a8n"]').click()`);
    await until('!document.getElementById("promotion").open && document.getElementById("moves").textContent.includes("a8=N")');
    assert(controller.state.position.fen.startsWith('N'));
    assert.equal(controller.state.match.players.a.draws, 1);
    assert.equal(await run('document.querySelector("[data-player=a] [data-stat=points]").textContent'), '1.5');
    await click('evaluate');
    await until('document.getElementById("evaluation").textContent !== "—"');
    assert.equal(controller.state.error, null);
    assert.deepEqual(errors, []);
    console.log('PASS FEN load, all promotion choices, knight promotion, and evaluation');

    // Drive two complete games through the actual UI and native engine. Both
    // human participants have distinct settings so every field can be checked
    // after the automatic swap, including separate identities for one evaluator.
    await run(`(() => {
      for (const [id, value] of Object.entries({ 'white-depth': '4', 'white-time': '111', 'white-features': '0x6', 'black-depth': '7', 'black-time': '222', 'black-features': '0xE' })) document.getElementById(id).value = value;
      document.getElementById('white-depth').dispatchEvent(new Event('change', { bubbles: true }));
    })()`);
    await until('window.shumi.state().then(state => state.settings.black.time === 222)');
    assert.equal(controller.state.match.completed, 2, 'Editing search settings must preserve earlier results');
    assert.equal(await run('document.querySelector("[data-player=a] [data-stat=points]").textContent'), '1.5');
    await click('reset-scores');
    await until('document.getElementById("completed-games").textContent === "0 games completed"');
    await run(`document.getElementById('color-mode').value = 'swap'; document.getElementById('color-mode').dispatchEvent(new Event('change', { bubbles: true }))`);
    await until('window.shumi.state().then(state => state.settings.swapColors)');
    await click('auto-reset');
    await until('window.shumi.state().then(state => state.settings.autoReset)');
    await run(`document.getElementById('fen').value = ${JSON.stringify(START_FEN)}`);
    await click('load-fen');
    await until('document.querySelectorAll(".square .piece").length === 32');
    async function foolsMate() {
      for (const move of ['f2f3', 'e7e5', 'g2g4', 'd8h4']) {
        await square(move.slice(0, 2)); await square(move.slice(2, 4));
        await until(`document.querySelector('#moves .latest')?.title === '${move}'`);
      }
    }
    await foolsMate();
    await until('document.querySelector("[data-player=b] [data-stat=wins]").textContent === "1"');
    await until('document.getElementById("white-label").textContent === "White · B"');
    assert.deepEqual(await run(`['white-depth', 'white-time', 'white-features', 'black-depth', 'black-time', 'black-features'].map(id => document.getElementById(id).value)`), ['7', '222', '0xE', '4', '111', '0x6']);
    assert.equal(await run('document.querySelector("[data-player=b] small").textContent'), 'Now White');
    assert.equal(await run('document.querySelector(".square").dataset.square'), 'h1', 'Color swaps preserve the board viewing direction');
    await run(`(() => {
      document.getElementById('white-depth').value = '8';
      document.getElementById('black-time').value = '333';
      document.getElementById('white-depth').dispatchEvent(new Event('change', { bubbles: true }));
    })()`);
    await until('window.shumi.state().then(state => state.settings.black.time === 333)');
    assert.equal(controller.state.match.completed, 1, 'Changing settings between automatic games must retain the result');
    assert.equal(controller.state.match.white, 'b');
    assert.equal(controller.state.match.players.b.settings.depth, 8);
    assert.equal(controller.state.match.players.a.settings.time, 333);
    assert.equal(await run('document.querySelector("[data-player=b] [data-stat=wins]").textContent'), '1');
    await click('auto-reset');
    await until('window.shumi.state().then(state => !state.settings.autoReset)');
    await foolsMate();
    await until('document.getElementById("completed-games").textContent === "2 games completed"');
    assert.equal(await run('document.querySelector("[data-player=a] [data-stat=wins]").textContent'), '1');
    assert.equal(await run('document.querySelector("[data-player=b] [data-stat=wins]").textContent'), '1');
    assert.equal(await run('document.querySelector("[data-player=a] [data-stat=losses]").textContent'), '1');
    assert.equal(await run('document.querySelector("[data-player=b] [data-stat=points]").textContent'), '1');
    await run('document.querySelector(".stats-panel").scrollIntoView({ block: "center" })');
    await capture('match-scores');
    const finalFen = controller.state.position.fen;
    await click('reset-scores');
    await until('document.getElementById("completed-games").textContent === "0 games completed"');
    assert.equal(controller.state.position.fen, finalFen);
    assert.equal(controller.state.match.white, 'b');
    assert.equal(controller.state.error, null);
    assert.deepEqual(errors, []);
    console.log('PASS automatic color swap, score retention through settings edits, player-based W/D/L/points, and explicit reset scores');

    await click('new-game');
    await until('window.shumi.state().then(state => state.position.result === "*")');
    await click('pause');
    await until('window.shumi.state().then(state => state.paused)');
    await run(`(() => {
      document.getElementById('white-player').value = 'shumi';
      document.getElementById('black-player').value = 'ivan';
      document.getElementById('white-player').dispatchEvent(new Event('change', { bubbles: true }));
    })()`);
    await until('document.getElementById("resign").disabled');
    assert.equal(controller.state.position.result, '*');
    console.log('PASS resignation unavailable for bot-versus-bot games');
  } catch (error) {
    failed = true;
    console.error(error);
  } finally {
    clearTimeout(deadline);
    await controller?.close();
    window?.destroy();
    app.exit(failed ? 1 : 0);
  }
});
