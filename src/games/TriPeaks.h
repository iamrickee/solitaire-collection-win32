#pragma once
#include "../core/Game.h"

// TriPeaks: 28 cards in three overlapping peaks, the bottom row face up.
// Play any uncovered card one rank above or below the face-up card (King and
// Ace wrap); a card turns over once both cards covering it are gone. When
// stuck, turn the next card from the deck (one pass). Win by clearing all
// three peaks. Each card in a run scores one more than the last; clearing a
// peak adds a bonus.
class TriPeaks : public Game {
public:
    explicit TriPeaks(Settings& settings);

    std::wstring Name() const override { return L"TriPeaks"; }
    std::wstring Rules() const override;
    std::wstring ScoreText() const override;

    double LayoutWidth() const override { return 11.6; }
    double LayoutHeight() const override { return 3.9; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;
    bool AllowDrag() const override { return false; }

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override;
    bool IsLost() const override;
    bool Click(int pile, int index) override;
    int DoubleClickTarget(int pile, int index) const override { (void)pile; (void)index; return -1; }
    bool ShowSlot(int pile) const override { return pile < kCell0; }

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;
    std::vector<int> SaveExtra() const override { return {chain_}; }
    void LoadExtra(const std::vector<int>& e) override { if (!e.empty()) chain_ = e[0]; }

private:
    static constexpr int kStock = 0;
    static constexpr int kWaste = 1;
    static constexpr int kCell0 = 2;   // 28 cells: 3 peaks, 6, 9, then the 10-card bottom row
    static constexpr int kCells = 28;

    // Row (0 = peaks) and horizontal slot (in half-card steps) of each cell.
    static int Row(int cell);
    static int Slot(int cell);
    bool Uncovered(int pile) const;
    bool Playable(const Card& c) const;

    int chain_ = 0;   // cards played since the last draw
};
