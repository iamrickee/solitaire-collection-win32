#include "GameRegistry.h"
#include "../games/Accordion.h"
#include "../games/BakersDozen.h"
#include "../games/BeleagueredCastle.h"
#include "../games/FortyThieves.h"
#include "../games/FreeCell.h"
#include "../games/Gaps.h"
#include "../games/Golf.h"
#include "../games/KingsCorners.h"
#include "../games/Klondike.h"
#include "../games/LaBelleLucie.h"
#include "../games/Pyramid.h"
#include "../games/RussianSolitaire.h"
#include "../games/Scorpion.h"
#include "../games/Spider.h"
#include "../games/Spiderette.h"
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
        {L"Accordion", &Make<Accordion>},
        {L"Baker's Dozen", &Make<BakersDozen>},
        {L"Beleaguered Castle", &Make<BeleagueredCastle>},
        {L"Forty Thieves", &Make<FortyThieves>},
        {L"Gaps", &Make<Gaps>},
        {L"Golf", &Make<Golf>},
        {L"Kings Corners", &Make<KingsCorners>},
        {L"La Belle Lucie", &Make<LaBelleLucie>},
        {L"Pyramid", &Make<Pyramid>},
        {L"Russian Solitaire", &Make<RussianSolitaire>},
        {L"Scorpion", &Make<Scorpion>},
        {L"Spider", &Make<Spider>},
        {L"Spiderette", &Make<Spiderette>},
        {L"Things", &Make<Things>},
        {L"TriPeaks", &Make<TriPeaks>},
        {L"Yukon", &Make<Yukon>},
    };
    return games;
}
