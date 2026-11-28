#include "Spider.h"
#include "../core/Deck.h"
#include "../../res/resource.h"
#include <algorithm>

namespace {
constexpr wchar_t kSection[] = L"Spider";
constexpr int kStartScore = 500;
constexpr int kMoveCost = 1;        // every move and every deal
constexpr int kRunBonus = 100;      // each completed K..A run
}

Spider::Spider(Settings& settings) : Game(settings) {
    piles_.resize(kColumn0 + kColumns);
    piles_[kStock].kind = PileKind::Stock;
    for (int i = 0; i < kFoundations; ++i) piles_[kFoundation0 + i].kind = PileKind::Foundation;
    for (int i = 0; i < kColumns; ++i) {
        piles_[kColumn0 + i].kind = PileKind::Tableau;
        piles_[kColumn0 + i].fan = Fan::Down;
    }
    LoadOptions();
}

void Spider::LoadOptions() {
    const int s = settings_.GetInt(kSection, L"Suits", 1);
    suits_ = (s == 2 || s == 4) ? s : 1;
}

std::wstring Spider::Label() const {
    return L"Spider (" + std::to_wstring(suits_) + (suits_ == 1 ? L" Suit)" : L" Suits)");
}

std::wstring Spider::ScoreText() const {
    return L"Score: " + std::to_wstring(score_) + L"   Deals left: " +
           std::to_wstring(piles_[kStock].Count() / kColumns);
}

void Spider::Layout(const RECT& area, int cardW, int cardH, int unit) {
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

    // Top row: stock on the left, completed runs on the right.
    piles_[kStock].x = colX(0);
    piles_[kStock].y = top;
    for (int i = 0; i < kFoundations; ++i) {
        piles_[kFoundation0 + i].x = colX(kColumns - kFoundations + i);
        piles_[kFoundation0 + i].y = top;
    }
    for (int i = 0; i < kColumns; ++i) {
        Pile& p = piles_[kColumn0 + i];
        p.x = colX(i);
        p.y = tabTop;
        p.maxExtent = std::max(cardH, int(area.bottom) - tabTop - gap);
    }
}

void Spider::Deal(uint32_t seed) {
    LoadOptions();
    // Always 104 cards: 8 sets of one suit, 4 sets of two, or 2 full decks.
    Deck deck = suits_ == 1 ? Deck(8, {Suit::Spades})
              : suits_ == 2 ? Deck(4, {Suit::Spades, Suit::Hearts})
                            : Deck(2);
    deck.Shuffle(seed);
    // 54 cards: 6 in the first four columns, 5 in the rest; top card face up.
    for (int col = 0; col < kColumns; ++col) {
        const int n = col < 4 ? 6 : 5;
        for (int i = 0; i < n; ++i) piles_[kColumn0 + col].cards.push_back(deck.Draw(i == n - 1));
    }
    while (!deck.Empty()) piles_[kStock].cards.push_back(deck.Draw(false));
    score_ = kStartScore;
}

bool Spider::CanPickUp(int pile, int index) const {
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

bool Spider::CanDrop(int from, int index, int to) const {
    if (!CanPickUp(from, index)) return false;
    const Pile& dst = piles_[to];
    if (dst.kind != PileKind::Tableau) return false;
    if (dst.Empty()) return true;
    // Any suit, one rank lower.
    return dst.Top().faceUp && piles_[from].cards[index].rank + 1 == dst.Top().rank;
}

void Spider::FlipTop(int column) {
    Pile& p = piles_[column];
    if (!p.Empty() && !p.Top().faceUp) p.Top().faceUp = true;
}

void Spider::CollectRun(int column) {
    Pile& p = piles_[column];
    if (p.Count() < 13) return;
    const int start = p.Count() - 13;
    if (p.cards[start].rank != kKing || !CanPickUp(column, start)) return;

    for (int i = 0; i < kFoundations; ++i) {
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

void Spider::OnMoved(int from, int to, int count) {
    (void)count;
    score_ = std::max(0, score_ - kMoveCost);
    FlipTop(from);
    CollectRun(to);
}

bool Spider::Click(int pile, int index) {
    (void)index;
    if (pile != kStock || Ended()) return false;
    Pile& stock = piles_[kStock];
    if (stock.Empty()) return false;
    // The usual rule: no dealing while any column is empty.
    for (int i = 0; i < kColumns; ++i) {
        if (piles_[kColumn0 + i].Empty()) {
            MessageBeep(MB_ICONWARNING);
            return false;
        }
    }
    PushUndo();
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

int Spider::CompletedRuns() const {
    int n = 0;
    for (int i = 0; i < kFoundations; ++i)
        if (!piles_[kFoundation0 + i].Empty()) ++n;
    return n;
}

bool Spider::IsWon() const { return CompletedRuns() == kFoundations; }

// ---- Options dialog --------------------------------------------------------

namespace {
INT_PTR CALLBACK OptionsProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* suits = reinterpret_cast<int*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG:
        suits = reinterpret_cast<int*>(lp);
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        CheckRadioButton(dlg, IDC_SP_SUIT1, IDC_SP_SUIT4,
                         *suits == 4 ? IDC_SP_SUIT4 : *suits == 2 ? IDC_SP_SUIT2 : IDC_SP_SUIT1);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDOK:
            *suits = IsDlgButtonChecked(dlg, IDC_SP_SUIT4) == BST_CHECKED   ? 4
                   : IsDlgButtonChecked(dlg, IDC_SP_SUIT2) == BST_CHECKED ? 2
                                                                           : 1;
            EndDialog(dlg, IDOK);
            return TRUE;
        case IDCANCEL:
            EndDialog(dlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}
}

bool Spider::ShowOptions(HWND owner) {
    int suits = suits_;
    if (DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_SPIDER_OPTIONS), owner, OptionsProc,
                        reinterpret_cast<LPARAM>(&suits)) != IDOK)
        return false;
    settings_.SetInt(kSection, L"Suits", suits);
    return suits != suits_;   // takes effect on the next deal
}

std::wstring Spider::Rules() const {
    return L"Goal: build eight runs from King down to Ace, each in one suit. A finished run is removed to a foundation.\n\n"
           L"Layout: 104 cards (two decks) with 54 dealt into 10 columns, top card face up. The other 50 form the deck.\n\n"
           L"• Build down regardless of suit, but only a run of one suit can move as a unit.\n"
           L"• Any card or run can go into an empty column.\n"
           L"• Click the deck to deal one card onto every column. You can't deal while a column is empty.\n\n"
           L"Spider Options sets the difficulty: one suit, two suits or four.\n\n"
           L"Score: starts at 500, −1 per move or deal, +100 per run removed. An undo costs 1 more, and the move it takes back isn't refunded.";
}
