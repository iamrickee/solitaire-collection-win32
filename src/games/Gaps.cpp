#include "Gaps.h"
#include "../core/Deck.h"
#include <algorithm>
#include <random>

Gaps::Gaps(Settings& settings) : Game(settings) {
    piles_.resize(kCell0 + kRows * kCols);
    piles_[kRedeal].kind = PileKind::Stock;
    for (int i = 0; i < kRows * kCols; ++i) piles_[kCell0 + i].kind = PileKind::Cell;
}

std::wstring Gaps::ScoreText() const {
    return L"Score: " + std::to_wstring(score_) + L"   Redeals left: " + std::to_wstring(kMaxRedeals - redeals_);
}

void Gaps::Layout(const RECT& area, int cardW, int cardH, int unit) {
    const int gap = Art::kGap * unit;
    const int vgap = gap / 2;
    const int step = cardW * 3 / 4;   // each card overlaps the right edge of the one before it
    const int rowW = (kCols - 1) * step + cardW;
    const int totalW = rowW + 2 * gap + cardW;
    const int left = area.left + std::max(gap, (int(area.right - area.left) - totalW) / 2);
    const int top = area.top + vgap;

    for (Pile& p : piles_) {
        p.cardW = cardW;
        p.cardH = cardH;
    }
    for (int r = 0; r < kRows; ++r)
        for (int c = 0; c < kCols; ++c) {
            Pile& p = piles_[CellPile(r, c)];
            p.x = left + c * step;
            p.y = top + r * (cardH + vgap);
        }
    // The redeal button to the right, between the middle rows.
    piles_[kRedeal].x = left + rowW + 2 * gap;
    piles_[kRedeal].y = top + 2 * (cardH + vgap) - cardH / 2 - vgap / 2;
}

void Gaps::Deal(uint32_t seed) {
    redeals_ = 0;
    Deck deck;
    deck.Shuffle(seed);
    for (int i = 0; i < kRows * kCols; ++i) {
        const Card c = deck.Draw(true);
        if (c.rank != kAce) piles_[kCell0 + i].cards.push_back(c);   // Aces leave the gaps
    }
    UpdateScore();
}

bool Gaps::Fits(int from, int to) const {
    if (from < kCell0 || to < kCell0 || from == to) return false;
    const Pile& src = piles_[from];
    if (src.Empty() || !piles_[to].Empty()) return false;
    const Card& c = src.Top();
    const int col = (to - kCell0) % kCols;
    if (col == 0) return c.rank == 2 && (from - kCell0) % kCols != 0;   // a 2 not already at a row start
    const Pile& left = piles_[to - 1];
    return !left.Empty() && left.Top().rank != kKing && left.Top().suit == c.suit &&
           c.rank == left.Top().rank + 1;
}

std::vector<int> Gaps::TargetsFor(int from) const {
    std::vector<int> out;
    for (int i = 0; i < kRows * kCols; ++i)
        if (Fits(from, kCell0 + i)) out.push_back(kCell0 + i);
    return out;
}

bool Gaps::CanPickUp(int pile, int index) const {
    return pile >= kCell0 && index == 0 && !TargetsFor(pile).empty();
}

bool Gaps::CanDrop(int from, int index, int to) const {
    return index == 0 && Fits(from, to);
}

bool Gaps::Click(int pile, int index) {
    if (Ended()) return false;
    if (pile == kRedeal) {
        if (redeals_ >= kMaxRedeals) return false;
        PushUndo();
        // Gather every card not in sequence, shuffle, and deal them back
        // with a gap right after each row's sequence.
        std::vector<Card> cards;
        int seq[kRows];
        for (int r = 0; r < kRows; ++r) {
            seq[r] = SequenceLength(r);
            for (int c = seq[r]; c < kCols; ++c) {
                auto& cell = piles_[CellPile(r, c)].cards;
                cards.insert(cards.end(), cell.begin(), cell.end());
                cell.clear();
            }
        }
        ++redeals_;
        std::mt19937 rng(Seed() + static_cast<uint32_t>(redeals_) * 0x9E3779B9u);   // reproducible per deal
        std::shuffle(cards.begin(), cards.end(), rng);
        size_t next = 0;
        for (int r = 0; r < kRows; ++r)
            for (int c = seq[r] + 1; c < kCols && next < cards.size(); ++c)
                piles_[CellPile(r, c)].cards.push_back(cards[next++]);
        UpdateScore();
        return true;
    }
    if (pile < kCell0) return false;
    if (index >= 0) {
        // A card with exactly one place to go goes there. (A 2 with several
        // row-start gaps is left selected so the next click picks one.)
        const std::vector<int> targets = TargetsFor(pile);
        return targets.size() == 1 && TryMove(pile, 0, targets[0]);
    }
    // Clicking a gap pulls in the card that belongs there, if it's clear which.
    for (int i = 0; i < kRows * kCols; ++i)
        if (Fits(kCell0 + i, pile) && piles_[kCell0 + i].Top().rank != 2) return TryMove(kCell0 + i, 0, pile);
    return false;
}

void Gaps::OnMoved(int from, int to, int count) {
    (void)from; (void)to; (void)count;
    UpdateScore();
}

int Gaps::SequenceLength(int row) const {
    int n = 0;
    while (n < kCols - 1) {
        const Pile& p = piles_[CellPile(row, n)];
        if (p.Empty() || p.Top().rank != n + 2) break;
        if (n > 0 && p.Top().suit != piles_[CellPile(row, 0)].Top().suit) break;
        ++n;
    }
    return n;
}

void Gaps::UpdateScore() {
    score_ = 0;
    for (int r = 0; r < kRows; ++r) score_ += SequenceLength(r);
}

bool Gaps::IsWon() const {
    for (int r = 0; r < kRows; ++r)
        if (SequenceLength(r) != kCols - 1) return false;
    return true;
}

// Stuck: no redeals left and every gap is dead (after a King, or at a row
// start with every 2 already at a row start).
bool Gaps::IsLost() const {
    if (redeals_ < kMaxRedeals || IsWon()) return false;
    for (int i = 0; i < kRows * kCols; ++i)
        if (CanPickUp(kCell0 + i, 0)) return false;
    return true;
}

std::wstring Gaps::SlotHint(int pile) const {
    if (pile == kRedeal && !Ended()) return redeals_ < kMaxRedeals ? L"↻" : L"✕";   // ↻ or ✕
    return {};
}
