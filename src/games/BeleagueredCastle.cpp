#include "BeleagueredCastle.h"
#include "../core/Deck.h"
#include <algorithm>

BeleagueredCastle::BeleagueredCastle(Settings& settings) : Game(settings) {
    piles_.resize(kRow0 + kRows);
    for (int i = 0; i < 4; ++i) piles_[kFoundation0 + i].kind = PileKind::Foundation;
    for (int i = 0; i < kRows; ++i) {
        piles_[kRow0 + i].kind = PileKind::Tableau;
        piles_[kRow0 + i].fan = Fan::Right;
    }
}

void BeleagueredCastle::Layout(const RECT& area, int cardW, int cardH, int unit) {
    const int gap = Art::kGap * unit;
    const int vgap = gap / 2;          // tighter rows keep four of them within a 2x scale
    const int sideW = 4 * cardW;       // room for a row to fan out to the right
    const int totalW = 2 * sideW + cardW + 2 * gap;
    const int left = area.left + std::max(gap, (int(area.right - area.left) - totalW) / 2);
    const int top = area.top + vgap;
    const int foundX = left + sideW + gap;

    for (Pile& p : piles_) {
        p.cardW = cardW;
        p.cardH = cardH;
        p.offFaceDown = Art::kFanDown * unit;
        p.offFaceUp = Art::kFanRight * unit;
        p.maxExtent = 0;
    }
    for (int i = 0; i < 4; ++i) {
        const int y = top + i * (cardH + vgap);
        piles_[kFoundation0 + i].x = foundX;
        piles_[kFoundation0 + i].y = y;
        Pile& l = piles_[kRow0 + i];
        l.x = left;
        l.y = y;
        l.maxExtent = sideW;
        Pile& r = piles_[kRow0 + 4 + i];
        r.x = foundX + cardW + gap;
        r.y = y;
        r.maxExtent = sideW;
    }
}

void BeleagueredCastle::Deal(uint32_t seed) {
    Deck deck;
    deck.Shuffle(seed);
    int next = 0;
    while (!deck.Empty()) {
        const Card c = deck.Draw(true);
        if (c.rank == kAce) piles_[kFoundation0 + static_cast<int>(c.suit)].cards.push_back(c);
        else piles_[kRow0 + next++ % kRows].cards.push_back(c);
    }
}

bool BeleagueredCastle::CanPickUp(int pile, int index) const {
    const Pile& p = piles_[pile];
    return p.kind == PileKind::Tableau && index >= 0 && index == p.Count() - 1;   // one card at a time
}

bool BeleagueredCastle::CanDrop(int from, int index, int to) const {
    if (!CanPickUp(from, index)) return false;
    const Card& c = piles_[from].cards[index];
    const Pile& dst = piles_[to];
    switch (dst.kind) {
    case PileKind::Foundation:
        return dst.Top().suit == c.suit && c.rank == dst.Top().rank + 1;
    case PileKind::Tableau:
        return dst.Empty() || c.rank + 1 == dst.Top().rank;   // any suit
    default:
        return false;
    }
}

void BeleagueredCastle::OnMoved(int from, int to, int count) {
    // Score = cards moved to the foundations (the Aces start there).
    if (piles_[to].kind == PileKind::Foundation) score_ += count;
    if (piles_[from].kind == PileKind::Foundation) score_ -= count;
}

// Safe when no row could still want it: every card one rank lower, of any
// suit, is already on a foundation.
bool BeleagueredCastle::SafeToFoundation(const Card& c) const {
    for (int i = 0; i < 4; ++i)
        if (piles_[kFoundation0 + i].Top().rank < c.rank - 1) return false;
    return true;
}

bool BeleagueredCastle::FindAutoMove(int& from, int& to) const {
    for (int p = kRow0; p < kRow0 + kRows; ++p) {
        const Pile& src = piles_[p];
        if (src.Empty() || !SafeToFoundation(src.Top())) continue;
        const int f = DoubleClickTarget(p, src.Count() - 1);
        if (f >= 0) { from = p; to = f; return true; }
    }
    return false;
}

bool BeleagueredCastle::IsWon() const {
    for (int i = 0; i < 4; ++i)
        if (piles_[kFoundation0 + i].Count() != 13) return false;
    return true;
}

// Stuck: no row is empty and no card can move.
// (With an empty row there's always a move, so that's left to the player.)
bool BeleagueredCastle::IsLost() const {
    if (IsWon()) return false;
    for (int i = 0; i < kRows; ++i)
        if (piles_[kRow0 + i].Empty()) return false;
    for (int from = kRow0; from < kRow0 + kRows; ++from) {
        const Pile& src = piles_[from];
        for (int to = 0; to < kRow0 + kRows; ++to)
            if (to != from && CanDrop(from, src.Count() - 1, to)) return false;
    }
    return true;
}
