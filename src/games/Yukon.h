#pragma once
#include "../core/Game.h"

// Yukon: Klondike's columns and foundations with no deck. Every card is dealt:
// column N has N face-down cards topped by five face-up ones (the first column
// just one card). Any face-up card can move with everything on top of it,
// in order or not, onto a card one rank higher of the other colour; only a
// King goes into an empty column. Standard Klondike scoring.
class Yukon : public Game {
public:
    explicit Yukon(Settings& settings);

    std::wstring Name() const override { return L"Yukon"; }
    std::wstring Rules() const override;

    double LayoutWidth() const override { return 8.2; }
    double LayoutHeight() const override { return 4.4; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override;
    bool CanAutoComplete() const override;
    std::wstring SlotHint(int pile) const override;

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;
    // Whether `c` may go on `onto` in a column: the other colour here.
    virtual bool Builds(const Card& c, const Card& onto) const { return !onto.SameColor(c); }

private:
    static constexpr int kFoundation0 = 0;   // 4 foundations
    static constexpr int kColumn0 = 4;       // 7 columns
};
