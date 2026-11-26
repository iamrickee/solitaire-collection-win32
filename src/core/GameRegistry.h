#pragma once
#include "Game.h"
#include <memory>
#include <vector>

struct GameInfo {
    const wchar_t* name;
    std::unique_ptr<Game> (*create)(Settings&);
};

// Every game in the collection. Adding a game = one new file in src/games
// plus one line in GameRegistry.cpp.
const std::vector<GameInfo>& AllGames();
