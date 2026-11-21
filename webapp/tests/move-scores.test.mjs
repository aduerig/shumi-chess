import test from 'node:test';
import assert from 'node:assert/strict';
import { EventEmitter } from 'node:events';
import { GameController } from '../dist/desktop/controller.js';
import { parseOptions } from '../dist/desktop/options.js';
import { START_FEN } from '../dist/shared/types.js';

// Control when searches finish and give each side a distinct score. Chess
// legality is covered by the native tests; these tests exercise score ownership.
function position(fen = START_FEN) {
  const turn = fen.split(' ')[1];
  return { fen, turn, legalMoves: [turn === 'w' ? 'e2e4' : 'e7e5'], inCheck: false,
    result: '*', drawReason: '', moves: [], pgn: '*', moveNumber: 1,
    phase: 'OPENING', whiteTimeMs: 0, blackTimeMs: 0, nnueAvailable: { nnue1: true, nnue2: true }, defaultFeatures: 14 };
}
class ControlledEngine extends EventEmitter {
  position = position();
  ready = Promise.resolve(this.position);
  search = null;
  async request(command, args = {}) {
    if (command === 'reset') {
      if (args.fen === 'invalid') throw new Error('Invalid FEN.');
      this.position = position(args.fen || START_FEN);
    } else if (command === 'search') {
      assert.equal(this.search, null);
      return new Promise(resolve => { this.search = resolve; });
    } else if (command === 'move') {
      const turn = this.position.turn;
      this.position.moves.push({ uci: args.move, san: args.move, color: turn, number: 1 });
      this.position.turn = turn === 'w' ? 'b' : 'w';
      this.position.legalMoves = [turn === 'w' ? 'e7e5' : 'g1f3'];
    } else if (command === 'evaluate') return { scoreCp: -880 };
    else if (command === 'stop') { if (this.search) this.finish(999); return { stopped: true }; }
    else throw new Error(`Unexpected command: ${command}`);
    return structuredClone(this.position);
  }
  finish(scoreCp) {
    const resolve = this.search;
    assert(resolve, 'A search must be active');
    this.search = null;
    resolve({ move: this.position.legalMoves[0], scoreCp, depth: 3, nodes: 50, elapsedMs: 20 });
  }
  async close() { if (this.search) this.finish(999); }
}
async function until(predicate) {
  const deadline = Date.now() + 2000;
  while (!predicate()) {
    if (Date.now() > deadline) throw new Error('Score test timed out.');
    await new Promise(resolve => setTimeout(resolve, 5));
  }
}
async function session(t, fen = START_FEN) {
  const engine = new ControlledEngine();
  const options = parseOptions(['-r0']);
  options.fen = fen;
  options.settings.white.player = 'shumi';
  options.settings.black.player = 'ivan';
  const controller = new GameController(engine, options);
  t.after(() => controller.close());
  await controller.initialize();
  return { engine, controller };
}
async function holdMove(controller, engine, score) {
  await until(() => engine.search);
  await controller.dispatch({ type: 'pause' });
  engine.finish(score);
  await until(() => controller.state.pendingMove);
}

test('each side keeps its committed score through searches, held moves, evaluation, and flipping', async t => {
  const { controller, engine } = await session(t);
  assert.deepEqual(controller.state.moveScores, { w: null, b: null });
  await controller.dispatch({ type: 'start' });
  await until(() => engine.search);
  engine.emit('progress', { scoreCp: 920, depth: 2, nodes: 20, elapsedMs: 10 });
  assert.deepEqual(controller.state.moveScores, { w: null, b: null });
  await holdMove(controller, engine, 135);
  assert.deepEqual(controller.state.moveScores, { w: null, b: null }, 'A held move has not been played');
  await controller.dispatch({ type: 'moveNow' });
  assert.deepEqual(controller.state.moveScores, { w: 135, b: null });
  await controller.dispatch({ type: 'evaluate' });
  await controller.dispatch({ type: 'flip' });
  assert.equal(controller.state.evaluation, -880);
  assert.deepEqual(controller.state.moveScores, { w: 135, b: null });

  await controller.dispatch({ type: 'pause' });
  await until(() => engine.search);
  engine.emit('progress', { scoreCp: -420, depth: 2, nodes: 20, elapsedMs: 10 });
  assert.deepEqual(controller.state.moveScores, { w: 135, b: null });
  await holdMove(controller, engine, -65);
  assert.deepEqual(controller.state.moveScores, { w: 135, b: null });
  await controller.dispatch({ type: 'moveNow' });
  assert.deepEqual(controller.state.moveScores, { w: 135, b: -65 });

  await controller.dispatch({ type: 'pause' });
  await holdMove(controller, engine, 210);
  assert.deepEqual(controller.state.moveScores, { w: 135, b: -65 });
  await controller.dispatch({ type: 'pause' });
  assert.deepEqual(controller.state.moveScores, { w: 210, b: -65 }, 'Resume updates only the side that moved');
  await controller.dispatch({ type: 'pause' });
  await controller.dispatch({ type: 'reset', fen: 'invalid' });
  assert.deepEqual(controller.state.moveScores, { w: 210, b: -65 }, 'Rejected position loads retain the scores');
  await controller.dispatch({ type: 'reset' });
  await controller.dispatch({ type: 'pause' });
  assert.deepEqual(controller.state.moveScores, { w: null, b: null });
});

test('a Black-to-move position assigns its first score to Black and discards stopped searches on reset', async t => {
  const fen = START_FEN.replace(' w ', ' b ');
  const { controller, engine } = await session(t, fen);
  await controller.dispatch({ type: 'start' });
  await holdMove(controller, engine, -240);
  await controller.dispatch({ type: 'moveNow' });
  assert.deepEqual(controller.state.moveScores, { w: null, b: -240 });
  await controller.dispatch({ type: 'pause' });
  await until(() => engine.search);
  await controller.dispatch({ type: 'reset', fen });
  await controller.dispatch({ type: 'pause' });
  assert.equal(controller.state.position.moves.length, 0);
  assert.deepEqual(controller.state.moveScores, { w: null, b: null }, 'A cancelled search cannot restore an old score');
});

test('a human reply leaves the engine score visible without inventing a human search score', async t => {
  const { controller, engine } = await session(t);
  const settings = structuredClone(controller.state.settings);
  settings.black.player = 'human';
  await controller.dispatch({ type: 'settings', settings });
  await controller.dispatch({ type: 'start' });
  await holdMove(controller, engine, 75);
  await controller.dispatch({ type: 'pause' });
  await controller.dispatch({ type: 'move', move: 'e7e5' });
  await controller.dispatch({ type: 'pause' });
  assert.deepEqual(controller.state.moveScores, { w: 75, b: null });
});
