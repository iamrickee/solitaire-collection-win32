#include "Pyramid.h"
#include "../core/Deck.h"
#include <algorithm>

Pyramid::Pyramid(Settings& settings) : Game(settings) {
    piles_.resize(kCell0 + kCells);
    piles_[kStock].kind = PileKind::Stock;
    piles_[kWaste].kind = PileKind::Waste;
    piles_[kDiscard].kind = PileKind::Foundation;
    for (int i = 0; i < kCells; ++i) piles_[kCell0 + i].kind = PileKind::Tableau;
}

std::wstring Pyramid::ScoreText() const {
    return L"Score: " + std::to_wstring(score_) + L"   Passes left: " +
           std::to_wstring(kRecycles - recycles_);
}

void Pyramid::Layout(const RECT& area, int cardW, int cardH, int unit) {
    const int gap = Art::kGap * unit;
    const int step = cardH / 2;   // each row covers the lower half of the row above
    const int totalW = kRows * cardW + (kRows - 1) * gap;
    const int left = area.left + std::max(gap, (int(area.right - area.left) - totalW) / 2);
    const int top = area.top + gap;
    const int centerX = left + totalW / 2;

    for (Pile& p : piles_) {
        p.cardW = cardW;
        p.cardH = cardH;
    }
    // Deck and waste top left, discards top right, beside the narrow top rows.
    piles_[kStock].x = left;
    piles_[kStock].y = top;
    piles_[kWaste].x = left + cardW + gap;
    piles_[kWaste].y = top;
    piles_[kDiscard].x = left + totalW - cardW;
    piles_[kDiscard].y = top;

    for (int row = 0; row < kRows; ++row) {
        const int rowW = (row + 1) * cardW + row * gap;
        for (int col = 0; col <= row; ++col) {
            Pile& p = piles_[CellIndex(row, col)];
            p.x = centerX - rowW / 2 + col * (cardW + gap);
            p.y = top + row * step;
        }
    }
}

void Pyramid::Deal(uint32_t seed) {
    recycles_ = 0;
    Deck deck;
    deck.Shuffle(seed);
    for (int i = 0; i < kCells; ++i) piles_[kCell0 + i].cards.push_back(deck.Draw(true));
    while (!deck.Empty()) piles_[kStock].cards.push_back(deck.Draw(false));
}

bool Pyramid::Uncovered(int pile) const {
    const int i = pile - kCell0;
    int row = 0;
    while ((row + 1) * (row + 2) / 2 <= i) ++row;
    if (row == kRows - 1) return true;
    const int col = i - row * (row + 1) / 2;
    return piles_[CellIndex(row + 1, col)].Empty() && piles_[CellIndex(row + 1, col + 1)].Empty();
}

bool Pyramid::Available(int pile) const {
    if (piles_[pile].Empty()) return false;
    if (pile == kWaste) return true;
    return pile >= kCell0 && Uncovered(pile);
}

bool Pyramid::CanPickUp(int pile, int index) const {
    return index >= 0 && index == piles_[pile].Count() - 1 && Available(pile);
}

bool Pyramid::CanDrop(int from, int index, int to) const {
    if (from == to || !CanPickUp(from, index)) return false;
    const Card& c = piles_[from].cards[index];
    if (to == kDiscard) return c.rank == kKing;
    return Available(to) && c.rank + piles_[to].Top().rank == 13;
}

bool Pyramid::Click(int pile, int index) {
    if (Ended()) return false;
    if (pile == kStock) {
        Pile& stock = piles_[kStock];
        Pile& waste = piles_[kWaste];
        if (stock.Empty() && !CanRecycle()) return false;
        PushUndo();   // marks the game as started
        if (!stock.Empty()) {
            Card c = stock.cards.back();
            stock.cards.pop_back();
            c.faceUp = true;
            waste.cards.push_back(c);
        } else {
            while (!waste.Empty()) {
                Card c = waste.cards.back();
                waste.cards.pop_back();
                c.faceUp = false;
                stock.cards.push_back(c);
            }
            ++recycles_;
        }
        return true;
    }
    // A King needs no partner: one click removes it.
    if (index >= 0 && CanDrop(pile, index, kDiscard)) return TryMove(pile, index, kDiscard);
    return false;
}

void Pyramid::OnMoved(int from, int to, int count) {
    (void)from; (void)count;
    // A card landed on its partner (or a King on the discards): clear them away.
    Pile& dst = piles_[to];
    if (to != kDiscard) {
        auto& d = piles_[kDiscard].cards;
        d.insert(d.end(), dst.cards.end() - 2, dst.cards.end());
        dst.cards.resize(dst.cards.size() - 2);
        score_ += 2;
    } else {
        ++score_;
    }
}

bool Pyramid::IsWon() const {
    for (int i = 0; i < kCells; ++i)
        if (!piles_[kCell0 + i].Empty()) return false;
    return true;
}

// Stuck: the deck is used up and no King or pair of free cards is left.
bool Pyramid::IsLost() const {
    if (!piles_[kStock].Empty() || CanRecycle()) return false;
    std::vector<int> free;
    if (Available(kWaste)) free.push_back(kWaste);
    for (int i = 0; i < kCells; ++i)
        if (Available(kCell0 + i)) free.push_back(kCell0 + i);
    for (size_t a = 0; a < free.size(); ++a) {
        const int ra = piles_[free[a]].Top().rank;
        if (ra == kKing) return false;
        for (size_t b = a + 1; b < free.size(); ++b)
            if (ra + piles_[free[b]].Top().rank == 13) return false;
    }
    return true;
}

std::wstring Pyramid::SlotHint(int pile) const {
    if (pile == kStock && !Ended() && !piles_[kWaste].Empty())
        return CanRecycle() ? L"↻" : L"✕";   // ↻ or ✕
    return {};
}
