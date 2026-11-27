#pragma once
#include "../core/Game.h"

// La Belle Lucie: 17 fans of three face-up cards and one single card.
// Build down in suit, one card at a time; an empty fan stays empty. When
// stuck, click the redeal button: the fans are gathered, shuffled and dealt
// again in threes (two redeals). Every card that can go to a foundation goes
// by itself (building in suit, nothing could need it). Score = cards on the
// foundations.
class LaBelleLucie : public Game {
public:
    explicit LaBelleLucie(Settings& settings);

    std::wstring Name() const override { return L"La Belle Lucie"; }
    std::wstring ScoreText() const override;

    double LayoutWidth() const override { return 11.0; }
    double LayoutHeight() const override { return 4.4; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override;
    bool IsLost() const override;
    bool Click(int pile, int index) override;
    bool CanAutoComplete() const override { return true; }
    std::wstring SlotHint(int pile) const override;
    bool ShowSlot(int pile) const override { return pile < kFan0; }   // empty fans just vanish

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;
    std::vector<int> SaveExtra() const override { return {redeals_}; }
    void LoadExtra(const std::vector<int>& e) override { if (!e.empty()) redeals_ = e[0]; }

private:
    static constexpr int kRedeal = 0;        // always-empty button for the redeal
    static constexpr int kFoundation0 = 1;   // 4 foundations
    static constexpr int kFan0 = 5;          // 18 fans
    static constexpr int kFans = 18;
    static constexpr int kMaxRedeals = 2;

    // Deals `cards` out in threes from the first fan.
    void DealFans(std::vector<Card>& cards);

    int redeals_ = 0;
};
