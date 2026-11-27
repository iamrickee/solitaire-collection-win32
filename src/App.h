#pragma once
#include "core/CardRenderer.h"
#include "core/Game.h"
#include "core/Settings.h"
#include <windows.h>
#include <memory>

// The main window: owns the current game and handles input, dragging,
// slide animations, painting, menus and dialogs for every game.
class App {
public:
    ~App();
    // firstDeal = 0 picks a random deal.
    bool Init(HINSTANCE inst, int showCmd, uint32_t firstDeal = 0);
    int Run();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT Handle(UINT msg, WPARAM wp, LPARAM lp);
    void OnCommand(int id);

    // Game management
    void SelectGame(int index, uint32_t deal = 0);
    void FitWindow(bool centerOnMonitor);
    void NewDeal(bool sameSeed);
    void AfterChange();
    void FinishCurrentGame();

    // Layout & drawing
    void UpdateLayout();
    void UpdateStatus();
    void Paint(HDC dc);
    void EnsureBackBuffer(int w, int h);

    // Input
    bool HitTest(POINT pt, int& pile, int& index) const;
    void OnLButtonDown(POINT pt);
    void OnMouseMove(POINT pt);
    void OnLButtonUp(POINT pt);
    void OnLButtonDblClk(POINT pt);
    void CancelDrag();
    int FindDropTarget() const;
    void HandleClick(int pile, int index);
    void ClearSelection();

    // Animation
    void StartAutoMove(int from, int to, bool chain) { StartMove(from, game_->Piles()[from].Count() - 1, to, chain); }
    void StartMove(int from, int index, int to, bool chain);
    void StartSlide(POINT to, int target, bool chain, int ms);
    void OnTimer();
    bool Busy() const { return slide_.active; }

    // Dialogs
    void ShowSettings();
    void ShowHighScores();

    HINSTANCE inst_ = nullptr;
    HWND hwnd_ = nullptr;
    HWND status_ = nullptr;
    HACCEL accel_ = nullptr;
    Settings settings_;
    CardRenderer cards_;
    std::unique_ptr<Game> game_;
    int gameIndex_ = 0;
    RECT board_{};

    // Back buffer for flicker-free painting.
    HDC backDC_ = nullptr;
    HBITMAP backBmp_ = nullptr;
    HGDIOBJ backOld_ = nullptr;
    int backW_ = 0, backH_ = 0;

    // Mouse press (may become a click or a drag).
    struct Press {
        bool active = false;
        int pile = -1, index = 0;
        POINT start{};
        bool canDrag = false;
    } press_;

    // Cards lifted off a pile (being dragged or sliding). They stay in the
    // pile logically; painting skips them and draws them at `pos` instead.
    struct Lifted {
        bool active = false;
        int pile = -1, index = 0;
        POINT pos{};    // top-left of the lead card
        POINT grab{};   // mouse offset within the lead card while dragging
        bool dragging = false;
    } lifted_;

    // Click-to-move: cards picked by a click, waiting for a destination click.
    struct Selection {
        int pile = -1, index = 0;
        bool Active() const { return pile >= 0; }
    } sel_;

    struct Slide {
        bool active = false;
        POINT from{}, to{};
        DWORD start = 0;
        int ms = 0;
        int target = -1;     // pile to move to when done, or -1 to just drop back
        bool chain = false;  // keep auto-playing to foundations afterwards
    } slide_;
};
