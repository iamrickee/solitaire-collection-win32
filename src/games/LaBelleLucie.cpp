#include "LaBelleLucie.h"
#include "../core/Deck.h"
#include <algorithm>
#include <random>

namespace {
constexpr int kFansPerRow[4] = {5, 5, 4, 4};
}

LaBelleLucie::LaBelleLucie(Settings& settings) : Game(settings) {
    piles_.resize(kFan0 + kFans);
    piles_[kRedeal].kind = PileKind::Stock;
    for (int i = 0; i < 4; ++i) piles_[kFoundation0 + i].kind = PileKind::Foundation;
    for (int i = 0; i < kFans; ++i) {
        piles_[kFan0 + i].kind = PileKind::Tableau;
        piles_[kFan0 + i].fan = Fan::Right;
    }
}

std::wstring LaBelleLucie::ScoreText() const {
    return L"Score: " + std::to_wstring(score_) + L"   Redeals left: " + std::to_wstring(kMaxRedeals - redeals_);
}

void LaBelleLucie::Layout(const RECT& area, int cardW, int cardH, int unit) {
    const int gap = Art::kGap * unit;
    const int vgap = gap / 2;          // tighter rows keep four of them within a 2x scale
    const int slotW = cardW * 7 / 4;   // a fan of three, with room to grow
    const int totalW = 5 * slotW + 5 * gap + gap + cardW;
    const int left = area.left + std::max(gap, (int(area.right - area.left) - totalW) / 2);
    const int top = area.top + vgap;
    auto slotX = [&](int c) { return left + c * (slotW + gap); };

    for (Pile& p : piles_) {
        p.cardW = cardW;
        p.cardH = cardH;
        p.offFaceDown = Art::kFanDown * unit;
        p.offFaceUp = Art::kFanRight * unit;
        p.maxExtent = 0;
    }
    int fan = 0;
    for (int row = 0; row < 4; ++row) {
        const int y = top + row * (cardH + vgap);
        for (int c = 0; c < kFansPerRow[row]; ++c, ++fan) {
            Pile& p = piles_[kFan0 + fan];
            p.x = slotX(c);
            p.y = y;
            p.maxExtent = slotW;
        }
        // Foundations in a column on the right.
        piles_[kFoundation0 + row].x = slotX(5) + gap;
        piles_[kFoundation0 + row].y = y;
    }
    // The redeal button in the last row's spare slot.
    piles_[kRedeal].x = slotX(4);
    piles_[kRedeal].y = top + 3 * (cardH + vgap);
}

void LaBelleLucie::DealFans(std::vector<Card>& cards) {
    for (int i = 0; i < kFans; ++i) piles_[kFan0 + i].cards.clear();
    for (size_t i = 0; i < cards.size(); ++i) {
        cards[i].faceUp = true;
        piles_[kFan0 + static_cast<int>(i / 3)].cards.push_back(cards[i]);
    }
}

void LaBelleLucie::Deal(uint32_t seed) {
    redeals_ = 0;
    Deck deck;
    deck.Shuffle(seed);
    std::vector<Card> cards;
    while (!deck.Empty()) cards.push_back(deck.Draw(true));
    DealFans(cards);   // 17 fans of three and a single card
}

bool LaBelleLucie::CanPickUp(int pile, int index) const {
    const Pile& p = piles_[pile];
    return p.kind == PileKind::Tableau && index >= 0 && index == p.Count() - 1;   // one card at a time
}

bool LaBelleLucie::CanDrop(int from, int index, int to) const {
    if (!CanPickUp(from, index)) return false;
    const Card& c = piles_[from].cards[index];
    const Pile& dst = piles_[to];
    switch (dst.kind) {
    case PileKind::Foundation:
        if (dst.Empty()) return c.rank == kAce;
        return dst.Top().suit == c.suit && c.rank == dst.Top().rank + 1;
    case PileKind::Tableau:   // in suit; empty fans stay empty
        return !dst.Empty() && dst.Top().suit == c.suit && c.rank + 1 == dst.Top().rank;
    default:
        return false;
    }
}

void LaBelleLucie::OnMoved(int from, int to, int count) {
    // Score = cards on the foundations.
    if (piles_[to].kind == PileKind::Foundation) score_ += count;
    if (piles_[from].kind == PileKind::Foundation) score_ -= count;
}

bool LaBelleLucie::Click(int pile, int index) {
    (void)index;
    if (pile != kRedeal || Ended() || redeals_ >= kMaxRedeals) return false;
    PushUndo();
    // Gather the fans in order, shuffle, and deal again in threes.
    std::vector<Card> cards;
    for (int i = 0; i < kFans; ++i) {
        auto& f = piles_[kFan0 + i].cards;
        cards.insert(cards.end(), f.begin(), f.end());
    }
    ++redeals_;
    std::mt19937 rng(Seed() + static_cast<uint32_t>(redeals_) * 0x9E3779B9u);   // reproducible per deal
    std::shuffle(cards.begin(), cards.end(), rng);
    DealFans(cards);
    return true;
}

bool LaBelleLucie::IsWon() const {
    for (int i = 0; i < 4; ++i)
        if (piles_[kFoundation0 + i].Count() != 13) return false;
    return true;
}

// Stuck: no redeals left and no card can move.
bool LaBelleLucie::IsLost() const {
    if (redeals_ < kMaxRedeals || IsWon()) return false;
    for (int from = kFan0; from < kFan0 + kFans; ++from) {
        const Pile& src = piles_[from];
        if (src.Empty()) continue;
        for (int to = kFoundation0; to < kFan0 + kFans; ++to)
            if (to != from && CanDrop(from, src.Count() - 1, to)) return false;
    }
    return true;
}

std::wstring LaBelleLucie::SlotHint(int pile) const {
    if (pile == kRedeal && !Ended()) return redeals_ < kMaxRedeals ? L"↻" : L"✕";   // ↻ or ✕
    return piles_[pile].kind == PileKind::Foundation ? L"A" : std::wstring{};
}
