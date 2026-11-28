# Solitaire Collection

A collection of solitaire card games for Windows, written in C++17 against the
Win32 API and GDI directly — no frameworks, no game engine, no third-party
libraries.

## Games

| Family              | Games                                                    |
|---------------------|----------------------------------------------------------|
| Klondike family     | Klondike, Yukon, Russian Solitaire                       |
| Open-board strategy | FreeCell, Baker's Dozen, Beleaguered Castle, La Belle Lucie |
| Spider family       | Spider, Spiderette, Scorpion                             |
| Two decks           | Forty Thieves                                            |
| Up-or-down chains   | Golf, TriPeaks, Things                                   |
| Pairing             | Pyramid, Kings Corners                                   |
| Puzzles             | Gaps, Accordion                                          |

**Things** is an original game: an endless Golf variant with two wild jokers,
where clearing a board deals a fresh one and the score keeps climbing.

Klondike has options for Draw 1 / Draw 3, Standard or Vegas scoring and a
cumulative Vegas bank. Spider can be played with 1, 2 or 4 suits. Every game's
rules and scoring are in the app under Help → How to Play (F1).

## Features

- Drag-and-drop, or click a card to select it and click where it should go.
  Games played by clicking (Pyramid, TriPeaks, Golf, Gaps and others) turn
  dragging off, so a slightly shaky click still counts
- Double-click sends a card to its foundation; right-click (or F3) auto-plays
  everything possible to the foundations. Where it's always safe, cards go to
  the foundations by themselves
- Undo, except for moves that reveal a new card in Golf, TriPeaks, Things and
  Kings Corners. In Spider, Spiderette and Scorpion each undo costs a point,
  and the move it takes back isn't refunded
- Automatic finish once the game is decided
- Best score saved per game and variant
- Numbered, reproducible deals: replay the current deal or start any deal by
  number. FreeCell deal numbers match Microsoft FreeCell
- Configurable background and card-back colors, optional four-color deck
- Pixel-art cards scaled by whole numbers; the window sizes itself to each
  game's layout; per-monitor DPI aware

## Requirements

**To run:** Windows 7 or later (Vista likely works but is untested). No
installs needed. The C++ runtime is built into the executable.

The source sticks to Windows XP-era APIs, so an XP build is possible with an
XP-capable toolset (e.g. VS2017 `v141_xp`), but that isn't tested.

**To build:**

- [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) or the
  free *Build Tools for Visual Studio 2022*, with the **Desktop development
  with C++** workload (this includes MSVC, the Windows SDK, CMake and Ninja)

## Building

### Option 1: build script

From a regular command prompt in the repository root:

```bat
build.bat            :: Release build -> build\Release\Solitaire.exe
build.bat Debug      :: Debug build   -> build\Debug\Solitaire.exe
```

The script locates Visual Studio with `vswhere`, sets up the MSVC environment,
then configures and builds with CMake + Ninja.

### Option 2: CMake manually

From a *Developer Command Prompt for VS 2022* (or any shell where
`vcvars64.bat` has been run):

```bat
cmake -S . -B build\Release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build\Release
```

### Option 3: Visual Studio

Open the folder in Visual Studio 2022 (*File → Open → Folder*). It detects
`CMakeLists.txt` automatically; choose `Solitaire.exe` as the startup item and
press F5.

## Running

```bat
build\Release\Solitaire.exe
```

The executable is self-contained (card art and dialogs are embedded as
resources), so it can be copied anywhere and run directly.

Optional command-line argument:

| Argument     | Effect                           |
|--------------|----------------------------------|
| `--deal N`   | Start on deal number `N`         |

Settings and high scores are stored in `%APPDATA%\Solitaire\solitaire.ini`.
Delete that file to reset everything.

## Controls

| Action                        | Input                                   |
|-------------------------------|-----------------------------------------|
| Move cards                    | Drag, or click card then click target   |
| Send card to foundation       | Double-click                            |
| Auto-play to foundations      | Right-click or F3                       |
| Deal from stock / redeal      | Click the stock or redeal button        |
| Undo                          | Ctrl+Z                                  |
| New game                      | F2                                      |
| Replay or pick a deal         | Play → Restart This Deal / New Game from Deal # |
| Switch game                   | Games menu                              |
| Options for the current game  | F4                                      |
| Colors and four-color deck    | Options → Appearance                    |
| How to play (every game)      | F1                                      |

## Architecture

```
src/
  main.cpp              entry point
  App.*                 main window: input, dragging, selection, slide animation,
                        painting, menus, dialogs
  core/
    Card.h              suit / rank / face-up
    Deck.*              builds and shuffles one or more decks (seeded, so deals are reproducible)
    Pile.*              a positioned stack of cards with fanning and hit-testing
    Game.*              abstract base: rules interface, undo, scoring, high-score recording
    CardRenderer.*      loads the sprite sheet, recolors and scales it, draws cards and empty slots
    Settings.*          INI-backed settings and high scores
    GameRegistry.*      list of available games (drives the Games menu)
  games/
    Klondike.*, ...     one file pair per game (Russian Solitaire is a header
                        that reuses Yukon)
res/                    sprite sheet, menus, dialogs, manifest
tools/                  script that builds res/cards.bmp from the art pack
```

The app knows nothing about specific games. Each game subclasses `Game`, lays
out its piles and answers rule questions (`CanPickUp`, `CanDrop`, `Click`,
`IsWon`, ...). Dragging, click-to-move, animation, undo and drawing are shared.
A game can also opt out of dragging (`AllowDrag`), hide empty piles
(`ShowSlot`), play cards to the foundations by itself (`FindAutoMove`,
`CanAutoComplete`), and charge points for undo (`UndoPenalty`).

Rendering is double-buffered GDI: each frame is drawn to an off-screen bitmap
and copied to the window in one `BitBlt`. Cards are drawn with `TransparentBlt`
from a sprite sheet that is recolored and nearest-neighbour scaled whenever the
window size or appearance settings change.

### Adding a game

1. Add `src/games/MyGame.h/.cpp` deriving from `Game`, including its `Rules()`
   text for Help → How to Play.
2. Add one line to `AllGames()` in `src/core/GameRegistry.cpp`.
3. Add the `.cpp` to `CMakeLists.txt`.

### Regenerating the card sheet

`res/cards.bmp` is checked in. To rebuild it from the original sheet
(`res/95ishdeck.gif`), which also draws the jokers:

```powershell
pwsh tools/make_cardsheet.ps1
```

## License

The source code is released under the [MIT License](LICENSE).

The card art in `res/cards.bmp` is derived from
[PC-style playing cards](https://opengameart.org/content/pc-style-playing-cards)
by devurandom, released under CC0 (public domain). The jokers are original to
this project.
