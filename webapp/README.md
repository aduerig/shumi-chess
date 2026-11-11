# Shumi Chess desktop

The replacement frontend runs locally on Windows using Electron, TypeScript,
HTML, and CSS. It has no Python runtime or build dependency. All application code,
native bridge code, build output, and local data are contained in `webapp/`.
The existing engine sources, C++ drivers, and Python frontend remain unchanged.

## Launch

**Double-click `webapp\Play.bat` in File Explorer.** It uses depth 9, 5000 ms,
and opening variety 2. For a desktop shortcut, right-click `Play.bat`, choose
**Show more options → Send to → Desktop (create shortcut)**. Create a shortcut
to the file rather than moving the batch file out of `webapp/`.

From PowerShell in the repository root, the short command is:

```powershell
.\webapp\Play.bat
```

You can still supply options, for example `.\webapp\Play.bat -d7 -t2000`, or use
the original npm command:

```powershell
clear; npm.cmd --prefix webapp start -- -d9 -t5000 -r2
```

The launcher builds incrementally, then opens the desktop window. Its initial
size respects the monitor's available work area. The board scales with window
height so the starting position and play controls fit without scrolling; the
rest of the page can scroll as analysis and other controls expand.

For a fresh checkout, first install the locked dependencies:

```powershell
npm.cmd --prefix webapp ci
```

Development prerequisites are Node.js 22 or later, npm, CMake 3.20 or later, and
Visual Studio C++ Build Tools with a Windows SDK. These are already installed
on the PC used for development. The first installation also downloads Electron.
Subsequent launches need no network connection. This is a source checkout launch,
not a packaged installer.

## Playing and watching

Choose Human, Shumi, Ivan, Slug, or NNUE independently for White and Black. NNUE
is available when the existing model loads. Human versus bot and bot versus bot
use the same controls. Choose **Start game** to begin; a human move also starts
the game. Click a piece, then a marked destination. Promotions offer all four
pieces, including underpromotion. Arrow keys move keyboard focus on the board;
Enter selects a square.

| Previous control | Replacement |
| --- | --- |
| Flip Board | **Flip board** or **F**; also works during a search |
| White / Black player cycle | Separate player dropdowns |
| Reset to FEN above | **Load position**; an empty FEN starts the normal position |
| Start a fresh game | **New game**, keeping player and search settings |
| Auto reset | **Continuous play**, with a short delay after each result |
| Repeated-game colors | **Between games → Keep colors** (default) or **Swap colors** |
| Wake up | **Move now**, stopping search and using its best available move |
| Export FEN | **Copy FEN**, also populating the position box and printing to the terminal |
| Export PGN | **Copy PGN**, also printing to the terminal |
| Pause shortcut | **Pause / Resume**, **1**, or Space |
| Evaluate shortcut | **Evaluate** in Analysis desk, or **2** |
| Exit | **Exit**, Escape, or the window close button |

Pause freezes the board. An active search can finish, but its move is held until
Resume. **Play ready move** can apply that held move while remaining paused.
**Resign** ends the game for the human player after confirmation, even during
the computer's turn or while paused. With two humans, it applies to the side to
move. The opponent receives the win in match scores and PGN; continuous play
and color swapping continue as configured. Resigning while paused holds the
final position until Resume or New game. The button is disabled for bot-versus-bot games.
New game, FEN loading, and closing stop an active search before changing or
releasing engine state. Old search results cannot leak into the next game.

Depth, requested time in milliseconds, and feature masks are editable per side.
Changes affect the next search. A zero feature mask selects the existing default
mask (`0xE`). Opening variety randomizes the first N computer moves using the
existing engine behavior; edits apply to the next game. The engine retains its
own time/depth stopping rules, so the requested time is not a strict wall-clock
deadline. Player changes also take effect after the current search finishes.

The move list uses SAN; exports include checks, checkmate, promotion, the result,
and setup tags for a custom position. Scores are shown from White's perspective.
Evaluate calls Shumi's static evaluation. The analysis drawer and launch terminal
show engine diagnostics. The small player timers measure engine thinking time,
not tournament clocks. Match scores count completed games during this session.

## Continuous matches and player scores

Set the two players, enable **Continuous play**, and select the behavior under
**Between games**:

- **Keep colors** is the default. For example, Ivan stays White and Shumi stays
  Black for every game.
- **Swap colors** alternates the two players after each completed game. The
  evaluator, depth, time, and feature mask all move with their player. The board's
  viewing direction stays as you set it with **Flip board**.

Players have stable labels **A** and **B**, initially White and Black. These labels
keep two configurations distinct even when both use Shumi. The **Match scores**
table follows each player across color swaps and shows wins, draws, losses, and
points (one per win, half per draw), plus the number of completed games. Hover
over a player name to see its search settings and completed games by color.

**Reset scores** clears the totals without changing the current board, settings,
or colors. Turning continuous play on/off, changing the color mode, or editing
player/search settings preserves scores. Choosing **New game** after a completed game also honors the selected
color mode; abandoning an unfinished game does not swap colors or add a result.
Pause can hold the final position between games, then Resume continues the match.

Totals belong to participants A and B for the session, including games where
their evaluator, depth, time, or feature mask changes. Settings shown in the
score table describe each participant's current configuration. Use **Reset
scores** when you want a fresh comparison. Every game played to a result counts;
loading an already finished FEN adds no result. Scores are held for the current
app session and are not saved when the app closes.

## Command line

Attached values such as `-d9` and separate values such as `-d 9` both work.

| Option | Meaning |
| --- | --- |
| `-d`, `--depth` | Common depth; default 9, range 1–30 |
| `-t`, `--time` | Common time in milliseconds; default 5000 |
| `-r`, `--rand` | Initial randomized computer moves; default 2 |
| `-wd`, `-bd` | White / Black depth override |
| `-wt`, `-bt` | White / Black time override |
| `-f`, `--feat` | Common feature bitmask |
| `-wf`, `-bf` | White / Black feature override |
| `--fen "..."` | Starting FEN |
| `--human` | Start with two human players |
| `--debug` | Build and run the native Debug configuration |
| `--asserts`, `--no-asserts` | Enable or disable forced engine assertions; enabled by default |
| `--help` | Print launcher help |

Feature masks accept decimal, hexadecimal, and binary. Debug builds retain the
compiler's normal debug assertions. Without `--human`, the initial matchup is
Human White versus Shumi Black. Command-line settings can be edited in the app.

## Application boundary

```text
HTML/CSS/TypeScript board and controls
                 |
        window.shumi application API
                 |
     Electron preload / IPC transport
                 |
       GameController (TypeScript)
                 |
        EngineClient / JSON lines
                 |
       shumi_bridge.exe (new C++)
           |                 |
        Engine           MinimaxAI
           |                 |
       GameBoard <--- search uses the same Engine
```

Each desktop window launches one bridge process. It owns one persistent Engine
(which owns its GameBoard) and one MinimaxAI referencing that Engine. The selected
White or Black evaluator and settings are supplied for each search; two MinimaxAI
objects are not needed. Engine supplies legal moves, game state, and results.
MinimaxAI supplies searching and evaluation. No chess legality is reimplemented
in the frontend.

The old Python `engine_communicator` remains available for the old frontend. Its
replacement responsibilities are split between `native/bridge.cpp` (adapting the
existing C++ API) and `GameController` (session state, scheduling, pause, settings,
resignation, and match scores). Resignation is recorded in the controller while
the engine retains the final board for evaluation. Nothing was moved into or
changed inside the existing Engine.
The bridge's CMake project independently compiles the existing shared sources;
it does not modify or build the existing drivers or top-level CMake project.

Bridge commands are `snapshot`, `reset`, `move`, `search`, `stop`, `evaluate`, and
`quit`. Each JSON-line request has a numeric `id` and `command`; responses use
the same ID with `ok` and `data` or `error`. `ready` and `progress` events arrive
asynchronously. Only protocol messages use stdout; engine diagnostics use stderr.
While searching, the bridge accepts stop/quit and rejects board access. The
controller publishes cached position snapshots, serializes commands, and discards
stale results after reset. Search runs outside the Electron UI thread.

For a later hosted web application, `app/ui/` and `app/shared/` contain no Electron
or Node imports. A browser transport can implement the same application API,
while a server hosts the controller and a native bridge per active session.
Authentication, session isolation, transport security, hosting, and deployment
belong to that future project. The current app does not start an HTTP server.

## Files and checks

| Path | Purpose |
| --- | --- |
| `Play.bat` | Double-click Windows launcher; forwards optional CLI arguments |
| `app/ui/`, `ui/` | Renderer, bundled classic SVG pieces, layout, and controls |
| `app/shared/types.ts` | Frontend application contract |
| `app/desktop/` | Desktop host, controller, CLI parsing, and native client |
| `native/` | C++ bridge, independent CMake target, vendored JSON parser |
| `scripts/` | Build, launch, renderer test, and CRLF checks |
| `tests/` | Native/controller integration and renderer regression tests |
| `data/session.log` | Local diagnostic log, appended across launches |
| `data/electron/` | Local Electron profile |

Generated `build/`, `dist/`, `node_modules/`, `data/`, and `artifacts/` are ignored.
All authored text uses CRLF; `.gitattributes` and `.editorconfig` preserve it.

```powershell
npm.cmd --prefix webapp run build
npm.cmd --prefix webapp test
npm.cmd --prefix webapp run test:ui
npm.cmd --prefix webapp run check:crlf
```

The renderer test uses an invisible Electron window and the real native bridge.
It checks piece/square geometry at multiple viewport sizes, legal move markers,
human moves, history, flip, FEN loading/export, promotion, and evaluation. Native
tests cover special moves, draws, invalid inputs, search cancellation, pause,
reset races, continuous play, and bot versus bot. Match tests additionally cover
fixed/alternating colors, full setting swaps, separate player identities,
win/draw/loss totals, changing configurations, and pausing between games. Renderer
tests play complete games through the UI to verify swaps and scores with the real
engine. Test profiles stay in `artifacts/`.

The bridge validates FEN before passing it to the core's assertion-based parser.
The unchanged core accepts FEN counters only up to 255; larger imported counters
are rejected with an explanation. Undo/redo, PGN import, restoring saved sessions,
and a distributable installer are not implemented in this version.

## Linux status

Electron supports Linux, and the renderer and JSON-line bridge transport are
portable. The launcher already selects the Unix executable name and CMake build
configuration outside Windows. A Linux installation would need its own npm
dependencies and a native C++ rebuild; Windows binaries are not reusable there.

This version has been built and tested on Windows only. Inspection found an
existing core portability issue: `src/weights.hpp` includes `Score.hpp`, but the
file is `src/score.hpp`. This must be resolved for a normal case-sensitive Linux
filesystem. The existing C++ was left unchanged as requested. A Linux compiler
and desktop test pass is still required before claiming Linux support.

See [THIRD_PARTY.md](THIRD_PARTY.md) for dependency attribution.
