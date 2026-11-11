import { EventEmitter } from 'node:events';
import { EngineClient } from './engine-client.js';
import { clearMatchScores, newMatch, recordMatchResult, updateMatchSettings } from './match.js';
import type { Options } from './options.js';
import { PLAYER_IDS, START_FEN, type Action, type AppState, type Color, type Position, type SearchResult, type Settings } from '../shared/types.js';

export class GameController extends EventEmitter {
  private queue: Promise<unknown> = Promise.resolve();
  private epoch = 0;
  private pending: SearchResult | null = null;
  private timer: ReturnType<typeof setTimeout> | null = null;
  private scheduleVersion = 0;
  private scored = false;
  private shuttingDown = false;
  state: AppState;

  constructor(readonly engine: EngineClient, readonly options: Options) {
    super();
    this.state = {
      position: null, settings: structuredClone(options.settings), flipped: false, paused: false,
      started: false, searching: false, busy: true, pendingMove: false, search: null,
      evaluation: null, error: null, connected: false, game: 1, match: newMatch(options.settings),
    };
    engine.on('progress', info => {
      if (!this.state.searching || this.state.busy) return;
      this.state.search = info;
      this.publish();
    });
    engine.on('log', text => this.emit('event', { type: 'log', text }));
    engine.on('failure', error => {
      this.epoch++; this.clearTimer(); this.pending = null;
      this.state.connected = false; this.state.searching = false;
      this.state.busy = false; this.state.started = false; this.state.pendingMove = false;
      this.state.error = error.message; this.publish();
    });
  }

  async initialize() {
    this.state.position = await this.engine.ready;
    this.state.position = await this.engine.request<Position>('reset', { fen: this.options.fen || START_FEN, random: this.state.settings.random });
    this.state.connected = true;
    this.state.busy = false;
    this.state.match.countCurrentGame = this.state.position.result === '*';
    this.recordResult();
    this.publish();
  }

  private publish() { this.emit('event', { type: 'state', state: structuredClone(this.state) }); }
  private clearTimer() { if (this.timer) clearTimeout(this.timer); this.timer = null; this.scheduleVersion++; }
  private serialize<T>(job: () => Promise<T>): Promise<T> {
    const result = this.queue.then(job);
    this.queue = result.catch(() => {});
    return result;
  }
  private error(error: unknown) {
    this.state.error = error instanceof Error ? error.message : String(error);
    this.publish();
  }

  dispatch(action: Action): Promise<void> {
    return this.serialize(async () => {
      try {
        if (!action || typeof action.type !== 'string') throw new Error('Invalid application action.');
        this.state.error = null;
        if (action.type === 'flip') this.state.flipped = !this.state.flipped;
        else if (action.type === 'clearError') this.state.error = null;
        else {
          if (!this.state.connected || !this.state.position) throw new Error('The engine is not connected.');
          switch (action.type) {
            case 'reset': await this.reset(action.fen ?? '', true); break;
            case 'resetScores': clearMatchScores(this.state.match); break;
            case 'resign': await this.resign(action.color); break;
            case 'start':
              if (this.state.position.result !== '*') throw new Error('Start a new game to play again.');
              this.state.started = true; this.state.paused = false;
              if (this.pending) await this.commitPending();
              break;
            case 'pause':
              this.state.paused = !this.state.paused;
              if (!this.state.paused && this.pending) await this.commitPending();
              break;
            case 'moveNow':
              if (this.pending) await this.commitPending();
              else if (this.state.searching) await this.engine.request('stop');
              else throw new Error('There is no computer move being calculated.');
              break;
            case 'move':
              if (this.state.position.result !== '*') throw new Error('This game has ended.');
              if (this.state.paused || this.state.searching || this.state.busy) throw new Error('Resume play before moving.');
              if (this.player().player !== 'human') throw new Error('It is the computer’s turn.');
              if (!this.state.position.legalMoves.includes(action.move)) throw new Error('That move is not legal.');
              this.state.started = true;
              this.state.position = await this.engine.request<Position>('move', { move: action.move });
              this.state.evaluation = null;
              this.recordResult();
              break;
            case 'settings': {
              const settings = this.validateSettings(action.settings);
              // Player/search edits take effect on the next search. A current search
              // completes with the settings it started with, as the UI explains.
              // Player identities and completed results survive configuration
              // edits. Only Reset scores clears the session's match totals.
              updateMatchSettings(this.state.match, settings);
              this.state.settings = settings;
              break;
            }
            case 'evaluate':
              if (this.state.searching || this.pending) throw new Error('Wait for the current search, or use Move now, before evaluating.');
              this.state.evaluation = (await this.engine.request<{ scoreCp: number }>('evaluate', { player: 3 })).scoreCp;
              this.state.search = null;
              break;
            default: throw new Error('Unknown application action.');
          }
        }
        this.publish(); this.schedule();
      } catch (error) { this.error(error); }
    });
  }

  private validateSettings(settings: Settings): Settings {
    if (!settings || typeof settings.autoReset !== 'boolean' || typeof settings.swapColors !== 'boolean') throw new Error('Invalid settings.');
    for (const side of [settings.white, settings.black]) {
      if (!side || !['human', 'shumi', 'ivan', 'slug', 'nnue'].includes(side.player)) throw new Error('Unknown player.');
      if (side.player === 'nnue' && !this.state.position?.nnueAvailable) throw new Error('The NNUE model is unavailable.');
      for (const [value, low, high, name] of [[side.depth, 1, 30, 'Depth'], [side.time, 0, 3600000, 'Time'], [side.features, 0, 2147483647, 'Features']] as const) {
        if (!Number.isInteger(value) || value < low || value > high) throw new Error(`${name} must be an integer between ${low} and ${high}.`);
      }
      side.features ||= 14;
    }
    if (!Number.isInteger(settings.random) || settings.random < 0 || settings.random > 100) throw new Error('Random opening moves must be between 0 and 100.');
    return structuredClone(settings);
  }

  private player() { return this.state.position?.turn === 'w' ? this.state.settings.white : this.state.settings.black; }
  private async resign(color: Color) {
    const position = this.state.position!;
    if (position.result !== '*') throw new Error('This game has ended.');
    if ((color !== 'w' && color !== 'b') || this.state.settings[color === 'w' ? 'white' : 'black'].player !== 'human')
      throw new Error('Only a human player can resign.');
    this.clearTimer(); this.epoch++;
    this.state.busy = true; this.publish();
    try {
      if (this.state.searching) await this.engine.request('stop');
      this.state.searching = false; this.pending = null; this.state.pendingMove = false;
      this.state.search = null; this.state.evaluation = null;
      const result = color === 'w' ? '0-1' : '1-0';
      // Resignation is a session result; the core retains the final board for
      // evaluation. All play goes through this controller until the next reset.
      this.state.position = {
        ...position, result, resigned: color, legalMoves: [], drawReason: '',
        pgn: position.pgn.replace(/\[Result "\*"\]/, `[Result "${result}"]`).replace(/\*\s*$/, result),
      };
      this.state.started = true;
      this.recordResult();
    } finally { this.state.busy = false; }
  }

  private async reset(fen: string, increment: boolean) {
    const swap = !fen.trim() && this.scored && this.state.match.countCurrentGame && this.state.settings.swapColors;
    this.clearTimer(); this.epoch++;
    this.state.busy = true; this.publish();
    try {
      if (this.state.searching) await this.engine.request('stop');
      this.state.searching = false; this.pending = null; this.state.pendingMove = false;
      this.state.started = false;
      const position = await this.engine.request<Position>('reset', { fen, random: this.state.settings.random });
      if (swap) {
        const settings = this.state.settings;
        [settings.white, settings.black] = [settings.black, settings.white];
        const match = this.state.match;
        [match.white, match.black] = [match.black, match.white];
      }
      this.state.position = position;
      if (increment) this.state.game++;
      this.state.started = true; this.state.paused = false;
      this.state.search = null; this.state.evaluation = null; this.scored = false;
      this.state.match.countCurrentGame = position.result === '*';
      this.recordResult();
    } finally { this.state.busy = false; }
  }

  private recordResult() {
    const result = this.state.position?.result;
    if (!result || result === '*' || this.scored) return;
    this.scored = true;
    recordMatchResult(this.state.match, result);
  }

  private async commitPending() {
    if (!this.pending) return;
    const result = this.pending;
    this.pending = null; this.state.pendingMove = false;
    this.state.position = await this.engine.request<Position>('move', { move: result.move });
    this.state.evaluation = result.scoreCp;
    this.recordResult();
  }

  private schedule() {
    this.clearTimer();
    const version = this.scheduleVersion;
    if (this.shuttingDown || !this.state.connected || !this.state.started || this.state.paused || this.state.searching || this.state.busy || this.pending) return;
    if (this.state.position?.result !== '*') {
      if (this.state.settings.autoReset) this.timer = setTimeout(() => {
        void this.serialize(async () => {
          if (version !== this.scheduleVersion || this.shuttingDown || !this.state.settings.autoReset || this.state.paused || !this.state.started || this.state.position?.result === '*') return;
          try { await this.reset('', true); this.publish(); this.schedule(); } catch (error) { this.error(error); }
        });
      }, 1800);
      return;
    }
    if (this.player().player === 'human') return;
    this.timer = setTimeout(() => { void this.serialize(async () => {
      if (version === this.scheduleVersion) this.beginSearch();
    }); }, 250);
  }

  private beginSearch() {
    if (this.shuttingDown || this.state.paused || this.state.searching || !this.state.started || this.state.position?.result !== '*') return;
    const player = this.player();
    if (player.player === 'human') return;
    const epoch = this.epoch;
    this.state.searching = true; this.state.search = null; this.publish();
    void this.engine.request<SearchResult>('search', { depth: player.depth, time: player.time, features: player.features, player: PLAYER_IDS[player.player] })
      .then(result => this.serialize(async () => {
        if (epoch !== this.epoch || this.shuttingDown) return;
        this.state.searching = false;
        this.state.search = result;
        if (!result.move) throw new Error('The engine did not return a move.');
        this.pending = result; this.state.pendingMove = true;
        if (!this.state.paused) await this.commitPending();
        this.publish(); this.schedule();
      }))
      .catch(error => {
        if (epoch !== this.epoch || this.shuttingDown) return;
        this.state.searching = false; this.state.started = false;
        this.error(error);
      });
  }

  async close() {
    this.shuttingDown = true; this.epoch++; this.clearTimer();
    await this.engine.close();
  }
}
