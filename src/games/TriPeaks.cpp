#include "TriPeaks.h"
#include "../core/Deck.h"
#include <algorithm>
#include <cstdlib>

namespace {
constexpr int kPeakBonus = 15;   // per peak cleared
constexpr int kRowStart[5] = {0, 3, 9, 18, 28};
}

TriPeaks::TriPeaks(Settings& settings) : Game(settings) {
    piles_.resize(kCell0 + kCells);
    piles_[kStock].kind = PileKind::Stock;
    piles_[kWaste].kind = PileKind::Waste;
    for (int i = 0; i < kCells; ++i) piles_[kCell0 + i].kind = PileKind::Tableau;
}

std::wstring TriPeaks::ScoreText() const {
    return L"Score: " + std::to_wstring(score_) + L"   Run: " + std::to_wstring(chain_) +
           L"   Deck: " + std::to_wstring(piles_[kStock].Count());
}

int TriPeaks::Row(int cell) {
    int r = 0;
    while (cell >= kRowStart[r + 1]) ++r;
    return r;
}

// Slots are half-card steps across the bottom row: bottom card i sits at 2i,
// and every other card sits midway over the two cards covering it.
int TriPeaks::Slot(int cell) {
    const int r = Row(cell), i = cell - kRowStart[r];
    switch (r) {
    case 0: return 6 * i + 3;
    case 1: return 6 * (i / 2) + 2 + 2 * (i % 2);
    case 2: return 2 * i + 1;
    default: return 2 * i;
    }
}

void TriPeaks::Layout(const RECT& area, int cardW, int cardH, int unit) {
    const int gap = Art::kGap * unit;
    const int step = cardH / 2;   // each row covers the lower half of the row above
    const int totalW = 10 * cardW + 9 * gap;
    const int left = area.left + std::max(gap, (int(area.right - area.left) - totalW) / 2);
    const int top = area.top + gap;
    const int centerX = left + totalW / 2;

    for (Pile& p : piles_) {
        p.cardW = cardW;
        p.cardH = cardH;
    }
    for (int i = 0; i < kCells; ++i) {
        Pile& p = piles_[kCell0 + i];
        p.x = left + Slot(i) * (cardW + gap) / 2;
        p.y = top + Row(i) * step;
    }
    // Deck and face-up card centred below the peaks.
    const int deckY = top + 3 * step + cardH + 2 * gap;
    piles_[kStock].x = centerX - cardW - gap / 2;
    piles_[kStock].y = deckY;
    piles_[kWaste].x = centerX + gap / 2;
    piles_[kWaste].y = deckY;
}

void TriPeaks::Deal(uint32_t seed) {
    chain_ = 0;
    Deck deck;
    deck.Shuffle(seed);
    for (int i = 0; i < kCells; ++i) piles_[kCell0 + i].cards.push_back(deck.Draw(Row(i) == 3));
    piles_[kWaste].cards.push_back(deck.Draw(true));
    while (!deck.Empty()) piles_[kStock].cards.push_back(deck.Draw(false));
}

bool TriPeaks::Uncovered(int pile) const {
    const int cell = pile - kCell0, r = Row(cell), s = Slot(cell);
    if (r == 3) return true;
    for (int j = kRowStart[r + 1]; j < kRowStart[r + 2]; ++j) {
        const int d = Slot(j) - s;
        if ((d == -1 || d == 1) && !piles_[kCell0 + j].Empty()) return false;
    }
    return true;
}

bool TriPeaks::Playable(const Card& c) const {
    const Pile& waste = piles_[kWaste];
    if (waste.Empty() || !c.faceUp) return false;
    const int d = std::abs(int(c.rank) - int(waste.Top().rank));
    return d == 1 || d == 12;   // King and Ace wrap
}

bool TriPeaks::CanPickUp(int pile, int index) const {
    const Pile& p = piles_[pile];
    return pile >= kCell0 && index == 0 && p.Count() == 1 && Playable(p.Top());
}

bool TriPeaks::CanDrop(int from, int index, int to) const {
    return to == kWaste && CanPickUp(from, index);
}

bool TriPeaks::Click(int pile, int index) {
    if (Ended()) return false;
    if (pile == kStock) {
        Pile& stock = piles_[kStock];
        if (stock.Empty()) return false;
        PushUndo();   // marks the game as started
        Card c = stock.cards.back();
        stock.cards.pop_back();
        c.faceUp = true;
        piles_[kWaste].cards.push_back(c);
        chain_ = 0;
        ClearUndo();  // drawing reveals a new card; no taking it back
        return true;
    }
    return CanPickUp(pile, index) && TryMove(pile, index, kWaste);
}

void TriPeaks::OnMoved(int from, int to, int count) {
    (void)to; (void)count;
    ++chain_;
    score_ += chain_;   // longer runs are worth more per card
    if (Row(from - kCell0) == 0) score_ += kPeakBonus;

    // Turn over any card this one was the last to cover.
    bool flipped = false;
    for (int i = 0; i < kCells; ++i) {
        Pile& p = piles_[kCell0 + i];
        if (!p.Empty() && !p.Top().faceUp && Uncovered(kCell0 + i)) {
            p.Top().faceUp = true;
            flipped = true;
        }
    }
    if (flipped) ClearUndo();   // a new card is showing; no taking it back
}

bool TriPeaks::IsWon() const {
    for (int i = 0; i < kCells; ++i)
        if (!piles_[kCell0 + i].Empty()) return false;
    return true;
}

bool TriPeaks::IsLost() const {
    if (!piles_[kStock].Empty() || IsWon()) return false;
    for (int i = 0; i < kCells; ++i)
        if (CanPickUp(kCell0 + i, 0)) return false;
    return true;
}

std::wstring TriPeaks::Rules() const {
    return L"Goal: clear all three peaks.\n\n"
           L"Layout: 28 cards in three overlapping peaks with the bottom row face up. One card starts the face-up pile; the rest form the deck.\n\n"
           L"• Click an uncovered card to play it onto the face-up pile if it is one rank higher or lower, in any suit. King and Ace wrap around.\n"
           L"• A card turns over once both cards covering it are gone.\n"
           L"• When stuck, click the deck to turn up its next card. There is only one pass through the deck.\n\n"
           L"Score: each card in a run scores one more than the last (1, 2, 3...); drawing starts a new run. Clearing a peak adds 15.";
}
