#include "GameRegistry.h"
#include "../games/Klondike.h"
#include "../games/Things.h"

namespace {
template <class T>
std::unique_ptr<Game> Make(Settings& s) { return std::make_unique<T>(s); }
}

const std::vector<GameInfo>& AllGames() {
    static const std::vector<GameInfo> games = {
        {L"Klondike", &Make<Klondike>},
        {L"Things", &Make<Things>},
    };
    return games;
}
