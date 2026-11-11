// Classic cburnett artwork, bundled locally from Lichess.
// Copyright Colin M. L. Burnett; GPL-2.0-or-later.
// Attribution and source revision: ui/pieces/cburnett/README.md.
export const pieceNames: Record<string, string> = { p: 'pawn', r: 'rook', n: 'knight', b: 'bishop', q: 'queen', k: 'king' };

export function pieceImage(piece: string): string {
  if (!/^[prnbqkPRNBQK]$/.test(piece)) return '';
  const side = piece === piece.toUpperCase() ? 'w' : 'b';
  return `<img class="piece" src="pieces/cburnett/${side}${piece.toUpperCase()}.svg" alt="" aria-hidden="true" draggable="false">`;
}
