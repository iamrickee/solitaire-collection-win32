#include "GameRegistry.h"
#include "../games/KingsCorners.h"
#include "../games/Klondike.h"
#include "../games/Spider.h"
#include "../games/Things.h"

namespace {
template <class T>
std::unique_ptr<Game> Make(Settings& s) { return std::make_unique<T>(s); }
}

const std::vector<GameInfo>& AllGames() {
    static const std::vector<GameInfo> games = {
        {L"Klondike", &Make<Klondike>},
        {L"Kings Corners", &Make<KingsCorners>},
        {L"Spider", &Make<Spider>},
        {L"Things", &Make<Things>},
    };
    return games;
}
