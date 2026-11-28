#pragma once
#include "../core/Game.h"

// Beleaguered Castle: the Aces start on the foundations in the middle; the
// other 48 cards are dealt face up into 8 rows of 6, four on each side.
// Build down regardless of suit, one card at a time; any card can go into an
// empty row. No deck. Cards go to the foundations by themselves once no row
// could need them. Score = cards moved to the foundations.
class BeleagueredCastle : public Game {
public:
    explicit BeleagueredCastle(Settings& settings);

    std::wstring Name() const override { return L"Beleaguered Castle"; }
    std::wstring Rules() const override;

    double LayoutWidth() const override { return 9.7; }
    double LayoutHeight() const override { return 4.4; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override;
    bool IsLost() const override;
    bool CanAutoComplete() const override { return true; }
    bool FindAutoMove(int& from, int& to) const override;

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;

private:
    static constexpr int kFoundation0 = 0;   // 4 foundations, stacked in the middle
    static constexpr int kRow0 = 4;          // 8 rows: 0-3 on the left, 4-7 on the right
    static constexpr int kRows = 8;

    bool SafeToFoundation(const Card& c) const;
};
