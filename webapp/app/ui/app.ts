import { PLAYER_NAMES, type AppApi, type AppState, type Action, type Color, type Player, type Settings } from '../shared/types.js';
import { pieceNames, pieceImage } from './pieces.js';

declare global { interface Window { shumi: AppApi; } }
const api = window.shumi;
let state: AppState;
let selected: string | null = null;
let boardKey = '';
let historyKey = '';
let previousFen = '';
let previousAssignments = '';
let toastTimer: ReturnType<typeof setTimeout>;
let logBuffer = '';
const $ = <T extends HTMLElement = HTMLElement>(id: string) => document.getElementById(id) as T;
const button = (id: string) => $<HTMLButtonElement>(id);
const input = (id: string) => $<HTMLInputElement>(id);
const text = (id: string, value: string) => { $(id).textContent = value; };

async function dispatch(action: Action) {
  try { await api.dispatch(action); } catch (error) { notify(String(error)); }
}
function notify(message: string) {
  clearTimeout(toastTimer); text('toast', message); $('toast').hidden = false;
  toastTimer = setTimeout(() => { $('toast').hidden = true; }, 2500);
}
function formatTime(milliseconds: number) {
  if (milliseconds < 60_000) return `${(milliseconds / 1000).toFixed(1)}s`;
  return `${Math.floor(milliseconds / 60_000)}m ${Math.floor(milliseconds / 1000) % 60}s`;
}
function scoreText(cp: number | null | undefined) {
  if (cp == null || Math.abs(cp) > 30000) return '—';
  if (Math.abs(cp) > 29000) return `${cp < 0 ? '−' : ''}M${Math.max(1, Math.ceil((30000 - Math.abs(cp)) / 2))}`;
  return `${cp > 0 ? '+' : ''}${(cp / 100).toFixed(2)}`;
}
function boardPieces() {
  const result = new Map<string, string>();
  const ranks = state.position!.fen.split(' ')[0].split('/');
  ranks.forEach((rank, index) => {
    let file = 0;
    for (const piece of rank) {
      if (/\d/.test(piece)) file += Number(piece);
      else { result.set(`${String.fromCharCode(97 + file)}${8 - index}`, piece); file++; }
    }
  });
  return result;
}
function humanCanMove() {
  if (!state.position || state.position.result !== '*' || state.paused || state.searching || state.busy || !state.connected) return false;
  return (state.position.turn === 'w' ? state.settings.white : state.settings.black).player === 'human';
}

function resigningColor(): Color | null {
  if (!state.position || state.position.result !== '*' || state.busy || !state.connected) return null;
  const white = state.settings.white.player === 'human';
  const black = state.settings.black.player === 'human';
  return white && black ? state.position.turn : white ? 'w' : black ? 'b' : null;
}

async function confirmResignation() {
  const color = resigningColor();
  if (!color) return;
  const game = state.game;
  const dialog = $<HTMLDialogElement>('resignation');
  text('resignation-title', `Resign as ${color === 'w' ? 'White' : 'Black'}?`);
  text('resignation-message', `${color === 'w' ? 'Black' : 'White'} wins this game if you resign.`);
  dialog.returnValue = 'cancel'; dialog.showModal();
  const confirmed = await new Promise<boolean>(resolve => dialog.addEventListener('close', () => resolve(dialog.returnValue === 'resign'), { once: true }));
  if (confirmed && state.game === game && resigningColor() === color) await dispatch({ type: 'resign', color });
}

function renderBoard(force = false) {
  if (!state.position) return;
  const position = state.position;
  if (previousFen !== position.fen) {
    selected = null;
    const dialog = $<HTMLDialogElement>('promotion');
    if (dialog.open) dialog.close('cancel');
    previousFen = position.fen;
  }
  const key = [position.fen, state.flipped, selected, humanCanMove(), position.inCheck].join('|');
  if (!force && key === boardKey) return;
  boardKey = key;
  const board = $('board');
  const focused = (document.activeElement as HTMLElement)?.dataset.square;
  board.replaceChildren();
  const pieces = boardPieces();
  const last = position.moves.at(-1)?.uci;
  const targets = new Set(selected ? position.legalMoves.filter(m => m.startsWith(selected!)).map(m => m.slice(2, 4)) : []);
  for (let row = 0; row < 8; row++) for (let col = 0; col < 8; col++) {
    const file = state.flipped ? 7 - col : col;
    const rank = state.flipped ? row + 1 : 8 - row;
    const square = `${String.fromCharCode(97 + file)}${rank}`;
    const piece = pieces.get(square);
    const cell = document.createElement('button');
    cell.type = 'button'; cell.className = `square ${(file + rank) % 2 === 1 ? 'dark' : ''}`;
    cell.dataset.square = square;
    cell.tabIndex = square === (focused || selected || 'e2') ? 0 : -1;
    if (last?.slice(0, 2) === square || last?.slice(2, 4) === square) cell.classList.add('last');
    if (selected === square) cell.classList.add('selected');
    if (targets.has(square)) cell.classList.add('legal', ...(piece ? ['capture'] : []));
    if (position.inCheck && piece === (position.turn === 'w' ? 'K' : 'k')) cell.classList.add('check');
    const pieceName = piece ? `${piece === piece.toUpperCase() ? 'White' : 'Black'} ${pieceNames[piece.toLowerCase()]}` : 'Empty';
    cell.setAttribute('aria-label', `${square}, ${pieceName}${targets.has(square) ? ', legal destination' : ''}`);
    cell.setAttribute('aria-pressed', String(selected === square));
    cell.setAttribute('aria-disabled', String(!humanCanMove()));
    if (piece) cell.innerHTML = pieceImage(piece);
    if (row === 7) {
      const label = document.createElement('span'); label.className = 'coord file'; label.textContent = square[0]; cell.append(label);
    }
    if (col === 0) {
      const label = document.createElement('span'); label.className = 'coord rank'; label.textContent = square[1]; cell.append(label);
    }
    cell.onclick = () => { void squareClicked(square); };
    cell.onkeydown = event => {
      const offset: Record<string, number> = { ArrowLeft: -1, ArrowRight: 1, ArrowUp: -8, ArrowDown: 8 };
      if (event.key in offset) {
        event.preventDefault();
        const cells = [...board.querySelectorAll<HTMLButtonElement>('button')];
        const target = cells[Math.max(0, Math.min(63, row * 8 + col + offset[event.key]))];
        cells.forEach(c => { c.tabIndex = c === target ? 0 : -1; }); target.focus();
      }
    };
    board.append(cell);
  }
  if (focused) board.querySelector<HTMLButtonElement>(`[data-square="${focused}"]`)?.focus();
}

async function squareClicked(square: string) {
  if (!humanCanMove()) return;
  const position = state.position!;
  const possible = selected ? position.legalMoves.filter(m => m.startsWith(selected! + square)) : [];
  if (possible.length) {
    let move = possible[0];
    if (possible.some(m => m.length === 5)) {
      const fen = position.fen;
      const choice = await choosePromotion(position.turn, possible);
      if (!choice || state.position?.fen !== fen || !humanCanMove()) return;
      move = choice;
    }
    selected = null;
    await dispatch({ type: 'move', move });
  } else {
    selected = selected === square ? null : position.legalMoves.some(m => m.startsWith(square)) ? square : null;
    renderBoard(true);
  }
}

function choosePromotion(color: Color, moves: string[]): Promise<string | null> {
  const dialog = $<HTMLDialogElement>('promotion');
  const options = $('promotion-options'); options.replaceChildren();
  for (const piece of ['q', 'r', 'b', 'n']) {
    const move = moves.find(m => m.endsWith(piece));
    if (!move) continue;
    const item = document.createElement('button');
    item.value = move;
    item.setAttribute('aria-label', `Promote to ${pieceNames[piece]}`);
    item.innerHTML = pieceImage(color === 'w' ? piece.toUpperCase() : piece);
    const label = document.createElement('span'); label.textContent = pieceNames[piece]; item.append(label);
    options.append(item);
  }
  dialog.returnValue = 'cancel'; dialog.showModal();
  return new Promise(resolve => dialog.addEventListener('close', () => resolve(dialog.returnValue === 'cancel' ? null : dialog.returnValue), { once: true }));
}

function createPlayerSettings() {
  for (const side of ['white', 'black'] as const) {
    const card = document.createElement('div'); card.className = 'player-card';
    card.innerHTML = `<div class="player-select-line"><span class="color-chip ${side}" aria-hidden="true"></span><label id="${side}-label" for="${side}-player">${side === 'white' ? 'White' : 'Black'}</label><select id="${side}-player" aria-label="${side} player"></select></div><div class="search-settings"><label>Depth<input id="${side}-depth" type="number" min="1" max="30" aria-label="${side} search depth"></label><label>Time · ms<input id="${side}-time" type="number" min="0" max="3600000" step="100" aria-label="${side} search time in milliseconds"></label><label>Features<input id="${side}-features" type="text" spellcheck="false" aria-label="${side} feature mask" title="Feature bitmask, decimal or hexadecimal; 0 selects defaults"></label></div>`;
    $('player-settings').append(card);
    const select = $<HTMLSelectElement>(`${side}-player`);
    for (const [value, name] of Object.entries(PLAYER_NAMES)) { const option = document.createElement('option'); option.value = value; option.text = name; select.add(option); }
    for (const suffix of ['player', 'depth', 'time', 'features']) $(`${side}-${suffix}`).addEventListener('change', applySettings);
  }
}
async function applySettings() {
  if (!state) return;
  const settings: Settings = structuredClone(state.settings);
  for (const side of ['white', 'black'] as const) {
    settings[side] = {
      player: $<HTMLSelectElement>(`${side}-player`).value as Player,
      depth: Number(input(`${side}-depth`).value), time: Number(input(`${side}-time`).value),
      features: Number(input(`${side}-features`).value),
    };
  }
  settings.random = Number(input('random').value); settings.autoReset = input('auto-reset').checked;
  settings.swapColors = $<HTMLSelectElement>('color-mode').value === 'swap';
  await dispatch({ type: 'settings', settings });
}
function renderPlayers() {
  const assignments = `${state.match.white}${state.match.black}`;
  const swapped = assignments !== previousAssignments;
  previousAssignments = assignments;
  for (const side of ['white', 'black'] as const) {
    const settings = state.settings[side];
    text(`${side}-label`, `${side === 'white' ? 'White' : 'Black'} · ${state.match[side].toUpperCase()}`);
    for (const key of ['player', 'depth', 'time', 'features'] as const) {
      const field = $<HTMLInputElement>(`${side}-${key}`);
      if (swapped || document.activeElement !== field) field.value = key === 'features' ? `0x${Number(settings[key]).toString(16).toUpperCase()}` : String(settings[key]);
    }
    $<HTMLSelectElement>(`${side}-player`).querySelector<HTMLOptionElement>('option[value="nnue"]')!.disabled = !state.position?.nnueAvailable;
  }
  input('auto-reset').checked = state.settings.autoReset;
  $<HTMLSelectElement>('color-mode').value = state.settings.swapColors ? 'swap' : 'keep';
  text('color-mode-hint', state.settings.swapColors ? 'Players swap colors after each completed game, taking all their settings with them.' : 'Each player keeps the same color.');
  if (document.activeElement !== input('random')) input('random').value = String(state.settings.random);
  for (const [id, color] of [['top-player', state.flipped ? 'w' : 'b'], ['bottom-player', state.flipped ? 'b' : 'w']] as const) {
    const side = color === 'w' ? 'white' : 'black';
    const settings = state.settings[side];
    const container = $(id); container.replaceChildren();
    const avatar = document.createElement('span'); avatar.className = `avatar ${side}`; avatar.innerHTML = pieceImage(color === 'w' ? 'K' : 'k');
    const label = document.createElement('span'); label.className = 'player-label'; label.textContent = PLAYER_NAMES[settings.player];
    const sub = document.createElement('span'); sub.className = 'player-sub'; sub.textContent = `${color === 'w' ? 'White' : 'Black'} · ${state.match[side].toUpperCase()}${settings.player === 'human' ? '' : ` · depth ${settings.depth}`}`;
    container.append(avatar, label, sub);
    if (state.position?.turn === color && state.position.result === '*') { const dot = document.createElement('span'); dot.className = 'turn-indicator'; container.append(dot); }
    const time = document.createElement('span'); time.className = 'time';
    time.title = 'Total engine thinking time this game'; time.textContent = formatTime((side === 'white' ? state.position?.whiteTimeMs : state.position?.blackTimeMs) ?? 0);
    container.append(time);
  }
}
function renderHistory() {
  const moves = state.position?.moves ?? [];
  const key = JSON.stringify(moves);
  if (key !== historyKey) {
    historyKey = key;
    const list = $('moves'); list.replaceChildren();
    if (!moves.length) list.innerHTML = '<p class="empty">Every game starts with a possibility.<br><span>Make a move or choose your players.</span></p>';
    const rows = new Map<number, HTMLElement>();
    for (const move of moves) {
      let row = rows.get(move.number);
      if (!row) {
        row = document.createElement('div'); row.className = 'move-row';
        for (const content of [String(move.number).padStart(2, '0'), '…', '…']) { const span = document.createElement('span'); span.textContent = content; row.append(span); }
        rows.set(move.number, row); list.append(row);
      }
      const span = row.children[move.color === 'w' ? 1 : 2] as HTMLElement;
      span.textContent = move.san; span.title = move.uci;
      if (move === moves.at(-1)) span.className = 'latest';
    }
    list.scrollTop = list.scrollHeight;
  }
  text('game-number', `Game ${state.game}`);
}
function renderMatch() {
  const match = state.match;
  text('completed-games', `${match.completed} ${match.completed === 1 ? 'game' : 'games'} completed`);
  text('match-mode', state.settings.swapColors ? 'Alternating colors' : 'Fixed colors');
  const rows = $('match-results'); rows.replaceChildren();
  for (const id of ['a', 'b'] as const) {
    const player = match.players[id];
    const row = document.createElement('tr'); row.dataset.player = id;
    const identity = document.createElement('td');
    const name = document.createElement('strong'); name.textContent = `${PLAYER_NAMES[player.settings.player]} ${id.toUpperCase()}`;
    const color = document.createElement('small'); color.textContent = match.white === id ? 'Now White' : 'Now Black';
    identity.title = `Depth ${player.settings.depth} · ${player.settings.time} ms · Features 0x${player.settings.features.toString(16).toUpperCase()}\nCompleted as White: ${player.gamesAsWhite}; as Black: ${player.gamesAsBlack}`;
    identity.append(name, color); row.append(identity);
    for (const [stat, value] of [['wins', player.wins], ['draws', player.draws], ['losses', player.losses], ['points', player.wins + player.draws / 2]] as const) {
      const cell = document.createElement('td'); cell.dataset.stat = stat; cell.textContent = String(value); row.append(cell);
    }
    rows.append(row);
  }
  text('score-note', match.countCurrentGame ? 'Win = 1 point · Draw = ½ · Reset scores clears totals' : 'Loaded finished positions do not add a result.');
  $('score-note').classList.toggle('score-pending', !match.countCurrentGame);
  button('reset-scores').disabled = !state.connected || state.busy || !match.completed;
}
function render() {
  renderBoard(); renderPlayers(); renderHistory(); renderMatch();
  const position = state.position;
  const ended = position && position.result !== '*';
  text('connection', state.connected ? 'Engine connected' : 'Engine offline');
  $('connection').classList.toggle('offline', !state.connected);
  let status = 'Choose your players, then start a game.';
  let title = 'Your next move.';
  if (!state.connected) status = state.busy ? 'Starting the engine…' : 'The engine is unavailable.';
  else if (state.busy) status = 'Preparing the position…';
  else if (ended) {
    title = position.result === '1/2-1/2' ? 'A game shared.' : `${position.result === '1-0' ? 'White' : 'Black'} wins.`;
    status = position.result === '1/2-1/2' ? `Draw · ${position.drawReason}` : position.resigned ? `${position.resigned === 'w' ? 'White' : 'Black'} resigned` : 'Checkmate';
    if (state.started && state.settings.autoReset) status += state.paused ? ' · continuous play paused' : ' · next game shortly';
  }
  else if (state.paused) { title = 'A moment to think.'; status = state.pendingMove ? 'Paused · computer move ready for Resume' : 'Paused · the board will stay here until Resume'; }
  else if (state.searching) { title = `${PLAYER_NAMES[(position?.turn === 'w' ? state.settings.white : state.settings.black).player]} is thinking.`; status = `Searching for ${position?.turn === 'w' ? 'White' : 'Black'}${state.search ? ` · depth ${state.search.depth}` : ''}`; }
  else if (state.started) { title = `${position?.turn === 'w' ? 'White' : 'Black'} to move.`; status = position?.inCheck ? 'Your king is in check.' : 'Select a piece to see its legal moves.'; }
  text('game-title', title); text('status', status); text('phase', position?.phase ?? '');
  $('status-dot').classList.toggle('thinking', state.searching && !state.paused);
  button('start').textContent = ended ? 'New game →' : state.paused ? 'Resume game →' : state.started ? 'Game in progress' : 'Start game →';
  button('start').disabled = !state.connected || state.busy || (state.started && !state.paused && !ended);
  button('pause').innerHTML = `${state.paused ? 'Resume' : 'Pause'} <kbd>1</kbd>`;
  button('pause').disabled = !state.connected || state.busy || (!!ended && !state.settings.autoReset && !state.paused);
  button('move-now').disabled = !state.connected || (!state.searching && !state.pendingMove) || state.busy;
  button('move-now').textContent = state.pendingMove ? 'Play ready move' : 'Move now';
  const resignColor = resigningColor();
  button('resign').disabled = !resignColor;
  button('resign').title = resignColor ? `Resign as ${resignColor === 'w' ? 'White' : 'Black'}` : 'Available to human players during a game';
  button('evaluate').disabled = !state.connected || state.searching || state.pendingMove || state.busy;
  for (const id of ['new-game', 'load-fen']) button(id).disabled = !state.connected || state.busy;
  for (const id of ['copy-fen', 'copy-pgn']) button(id).disabled = !position;
  text('evaluation', scoreText(state.search?.scoreCp ?? state.evaluation));
  text('depth', state.search ? String(state.search.depth) : '—');
  text('nodes', state.search ? state.search.nodes.toLocaleString() : '—');
  text('elapsed', state.search ? formatTime(state.search.elapsedMs) : '—');
  text('nps', state.search?.elapsedMs ? Math.round(state.search.nodes * 1000 / state.search.elapsedMs).toLocaleString() : '—');
  $('error').hidden = !state.error; $('error').querySelector('span')!.textContent = state.error ?? '';
}

createPlayerSettings();
button('flip').onclick = () => { void dispatch({ type: 'flip' }); };
button('start').onclick = () => { void dispatch(state.position?.result !== '*' ? { type: 'reset' } : { type: 'start' }); };
button('pause').onclick = () => { void dispatch({ type: 'pause' }); };
button('move-now').onclick = () => { void dispatch({ type: 'moveNow' }); };
button('resign').onclick = () => { void confirmResignation(); };
button('new-game').onclick = () => { void dispatch({ type: 'reset' }); };
button('reset-scores').onclick = () => { void dispatch({ type: 'resetScores' }); };
button('load-fen').onclick = () => { void dispatch({ type: 'reset', fen: $<HTMLTextAreaElement>('fen').value.trim() }); };
button('evaluate').onclick = () => { $('analysis').setAttribute('open', ''); void dispatch({ type: 'evaluate' }); };
button('dismiss-error').onclick = () => { void dispatch({ type: 'clearError' }); };
button('quit').onclick = () => { void api.quit(); };
input('auto-reset').onchange = applySettings;
$('color-mode').onchange = applySettings;
input('random').onchange = applySettings;
for (const kind of ['fen', 'pgn'] as const) button(`copy-${kind}`).onclick = async () => {
  try {
    await api.copy(kind);
    if (kind === 'fen' && state.position) $<HTMLTextAreaElement>('fen').value = state.position.fen;
    notify(`${kind.toUpperCase()} copied to clipboard`);
  } catch (error) { notify(String(error)); }
};
document.addEventListener('keydown', event => {
  if (document.querySelector('dialog[open]')) return;
  if (event.key === 'Escape') { void api.quit(); return; }
  if ((event.target as HTMLElement).closest('input,textarea,select')) return;
  if (event.key === '1' || event.key === ' ') { event.preventDefault(); if (!button('pause').disabled) void dispatch({ type: 'pause' }); }
  else if (event.key === '2' && !button('evaluate').disabled) { $('analysis').setAttribute('open', ''); void dispatch({ type: 'evaluate' }); }
  else if (event.key.toLowerCase() === 'f') void dispatch({ type: 'flip' });
});
api.subscribe(event => {
  if (event.type === 'state') { state = event.state; render(); }
  else {
    logBuffer = (logBuffer + event.text.replace(/\x1b\[[0-9;]*[A-Za-z]/g, '')).slice(-30_000);
    const log = $('log'); const atEnd = log.scrollHeight - log.scrollTop - log.clientHeight < 40;
    log.textContent = logBuffer;
    if (atEnd) log.scrollTop = log.scrollHeight;
  }
});
state = await api.state();
render();
