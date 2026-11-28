#pragma once
#include "../core/Game.h"

// Golf: 7 columns of 5 face-up cards; the rest is the stock, turned one card
// at a time, one pass. Play a column's bottom card onto the face-up card when
// it's one rank above or below, any suit. No wrap between Ace and King, and
// nothing may be played on a King. Win by clearing the columns.
// Golf scores in strokes: cards left in the columns, or minus the stock cards
// left if you clear them (lower is better). The high score is 35 - strokes.
class Golf : public Game {
public:
    explicit Golf(Settings& settings);

    std::wstring Name() const override { return L"Golf"; }
    std::wstring Rules() const override;
    std::wstring ScoreText() const override;

    double LayoutWidth() const override { return 8.3; }
    double LayoutHeight() const override { return 3.8; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;
    bool AllowDrag() const override { return false; }

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override;
    bool IsLost() const override;
    bool Click(int pile, int index) override;

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;

private:
    static constexpr int kStock = 0;
    static constexpr int kWaste = 1;
    static constexpr int kColumn0 = 2;
    static constexpr int kColumns = 7;
    static constexpr int kDepth = 5;

    bool Playable(const Card& c) const;
    int Strokes() const;
    void UpdateScore();
};
