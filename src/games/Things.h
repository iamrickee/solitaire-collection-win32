#pragma once
#include "../core/Game.h"

// Things: a Golf-style endless game. Play column cards one rank above or below
// the face-up card (no wrap; jokers are wild). Clearing all 8 columns deals a
// fresh board and the score keeps climbing until no moves remain.
class Things : public Game {
public:
    explicit Things(Settings& settings);

    std::wstring Name() const override { return L"Things"; }
    std::wstring ScoreText() const override;

    double LayoutWidth() const override { return 9.4; }
    double LayoutHeight() const override { return 3.8; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;
    bool AllowDrag() const override { return false; }

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override { return false; }
    bool IsLost() const override;
    bool Click(int pile, int index) override;

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;
    std::vector<int> SaveExtra() const override { return {chain_, board_}; }
    void LoadExtra(const std::vector<int>& e) override;

private:
    static constexpr int kStock = 0;
    static constexpr int kWaste = 1;
    static constexpr int kColumn0 = 2;
    static constexpr int kColumns = 8;
    static constexpr int kDepth = 5;

    void DealBoard();
    bool Playable(const Card& c) const;
    bool BoardEmpty() const;

    uint32_t seed_ = 0;
    int board_ = 1;   // 1-based board number within this game
    int chain_ = 0;   // cards played since the last draw
};
