#include "Things.h"
#include "../core/Deck.h"
#include <algorithm>
#include <cstdlib>

namespace {
constexpr int kClearBonusPerBoard = 100;   // clearing board N scores N x 100
constexpr int kBonusPerStockCard = 10;     // plus this for each card left to draw
}

Things::Things(Settings& settings) : Game(settings) {
    piles_.resize(kColumn0 + kColumns);
    piles_[kStock].kind = PileKind::Stock;
    piles_[kWaste].kind = PileKind::Waste;
    for (int i = 0; i < kColumns; ++i) {
        piles_[kColumn0 + i].kind = PileKind::Tableau;
        piles_[kColumn0 + i].fan = Fan::Down;
    }
}

std::wstring Things::ScoreText() const {
    return L"Score: " + std::to_wstring(score_) + L"   Board: " + std::to_wstring(board_) +
           L"   Chain: " + std::to_wstring(chain_) + L"   Deck: " + std::to_wstring(piles_[kStock].Count());
}

void Things::Layout(const RECT& area, int cardW, int cardH, int unit) {
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
    // Top row: draw pile and face-up card.
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

void Things::Deal(uint32_t seed) {
    seed_ = seed;
    board_ = 1;
    chain_ = 0;
    DealBoard();
}

void Things::DealBoard() {
    for (Pile& p : piles_) p.cards.clear();
    Deck deck;
    deck.AddJokers(2);
    // Each board in a game gets its own shuffle, reproducible from the deal number.
    deck.Shuffle(seed_ + static_cast<uint32_t>(board_ - 1) * 0x9E3779B9u);
    for (int row = 0; row < kDepth; ++row)
        for (int col = 0; col < kColumns; ++col)
            piles_[kColumn0 + col].cards.push_back(deck.Draw(true));
    piles_[kWaste].cards.push_back(deck.Draw(true));
    while (!deck.Empty()) piles_[kStock].cards.push_back(deck.Draw(false));
}

bool Things::Playable(const Card& c) const {
    const Pile& waste = piles_[kWaste];
    if (waste.Empty()) return false;
    const Card& top = waste.Top();
    if (c.IsJoker() || top.IsJoker()) return true;
    return std::abs(int(c.rank) - int(top.rank)) == 1;
}

bool Things::CanPickUp(int pile, int index) const {
    const Pile& p = piles_[pile];
    return p.kind == PileKind::Tableau && index >= 0 && index == p.Count() - 1 && Playable(p.Top());
}

bool Things::CanDrop(int from, int index, int to) const {
    return to == kWaste && CanPickUp(from, index);
}

bool Things::Click(int pile, int index) {
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

void Things::OnMoved(int from, int to, int count) {
    (void)from; (void)to; (void)count;
    ++chain_;
    score_ += chain_;   // longer chains are worth more per card

    if (BoardEmpty()) {
        score_ += kClearBonusPerBoard * board_ + kBonusPerStockCard * piles_[kStock].Count();
        ++board_;
        chain_ = 0;
        DealBoard();
        ClearUndo();
    }
}

void Things::LoadExtra(const std::vector<int>& e) {
    if (e.size() >= 2) {
        chain_ = e[0];
        board_ = e[1];
    }
}

bool Things::BoardEmpty() const {
    for (int i = 0; i < kColumns; ++i)
        if (!piles_[kColumn0 + i].Empty()) return false;
    return true;
}

bool Things::IsLost() const {
    if (!piles_[kStock].Empty()) return false;
    for (int i = 0; i < kColumns; ++i) {
        const Pile& p = piles_[kColumn0 + i];
        if (!p.Empty() && Playable(p.Top())) return false;
    }
    return true;
}
