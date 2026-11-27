#include "GameRegistry.h"
#include "../games/FreeCell.h"
#include "../games/KingsCorners.h"
#include "../games/Klondike.h"
#include "../games/Pyramid.h"
#include "../games/Spider.h"
#include "../games/Things.h"
#include "../games/TriPeaks.h"
#include "../games/Yukon.h"

namespace {
template <class T>
std::unique_ptr<Game> Make(Settings& s) { return std::make_unique<T>(s); }
}

const std::vector<GameInfo>& AllGames() {
    static const std::vector<GameInfo> games = {
        {L"Klondike", &Make<Klondike>},
        {L"FreeCell", &Make<FreeCell>},
        {L"Kings Corners", &Make<KingsCorners>},
        {L"Pyramid", &Make<Pyramid>},
        {L"Spider", &Make<Spider>},
        {L"Things", &Make<Things>},
        {L"TriPeaks", &Make<TriPeaks>},
        {L"Yukon", &Make<Yukon>},
    };
    return games;
}
