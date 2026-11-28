#include "Spiderette.h"
#include "../core/Deck.h"
#include <algorithm>

namespace {
constexpr int kStartScore = 500;
constexpr int kMoveCost = 1;     // every move and every deal
constexpr int kRunBonus = 100;   // each completed K..A run
}

Spiderette::Spiderette(Settings& settings) : Game(settings) {
    piles_.resize(kColumn0 + kColumns);
    piles_[kStock].kind = PileKind::Stock;
    for (int i = 0; i < 4; ++i) piles_[kFoundation0 + i].kind = PileKind::Foundation;
    for (int i = 0; i < kColumns; ++i) {
        piles_[kColumn0 + i].kind = PileKind::Tableau;
        piles_[kColumn0 + i].fan = Fan::Down;
    }
}

std::wstring Spiderette::ScoreText() const {
    const int n = piles_[kStock].Count();
    return L"Score: " + std::to_wstring(score_) + L"   Deals left: " +
           std::to_wstring((n + kColumns - 1) / kColumns);
}

void Spiderette::Layout(const RECT& area, int cardW, int cardH, int unit) {
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

    // Top row: deck on the left, completed runs on the right.
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

void Spiderette::Deal(uint32_t seed) {
    Deck deck;
    deck.Shuffle(seed);
    for (int col = 0; col < kColumns; ++col)
        for (int row = 0; row <= col; ++row)
            piles_[kColumn0 + col].cards.push_back(deck.Draw(row == col));
    while (!deck.Empty()) piles_[kStock].cards.push_back(deck.Draw(false));
    score_ = kStartScore;
}

bool Spiderette::CanPickUp(int pile, int index) const {
    const Pile& p = piles_[pile];
    if (p.kind != PileKind::Tableau || index < 0 || index >= p.Count() || !p.cards[index].faceUp) return false;
    // Only a same-suit descending run moves as a unit.
    for (int i = index; i + 1 < p.Count(); ++i) {
        const Card& a = p.cards[i];
        const Card& b = p.cards[i + 1];
        if (a.suit != b.suit || a.rank != b.rank + 1) return false;
    }
    return true;
}

bool Spiderette::CanDrop(int from, int index, int to) const {
    if (!CanPickUp(from, index)) return false;
    const Pile& dst = piles_[to];
    if (dst.kind != PileKind::Tableau) return false;
    if (dst.Empty()) return true;
    // Any suit, one rank lower.
    return dst.Top().faceUp && piles_[from].cards[index].rank + 1 == dst.Top().rank;
}

void Spiderette::FlipTop(int column) {
    Pile& p = piles_[column];
    if (!p.Empty() && !p.Top().faceUp) p.Top().faceUp = true;
}

void Spiderette::CollectRun(int column) {
    Pile& p = piles_[column];
    if (p.Count() < 13) return;
    const int start = p.Count() - 13;
    if (p.cards[start].rank != kKing || !CanPickUp(column, start)) return;

    for (int i = 0; i < 4; ++i) {
        Pile& f = piles_[kFoundation0 + i];
        if (!f.Empty()) continue;
        // Stored Ace first so the King shows on top of the slot.
        f.cards.assign(p.cards.rbegin(), p.cards.rbegin() + 13);
        p.cards.erase(p.cards.begin() + start, p.cards.end());
        score_ += kRunBonus;
        FlipTop(column);
        return;
    }
}

void Spiderette::OnMoved(int from, int to, int count) {
    (void)count;
    score_ = std::max(0, score_ - kMoveCost);
    FlipTop(from);
    CollectRun(to);
}

bool Spiderette::Click(int pile, int index) {
    (void)index;
    if (pile != kStock || Ended()) return false;
    Pile& stock = piles_[kStock];
    if (stock.Empty()) return false;
    // As in Spider: no dealing while any column is empty.
    for (int i = 0; i < kColumns; ++i) {
        if (piles_[kColumn0 + i].Empty()) {
            MessageBeep(MB_ICONWARNING);
            return false;
        }
    }
    PushUndo();
    // 24 cards: three full deals, then the last 3 go on the first three columns.
    for (int i = 0; i < kColumns && !stock.Empty(); ++i) {
        Card c = stock.cards.back();
        stock.cards.pop_back();
        c.faceUp = true;
        piles_[kColumn0 + i].cards.push_back(c);
    }
    score_ = std::max(0, score_ - kMoveCost);
    for (int i = 0; i < kColumns; ++i) CollectRun(kColumn0 + i);
    return true;
}

bool Spiderette::IsWon() const {
    for (int i = 0; i < 4; ++i)
        if (piles_[kFoundation0 + i].Empty()) return false;
    return true;
}

std::wstring Spiderette::Rules() const {
    return L"Goal: build four runs from King down to Ace, each in one suit. A finished run is removed to a foundation.\n\n"
           L"Layout: Spider with one deck in a Klondike layout: 7 columns of 1 to 7 cards, top card face up. The other 24 cards form the deck.\n\n"
           L"• Build down regardless of suit, but only a run of one suit can move as a unit.\n"
           L"• Any card or run can go into an empty column.\n"
           L"• Click the deck to deal one card onto every column. You can't deal while a column is empty.\n\n"
           L"Score: starts at 500, −1 per move or deal, +100 per run removed. An undo costs 1 more, and the move it takes back isn't refunded.";
}
