#include "BakersDozen.h"
#include "../core/Deck.h"
#include <algorithm>

BakersDozen::BakersDozen(Settings& settings) : Game(settings) {
    piles_.resize(kColumn0 + kColumns);
    for (int i = 0; i < 4; ++i) piles_[kFoundation0 + i].kind = PileKind::Foundation;
    for (int i = 0; i < kColumns; ++i) {
        piles_[kColumn0 + i].kind = PileKind::Tableau;
        piles_[kColumn0 + i].fan = Fan::Down;
    }
}

void BakersDozen::Layout(const RECT& area, int cardW, int cardH, int unit) {
    const int gap = Art::kGap * unit;
    // Seven columns, a wider gap, then the foundations in a 2x2 block.
    const int totalW = 9 * cardW + 9 * gap;
    const int left = area.left + std::max(gap, (int(area.right - area.left) - totalW) / 2);
    const int top = area.top + gap;
    const int rowH = std::max(cardH, (int(area.bottom - area.top) - 3 * gap) / 2);

    for (Pile& p : piles_) {
        p.cardW = cardW;
        p.cardH = cardH;
        p.offFaceDown = Art::kFanDown * unit;
        p.offFaceUp = Art::kFanUp * unit;
        p.maxExtent = 0;
    }
    auto colX = [&](int c) { return left + c * (cardW + gap); };

    for (int i = 0; i < kColumns; ++i) {
        Pile& p = piles_[kColumn0 + i];
        const bool topRow = i < kTopRow;
        p.x = colX(topRow ? i : i - kTopRow);
        p.y = topRow ? top : top + rowH + gap;
        p.maxExtent = rowH;
    }
    const int foundX = colX(kTopRow) + gap;
    for (int i = 0; i < 4; ++i) {
        piles_[kFoundation0 + i].x = foundX + (i % 2) * (cardW + gap);
        piles_[kFoundation0 + i].y = top + (i / 2) * (cardH + gap);
    }
}

void BakersDozen::Deal(uint32_t seed) {
    Deck deck;
    deck.Shuffle(seed);
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < kColumns; ++col)
            piles_[kColumn0 + col].cards.push_back(deck.Draw(true));
    // Kings go to the bottom of their columns, where they block nothing.
    for (int col = 0; col < kColumns; ++col) {
        auto& cards = piles_[kColumn0 + col].cards;
        std::stable_partition(cards.begin(), cards.end(), [](const Card& c) { return c.rank == kKing; });
    }
}

bool BakersDozen::CanPickUp(int pile, int index) const {
    const Pile& p = piles_[pile];
    return p.kind == PileKind::Tableau && index >= 0 && index == p.Count() - 1;   // one card at a time
}

bool BakersDozen::CanDrop(int from, int index, int to) const {
    if (!CanPickUp(from, index)) return false;
    const Card& c = piles_[from].cards[index];
    const Pile& dst = piles_[to];
    switch (dst.kind) {
    case PileKind::Foundation:
        if (dst.Empty()) return c.rank == kAce;
        return dst.Top().suit == c.suit && c.rank == dst.Top().rank + 1;
    case PileKind::Tableau:
        return !dst.Empty() && c.rank + 1 == dst.Top().rank;   // any suit; empty columns stay empty
    default:
        return false;
    }
}

void BakersDozen::OnMoved(int from, int to, int count) {
    // Score = cards on the foundations.
    if (piles_[to].kind == PileKind::Foundation) score_ += count;
    if (piles_[from].kind == PileKind::Foundation) score_ -= count;
}

// Safe when no column could still want it: every card one rank lower, of any
// suit, is already on a foundation.
bool BakersDozen::SafeToFoundation(const Card& c) const {
    if (c.rank <= 2) return true;
    int lowest = kKing;
    int started = 0;
    for (int i = 0; i < 4; ++i) {
        const Pile& f = piles_[kFoundation0 + i];
        if (f.Empty()) continue;
        ++started;
        lowest = std::min<int>(lowest, f.Top().rank);
    }
    return started == 4 && lowest >= c.rank - 1;
}

bool BakersDozen::FindAutoMove(int& from, int& to) const {
    for (int p = kColumn0; p < kColumn0 + kColumns; ++p) {
        const Pile& src = piles_[p];
        if (src.Empty() || !SafeToFoundation(src.Top())) continue;
        const int f = DoubleClickTarget(p, src.Count() - 1);
        if (f >= 0) { from = p; to = f; return true; }
    }
    return false;
}

bool BakersDozen::IsWon() const {
    for (int i = 0; i < 4; ++i)
        if (piles_[kFoundation0 + i].Count() != 13) return false;
    return true;
}

// Stuck: no card can move anywhere.
bool BakersDozen::IsLost() const {
    if (IsWon()) return false;
    for (int from = kColumn0; from < kColumn0 + kColumns; ++from) {
        const Pile& src = piles_[from];
        if (src.Empty()) continue;
        for (int to = 0; to < kColumn0 + kColumns; ++to)
            if (to != from && CanDrop(from, src.Count() - 1, to)) return false;
    }
    return true;
}

std::wstring BakersDozen::SlotHint(int pile) const {
    return piles_[pile].kind == PileKind::Foundation ? L"A" : std::wstring{};
}

std::wstring BakersDozen::Rules() const {
    return L"Goal: move all 52 cards to the four foundations, building each up in one suit from Ace to King.\n\n"
           L"Layout: 13 columns of 4 face-up cards. Any King is moved to the bottom of its column. There is no deck.\n\n"
           L"• Build columns down regardless of suit, one card at a time.\n"
           L"• An empty column stays empty.\n"
           L"• Cards go to the foundations by themselves once no column could need them.\n\n"
           L"Score: cards on the foundations.";
}
