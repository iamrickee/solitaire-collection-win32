#include "Accordion.h"
#include "../core/Deck.h"
#include <algorithm>

Accordion::Accordion(Settings& settings) : Game(settings) {
    piles_.resize(52);
    for (Pile& p : piles_) p.kind = PileKind::Cell;
}

std::wstring Accordion::ScoreText() const {
    return L"Score: " + std::to_wstring(score_) + L"   Piles: " + std::to_wstring(PileCount());
}

void Accordion::Layout(const RECT& area, int cardW, int cardH, int unit) {
    const int gap = Art::kGap * unit;
    const int vgap = gap / 2;
    const int step = cardW * 3 / 4;   // each card overlaps the right edge of the one before it
    const int rowW = (kCols - 1) * step + cardW;
    const int left = area.left + std::max(gap, (int(area.right - area.left) - rowW) / 2);
    const int top = area.top + vgap;

    for (int i = 0; i < static_cast<int>(piles_.size()); ++i) {
        Pile& p = piles_[i];
        p.cardW = cardW;
        p.cardH = cardH;
        p.x = left + (i % kCols) * step;
        p.y = top + (i / kCols) * (cardH + vgap);
    }
}

void Accordion::Deal(uint32_t seed) {
    Deck deck;
    deck.Shuffle(seed);
    for (Pile& p : piles_) p.cards.push_back(deck.Draw(true));
    score_ = 0;
}

// The piles are always packed at the front of piles_.
int Accordion::PileCount() const {
    int n = 0;
    while (n < static_cast<int>(piles_.size()) && !piles_[n].Empty()) ++n;
    return n;
}

bool Accordion::Fits(int from, int to) const {
    if (to < 0 || from >= static_cast<int>(piles_.size())) return false;
    if (from - to != 1 && from - to != 3) return false;
    const Pile& src = piles_[from];
    const Pile& dst = piles_[to];
    if (src.Empty() || dst.Empty()) return false;
    return src.Top().suit == dst.Top().suit || src.Top().rank == dst.Top().rank;
}

std::vector<int> Accordion::TargetsFor(int from) const {
    std::vector<int> out;
    for (int d : {1, 3})
        if (Fits(from, from - d)) out.push_back(from - d);
    return out;
}

// The app moves the top card; OnMoved brings the rest of the pile along.
bool Accordion::CanPickUp(int pile, int index) const {
    const Pile& p = piles_[pile];
    return !p.Empty() && index == p.Count() - 1 && !TargetsFor(pile).empty();
}

bool Accordion::CanDrop(int from, int index, int to) const {
    return index == piles_[from].Count() - 1 && Fits(from, to);
}

bool Accordion::Click(int pile, int index) {
    if (Ended() || index < 0) return false;
    // A pile with exactly one move makes it. (With two it's left selected so
    // the next click picks the destination.)
    const std::vector<int> targets = TargetsFor(pile);
    return targets.size() == 1 && TryMove(pile, piles_[pile].Count() - 1, targets[0]);
}

void Accordion::OnMoved(int from, int to, int count) {
    (void)count;
    // Slip the rest of the moved pile under its top card, keeping its order.
    auto& src = piles_[from].cards;
    auto& dst = piles_[to].cards;
    dst.insert(dst.end() - 1, src.begin(), src.end());
    src.clear();
    // Close up the line.
    for (int i = from; i + 1 < static_cast<int>(piles_.size()); ++i) std::swap(piles_[i].cards, piles_[i + 1].cards);
    score_ = 52 - PileCount();
}

bool Accordion::IsWon() const {
    return PileCount() == 1;
}

// Stuck: more than one pile and none can move.
bool Accordion::IsLost() const {
    if (IsWon()) return false;
    const int n = PileCount();
    for (int i = 1; i < n; ++i)
        if (!TargetsFor(i).empty()) return false;
    return true;
}

std::wstring Accordion::Rules() const {
    return L"Goal: gather all 52 cards into a single pile.\n\n"
           L"Layout: every card is dealt face up in one long line, shown wrapped into rows of 13 that read left to right, top to bottom.\n\n"
           L"• A pile can move onto the pile directly to its left, or onto the pile three places to its left, if their top cards match in suit or rank.\n"
           L"• The whole pile moves, and the line closes up behind it.\n"
           L"• Click a pile to move it. If it could go either way, it is selected and you click where it goes.\n\n"
           L"The game ends when no pile can move. Finishing with only a few piles is a good result.\n\n"
           L"Score: cards stacked (52 minus the piles left).";
}
