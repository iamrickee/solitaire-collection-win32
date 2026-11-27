#include "FortyThieves.h"
#include "../core/Deck.h"
#include <algorithm>

FortyThieves::FortyThieves(Settings& settings) : Game(settings) {
    piles_.resize(kColumn0 + kColumns);
    piles_[kStock].kind = PileKind::Stock;
    piles_[kWaste].kind = PileKind::Waste;
    for (int i = 0; i < 8; ++i) piles_[kFoundation0 + i].kind = PileKind::Foundation;
    for (int i = 0; i < kColumns; ++i) {
        piles_[kColumn0 + i].kind = PileKind::Tableau;
        piles_[kColumn0 + i].fan = Fan::Down;
    }
}

void FortyThieves::Layout(const RECT& area, int cardW, int cardH, int unit) {
    const int gap = Art::kGap * unit;
    const int totalW = kColumns * cardW + (kColumns - 1) * gap;
    const int left = area.left + std::max(gap, (int(area.right - area.left) - totalW) / 2);
    const int top = area.top + gap;
    const int tabTop = top + cardH + 3 * gap;

    for (Pile& p : piles_) {
        p.cardW = cardW;
        p.cardH = cardH;
        p.offFaceDown = Art::kFanDown * unit;
        p.offFaceUp = Art::kFanUp * unit;
        p.maxExtent = 0;
    }
    auto colX = [&](int c) { return left + c * (cardW + gap); };

    // Top row: deck, waste, then the eight foundations.
    piles_[kStock].x = colX(0);
    piles_[kStock].y = top;
    piles_[kWaste].x = colX(1);
    piles_[kWaste].y = top;
    for (int i = 0; i < 8; ++i) {
        piles_[kFoundation0 + i].x = colX(2 + i);
        piles_[kFoundation0 + i].y = top;
    }
    for (int i = 0; i < kColumns; ++i) {
        Pile& p = piles_[kColumn0 + i];
        p.x = colX(i);
        p.y = tabTop;
        p.maxExtent = std::max(cardH, int(area.bottom) - tabTop - gap);
    }
}

void FortyThieves::Deal(uint32_t seed) {
    Deck deck(2);
    deck.Shuffle(seed);
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < kColumns; ++col)
            piles_[kColumn0 + col].cards.push_back(deck.Draw(true));
    while (!deck.Empty()) piles_[kStock].cards.push_back(deck.Draw(false));
}

bool FortyThieves::CanPickUp(int pile, int index) const {
    const Pile& p = piles_[pile];
    if (index < 0 || index != p.Count() - 1) return false;   // one card at a time
    return p.kind == PileKind::Tableau || p.kind == PileKind::Waste;
}

bool FortyThieves::CanDrop(int from, int index, int to) const {
    if (!CanPickUp(from, index)) return false;
    const Card& c = piles_[from].cards[index];
    const Pile& dst = piles_[to];
    switch (dst.kind) {
    case PileKind::Foundation:
        if (dst.Empty()) return c.rank == kAce;
        return dst.Top().suit == c.suit && c.rank == dst.Top().rank + 1;
    case PileKind::Tableau:
        if (dst.Empty()) return true;
        return dst.Top().suit == c.suit && c.rank + 1 == dst.Top().rank;
    default:
        return false;
    }
}

void FortyThieves::OnMoved(int from, int to, int count) {
    // Score = cards on the foundations.
    if (piles_[to].kind == PileKind::Foundation) score_ += count;
    if (piles_[from].kind == PileKind::Foundation) score_ -= count;
}

bool FortyThieves::Click(int pile, int index) {
    (void)index;
    if (pile != kStock || Ended()) return false;
    Pile& stock = piles_[kStock];
    if (stock.Empty()) return false;   // one pass only
    PushUndo();
    Card c = stock.cards.back();
    stock.cards.pop_back();
    c.faceUp = true;
    piles_[kWaste].cards.push_back(c);
    return true;
}

// Safe when no column could still want it: the only cards that build on it
// are the two of its suit one rank lower, and both are on foundations once
// the other foundation of its suit has reached that rank (the receiving one
// is there already).
bool FortyThieves::SafeToFoundation(const Card& c, int foundation) const {
    if (c.rank <= 2) return true;
    for (int i = 0; i < 8; ++i) {
        const Pile& f = piles_[kFoundation0 + i];
        if (kFoundation0 + i == foundation || f.Empty() || f.Top().suit != c.suit) continue;
        if (f.Top().rank >= c.rank - 1) return true;
    }
    return false;
}

bool FortyThieves::FindAutoMove(int& from, int& to) const {
    for (int p = kWaste; p < kColumn0 + kColumns; ++p) {
        const Pile& src = piles_[p];
        if (src.Empty() || src.kind == PileKind::Foundation) continue;
        const int f = DoubleClickTarget(p, src.Count() - 1);
        if (f >= 0 && SafeToFoundation(src.Top(), f)) { from = p; to = f; return true; }
    }
    return false;
}

bool FortyThieves::IsWon() const {
    for (int i = 0; i < 8; ++i)
        if (piles_[kFoundation0 + i].Count() != 13) return false;
    return true;
}

// Stuck: the deck is used up, no column is empty, and no card can move.
// (With an empty column there's always a move, so that's left to the player.)
bool FortyThieves::IsLost() const {
    if (!piles_[kStock].Empty() || IsWon()) return false;
    for (int i = 0; i < kColumns; ++i)
        if (piles_[kColumn0 + i].Empty()) return false;
    for (int from = kWaste; from < kColumn0 + kColumns; ++from) {
        const Pile& src = piles_[from];
        if (src.Empty() || src.kind == PileKind::Foundation) continue;
        for (int to = kFoundation0; to < kColumn0 + kColumns; ++to)
            if (to != from && CanDrop(from, src.Count() - 1, to)) return false;
    }
    return true;
}

std::wstring FortyThieves::SlotHint(int pile) const {
    if (pile == kStock && !Ended() && !piles_[kWaste].Empty()) return L"✕";   // ✕: no redeal
    return piles_[pile].kind == PileKind::Foundation ? L"A" : std::wstring{};
}
