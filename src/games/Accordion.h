#pragma once
#include "../core/Game.h"

// Accordion: all 52 cards dealt face up in a line (shown wrapped into rows of
// 13). A pile may move onto the pile directly to its left, or three to its
// left, when their top cards match in suit or rank; the line then closes up.
// Win by gathering everything into one pile. Click a pile to move it; one
// with two possible moves is selected first. Score = cards stacked
// (52 minus the piles left).
class Accordion : public Game {
public:
    explicit Accordion(Settings& settings);

    std::wstring Name() const override { return L"Accordion"; }
    std::wstring Rules() const override;
    std::wstring ScoreText() const override;

    double LayoutWidth() const override { return 10.4; }
    double LayoutHeight() const override { return 4.4; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;
    bool AllowDrag() const override { return false; }
    bool ShowSlot(int pile) const override { (void)pile; return false; }   // the line just gets shorter

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override;
    bool IsLost() const override;
    bool Click(int pile, int index) override;
    int DoubleClickTarget(int pile, int index) const override { (void)pile; (void)index; return -1; }

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;

private:
    static constexpr int kCols = 13;   // piles per row on screen

    int PileCount() const;
    // Whether pile `from` may move onto pile `to` (1 or 3 to its left, matching).
    bool Fits(int from, int to) const;
    std::vector<int> TargetsFor(int from) const;
};
