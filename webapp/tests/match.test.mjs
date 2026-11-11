import test from 'node:test';
import assert from 'node:assert/strict';
import { EventEmitter } from 'node:events';
import { GameController } from '../dist/desktop/controller.js';
import { parseOptions } from '../dist/desktop/options.js';
import { PLAYER_IDS, START_FEN } from '../dist/shared/types.js';

// Deterministic two-ply games exercise session accounting/scheduling. Actual
// legality, search, mate, and draws are covered separately with the native engine.
function initialPosition() {
  return { fen: START_FEN, turn: 'w', legalMoves: ['e2e4'], inCheck: false,
    result: '*', drawReason: '', moves: [], pgn: '*', moveNumber: 1,
    phase: 'OPENING', whiteTimeMs: 0, blackTimeMs: 0, nnueAvailable: true, defaultFeatures: 14 };
}
class ScriptedEngine extends EventEmitter {
  position = initialPosition();
  ready = Promise.resolve(initialPosition());
  searches = [];
  finished = 0;
  constructor(outcomes) { super(); this.outcomes = outcomes; }
  async request(command, args = {}) {
    if (command === 'reset') this.position = initialPosition();
    else if (command === 'search') {
      this.searches.push({ ...args });
      return { move: this.position.legalMoves[0], scoreCp: 0, depth: args.depth, nodes: 10, elapsedMs: 1 };
    } else if (command === 'move') {
      assert(this.position.legalMoves.includes(args.move));
      this.position.moves.push({ uci: args.move, san: args.move, color: this.position.turn, number: 1 });
      if (this.position.turn === 'w') { this.position.turn = 'b'; this.position.legalMoves = ['e7e5']; }
      else {
        this.position.result = this.outcomes[this.finished++ % this.outcomes.length];
        this.position.legalMoves = []; this.position.turn = 'w';
      }
    } else if (command === 'stop') return { stopped: true };
    else throw new Error(`Unexpected test command: ${command}`);
    return structuredClone(this.position);
  }
  async close() {}
}
async function until(predicate) {
  const end = Date.now() + 10000;
  while (!predicate()) {
    if (Date.now() > end) throw new Error('Match did not reach the expected state.');
    await new Promise(resolve => setTimeout(resolve, 10));
  }
}
async function session(t, { swap = false, auto = true, same = false, outcomes = ['1-0', '0-1', '1/2-1/2'] } = {}) {
  const options = parseOptions(['-r0']);
  options.settings.white = { player: 'shumi', depth: 3, time: 120, features: 6 };
  options.settings.black = { player: same ? 'shumi' : 'ivan', depth: 5, time: 240, features: 14 };
  options.settings.autoReset = auto; options.settings.swapColors = swap;
  const engine = new ScriptedEngine(outcomes);
  const controller = new GameController(engine, options);
  t.after(() => controller.close());
  await controller.initialize();
  return { controller, engine, original: structuredClone(options.settings) };
}

test('keeping colors is the default', () => {
  assert.equal(parseOptions([]).settings.swapColors, false);
});

for (const swap of [false, true]) test(`${swap ? 'alternating' : 'fixed'} colors preserve player profiles and totals across wins and a draw`, { timeout: 20000 }, async t => {
  const { controller, engine, original } = await session(t, { swap });
  await controller.dispatch({ type: 'flip' });
  await controller.dispatch({ type: 'start' });
  await until(() => controller.state.match.completed === 3);
  await controller.dispatch({ type: 'settings', settings: { ...controller.state.settings, autoReset: false } });
  const match = controller.state.match;
  assert.equal(controller.state.flipped, true, 'Color alternation must not change the board view');
  assert.equal(match.white, 'a'); assert.equal(match.black, 'b');
  assert.deepEqual(match.players.a, { settings: original.white, wins: swap ? 2 : 1, draws: 1, losses: swap ? 0 : 1, gamesAsWhite: swap ? 2 : 3, gamesAsBlack: swap ? 1 : 0 });
  assert.deepEqual(match.players.b, { settings: original.black, wins: swap ? 0 : 1, draws: 1, losses: swap ? 2 : 1, gamesAsWhite: swap ? 1 : 0, gamesAsBlack: swap ? 2 : 3 });
  assert.equal(engine.searches.length, 6);
  for (let game = 0; game < 3; game++) {
    const profiles = swap && game % 2 ? [original.black, original.white] : [original.white, original.black];
    profiles.forEach((profile, ply) => assert.deepEqual(engine.searches[game * 2 + ply], {
      player: PLAYER_IDS[profile.player], depth: profile.depth, time: profile.time, features: profile.features,
    }));
  }
  await controller.dispatch({ type: 'resetScores' });
  assert.equal(match.completed, 0);
  assert.equal(match.players.a.wins + match.players.a.draws + match.players.a.losses, 0);
  assert.equal(match.players.b.wins + match.players.b.draws + match.players.b.losses, 0);
  assert.deepEqual(controller.state.settings.white, original.white);
  assert.equal(controller.state.position.result, '1/2-1/2');
  assert.equal(controller.state.error, null);
});

test('two Shumi configurations remain separate competitors while changing colors', { timeout: 15000 }, async t => {
  const { controller, original } = await session(t, { same: true, swap: true, outcomes: ['1-0'] });
  await controller.dispatch({ type: 'start' });
  await until(() => controller.state.match.completed === 2);
  await controller.dispatch({ type: 'settings', settings: { ...controller.state.settings, autoReset: false } });
  assert.equal(controller.state.match.white, 'b');
  assert.equal(controller.state.match.players.a.wins, 1);
  assert.equal(controller.state.match.players.b.wins, 1);
  assert.equal(controller.state.match.players.a.losses, 1);
  assert.equal(controller.state.match.players.b.losses, 1);
  assert.deepEqual(controller.state.settings.white, original.black);
  assert.deepEqual(controller.state.settings.black, original.white);
});

for (const swap of [false, true]) test(`settings edits preserve results and count ongoing games with ${swap ? 'alternating' : 'fixed'} colors`, { timeout: 15000 }, async t => {
  const { controller } = await session(t, { swap, outcomes: ['1-0', '0-1'] });
  await controller.dispatch({ type: 'start' });
  await until(() => controller.state.match.completed === 1);
  await controller.dispatch({ type: 'pause' });
  const firstMatch = structuredClone(controller.state.match);
  const settings = structuredClone(controller.state.settings);
  settings.white.player = 'slug'; settings.white.features = 10;
  await controller.dispatch({ type: 'settings', settings });
  assert.equal(controller.state.match.completed, firstMatch.completed);
  assert.equal(controller.state.match.players.a.wins, 1);
  assert.equal(controller.state.match.players.b.losses, 1);
  assert.equal(controller.state.match.players.a.settings.player, 'slug');
  await controller.dispatch({ type: 'pause' });
  await until(() => controller.state.game === 2 && controller.state.position.moves.length === 1);
  await controller.dispatch({ type: 'pause' });
  const white = controller.state.match.white;
  const black = controller.state.match.black;
  assert.equal(white, swap ? 'b' : 'a');
  assert.equal(black, swap ? 'a' : 'b');
  const changed = structuredClone(controller.state.settings);
  changed.white.time = 350; changed.black.depth++;
  await controller.dispatch({ type: 'settings', settings: changed });
  assert.equal(controller.state.match.completed, 1);
  assert.equal(controller.state.match.countCurrentGame, true, 'Editing settings must not discard the ongoing game');
  assert.equal(controller.state.match.white, white, 'Edits must preserve identities after a color swap');
  assert.equal(controller.state.match.black, black);
  assert.equal(controller.state.match.players[white].settings.time, 350);
  assert.equal(controller.state.match.players[black].settings.depth, changed.black.depth);
  await controller.dispatch({ type: 'pause' });
  await until(() => controller.state.match.completed === 2);
  await controller.dispatch({ type: 'settings', settings: { ...controller.state.settings, autoReset: false } });
  assert.equal(controller.state.match.players.a.wins, swap ? 2 : 1);
  assert.equal(controller.state.match.players.b.wins, swap ? 0 : 1);
  await controller.dispatch({ type: 'reset' });
  await controller.dispatch({ type: 'pause' });
  assert.equal(controller.state.match.completed, 2, 'Starting another game must retain both results');
  const assignments = [controller.state.match.white, controller.state.match.black];
  await controller.dispatch({ type: 'resetScores' });
  assert.equal(controller.state.match.completed, 0);
  assert.deepEqual([controller.state.match.white, controller.state.match.black], assignments);
  assert.equal(controller.state.error, null);
});

test('pause holds the completed game and resumes with one color swap', { timeout: 12000 }, async t => {
  const { controller } = await session(t, { swap: true });
  await controller.dispatch({ type: 'start' });
  await until(() => controller.state.match.completed === 1);
  await controller.dispatch({ type: 'pause' });
  await new Promise(resolve => setTimeout(resolve, 1950));
  assert.equal(controller.state.game, 1);
  assert.equal(controller.state.match.white, 'a');
  await controller.dispatch({ type: 'pause' });
  await until(() => controller.state.game === 2);
  await controller.dispatch({ type: 'pause' });
  assert.equal(controller.state.match.white, 'b');
  assert.equal(controller.state.match.completed, 1);
  assert.equal(controller.state.position.moves.length, 0);
});
