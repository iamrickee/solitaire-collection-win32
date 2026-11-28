#include "Klondike.h"
#include "../core/Deck.h"
#include "../../res/resource.h"
#include <algorithm>

namespace {
constexpr wchar_t kSection[] = L"Klondike";

// Standard (Windows-style) scoring.
constexpr int kStdWasteToTableau = 5;
constexpr int kStdToFoundation = 10;
constexpr int kStdTurnOver = 5;
constexpr int kStdFoundationToTableau = -15;
constexpr int kStdRecycleDraw1 = -100;
constexpr int kStdRecycleDraw3 = -20;

// Vegas scoring: buy in for $52, win $5 per card on the foundations.
constexpr int kVegasAnte = -52;
constexpr int kVegasPerCard = 5;
}

Klondike::Klondike(Settings& settings) : Game(settings) {
    piles_.resize(13);
    piles_[kStock].kind = PileKind::Stock;
    piles_[kWaste].kind = PileKind::Waste;
    for (int i = 0; i < 4; ++i) piles_[kFoundation0 + i].kind = PileKind::Foundation;
    for (int i = 0; i < 7; ++i) {
        piles_[kTableau0 + i].kind = PileKind::Tableau;
        piles_[kTableau0 + i].fan = Fan::Down;
    }
    LoadOptions();
}

void Klondike::LoadOptions() {
    draw_ = settings_.GetInt(kSection, L"Draw", 1) == 3 ? 3 : 1;
    scoring_ = settings_.GetInt(kSection, L"Scoring", 0) == 1 ? Scoring::Vegas : Scoring::Standard;
    cumulative_ = settings_.GetInt(kSection, L"Cumulative", 0) != 0;
    piles_[kWaste].fan = draw_ == 3 ? Fan::Right : Fan::None;
    piles_[kWaste].maxFanned = draw_ == 3 ? 3 : 0;
}

std::wstring Klondike::Label() const {
    std::wstring s = L"Klondike (Draw " + std::to_wstring(draw_);
    s += scoring_ == Scoring::Vegas ? L", Vegas)" : L", Standard)";
    return s;
}

std::wstring Klondike::ScoreText() const {
    if (scoring_ == Scoring::Standard) return L"Score: " + std::to_wstring(score_);
    auto money = [](int v) { return (v < 0 ? L"-$" : L"$") + std::to_wstring(std::abs(v)); };
    std::wstring s = L"Score: " + money(score_);
    if (cumulative_) {
        // The bank is committed when the game ends; until then show it live.
        int bank = settings_.GetInt(kSection, L"VegasBank", 0) + (Ended() ? 0 : score_);
        s += L"   Bank: " + money(bank);
    }
    return s;
}

void Klondike::Layout(const RECT& area, int cardW, int cardH, int unit) {
    const int gap = Art::kGap * unit;
    const int totalW = 7 * cardW + 6 * gap;
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

    piles_[kStock].x = colX(0);
    piles_[kStock].y = top;
    piles_[kWaste].x = colX(1);
    piles_[kWaste].y = top;
    piles_[kWaste].offFaceUp = Art::kFanRight * unit;
    for (int i = 0; i < 4; ++i) {
        piles_[kFoundation0 + i].x = colX(3 + i);
        piles_[kFoundation0 + i].y = top;
    }
    for (int i = 0; i < 7; ++i) {
        Pile& p = piles_[kTableau0 + i];
        p.x = colX(i);
        p.y = tabTop;
        p.maxExtent = std::max(cardH, int(area.bottom) - tabTop - gap);
    }
}

void Klondike::Deal(uint32_t seed) {
    LoadOptions();
    recycles_ = 0;
    Deck deck;
    deck.Shuffle(seed);
    for (int col = 0; col < 7; ++col)
        for (int row = 0; row <= col; ++row)
            piles_[kTableau0 + col].cards.push_back(deck.Draw(row == col));
    while (!deck.Empty()) piles_[kStock].cards.push_back(deck.Draw(false));
    score_ = scoring_ == Scoring::Vegas ? kVegasAnte : 0;
}

bool Klondike::CanPickUp(int pile, int index) const {
    const Pile& p = piles_[pile];
    if (index < 0 || index >= p.Count() || !p.cards[index].faceUp) return false;
    switch (p.kind) {
    case PileKind::Waste:
    case PileKind::Foundation:
        return index == p.Count() - 1;
    case PileKind::Tableau:
        for (int i = index; i + 1 < p.Count(); ++i) {
            const Card& a = p.cards[i];
            const Card& b = p.cards[i + 1];
            if (!b.faceUp || a.SameColor(b) || a.rank != b.rank + 1) return false;
        }
        return true;
    default:
        return false;
    }
}

bool Klondike::CanDrop(int from, int index, int to) const {
    if (!CanPickUp(from, index)) return false;
    const Card& moving = piles_[from].cards[index];
    const int count = piles_[from].Count() - index;
    const Pile& dst = piles_[to];

    switch (dst.kind) {
    case PileKind::Foundation:
        if (count != 1) return false;
        if (dst.Empty()) return moving.rank == kAce;
        return dst.Top().suit == moving.suit && moving.rank == dst.Top().rank + 1;
    case PileKind::Tableau:
        if (dst.Empty()) return moving.rank == kKing;
        return dst.Top().faceUp && !dst.Top().SameColor(moving) && moving.rank + 1 == dst.Top().rank;
    default:
        return false;
    }
}

void Klondike::AddScore(int delta) {
    score_ += delta;
    if (scoring_ == Scoring::Standard) score_ = std::max(0, score_);
}

void Klondike::OnMoved(int from, int to, int count) {
    (void)count;
    const PileKind fk = piles_[from].kind, tk = piles_[to].kind;
    if (scoring_ == Scoring::Standard) {
        if (tk == PileKind::Foundation && fk != PileKind::Foundation) AddScore(kStdToFoundation);
        else if (fk == PileKind::Waste && tk == PileKind::Tableau) AddScore(kStdWasteToTableau);
        else if (fk == PileKind::Foundation && tk == PileKind::Tableau) AddScore(kStdFoundationToTableau);
    } else {
        if (tk == PileKind::Foundation && fk != PileKind::Foundation) AddScore(kVegasPerCard);
        else if (fk == PileKind::Foundation && tk == PileKind::Tableau) AddScore(-kVegasPerCard);
    }

    // Turn over the card a tableau move exposed.
    Pile& src = piles_[from];
    if (fk == PileKind::Tableau && !src.Empty() && !src.Top().faceUp) {
        src.Top().faceUp = true;
        if (scoring_ == Scoring::Standard) AddScore(kStdTurnOver);
    }
}

bool Klondike::CanRecycle() const {
    if (scoring_ == Scoring::Standard) return true;
    // Vegas: draw 1 gets a single pass, draw 3 gets three passes.
    return recycles_ < (draw_ == 3 ? 2 : 0);
}

bool Klondike::Click(int pile, int index) {
    (void)index;
    if (pile != kStock || Ended()) return false;
    Pile& stock = piles_[kStock];
    Pile& waste = piles_[kWaste];

    if (!stock.Empty()) {
        PushUndo();
        for (int i = 0; i < draw_ && !stock.Empty(); ++i) {
            Card c = stock.cards.back();
            stock.cards.pop_back();
            c.faceUp = true;
            waste.cards.push_back(c);
        }
        return true;
    }
    if (waste.Empty() || !CanRecycle()) return false;

    PushUndo();
    while (!waste.Empty()) {
        Card c = waste.cards.back();
        waste.cards.pop_back();
        c.faceUp = false;
        stock.cards.push_back(c);
    }
    ++recycles_;
    if (scoring_ == Scoring::Standard) AddScore(draw_ == 3 ? kStdRecycleDraw3 : kStdRecycleDraw1);
    return true;
}

bool Klondike::IsWon() const {
    for (int i = 0; i < 4; ++i)
        if (piles_[kFoundation0 + i].Count() != 13) return false;
    return true;
}

bool Klondike::CanAutoComplete() const {
    if (!piles_[kStock].Empty() || !piles_[kWaste].Empty()) return false;
    for (int i = 0; i < 7; ++i)
        for (const Card& c : piles_[kTableau0 + i].cards)
            if (!c.faceUp) return false;
    return !IsWon();
}

std::wstring Klondike::SlotHint(int pile) const {
    if (pile == kStock && !Ended()) {
        if (piles_[kWaste].Empty()) return {};
        return CanRecycle() ? L"↻" : L"✕";   // ↻ or ✕
    }
    if (piles_[pile].kind == PileKind::Foundation) return L"A";
    return {};
}

void Klondike::OnEnded() {
    if (scoring_ == Scoring::Vegas && cumulative_)
        settings_.SetInt(kSection, L"VegasBank", settings_.GetInt(kSection, L"VegasBank", 0) + score_);
}

// ---- Options dialog --------------------------------------------------------

namespace {
struct OptionsData {
    int draw;
    int scoring;
    bool cumulative;
};

INT_PTR CALLBACK OptionsProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* data = reinterpret_cast<OptionsData*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG:
        data = reinterpret_cast<OptionsData*>(lp);
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        CheckRadioButton(dlg, IDC_KL_DRAW1, IDC_KL_DRAW3, data->draw == 3 ? IDC_KL_DRAW3 : IDC_KL_DRAW1);
        CheckRadioButton(dlg, IDC_KL_STANDARD, IDC_KL_VEGAS, data->scoring == 1 ? IDC_KL_VEGAS : IDC_KL_STANDARD);
        CheckDlgButton(dlg, IDC_KL_CUMULATIVE, data->cumulative ? BST_CHECKED : BST_UNCHECKED);
        EnableWindow(GetDlgItem(dlg, IDC_KL_CUMULATIVE), data->scoring == 1);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_KL_STANDARD:
        case IDC_KL_VEGAS:
            EnableWindow(GetDlgItem(dlg, IDC_KL_CUMULATIVE), IsDlgButtonChecked(dlg, IDC_KL_VEGAS) == BST_CHECKED);
            return TRUE;
        case IDOK:
            data->draw = IsDlgButtonChecked(dlg, IDC_KL_DRAW3) == BST_CHECKED ? 3 : 1;
            data->scoring = IsDlgButtonChecked(dlg, IDC_KL_VEGAS) == BST_CHECKED ? 1 : 0;
            data->cumulative = IsDlgButtonChecked(dlg, IDC_KL_CUMULATIVE) == BST_CHECKED;
            EndDialog(dlg, IDOK);
            return TRUE;
        case IDC_KL_RESETBANK:   // handled by the caller
            EndDialog(dlg, IDC_KL_RESETBANK);
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

bool Klondike::ShowOptions(HWND owner) {
    OptionsData data{draw_, static_cast<int>(scoring_), cumulative_};
    INT_PTR r = DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_KLONDIKE_OPTIONS), owner,
                                OptionsProc, reinterpret_cast<LPARAM>(&data));
    if (r == IDC_KL_RESETBANK) {
        if (MessageBoxW(owner, L"Reset the Vegas bank to $0?", L"Klondike", MB_YESNO | MB_ICONQUESTION) == IDYES)
            settings_.SetInt(kSection, L"VegasBank", 0);
        return false;
    }
    if (r != IDOK) return false;

    const bool changed = data.draw != draw_ || data.scoring != static_cast<int>(scoring_) ||
                         data.cumulative != cumulative_;
    settings_.SetInt(kSection, L"Draw", data.draw);
    settings_.SetInt(kSection, L"Scoring", data.scoring);
    settings_.SetInt(kSection, L"Cumulative", data.cumulative ? 1 : 0);
    // Options take effect on the next deal (Deal() reloads them).
    return changed;
}

std::wstring Klondike::Rules() const {
    return L"Goal: move all 52 cards to the four foundations, building each up in one suit from Ace to King.\n\n"
           L"Layout: 7 columns of 1 to 7 cards with the top card face up. The rest of the cards form the deck.\n\n"
           L"• Build columns down in alternating colors (a red 6 on a black 7). An ordered run moves as a unit.\n"
           L"• Only a King, or a run starting with one, goes into an empty column.\n"
           L"• A face-down card turns over when it is uncovered.\n"
           L"• Click the deck to turn 1 or 3 cards onto the waste; the top waste card can be played. Click the empty deck to turn the waste over again.\n\n"
           L"Standard scoring: +10 per card to a foundation, +5 waste to column, +5 for turning a card over, −15 for taking a card back off a foundation, and −100 per pass through the deck (−20 when drawing three). Unlimited passes.\n\n"
           L"Vegas scoring: you pay $52 to start and win $5 for every card on the foundations. One pass when drawing one, three when drawing three. The cumulative bank carries your total from game to game.\n\n"
           L"Draw count and scoring are in Klondike Options.";
}
