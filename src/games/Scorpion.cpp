#include "Scorpion.h"
#include "../core/Deck.h"
#include <algorithm>

namespace {
constexpr int kStartScore = 500;
constexpr int kMoveCost = 1;     // every move and the reserve deal
constexpr int kRunBonus = 100;   // each completed K..A run
}

Scorpion::Scorpion(Settings& settings) : Game(settings) {
    piles_.resize(kColumn0 + kColumns);
    piles_[kStock].kind = PileKind::Stock;
    for (int i = 0; i < 4; ++i) piles_[kFoundation0 + i].kind = PileKind::Foundation;
    for (int i = 0; i < kColumns; ++i) {
        piles_[kColumn0 + i].kind = PileKind::Tableau;
        piles_[kColumn0 + i].fan = Fan::Down;
    }
}

void Scorpion::Layout(const RECT& area, int cardW, int cardH, int unit) {
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

    // Top row: reserve on the left, completed runs on the right.
    piles_[kStock].x = colX(0);
    piles_[kStock].y = top;
    for (int i = 0; i < 4; ++i) {
        piles_[kFoundation0 + i].x = colX(3 + i);
        piles_[kFoundation0 + i].y = top;
    }
    for (int i = 0; i < kColumns; ++i) {
        Pile& p = piles_[kColumn0 + i];
        p.x = colX(i);
        p.y = tabTop;
        p.maxExtent = std::max(cardH, int(area.bottom) - tabTop - gap);
    }
}

void Scorpion::Deal(uint32_t seed) {
    Deck deck;
    deck.Shuffle(seed);
    for (int col = 0; col < kColumns; ++col)
        for (int row = 0; row < 7; ++row)
            piles_[kColumn0 + col].cards.push_back(deck.Draw(col >= 4 || row >= 3));
    while (!deck.Empty()) piles_[kStock].cards.push_back(deck.Draw(false));
    score_ = kStartScore;
}

bool Scorpion::CanPickUp(int pile, int index) const {
    const Pile& p = piles_[pile];
    // Any face-up card in a column takes the cards on top of it along.
    return p.kind == PileKind::Tableau && index >= 0 && index < p.Count() && p.cards[index].faceUp;
}

bool Scorpion::CanDrop(int from, int index, int to) const {
    if (!CanPickUp(from, index)) return false;
    const Card& moving = piles_[from].cards[index];
    const Pile& dst = piles_[to];
    if (dst.kind != PileKind::Tableau) return false;
    if (dst.Empty()) return moving.rank == kKing;
    return dst.Top().faceUp && dst.Top().suit == moving.suit && moving.rank + 1 == dst.Top().rank;
}

void Scorpion::CollectRun(int column) {
    Pile& p = piles_[column];
    if (p.Count() < 13) return;
    const int start = p.Count() - 13;
    for (int i = start; i < p.Count(); ++i) {
        const Card& c = p.cards[i];
        if (!c.faceUp || c.suit != p.cards[start].suit || c.rank != kKing - (i - start)) return;
    }
    for (int i = 0; i < 4; ++i) {
        Pile& f = piles_[kFoundation0 + i];
        if (!f.Empty()) continue;
        // Stored Ace first so the King shows on top of the slot.
        f.cards.assign(p.cards.rbegin(), p.cards.rbegin() + 13);
        p.cards.erase(p.cards.begin() + start, p.cards.end());
        score_ += kRunBonus;
        if (!p.Empty()) p.Top().faceUp = true;
        return;
    }
}

void Scorpion::OnMoved(int from, int to, int count) {
    (void)count;
    score_ = std::max(0, score_ - kMoveCost);
    Pile& src = piles_[from];
    if (!src.Empty()) src.Top().faceUp = true;
    CollectRun(to);
}

bool Scorpion::Click(int pile, int index) {
    (void)index;
    if (pile != kStock || Ended()) return false;
    Pile& stock = piles_[kStock];
    if (stock.Empty()) return false;
    PushUndo();
    for (int i = 0; i < 3 && !stock.Empty(); ++i) {
        Card c = stock.cards.back();
        stock.cards.pop_back();
        c.faceUp = true;
        piles_[kColumn0 + i].cards.push_back(c);
    }
    score_ = std::max(0, score_ - kMoveCost);
    for (int i = 0; i < 3; ++i) CollectRun(kColumn0 + i);
    return true;
}

bool Scorpion::IsWon() const {
    for (int i = 0; i < 4; ++i)
        if (piles_[kFoundation0 + i].Empty()) return false;
    return true;
}
