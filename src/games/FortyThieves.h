#pragma once
#include "../core/Game.h"

// Forty Thieves: two decks, 10 columns of 4 face-up cards, 8 foundations.
// Build columns down in suit, moving one card at a time; any card can go
// into an empty column. The deck turns one card at a time onto the waste,
// with no redeal. Cards go to the foundations by themselves once no column
// could need them. Score = cards on the foundations.
class FortyThieves : public Game {
public:
    explicit FortyThieves(Settings& settings);

    std::wstring Name() const override { return L"Forty Thieves"; }
    std::wstring Rules() const override;

    double LayoutWidth() const override { return 11.6; }
    double LayoutHeight() const override { return 4.4; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override;
    bool IsLost() const override;
    bool Click(int pile, int index) override;
    bool CanAutoComplete() const override { return true; }
    bool FindAutoMove(int& from, int& to) const override;
    std::wstring SlotHint(int pile) const override;

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;

private:
    static constexpr int kStock = 0;
    static constexpr int kWaste = 1;
    static constexpr int kFoundation0 = 2;   // 8 foundations, two per suit
    static constexpr int kColumn0 = 10;      // 10 columns
    static constexpr int kColumns = 10;

    bool SafeToFoundation(const Card& c, int foundation) const;
};
