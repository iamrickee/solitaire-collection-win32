#pragma once
#include "../core/Game.h"

// Spider: two decks, 10 columns. Build down regardless of suit, move only
// same-suit runs, and a full K..A run of one suit is removed. Options: 1, 2 or
// 4 suits. Scoring as in Windows: start at 500, -1 per move, +100 per run.
class Spider : public Game {
public:
    explicit Spider(Settings& settings);

    std::wstring Name() const override { return L"Spider"; }
    std::wstring Label() const override;
    std::wstring ScoreText() const override;

    double LayoutWidth() const override { return 11.6; }
    double LayoutHeight() const override { return 4.4; }
    void Layout(const RECT& area, int cardW, int cardH, int unit) override;

    bool CanPickUp(int pile, int index) const override;
    bool CanDrop(int from, int index, int to) const override;
    bool IsWon() const override;
    bool Click(int pile, int index) override;
    int DoubleClickTarget(int pile, int index) const override { (void)pile; (void)index; return -1; }

    bool HasOptions() const override { return true; }
    bool ShowOptions(HWND owner) override;

protected:
    void Deal(uint32_t seed) override;
    void OnMoved(int from, int to, int count) override;

private:
    static constexpr int kStock = 0;
    static constexpr int kFoundation0 = 1;   // 8 slots for completed runs
    static constexpr int kFoundations = 8;
    static constexpr int kColumn0 = 9;       // 10 columns
    static constexpr int kColumns = 10;

    void LoadOptions();
    // Removes a completed K..A run from the top of a column, if there is one.
    void CollectRun(int column);
    void FlipTop(int column);
    int CompletedRuns() const;

    int suits_ = 1;
};
