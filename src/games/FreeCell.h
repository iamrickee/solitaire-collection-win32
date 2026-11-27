#pragma once
#include "../core/Game.h"

// FreeCell: all 52 cards face up in 8 columns, 4 free cells, 4 foundations.
// Build columns down in alternating colors. A run moves as a unit when there
// is room to do it one card at a time through free cells and empty columns.
// Deals use the Microsoft FreeCell numbering, so deal #N here matches #N
// there. Score = cards on the foundations.
class FreeCell : public Game {
public:
    explicit FreeCell(Settings& settings);

    std::wstring Name() const override { return L"FreeCell"; }

    double LayoutWidth() const override { return 9.4; }
    double LayoutHeight() const override { return 4.4; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override;
    bool IsLost() const override;
    int DoubleClickTarget(int pile, int index) const override;
    // Cards go to the foundations by themselves once no column could need them.
    bool CanAutoComplete() const override { return true; }
    bool FindAutoMove(int& from, int& to) const override;
    std::wstring SlotHint(int pile) const override;

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;

private:
    static constexpr int kCell0 = 0;         // 4 free cells
    static constexpr int kFoundation0 = 4;   // 4 foundations
    static constexpr int kColumn0 = 8;       // 8 columns

    bool IsRun(const Pile& p, int index) const;
    // Most cards movable at once with the free cells and empty columns available.
    int MaxMove(bool toEmptyColumn) const;
    int FoundationRank(Suit s) const;
    bool SafeToFoundation(const Card& c) const;
};
