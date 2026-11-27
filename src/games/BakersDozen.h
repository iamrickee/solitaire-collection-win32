#pragma once
#include "../core/Game.h"

// Baker's Dozen: 13 columns of 4 face-up cards, with Kings moved to the
// bottom of their columns. Build down regardless of suit, one card at a
// time; an empty column stays empty. No deck. Cards go to the foundations by
// themselves once no column could need them. Score = cards on the foundations.
class BakersDozen : public Game {
public:
    explicit BakersDozen(Settings& settings);

    std::wstring Name() const override { return L"Baker's Dozen"; }

    double LayoutWidth() const override { return 10.8; }
    double LayoutHeight() const override { return 4.4; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override;
    bool IsLost() const override;
    bool CanAutoComplete() const override { return true; }
    bool FindAutoMove(int& from, int& to) const override;
    std::wstring SlotHint(int pile) const override;

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;

private:
    static constexpr int kFoundation0 = 0;   // 4 foundations
    static constexpr int kColumn0 = 4;       // 13 columns: 7 in the top row, 6 below
    static constexpr int kColumns = 13;
    static constexpr int kTopRow = 7;

    bool SafeToFoundation(const Card& c) const;
};
