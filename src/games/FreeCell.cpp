#include "FreeCell.h"
#include <algorithm>

FreeCell::FreeCell(Settings& settings) : Game(settings) {
    piles_.resize(kColumn0 + 8);
    for (int i = 0; i < 4; ++i) {
        piles_[kCell0 + i].kind = PileKind::Cell;
        piles_[kFoundation0 + i].kind = PileKind::Foundation;
    }
    for (int i = 0; i < 8; ++i) {
        piles_[kColumn0 + i].kind = PileKind::Tableau;
        piles_[kColumn0 + i].fan = Fan::Down;
    }
}

void FreeCell::Layout(const RECT& area, int cardW, int cardH, int unit) {
    const int gap = Art::kGap * unit;
    const int totalW = 8 * cardW + 7 * gap;
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
    for (int i = 0; i < 4; ++i) {
        piles_[kCell0 + i].x = colX(i);
        piles_[kCell0 + i].y = top;
        piles_[kFoundation0 + i].x = colX(4 + i);
        piles_[kFoundation0 + i].y = top;
    }
    for (int i = 0; i < 8; ++i) {
        Pile& p = piles_[kColumn0 + i];
        p.x = colX(i);
        p.y = tabTop;
        p.maxExtent = std::max(cardH, int(area.bottom) - tabTop - gap);
    }
}

// Microsoft FreeCell's deal: its C runtime rand() picks from the remaining
// cards, which are numbered rank * 4 + suit (clubs, diamonds, hearts,
// spades), and they're dealt left to right across the eight columns.
void FreeCell::Deal(uint32_t seed) {
    uint32_t state = seed;
    auto rnd = [&state] {
        state = (state * 214013u + 2531011u) & 0x7FFFFFFFu;
        return int(state >> 16);
    };
    int deck[52];
    for (int i = 0; i < 52; ++i) deck[i] = i;
    int left = 52;
    for (int k = 0; k < 52; ++k) {
        const int j = rnd() % left;
        const int c = deck[j];
        deck[j] = deck[--left];
        Card card{static_cast<Suit>(c % 4), static_cast<uint8_t>(c / 4 + 1), true};
        piles_[kColumn0 + k % 8].cards.push_back(card);
    }
}

bool FreeCell::IsRun(const Pile& p, int index) const {
    for (int i = index; i + 1 < p.Count(); ++i) {
        const Card& a = p.cards[i];
        const Card& b = p.cards[i + 1];
        if (a.SameColor(b) || a.rank != b.rank + 1) return false;
    }
    return true;
}

int FreeCell::MaxMove(bool toEmptyColumn) const {
    int cells = 0, columns = 0;
    for (int i = 0; i < 4; ++i)
        if (piles_[kCell0 + i].Empty()) ++cells;
    for (int i = 0; i < 8; ++i)
        if (piles_[kColumn0 + i].Empty()) ++columns;
    if (toEmptyColumn) --columns;   // the target can't also hold cards on the way
    return (cells + 1) << std::max(0, columns);
}

bool FreeCell::CanPickUp(int pile, int index) const {
    const Pile& p = piles_[pile];
    if (index < 0 || index >= p.Count()) return false;
    switch (p.kind) {
    case PileKind::Cell:
        return true;
    case PileKind::Tableau:
        return IsRun(p, index) && p.Count() - index <= MaxMove(false);
    default:
        return false;   // foundation cards stay put
    }
}

bool FreeCell::CanDrop(int from, int index, int to) const {
    if (!CanPickUp(from, index)) return false;
    const Card& c = piles_[from].cards[index];
    const int count = piles_[from].Count() - index;
    const Pile& dst = piles_[to];
    switch (dst.kind) {
    case PileKind::Cell:
        return count == 1 && dst.Empty();
    case PileKind::Foundation:
        if (count != 1) return false;
        if (dst.Empty()) return c.rank == kAce;
        return dst.Top().suit == c.suit && c.rank == dst.Top().rank + 1;
    case PileKind::Tableau:
        if (dst.Empty()) return count <= MaxMove(true);
        return !dst.Top().SameColor(c) && c.rank + 1 == dst.Top().rank;
    default:
        return false;
    }
}

void FreeCell::OnMoved(int from, int to, int count) {
    // Score = cards on the foundations.
    if (piles_[to].kind == PileKind::Foundation) score_ += count;
    if (piles_[from].kind == PileKind::Foundation) score_ -= count;
}

int FreeCell::DoubleClickTarget(int pile, int index) const {
    // Foundation if it fits, else (from a column) the first free cell.
    const int f = Game::DoubleClickTarget(pile, index);
    if (f >= 0 || piles_[pile].kind != PileKind::Tableau || index != piles_[pile].Count() - 1) return f;
    for (int i = 0; i < 4; ++i)
        if (piles_[kCell0 + i].Empty()) return kCell0 + i;
    return -1;
}

int FreeCell::FoundationRank(Suit s) const {
    for (int i = 0; i < 4; ++i) {
        const Pile& f = piles_[kFoundation0 + i];
        if (!f.Empty() && f.Top().suit == s) return f.Top().rank;
    }
    return 0;
}

// Safe when no column could still want it: both opposite-colour foundations
// are at least one rank below it (so nothing it could hold is still out).
bool FreeCell::SafeToFoundation(const Card& c) const {
    if (c.rank <= 2) return true;
    const Suit opp[2] = {c.IsRed() ? Suit::Clubs : Suit::Diamonds, c.IsRed() ? Suit::Spades : Suit::Hearts};
    return FoundationRank(opp[0]) >= c.rank - 1 && FoundationRank(opp[1]) >= c.rank - 1;
}

bool FreeCell::FindAutoMove(int& from, int& to) const {
    for (int p = 0; p < static_cast<int>(piles_.size()); ++p) {
        const Pile& src = piles_[p];
        if (src.Empty() || src.kind == PileKind::Foundation || !SafeToFoundation(src.Top())) continue;
        const int f = Game::DoubleClickTarget(p, src.Count() - 1);
        if (f >= 0) { from = p; to = f; return true; }
    }
    return false;
}

bool FreeCell::IsWon() const {
    for (int i = 0; i < 4; ++i)
        if (piles_[kFoundation0 + i].Count() != 13) return false;
    return true;
}

// Stuck: no single card can move anywhere (runs are made of single moves).
bool FreeCell::IsLost() const {
    for (int from = 0; from < static_cast<int>(piles_.size()); ++from) {
        const Pile& src = piles_[from];
        if (src.Empty() || src.kind == PileKind::Foundation) continue;
        for (int to = 0; to < static_cast<int>(piles_.size()); ++to)
            if (to != from && CanDrop(from, src.Count() - 1, to)) return false;
    }
    return true;
}

std::wstring FreeCell::SlotHint(int pile) const {
    return piles_[pile].kind == PileKind::Foundation ? L"A" : std::wstring{};
}

std::wstring FreeCell::Rules() const {
    return L"Goal: move all 52 cards to the four foundations, building each up in one suit from Ace to King.\n\n"
           L"Layout: every card is dealt face up into 8 columns. Above them are 4 free cells and the 4 foundations.\n\n"
           L"• Build columns down in alternating colors.\n"
           L"• Each free cell holds any one card.\n"
           L"• Any card or run can go into an empty column.\n"
           L"• A run moves as a unit when there are enough free cells and empty columns to move it one card at a time.\n\n"
           L"Nearly every deal can be won. Deal numbers match Microsoft FreeCell, so deal #11982 is the famous unwinnable one.\n\n"
           L"Score: cards on the foundations.";
}
