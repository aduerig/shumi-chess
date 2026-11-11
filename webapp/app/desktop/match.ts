import type { MatchStats, PlayerSettings, PlayerStats, Settings } from '../shared/types.js';

function emptyStats(settings: PlayerSettings): PlayerStats {
  return { settings: structuredClone(settings), wins: 0, draws: 0, losses: 0, gamesAsWhite: 0, gamesAsBlack: 0 };
}

export function newMatch(settings: Settings, countCurrentGame = true): MatchStats {
  return {
    players: { a: emptyStats(settings.white), b: emptyStats(settings.black) },
    white: 'a', black: 'b', completed: 0, countCurrentGame,
  };
}

export function updateMatchSettings(match: MatchStats, settings: Settings) {
  match.players[match.white].settings = structuredClone(settings.white);
  match.players[match.black].settings = structuredClone(settings.black);
}

export function clearMatchScores(match: MatchStats) {
  match.players.a = emptyStats(match.players.a.settings);
  match.players.b = emptyStats(match.players.b.settings);
  match.completed = 0;
}

// The controller calls this once per eligible completed game, before swapping
// colors. Participant IDs remain stable even when both use the same evaluator.
export function recordMatchResult(match: MatchStats, result: string) {
  if (!match.countCurrentGame || !['1-0', '0-1', '1/2-1/2'].includes(result)) return;
  const white = match.players[match.white];
  const black = match.players[match.black];
  white.gamesAsWhite++; black.gamesAsBlack++; match.completed++;
  if (result === '1/2-1/2') { white.draws++; black.draws++; }
  else {
    const [winner, loser] = result === '1-0' ? [white, black] : [black, white];
    winner.wins++; loser.losses++;
  }
}
