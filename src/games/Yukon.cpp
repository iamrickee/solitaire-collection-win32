#include "Yukon.h"
#include "../core/Deck.h"
#include <algorithm>

namespace {
// Standard (Windows Klondike) scoring.
constexpr int kToFoundation = 10;
constexpr int kTurnOver = 5;
constexpr int kFoundationToColumn = -15;
}

Yukon::Yukon(Settings& settings) : Game(settings) {
    piles_.resize(kColumn0 + 7);
    for (int i = 0; i < 4; ++i) piles_[kFoundation0 + i].kind = PileKind::Foundation;
    for (int i = 0; i < 7; ++i) {
        piles_[kColumn0 + i].kind = PileKind::Tableau;
        piles_[kColumn0 + i].fan = Fan::Down;
    }
}

void Yukon::Layout(const RECT& area, int cardW, int cardH, int unit) {
    const int gap = Art::kGap * unit;
    const int totalW = 7 * cardW + 6 * gap;
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
    // Foundations where Klondike has them.
    for (int i = 0; i < 4; ++i) {
        piles_[kFoundation0 + i].x = colX(3 + i);
        piles_[kFoundation0 + i].y = top;
    }
    for (int i = 0; i < 7; ++i) {
        Pile& p = piles_[kColumn0 + i];
        p.x = colX(i);
        p.y = tabTop;
        p.maxExtent = std::max(cardH, int(area.bottom) - tabTop - gap);
    }
}

void Yukon::Deal(uint32_t seed) {
    Deck deck;
    deck.Shuffle(seed);
    piles_[kColumn0].cards.push_back(deck.Draw(true));
    for (int col = 1; col < 7; ++col)
        for (int row = 0; row < col + 5; ++row)
            piles_[kColumn0 + col].cards.push_back(deck.Draw(row >= col));
}

bool Yukon::CanPickUp(int pile, int index) const {
    const Pile& p = piles_[pile];
    if (index < 0 || index >= p.Count() || !p.cards[index].faceUp) return false;
    // Any face-up card in a column takes the cards on top of it along.
    return p.kind == PileKind::Tableau || index == p.Count() - 1;
}

bool Yukon::CanDrop(int from, int index, int to) const {
    if (!CanPickUp(from, index)) return false;
    const Card& moving = piles_[from].cards[index];
    const int count = piles_[from].Count() - index;
    const Pile& dst = piles_[to];
    switch (dst.kind) {
    case PileKind::Foundation:
        if (count != 1) return false;
        if (dst.Empty()) return moving.rank == kAce;
        return dst.Top().suit == moving.suit && moving.rank == dst.Top().rank + 1;
    case PileKind::Tableau:
        if (dst.Empty()) return moving.rank == kKing;
        return dst.Top().faceUp && Builds(moving, dst.Top()) && moving.rank + 1 == dst.Top().rank;
    default:
        return false;
    }
}

void Yukon::OnMoved(int from, int to, int count) {
    (void)count;
    const PileKind fk = piles_[from].kind, tk = piles_[to].kind;
    if (tk == PileKind::Foundation && fk != PileKind::Foundation) score_ += kToFoundation;
    else if (fk == PileKind::Foundation && tk == PileKind::Tableau) score_ = std::max(0, score_ + kFoundationToColumn);

    // Turn over the card a column move exposed.
    Pile& src = piles_[from];
    if (fk == PileKind::Tableau && !src.Empty() && !src.Top().faceUp) {
        src.Top().faceUp = true;
        score_ += kTurnOver;
    }
}

bool Yukon::IsWon() const {
    for (int i = 0; i < 4; ++i)
        if (piles_[kFoundation0 + i].Count() != 13) return false;
    return true;
}

bool Yukon::CanAutoComplete() const {
    for (int i = 0; i < 7; ++i)
        for (const Card& c : piles_[kColumn0 + i].cards)
            if (!c.faceUp) return false;
    return !IsWon();
}

std::wstring Yukon::SlotHint(int pile) const {
    return piles_[pile].kind == PileKind::Foundation ? L"A" : std::wstring{};
}
