#pragma once
#include "../core/Game.h"

// Classic Klondike. Options: draw 1/3, Standard or Vegas scoring, Vegas cumulative.
class Klondike : public Game {
public:
    explicit Klondike(Settings& settings);

    std::wstring Name() const override { return L"Klondike"; }
    std::wstring Label() const override;
    std::wstring ScoreText() const override;

    double LayoutWidth() const override { return 8.2; }
    double LayoutHeight() const override { return 3.8; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override;
    bool Click(int pile, int index) override;
    bool CanAutoComplete() const override;
    std::wstring SlotHint(int pile) const override;

    bool HasOptions() const override { return true; }
    bool ShowOptions(HWND owner) override;

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;
    void OnEnded() override;
    std::vector<int> SaveExtra() const override { return {recycles_}; }
    void LoadExtra(const std::vector<int>& e) override { recycles_ = e.empty() ? 0 : e[0]; }

private:
    enum class Scoring { Standard = 0, Vegas = 1 };

    // Pile indices.
    static constexpr int kStock = 0;
    static constexpr int kWaste = 1;
    static constexpr int kFoundation0 = 2;   // 4 foundations
    static constexpr int kTableau0 = 6;      // 7 tableau columns

    bool CanRecycle() const;
    void AddScore(int delta);
    void LoadOptions();

    int draw_ = 1;
    Scoring scoring_ = Scoring::Standard;
    bool cumulative_ = false;
    int recycles_ = 0;
};
