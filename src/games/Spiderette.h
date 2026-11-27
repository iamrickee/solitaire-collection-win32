#pragma once
#include "../core/Game.h"

// Spiderette: Spider with one deck in a Klondike layout. 7 columns of 1 to 7
// cards, top card face up. Build down regardless of suit, move only same-suit
// runs, any card goes into an empty column. Click the deck to deal a card
// onto every column (not while one is empty). A full King-to-Ace run of one
// suit is removed; remove all four to win. Scoring as in Spider: start at
// 500, -1 per move, +100 per run.
class Spiderette : public Game {
public:
    explicit Spiderette(Settings& settings);

    std::wstring Name() const override { return L"Spiderette"; }
    std::wstring ScoreText() const override;

    double LayoutWidth() const override { return 8.2; }
    double LayoutHeight() const override { return 4.4; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override;
    bool Click(int pile, int index) override;
    int DoubleClickTarget(int pile, int index) const override { (void)pile; (void)index; return -1; }

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;

private:
    static constexpr int kStock = 0;
    static constexpr int kFoundation0 = 1;   // 4 slots for completed runs
    static constexpr int kColumn0 = 5;       // 7 columns
    static constexpr int kColumns = 7;

    // Removes a completed K..A run from the top of a column, if there is one.
    void CollectRun(int column);
    void FlipTop(int column);
};
