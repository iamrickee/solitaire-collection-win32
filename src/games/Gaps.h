#pragma once
#include "../core/Game.h"

// Gaps (also called Montana): all 52 cards in 4 rows of 13, then the Aces
// are taken out, leaving four gaps. Fill a gap with the card one rank above
// the card to its left, in the same suit; a gap at the start of a row takes
// any 2, and a gap after a King stays dead. Click a card to move it into its
// gap. Win by making every row run 2 to King in one suit. When stuck, the
// redeal button gathers every card not yet in sequence, shuffles, and deals
// them again with a gap right after each row's sequence (two redeals).
// Score = cards in sequence.
class Gaps : public Game {
public:
    explicit Gaps(Settings& settings);

    std::wstring Name() const override { return L"Gaps"; }
    std::wstring Rules() const override;
    std::wstring ScoreText() const override;

    double LayoutWidth() const override { return 11.2; }
    double LayoutHeight() const override { return 4.4; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;
    bool AllowDrag() const override { return false; }

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override;
    bool IsLost() const override;
    bool Click(int pile, int index) override;
    int DoubleClickTarget(int pile, int index) const override { (void)pile; (void)index; return -1; }
    std::wstring SlotHint(int pile) const override;

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;
    std::vector<int> SaveExtra() const override { return {redeals_}; }
    void LoadExtra(const std::vector<int>& e) override { if (!e.empty()) redeals_ = e[0]; }

private:
    static constexpr int kRedeal = 0;   // always-empty button for the redeal
    static constexpr int kCell0 = 1;    // 52 cells, row by row
    static constexpr int kRows = 4;
    static constexpr int kCols = 13;
    static constexpr int kMaxRedeals = 2;

    static int CellPile(int row, int col) { return kCell0 + row * kCols + col; }
    // Whether the card in pile `from` may fill the gap in pile `to`.
    bool Fits(int from, int to) const;
    // Gaps a card could move to.
    std::vector<int> TargetsFor(int from) const;
    // Leading cards of a row already in order 2, 3, 4... of one suit.
    int SequenceLength(int row) const;
    void UpdateScore();

    int redeals_ = 0;
};
