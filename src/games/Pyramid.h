#pragma once
#include "../core/Game.h"

// Pyramid: 28 cards in a 7-row pyramid, each row overlapping the one above.
// Remove pairs of uncovered cards that add up to 13 (J = 11, Q = 12; a King
// goes on its own). The top waste card pairs too. The deck turns one card at
// a time and can be gone through three times. Win by clearing the pyramid.
// Score = cards removed.
class Pyramid : public Game {
public:
    explicit Pyramid(Settings& settings);

    std::wstring Name() const override { return L"Pyramid"; }
    std::wstring Rules() const override;
    std::wstring ScoreText() const override;

    double LayoutWidth() const override { return 8.2; }
    double LayoutHeight() const override { return 4.3; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override;
    bool IsLost() const override;
    bool Click(int pile, int index) override;
    int DoubleClickTarget(int pile, int index) const override { (void)pile; (void)index; return -1; }
    std::wstring SlotHint(int pile) const override;
    bool ShowSlot(int pile) const override { return pile < kCell0; }
    bool AllowDrag() const override { return false; }

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;
    std::vector<int> SaveExtra() const override { return {recycles_}; }
    void LoadExtra(const std::vector<int>& e) override { if (!e.empty()) recycles_ = e[0]; }

private:
    static constexpr int kStock = 0;
    static constexpr int kWaste = 1;
    static constexpr int kDiscard = 2;
    static constexpr int kCell0 = 3;      // 28 pyramid spots, row by row from the top
    static constexpr int kRows = 7;
    static constexpr int kCells = kRows * (kRows + 1) / 2;
    static constexpr int kRecycles = 2;   // three passes through the deck

    static int CellIndex(int row, int col) { return kCell0 + row * (row + 1) / 2 + col; }
    // A pyramid card is free once both cards covering it are gone.
    bool Uncovered(int pile) const;
    bool Available(int pile) const;
    bool CanRecycle() const { return recycles_ < kRecycles && !piles_[kWaste].Empty(); }

    int recycles_ = 0;
};
