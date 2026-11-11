import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { EngineClient } from '../dist/desktop/engine-client.js';
import { GameController } from '../dist/desktop/controller.js';
import { parseOptions } from '../dist/desktop/options.js';
import { START_FEN } from '../dist/shared/types.js';

const root = fileURLToPath(new URL('../', import.meta.url));
const executable = path.join(root, 'build', 'bin', ...(process.platform === 'win32' ? ['Release', 'shumi_bridge.exe'] : ['shumi_bridge']));
async function client(t) {
  const engine = new EngineClient(executable, path.dirname(root));
  let diagnostics = '';
  engine.on('log', text => { diagnostics = (diagnostics + text).slice(-8000); });
  engine.on('failure', error => console.error(error.message, diagnostics));
  t.after(() => engine.close());
  await engine.ready;
  return engine;
}
async function until(predicate, timeout = 10000) {
  const deadline = Date.now() + timeout;
  while (!predicate()) {
    if (Date.now() > deadline) throw new Error('Timed out waiting for application state.');
    await new Promise(resolve => setTimeout(resolve, 10));
  }
}

test('legacy CLI arguments preserve side overrides and accept hexadecimal masks', () => {
  const options = parseOptions(['-d9', '-t5000', '-r2', '-wd6', '-bt1000', '-wf0x6', '--human']);
  assert.equal(options.settings.white.depth, 6);
  assert.equal(options.settings.black.depth, 9);
  assert.equal(options.settings.white.time, 5000);
  assert.equal(options.settings.black.time, 1000);
  assert.equal(options.settings.white.features, 6);
  assert.equal(options.settings.black.player, 'human');
  assert.throws(() => parseOptions(['-d0']));
  assert.throws(() => parseOptions(['--unknown']));
});

test('bridge uses engine legality and keeps position after rejected input', { timeout: 15000 }, async t => {
  const engine = await client(t);
  const start = await engine.request('snapshot');
  assert.equal(start.fen, START_FEN);
  assert.equal(start.legalMoves.length, 20);
  await assert.rejects(engine.request('move', { move: 'e2e5' }), /not legal/);
  for (const fen of ['not a FEN', '8/8/8/8/8/8/8/8 w - - 0 1', '4k3/8/8/8/8/8/8/4K3 w K - 0 1', '4k3/8/8/8/8/8/8/P3K3 w - - 0 1', '4k3/8/8/8/8/8/4K3/8 w - - 0 999']) {
    await assert.rejects(engine.request('reset', { fen }));
    assert.equal((await engine.request('snapshot')).fen, START_FEN);
  }
  const after = await engine.request('move', { move: 'e2e4' });
  assert.equal(after.turn, 'b');
  assert.equal(after.moves[0].san, 'e4');
  assert.match(after.pgn, /1\. e4 \*/);
});

test('castling, en passant, all promotions, checkmate, and FEN move numbers', { timeout: 15000 }, async t => {
  const engine = await client(t);
  await engine.request('reset', { fen: 'r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1' });
  let after = await engine.request('move', { move: 'e1g1' });
  assert.match(after.fen, /R4RK1 b kq/);
  assert.equal(after.moves[0].san, 'O-O');
  await engine.request('reset', { fen: '4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1' });
  after = await engine.request('move', { move: 'e5d6' });
  assert.match(after.fen, /^4k3\/8\/3P4\/8/);
  for (const piece of ['q', 'r', 'b', 'n']) {
    const position = await engine.request('reset', { fen: '4k3/P7/8/8/8/8/8/4K3 w - - 0 1' });
    assert(position.legalMoves.includes(`a7a8${piece}`));
    after = await engine.request('move', { move: `a7a8${piece}` });
    assert(after.fen.startsWith(piece.toUpperCase()));
  }
  await engine.request('reset', { fen: START_FEN });
  for (const move of ['f2f3', 'e7e5', 'g2g4', 'd8h4']) after = await engine.request('move', { move });
  assert.equal(after.result, '0-1');
  assert.equal(after.moves.at(-1).san, 'Qh4#');
  assert(after.pgn.endsWith('0-1'));
  await engine.request('reset', { fen: '4k3/8/8/8/8/8/P7/4K3 b - - 0 42' });
  after = await engine.request('move', { move: 'e8d8' });
  assert.equal(after.moves[0].number, 42);
  assert.match(after.pgn, /42\.\.\. Kd8/);
  assert.equal(after.moveNumber, 43);
});

test('repetition and draw reasons come from the engine', { timeout: 15000 }, async t => {
  const engine = await client(t);
  let after;
  for (let i = 0; i < 2; i++) for (const move of ['g1f3', 'g8f6', 'f3g1', 'f6g8']) after = await engine.request('move', { move });
  assert.equal(after.result, '1/2-1/2');
  assert.equal(after.drawReason, 'Threefold repetition');
  after = await engine.request('reset', { fen: '7k/5Q2/6K1/8/8/8/8/8 b - - 0 1' });
  assert.equal(after.drawReason, 'Stalemate');
});

test('search preserves the board and can be stopped before resetting', { timeout: 20000 }, async t => {
  const engine = await client(t);
  const search = engine.request('search', { depth: 20, time: 10000, player: 3, features: 14 });
  await engine.request('stop');
  const answer = await search;
  assert.match(answer.move, /^[a-h][1-8][a-h][1-8][qrbn]?$/);
  assert.equal((await engine.request('snapshot')).fen, START_FEN);
  const next = await engine.request('reset', { fen: START_FEN, random: 0 });
  assert.equal(next.moves.length, 0);
  const result = await engine.request('search', { depth: 2, time: 100, player: 2, features: 14 });
  assert(next.legalMoves.includes(result.move));
  for (const player of next.nnueAvailable ? [1, 0] : [1]) {
    const answer = await engine.request('search', { depth: 1, time: 30, player, features: 14 });
    assert(next.legalMoves.includes(answer.move));
  }
});

test('pause holds the AI move, resume commits it, and reset rejects stale search results', { timeout: 20000 }, async t => {
  const engine = await client(t);
  const options = parseOptions(['-d3', '-t200', '-r0']);
  options.settings.white.player = 'shumi'; options.settings.black.player = 'human';
  const controller = new GameController(engine, options);
  t.after(() => controller.close());
  await controller.initialize();
  await controller.dispatch({ type: 'start' });
  await until(() => controller.state.searching);
  await controller.dispatch({ type: 'pause' });
  await until(() => controller.state.pendingMove);
  assert.equal(controller.state.position.fen, START_FEN);
  assert.equal(controller.state.position.moves.length, 0);
  await controller.dispatch({ type: 'pause' });
  assert.equal(controller.state.position.moves.length, 1);
  await controller.dispatch({ type: 'reset' });
  await until(() => controller.state.searching);
  await controller.dispatch({ type: 'reset' });
  await controller.dispatch({ type: 'pause' });
  await new Promise(resolve => setTimeout(resolve, 350));
  assert.equal(controller.state.position.fen, START_FEN);
  assert.equal(controller.state.position.moves.length, 0);
  assert.equal(controller.state.error, null);
});

test('bot versus bot advances, flips independently, and stops cleanly on close', { timeout: 15000 }, async t => {
  const engine = await client(t);
  const options = parseOptions(['-d2', '-t100', '-r0']);
  options.settings.white.player = 'shumi'; options.settings.black.player = 'ivan';
  const controller = new GameController(engine, options);
  t.after(() => controller.close());
  await controller.initialize();
  await controller.dispatch({ type: 'start' });
  await until(() => controller.state.position.moves.length >= 4);
  await controller.dispatch({ type: 'pause' });
  const fen = controller.state.position.fen;
  await controller.dispatch({ type: 'flip' });
  assert.equal(controller.state.flipped, true);
  assert.equal(controller.state.position.fen, fen);
  await controller.close();
});

test('continuous play records the result once and starts the next game', { timeout: 15000 }, async t => {
  const engine = await client(t);
  const options = parseOptions(['--human', '-r0']);
  options.settings.autoReset = true;
  const controller = new GameController(engine, options);
  t.after(() => controller.close());
  await controller.initialize();
  for (const move of ['f2f3', 'e7e5', 'g2g4', 'd8h4']) await controller.dispatch({ type: 'move', move });
  assert.equal(controller.state.position.result, '0-1');
  assert.equal(controller.state.match.players.b.wins, 1);
  await until(() => controller.state.game === 2);
  assert.equal(controller.state.position.fen, START_FEN);
  assert.equal(controller.state.position.moves.length, 0);
  assert.equal(controller.state.match.players.b.wins, 1);
  assert.equal(controller.state.started, true);
  assert.equal(controller.state.error, null);
});

test('human resignation stops an active search, exports the result, and prevents further play', { timeout: 15000 }, async t => {
  const engine = await client(t);
  const controller = new GameController(engine, parseOptions(['-d20', '-t10000', '-r0']));
  t.after(() => controller.close());
  await controller.initialize();
  await controller.dispatch({ type: 'resign', color: 'b' });
  assert.match(controller.state.error, /Only a human/);
  assert.equal(controller.state.position.result, '*');
  await controller.dispatch({ type: 'move', move: 'e2e4' });
  const fen = controller.state.position.fen;
  await until(() => controller.state.searching);
  await controller.dispatch({ type: 'resign', color: 'w' });
  assert.equal(controller.state.error, null);
  assert.equal(controller.state.searching, false);
  assert.equal(controller.state.pendingMove, false);
  assert.equal(controller.state.position.result, '0-1');
  assert.equal(controller.state.position.resigned, 'w');
  assert.equal(controller.state.position.fen, fen);
  assert.equal(controller.state.position.moves.length, 1);
  assert.deepEqual(controller.state.position.legalMoves, []);
  assert.match(controller.state.position.pgn, /\[Result "0-1"\]/);
  assert.match(controller.state.position.pgn, /1\. e4 0-1$/);
  assert.equal(controller.state.match.players.a.losses, 1);
  assert.equal(controller.state.match.players.b.wins, 1);
  for (const action of [{ type: 'resign', color: 'w' }, { type: 'move', move: 'e7e5' }, { type: 'start' }, { type: 'moveNow' }]) {
    await controller.dispatch(action);
    assert(controller.state.error);
    assert.equal(controller.state.match.completed, 1);
    assert.equal(controller.state.position.fen, fen);
  }
  await controller.dispatch({ type: 'evaluate' });
  assert.equal(controller.state.error, null);
  assert.equal(controller.state.position.result, '0-1');
  await controller.dispatch({ type: 'reset' });
  assert.equal(controller.state.position.result, '*');
  assert.equal(controller.state.position.resigned, undefined);
  assert.equal(controller.state.position.legalMoves.length, 20);
  assert.equal(controller.state.match.completed, 1);
});

test('Black can resign while paused with a computer move held, without applying that move', { timeout: 15000 }, async t => {
  const engine = await client(t);
  const options = parseOptions(['-d3', '-t200', '-r0']);
  options.settings.white.player = 'shumi'; options.settings.black.player = 'human';
  const controller = new GameController(engine, options);
  t.after(() => controller.close());
  await controller.initialize();
  await controller.dispatch({ type: 'start' });
  await until(() => controller.state.searching);
  await controller.dispatch({ type: 'pause' });
  await until(() => controller.state.pendingMove);
  await controller.dispatch({ type: 'resign', color: 'b' });
  assert.equal(controller.state.error, null);
  assert.equal(controller.state.paused, true);
  assert.equal(controller.state.pendingMove, false);
  assert.equal(controller.state.position.fen, START_FEN);
  assert.equal(controller.state.position.moves.length, 0);
  assert.equal(controller.state.position.result, '1-0');
  assert.equal(controller.state.position.resigned, 'b');
  assert.match(controller.state.position.pgn, /\[Result "1-0"\]/);
  assert(controller.state.position.pgn.endsWith('1-0'));
  assert.equal(controller.state.match.players.a.wins, 1);
  await controller.dispatch({ type: 'pause' });
  assert.equal(controller.state.position.moves.length, 0);
  assert.equal(controller.state.match.completed, 1);
});

test('resigned games repeat and swap colors with scores following the players', { timeout: 15000 }, async t => {
  const engine = await client(t);
  const options = parseOptions(['--human', '-r0']);
  options.settings.autoReset = true; options.settings.swapColors = true;
  const controller = new GameController(engine, options);
  t.after(() => controller.close());
  await controller.initialize();
  await controller.dispatch({ type: 'move', move: 'e2e4' });
  await controller.dispatch({ type: 'resign', color: 'b' });
  assert.equal(controller.state.match.players.a.wins, 1);
  await until(() => controller.state.game === 2);
  assert.equal(controller.state.position.result, '*');
  assert.equal(controller.state.match.white, 'b');
  assert.equal(controller.state.match.black, 'a');
  await controller.dispatch({ type: 'settings', settings: { ...controller.state.settings, autoReset: false } });
  await controller.dispatch({ type: 'resign', color: 'w' });
  assert.equal(controller.state.match.completed, 2);
  assert.equal(controller.state.match.players.a.wins, 2);
  assert.equal(controller.state.match.players.b.losses, 2);
  assert.equal(controller.state.error, null);
});
