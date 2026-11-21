export type Color = 'w' | 'b';
export type NNUEPlayer = 'nnue1' | 'nnue2';
export type Player = 'human' | 'shumi' | 'ivan' | 'slug' | NNUEPlayer;
export interface PlayerSettings { player: Player; depth: number; time: number; features: number; }
export interface Settings { white: PlayerSettings; black: PlayerSettings; random: number; autoReset: boolean; swapColors: boolean; }
export type ParticipantId = 'a' | 'b';
export interface PlayerStats {
  settings: PlayerSettings; wins: number; draws: number; losses: number;
  gamesAsWhite: number; gamesAsBlack: number;
}
export interface MatchStats {
  players: Record<ParticipantId, PlayerStats>;
  white: ParticipantId; black: ParticipantId; completed: number; countCurrentGame: boolean;
}
export interface MoveRecord { uci: string; san: string; color: Color; number: number; }
export interface Position {
  fen: string; turn: Color; legalMoves: string[]; inCheck: boolean;
  result: string; drawReason: string; resigned?: Color; moves: MoveRecord[]; pgn: string;
  moveNumber: number; phase: string; whiteTimeMs: number; blackTimeMs: number;
  nnueAvailable: Record<NNUEPlayer, boolean>; defaultFeatures: number;
}
export interface SearchInfo { depth: number; nodes: number; elapsedMs: number; scoreCp: number; }
export interface SearchResult extends SearchInfo { move: string; }
export interface AppState {
  position: Position | null; settings: Settings; flipped: boolean; paused: boolean;
  started: boolean; searching: boolean; busy: boolean; pendingMove: boolean;
  search: SearchInfo | null; evaluation: number | null; error: string | null;
  moveScores: Record<Color, number | null>;
  fenRecording: boolean;
  connected: boolean; game: number; match: MatchStats;
}
export type Action =
  | { type: 'start' | 'pause' | 'flip' | 'moveNow' | 'evaluate' | 'clearError' | 'resetScores' | 'clearFens' }
  | { type: 'recordFens'; enabled: boolean }
  | { type: 'reset'; fen?: string }
  | { type: 'move'; move: string }
  | { type: 'resign'; color: Color }
  | { type: 'settings'; settings: Settings };
export type AppEvent = { type: 'state'; state: AppState } | { type: 'log'; text: string };
export interface AppApi {
  state(): Promise<AppState>;
  dispatch(action: Action): Promise<void>;
  copy(kind: 'fen' | 'pgn'): Promise<void>;
  quit(): Promise<void>;
  subscribe(callback: (event: AppEvent) => void): () => void;
}
export const PLAYER_NAMES: Record<Player, string> = { human: 'Human', shumi: 'Shumi', ivan: 'Ivan', slug: 'Slug', nnue1: 'NNUE1', nnue2: 'NNUE2' };
export const PLAYER_IDS: Record<Exclude<Player, 'human'>, number> = { shumi: 3, ivan: 2, slug: 1, nnue1: 0, nnue2: 4 };
export const START_FEN = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';
