#pragma once
#include "Pile.h"
#include "Settings.h"
#include <cstdint>
#include <string>
#include <vector>

// Base class for every solitaire game. A game owns its piles and decides the
// rules; the app handles input, drawing, dragging and animation generically.
class Game {
public:
    explicit Game(Settings& settings) : settings_(settings) {}
    virtual ~Game() = default;

    // ---- Identity ---------------------------------------------------------
    virtual std::wstring Name() const = 0;
    // Name + variant, e.g. "Klondike (Draw 3, Vegas)". Used as the high-score key.
    virtual std::wstring Label() const { return Name(); }
    // Status-bar score text.
    virtual std::wstring ScoreText() const { return L"Score: " + std::to_wstring(score_); }

    // ---- Lifecycle --------------------------------------------------------
    void NewGame(uint32_t seed);
    uint32_t Seed() const { return seed_; }
    bool Started() const { return started_; }
    bool Ended() const { return ended_; }
    // Finish the current game (win, abandon, or app exit). Records the high score.
    // Returns true if it was a new best. Safe to call more than once.
    bool End();

    // ---- Layout -----------------------------------------------------------
    // Space the game needs, in card widths/heights, to pick a card scale.
    virtual double LayoutWidth() const = 0;
    virtual double LayoutHeight() const = 0;
    // Position piles inside `area` for the given card size. `unit` = 1 art pixel.
    virtual void Layout(const RECT& area, int cardW, int cardH, int unit) = 0;

    // ---- Rules ------------------------------------------------------------
    virtual bool CanPickUp(int pile, int index) const = 0;
    virtual bool CanDrop(int from, int index, int to) const = 0;
    virtual bool IsWon() const = 0;

    // Single click that isn't a drag (e.g. the stock). index may be kHitSlot.
    // Returns true if the game state changed.
    virtual bool Click(int pile, int index) { (void)pile; (void)index; return false; }
    // Where a double-clicked card should go, or -1. Default: any foundation.
    virtual int DoubleClickTarget(int pile, int index) const;
    // Next card that can go to a foundation (right-click autoplay / auto-finish).
    virtual bool FindAutoMove(int& from, int& to) const;
    // True once the rest of the game can be finished automatically.
    virtual bool CanAutoComplete() const { return false; }
    // Optional glyph drawn on an empty pile's slot (e.g. recycle mark on the stock).
    virtual std::wstring SlotHint(int pile) const { (void)pile; return {}; }

    // ---- Options ----------------------------------------------------------
    virtual bool HasOptions() const { return false; }
    // Shows the game's options dialog. Returns true if a new deal is required.
    virtual bool ShowOptions(HWND owner) { (void)owner; return false; }

    // ---- Moves & undo -----------------------------------------------------
    // Validates with CanDrop, records undo, moves, then calls OnMoved.
    bool TryMove(int from, int index, int to);
    bool CanUndo() const { return !undo_.empty() && !ended_; }
    bool Undo();

    int Score() const { return score_; }
    std::vector<Pile>& Piles() { return piles_; }
    const std::vector<Pile>& Piles() const { return piles_; }

protected:
    virtual void Deal(uint32_t seed) = 0;
    // Called after a successful TryMove (scoring, flipping exposed cards, ...).
    virtual void OnMoved(int from, int to, int count) { (void)from; (void)to; (void)count; }
    // Called once from End(), before the score is recorded.
    virtual void OnEnded() {}
    // Extra per-game state that undo must restore (e.g. stock passes).
    virtual std::vector<int> SaveExtra() const { return {}; }
    virtual void LoadExtra(const std::vector<int>& extra) { (void)extra; }

    // Snapshot state before a change. Also marks the game as started.
    void PushUndo();
    void MoveCards(int from, int index, int to);
    bool IsKind(int pile, PileKind kind) const { return piles_[pile].kind == kind; }

    Settings& settings_;
    std::vector<Pile> piles_;
    int score_ = 0;

private:
    struct Snapshot {
        std::vector<std::vector<Card>> cards;
        int score;
        std::vector<int> extra;
    };
    std::vector<Snapshot> undo_;
    uint32_t seed_ = 0;
    bool started_ = false;
    bool ended_ = false;
};
