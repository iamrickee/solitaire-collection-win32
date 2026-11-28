#include "KingsCorners.h"
#include "../core/Deck.h"
#include <algorithm>

KingsCorners::KingsCorners(Settings& settings) : Game(settings) {
    piles_.resize(kCell0 + kCells);
    piles_[kStock].kind = PileKind::Stock;
    piles_[kNext].kind = PileKind::Waste;
    piles_[kDiscard].kind = PileKind::Foundation;
    for (int i = 0; i < kCells; ++i) piles_[kCell0 + i].kind = PileKind::Cell;
}

std::wstring KingsCorners::ScoreText() const {
    return L"Score: " + std::to_wstring(score_) + L"   Deck: " +
           std::to_wstring(piles_[kStock].Count() + piles_[kNext].Count());
}

int KingsCorners::HomeRank(int cell) {
    const int r = cell / 4, c = cell % 4;
    const bool edgeRow = r == 0 || r == 3, edgeCol = c == 0 || c == 3;
    if (edgeRow && edgeCol) return kKing;
    if (edgeRow) return kQueen;
    if (edgeCol) return kJack;
    return 0;
}

void KingsCorners::Layout(const RECT& area, int cardW, int cardH, int unit) {
    const int gap = Art::kGap * unit;
    const int vgap = gap / 2;   // tighter rows keep the grid short enough for a 2x scale
    // Deck column, a wider gap, the 4x4 grid, a wider gap, the discard pile.
    const int totalW = 6 * cardW + 5 * gap + 4 * gap;
    const int totalH = 4 * cardH + 3 * vgap;
    const int left = area.left + std::max(gap, (int(area.right - area.left) - totalW) / 2);
    const int top = area.top + std::max(vgap, (int(area.bottom - area.top) - totalH) / 2);
    const int gridX = left + cardW + 3 * gap;

    for (Pile& p : piles_) {
        p.cardW = cardW;
        p.cardH = cardH;
    }
    piles_[kStock].x = left;
    piles_[kStock].y = top + cardH / 2;
    piles_[kNext].x = left;
    piles_[kNext].y = top + 2 * cardH + 3 * vgap - cardH / 2;
    piles_[kDiscard].x = gridX + 4 * cardW + 3 * gap + 3 * gap;
    piles_[kDiscard].y = top + cardH / 2;
    for (int i = 0; i < kCells; ++i) {
        piles_[kCell0 + i].x = gridX + (i % 4) * (cardW + gap);
        piles_[kCell0 + i].y = top + (i / 4) * (cardH + vgap);
    }
}

void KingsCorners::Deal(uint32_t seed) {
    Deck deck;
    deck.Shuffle(seed);
    while (!deck.Empty()) piles_[kStock].cards.push_back(deck.Draw(false));
    TurnNext();
}

void KingsCorners::TurnNext() {
    Pile& stock = piles_[kStock];
    if (stock.Empty() || !piles_[kNext].Empty()) return;
    Card c = stock.cards.back();
    stock.cards.pop_back();
    c.faceUp = true;
    piles_[kNext].cards.push_back(c);
}

bool KingsCorners::Pairs(const Card& a, const Card& b) {
    return !IsFace(a) && !IsFace(b) && a.rank + b.rank == 10;
}

bool KingsCorners::CanPlace(const Card& c, int pile) const {
    if (pile < kCell0 || !piles_[pile].Empty()) return false;
    return !IsFace(c) || HomeRank(pile - kCell0) == c.rank;
}

bool KingsCorners::Placing() const { return !piles_[kNext].Empty(); }

bool KingsCorners::GridFull() const {
    for (int i = 0; i < kCells; ++i)
        if (piles_[kCell0 + i].Empty()) return false;
    return true;
}

bool KingsCorners::CanPickUp(int pile, int index) const {
    const Pile& p = piles_[pile];
    if (index < 0 || index != p.Count() - 1) return false;
    if (pile == kNext) return true;
    // Number cards pair off only in the removal stage.
    return !Placing() && p.kind == PileKind::Cell && !IsFace(p.Top());
}

bool KingsCorners::CanDrop(int from, int index, int to) const {
    if (!CanPickUp(from, index)) return false;
    const Card& c = piles_[from].cards[index];
    if (from == kNext) return CanPlace(c, to);
    if (to == kDiscard) return c.rank == 10;
    const Pile& dst = piles_[to];
    return dst.kind == PileKind::Cell && !dst.Empty() && Pairs(c, dst.Top());
}

int KingsCorners::DoubleClickTarget(int pile, int index) const {
    return CanDrop(pile, index, kDiscard) ? kDiscard : -1;
}

bool KingsCorners::Click(int pile, int index) {
    if (Ended()) return false;
    // The deck ends the removal stage: turn up a card to fill the gaps again.
    if (pile == kStock) {
        if (Placing() || GridFull() || piles_[kStock].Empty()) return false;
        PushUndo();   // marks the game as started
        TurnNext();
        ClearUndo();  // a new card is showing; no taking it back
        return true;
    }
    // A 10 needs no partner: one click removes it.
    if (pile >= kCell0 && index >= 0 && CanDrop(pile, index, kDiscard)) return TryMove(pile, index, kDiscard);
    // Clicking an empty grid cell places the next card there.
    if (pile >= kCell0 && piles_[pile].Empty() && Placing()) return TryMove(kNext, 0, pile);
    return false;
}

void KingsCorners::OnMoved(int from, int to, int count) {
    (void)count;
    if (from == kNext) {
        Card& placed = piles_[to].Top();
        if (IsFace(placed)) {
            placed.faceUp = false;   // face cards sit face down in their homes
            ++score_;
        }
        // Keep placing until the grid is full; then it's the removal stage.
        if (!GridFull()) TurnNext();
        ClearUndo();   // no taking a placement back
        return;
    }
    // A pair landed on its partner (or a 10 on the discard): clear them away.
    Pile& dst = piles_[to];
    if (to != kDiscard) {
        auto& d = piles_[kDiscard].cards;
        d.insert(d.end(), dst.cards.begin(), dst.cards.end());
        score_ += dst.Count();
        dst.cards.clear();
    } else {
        ++score_;
    }
}

bool KingsCorners::AnyRemoval() const {
    for (int i = 0; i < kCells; ++i) {
        const Pile& a = piles_[kCell0 + i];
        if (a.Empty() || IsFace(a.Top())) continue;
        if (a.Top().rank == 10) return true;
        for (int j = i + 1; j < kCells; ++j) {
            const Pile& b = piles_[kCell0 + j];
            if (!b.Empty() && Pairs(a.Top(), b.Top())) return true;
        }
    }
    return false;
}

bool KingsCorners::IsWon() const {
    for (int i = 0; i < kCells; ++i) {
        const int home = HomeRank(i);
        const Pile& p = piles_[kCell0 + i];
        if (home && (p.Empty() || p.Top().rank != home)) return false;
    }
    return true;
}

bool KingsCorners::IsLost() const {
    if (!Placing()) return GridFull() && !AnyRemoval();   // full grid, nothing to pair off
    const Card& c = piles_[kNext].Top();                   // a face card with its homes taken
    for (int i = 0; i < kCells; ++i)
        if (CanPlace(c, kCell0 + i)) return false;
    return true;
}

std::wstring KingsCorners::SlotHint(int pile) const {
    if (pile < kCell0) return {};
    switch (HomeRank(pile - kCell0)) {
    case kKing: return L"K";
    case kQueen: return L"Q";
    case kJack: return L"J";
    default: return {};
    }
}

std::wstring KingsCorners::Rules() const {
    return L"Goal: get all twelve face cards into their homes in a 4×4 grid.\n\n"
           L"• Kings go in the corners, Queens in the middle of the top and bottom rows, and Jacks in the middle of the side columns. A face card sits face down once it is home.\n"
           L"• Cards come from the deck one at a time. Click a cell to place the card: number cards can go in any empty cell.\n"
           L"• When the grid is full, remove number cards in pairs that add up to 10 (Ace counts 1). A 10 is removed on its own.\n"
           L"• Then click the deck to fill the empty cells again.\n"
           L"• Placing a card or drawing from the deck can't be undone.\n\n"
           L"You lose when a face card has nowhere to go, or the grid is full with nothing to remove.\n\n"
           L"Score: +1 per face card home and +1 per card removed.";
}
