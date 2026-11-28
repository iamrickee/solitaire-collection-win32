#include "Golf.h"
#include "../core/Deck.h"
#include <algorithm>
#include <cstdlib>

Golf::Golf(Settings& settings) : Game(settings) {
    piles_.resize(kColumn0 + kColumns);
    piles_[kStock].kind = PileKind::Stock;
    piles_[kWaste].kind = PileKind::Waste;
    for (int i = 0; i < kColumns; ++i) {
        piles_[kColumn0 + i].kind = PileKind::Tableau;
        piles_[kColumn0 + i].fan = Fan::Down;
    }
}

std::wstring Golf::ScoreText() const {
    return L"Score: " + std::to_wstring(score_) + L"   Strokes: " + std::to_wstring(Strokes()) +
           L"   Deck: " + std::to_wstring(piles_[kStock].Count());
}

void Golf::Layout(const RECT& area, int cardW, int cardH, int unit) {
    const int gap = Art::kGap * unit;
    const int totalW = kColumns * cardW + (kColumns - 1) * gap;
    const int left = area.left + std::max(gap, (int(area.right - area.left) - totalW) / 2);
    const int top = area.top + gap;

    for (Pile& p : piles_) {
        p.cardW = cardW;
        p.cardH = cardH;
        p.offFaceDown = Art::kFanDown * unit;
        p.offFaceUp = Art::kFanUp * unit;
    }
    // Top row: stock and face-up card.
    piles_[kStock].x = left;
    piles_[kStock].y = top;
    piles_[kWaste].x = left + cardW + gap;
    piles_[kWaste].y = top;

    // The columns below.
    const int colY = top + cardH + 3 * gap;
    for (int i = 0; i < kColumns; ++i) {
        piles_[kColumn0 + i].x = left + i * (cardW + gap);
        piles_[kColumn0 + i].y = colY;
    }
}

void Golf::Deal(uint32_t seed) {
    Deck deck;
    deck.Shuffle(seed);
    for (int row = 0; row < kDepth; ++row)
        for (int col = 0; col < kColumns; ++col)
            piles_[kColumn0 + col].cards.push_back(deck.Draw(true));
    piles_[kWaste].cards.push_back(deck.Draw(true));   // the first stock card starts the pile
    while (!deck.Empty()) piles_[kStock].cards.push_back(deck.Draw(false));
    UpdateScore();
}

bool Golf::Playable(const Card& c) const {
    const Pile& waste = piles_[kWaste];
    if (waste.Empty()) return false;
    const Card& top = waste.Top();
    return top.rank != kKing && std::abs(int(c.rank) - int(top.rank)) == 1;
}

bool Golf::CanPickUp(int pile, int index) const {
    const Pile& p = piles_[pile];
    return p.kind == PileKind::Tableau && index >= 0 && index == p.Count() - 1 && Playable(p.Top());
}

bool Golf::CanDrop(int from, int index, int to) const {
    return to == kWaste && CanPickUp(from, index);
}

bool Golf::Click(int pile, int index) {
    if (Ended()) return false;
    if (pile == kStock) {
        Pile& stock = piles_[kStock];
        if (stock.Empty()) return false;
        PushUndo();
        Card c = stock.cards.back();
        stock.cards.pop_back();
        c.faceUp = true;
        piles_[kWaste].cards.push_back(c);
        ClearUndo();   // drawing reveals a new card; no taking it back (as in Things)
        return true;
    }
    return CanPickUp(pile, index) && TryMove(pile, index, kWaste);
}

void Golf::OnMoved(int from, int to, int count) {
    (void)from; (void)to; (void)count;
    UpdateScore();
}

// Cards left in the columns; once they're cleared, minus the cards left in the stock.
int Golf::Strokes() const {
    int left = 0;
    for (int i = 0; i < kColumns; ++i) left += piles_[kColumn0 + i].Count();
    return left > 0 ? left : -piles_[kStock].Count();
}

void Golf::UpdateScore() {
    score_ = kColumns * kDepth - Strokes();
}

bool Golf::IsWon() const {
    for (int i = 0; i < kColumns; ++i)
        if (!piles_[kColumn0 + i].Empty()) return false;
    return true;
}

bool Golf::IsLost() const {
    if (!piles_[kStock].Empty() || IsWon()) return false;
    for (int i = 0; i < kColumns; ++i) {
        const Pile& p = piles_[kColumn0 + i];
        if (!p.Empty() && Playable(p.Top())) return false;
    }
    return true;
}

std::wstring Golf::Rules() const {
    return L"Goal: clear all 35 cards from the columns.\n\n"
           L"Layout: 7 columns of 5 face-up cards. One card starts the face-up pile; the other 16 form the deck.\n\n"
           L"• Click the bottom card of a column to play it onto the face-up pile if it is one rank higher or lower, in any suit.\n"
           L"• Aces and Kings don't wrap, and nothing can be played on a King.\n"
           L"• Click the deck to turn up its next card. There is only one pass through the deck, and a draw can't be undone.\n\n"
           L"Strokes, as in golf: the cards left in the columns, or minus the cards left in the deck if you clear them. Lower is better.\n\n"
           L"Score: 35 minus strokes.";
}
