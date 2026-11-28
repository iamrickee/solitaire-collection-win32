# Solitaire Collection

A collection of solitaire card games for Windows, written in C++17 against the
Win32 API and GDI directly — no frameworks, no game engine, no third-party
libraries.

Currently included: **Klondike** (Draw 1 / Draw 3, Standard or Vegas scoring,
optional cumulative Vegas bank). The engine is built so more games can be added
as single source files.

## Features

- Drag-and-drop, or click a card to select it and click where it should go
- Double-click sends a card to its foundation; right-click (or F3) auto-plays
  everything possible to the foundations
- Unlimited undo
- Automatic finish once every card is face up
- Best score saved per game and variant
- Configurable background and card-back colors, optional four-color deck
- Pixel-art cards scaled by whole numbers to fit the window; per-monitor DPI aware
- Numbered, reproducible deals

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
| Deal from stock               | Click the stock                         |
| Undo                          | Ctrl+Z                                  |
| New game                      | F2                                      |
| Options for the current game  | F4                                      |
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
    Klondike.*          one file pair per game
res/                    sprite sheet, menus, dialogs, manifest
tools/                  script that builds res/cards.bmp from the art pack
```

The app knows nothing about specific games. Each game subclasses `Game`, lays
out its piles and answers rule questions (`CanPickUp`, `CanDrop`, `Click`,
`IsWon`, ...). Dragging, click-to-move, animation, undo and drawing are shared.

Rendering is double-buffered GDI: each frame is drawn to an off-screen bitmap
and copied to the window in one `BitBlt`. Cards are drawn with `TransparentBlt`
from a sprite sheet that is recolored and nearest-neighbour scaled whenever the
window size or appearance settings change.

### Adding a game

1. Add `src/games/MyGame.h/.cpp` deriving from `Game`.
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
