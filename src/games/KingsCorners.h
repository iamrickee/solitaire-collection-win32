#pragma once
#include "../core/Game.h"

// Kings Corners: fill a 4x4 grid one card at a time. Kings belong in the
// corners, Queens in the middle of the top and bottom rows, Jacks in the
// middle of the side columns (placed face down); number cards go anywhere.
// Play alternates in stages: fill every empty cell, then remove number cards
// in pairs that add up to 10 (Ace = 1; a 10 goes on its own), then click the
// deck to fill again. Win by getting all twelve face cards home.
class KingsCorners : public Game {
public:
    explicit KingsCorners(Settings& settings);

    std::wstring Name() const override { return L"Kings Corners"; }
    std::wstring Rules() const override;
    std::wstring ScoreText() const override;

    double LayoutWidth() const override { return 7.6; }
    double LayoutHeight() const override { return 4.3; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override;
    bool IsLost() const override;
    bool Click(int pile, int index) override;
    int DoubleClickTarget(int pile, int index) const override;
    std::wstring SlotHint(int pile) const override;
    bool AllowDrag() const override { return false; }

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;

private:
    static constexpr int kStock = 0;     // face down
    static constexpr int kNext = 1;      // the card to place, face up
    static constexpr int kDiscard = 2;   // removed number cards
    static constexpr int kCell0 = 3;     // 16 grid cells, row by row
    static constexpr int kCells = 16;

    // Rank a grid cell is reserved for (kKing/kQueen/kJack), or 0 for the centre.
    static int HomeRank(int cell);
    static bool IsFace(const Card& c) { return c.rank >= kJack; }
    static bool Pairs(const Card& a, const Card& b);
    bool CanPlace(const Card& c, int pile) const;
    bool AnyRemoval() const;
    // Placing stage while a next card is showing; removal stage otherwise.
    bool Placing() const;
    bool GridFull() const;
    void TurnNext();
};
