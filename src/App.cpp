#include "App.h"
#include "core/GameRegistry.h"
#include "../res/resource.h"
#include <commctrl.h>
#include <commdlg.h>
#include <windowsx.h>
#include <algorithm>
#include <cmath>
#include <random>
#include <string>

namespace {
constexpr wchar_t kClassName[] = L"SolitaireCollectionWnd";
constexpr wchar_t kTitle[] = L"Solitaire";
constexpr UINT_PTR kAnimTimer = 1;
constexpr int kSnapBackMs = 150;
constexpr int kAutoMoveMs = 110;

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

// Per-monitor DPI functions exist only on Windows 10 1607+. Look them up at
// runtime and fall back to the system DPI so the exe still runs on older Windows.
UINT WindowDpi(HWND hwnd) {
    using Fn = UINT(WINAPI*)(HWND);
    static const Fn fn = reinterpret_cast<Fn>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    if (fn) return fn(hwnd);
    HDC dc = GetDC(nullptr);
    const int dpi = GetDeviceCaps(dc, LOGPIXELSY);
    ReleaseDC(nullptr, dc);
    return dpi > 0 ? static_cast<UINT>(dpi) : 96;
}

BOOL AdjustWindowRectForDpi(RECT* r, DWORD style, BOOL menu, DWORD exStyle, UINT dpi) {
    using Fn = BOOL(WINAPI*)(RECT*, DWORD, BOOL, DWORD, UINT);
    static const Fn fn = reinterpret_cast<Fn>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "AdjustWindowRectExForDpi"));
    return fn ? fn(r, style, menu, exStyle, dpi) : AdjustWindowRectEx(r, style, menu, exStyle);
}

uint32_t RandomDeal() {
    static std::random_device rd;
    return rd() % 1000000 + 1;
}

int Area(const RECT& a, const RECT& b) {
    RECT r;
    if (!IntersectRect(&r, &a, &b)) return 0;
    return (r.right - r.left) * (r.bottom - r.top);
}

std::wstring FormatScore(const std::wstring& label, int score) {
    if (label.find(L"Vegas") != std::wstring::npos)
        return (score < 0 ? L"-$" : L"$") + std::to_wstring(std::abs(score));
    return std::to_wstring(score);
}
}

App::~App() {
    if (backDC_) {
        SelectObject(backDC_, backOld_);
        DeleteDC(backDC_);
    }
    if (backBmp_) DeleteObject(backBmp_);
}

bool App::Init(HINSTANCE inst, int showCmd, uint32_t firstDeal) {
    inst_ = inst;
    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);

    if (!cards_.Load(inst, IDB_CARDS)) {
        MessageBoxW(nullptr, L"Could not load the card images.", kTitle, MB_ICONERROR);
        return false;
    }

    WNDCLASSEXW wc{sizeof(wc)};
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = static_cast<HICON>(LoadImageW(inst, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON,
                                             GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0));
    wc.hIconSm = static_cast<HICON>(LoadImageW(inst, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON,
                                               GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
    wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MAINMENU);
    wc.lpszClassName = kClassName;
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(0, kClassName, kTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                            CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, inst, this);
    if (!hwnd_) return false;
    accel_ = LoadAcceleratorsW(inst, MAKEINTRESOURCEW(IDR_ACCEL));

    status_ = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                              0, 0, 0, 0, hwnd_, nullptr, inst, nullptr);

    // Games menu, built from the registry.
    HMENU games = GetSubMenu(GetMenu(hwnd_), 1);
    DeleteMenu(games, IDM_GAMES_PLACEHOLDER, MF_BYCOMMAND);
    const auto& all = AllGames();
    for (size_t i = 0; i < all.size(); ++i)
        AppendMenuW(games, MF_STRING, IDM_GAME_FIRST + i, all[i].name);

    // Restore the last game played.
    std::wstring last = settings_.GetString(L"App", L"Game", all[0].name);
    int index = 0;
    for (size_t i = 0; i < all.size(); ++i)
        if (last == all[i].name) index = static_cast<int>(i);
    SelectGame(index, firstDeal);
    FitWindow(true);

    ShowWindow(hwnd_, showCmd);
    UpdateWindow(hwnd_);
    return true;
}

// Size the window for the largest card scale (up to 3x per 96 DPI) at which
// the current game fits comfortably on the monitor. Games differ in shape, so
// this runs on every game switch too. On startup the window is centred on the
// monitor; afterwards it keeps its centre (pulled back on screen if needed).
void App::FitWindow(bool centerOnMonitor) {
    if (IsZoomed(hwnd_) || IsIconic(hwnd_)) return;   // leave a maximized window alone
    const UINT dpi = WindowDpi(hwnd_);
    MONITORINFO mi{sizeof(mi)};
    GetMonitorInfoW(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &mi);
    const RECT& work = mi.rcWork;
    const int workW = work.right - work.left, workH = work.bottom - work.top;
    RECT sb;
    GetWindowRect(status_, &sb);
    RECT r{};
    for (int scale = std::max(1, MulDiv(3, dpi, 96)); scale >= 1; --scale) {
        r = {0, 0, int(std::ceil(game_->LayoutWidth() * CardRenderer::kArtW * scale)),
             int(std::ceil(game_->LayoutHeight() * CardRenderer::kArtH * scale)) + (sb.bottom - sb.top)};
        AdjustWindowRectForDpi(&r, WS_OVERLAPPEDWINDOW, TRUE, 0, dpi);
        if (r.right - r.left <= workW * 9 / 10 && r.bottom - r.top <= workH * 9 / 10) break;
    }
    const int w = r.right - r.left, h = r.bottom - r.top;
    int cx = work.left + workW / 2, cy = work.top + workH / 2;
    if (!centerOnMonitor) {
        RECT cur;
        GetWindowRect(hwnd_, &cur);
        cx = (cur.left + cur.right) / 2;
        cy = (cur.top + cur.bottom) / 2;
    }
    const int x = std::clamp(cx - w / 2, int(work.left), std::max(int(work.left), int(work.right) - w));
    const int y = std::clamp(cy - h / 2, int(work.top), std::max(int(work.top), int(work.bottom) - h));
    SetWindowPos(hwnd_, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}

int App::Run() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (!TranslateAcceleratorW(hwnd_, accel_, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK App::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    App* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        app = static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
        app->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    return app ? app->Handle(msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT App::Handle(UINT msg, WPARAM wp, LPARAM lp) {
    const POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
    switch (msg) {
    case WM_SIZE:
        if (status_) SendMessageW(status_, WM_SIZE, 0, 0);
        UpdateLayout();
        UpdateStatus();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    case WM_GETMINMAXINFO: {
        auto* mmi = reinterpret_cast<MINMAXINFO*>(lp);
        mmi->ptMinTrackSize = {480, 360};
        return 0;
    }
    case WM_DPICHANGED: {
        const RECT* r = reinterpret_cast<const RECT*>(lp);
        SetWindowPos(hwnd_, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd_, &ps);
        Paint(dc);
        EndPaint(hwnd_, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:    OnLButtonDown(pt); return 0;
    case WM_MOUSEMOVE:      OnMouseMove(pt); return 0;
    case WM_LBUTTONUP:      OnLButtonUp(pt); return 0;
    case WM_LBUTTONDBLCLK:  OnLButtonDblClk(pt); return 0;
    case WM_RBUTTONDOWN:    OnCommand(IDM_AUTOPLAY); return 0;
    case WM_CAPTURECHANGED:
        if (reinterpret_cast<HWND>(lp) != hwnd_) CancelDrag();
        return 0;
    case WM_TIMER:
        if (wp == kAnimTimer) OnTimer();
        return 0;
    case WM_INITMENUPOPUP: {
        HMENU m = reinterpret_cast<HMENU>(wp);
        EnableMenuItem(m, IDM_UNDO, MF_BYCOMMAND | (game_->CanUndo() && !Busy() ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(m, IDM_OPTIONS, MF_BYCOMMAND | (game_->HasOptions() ? MF_ENABLED : MF_GRAYED));
        const int n = static_cast<int>(AllGames().size());
        CheckMenuRadioItem(m, IDM_GAME_FIRST, IDM_GAME_FIRST + n - 1, IDM_GAME_FIRST + gameIndex_, MF_BYCOMMAND);
        return 0;
    }
    case WM_COMMAND:
        OnCommand(LOWORD(wp));
        return 0;
    case WM_CLOSE:
        FinishCurrentGame();
        DestroyWindow(hwnd_);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd_, msg, wp, lp);
}

void App::OnCommand(int id) {
    if (id >= IDM_GAME_FIRST && id < IDM_GAME_FIRST + static_cast<int>(AllGames().size())) {
        if (Busy()) return;
        SelectGame(id - IDM_GAME_FIRST);
        FitWindow(false);
        return;
    }
    switch (id) {
    case IDM_NEW:
        if (!Busy()) NewDeal(0);
        break;
    case IDM_NEWDEAL:
        if (!Busy()) {
            const uint32_t deal = AskDealNumber();
            if (deal) NewDeal(deal);
        }
        break;
    case IDM_RESTART:
        if (!Busy()) NewDeal(game_->Seed());
        break;
    case IDM_UNDO:
        if (!Busy() && !lifted_.active && game_->Undo()) {
            sel_ = {};
            UpdateStatus();
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        break;
    case IDM_AUTOPLAY: {
        int from, to;
        if (!Busy() && !lifted_.active && game_->FindAutoMove(from, to)) StartAutoMove(from, to, true);
        break;
    }
    case IDM_OPTIONS:
        if (!Busy() && game_->HasOptions() && game_->ShowOptions(hwnd_)) NewDeal(0);
        UpdateStatus();
        break;
    case IDM_SETTINGS:
        ShowSettings();
        break;
    case IDM_HIGHSCORES:
        ShowHighScores();
        break;
    case IDM_RULES:
        ShowRules();
        break;
    case IDM_ABOUT:
        DialogBoxW(inst_, MAKEINTRESOURCEW(IDD_ABOUT), hwnd_,
                   [](HWND d, UINT m, WPARAM w, LPARAM) -> INT_PTR {
                       if (m == WM_COMMAND && (LOWORD(w) == IDOK || LOWORD(w) == IDCANCEL)) {
                           EndDialog(d, 0);
                           return TRUE;
                       }
                       return m == WM_INITDIALOG;
                   });
        break;
    case IDM_EXIT:
        SendMessageW(hwnd_, WM_CLOSE, 0, 0);
        break;
    }
}

// ---- Game management --------------------------------------------------------

void App::FinishCurrentGame() {
    if (game_) game_->End();
}

void App::SelectGame(int index, uint32_t deal) {
    FinishCurrentGame();
    lifted_ = {};
    press_ = {};
    sel_ = {};
    gameIndex_ = index;
    const GameInfo& info = AllGames()[index];
    game_ = info.create(settings_);
    settings_.SetString(L"App", L"Game", info.name);
    game_->NewGame(deal ? deal : RandomDeal());
    // The game-options item names the current game.
    const std::wstring options = game_->Name() + L" &Options...\tF4";
    ModifyMenuW(GetMenu(hwnd_), IDM_OPTIONS, MF_BYCOMMAND | MF_STRING, IDM_OPTIONS, options.c_str());
    UpdateLayout();
    UpdateStatus();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::NewDeal(uint32_t deal) {
    FinishCurrentGame();
    lifted_ = {};
    press_ = {};
    sel_ = {};
    game_->NewGame(deal ? deal : RandomDeal());
    UpdateLayout();
    UpdateStatus();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

// Call after any change to the game state.
void App::AfterChange() {
    sel_ = {};
    UpdateStatus();
    InvalidateRect(hwnd_, nullptr, FALSE);

    const bool won = game_->IsWon(), lost = !won && game_->IsLost();
    if ((won || lost) && !game_->Ended()) {
        const bool best = game_->End();
        UpdateStatus();
        UpdateWindow(hwnd_);
        std::wstring text = (won ? L"You won!\n\n" : L"Game over! No moves left.\n\n") + game_->ScoreText();
        if (best) text += L"\nNew high score!";
        text += L"\n\nDeal again?";
        if (MessageBoxW(hwnd_, text.c_str(), game_->Name().c_str(), MB_YESNO | MB_ICONINFORMATION) == IDYES)
            NewDeal(0);
        return;
    }

    int from, to;
    if (!Busy() && game_->CanAutoComplete() && game_->FindAutoMove(from, to)) StartAutoMove(from, to, true);
}

// ---- Layout & drawing -------------------------------------------------------

void App::UpdateLayout() {
    if (!game_) return;
    RECT client;
    GetClientRect(hwnd_, &client);
    int statusH = 0;
    if (status_) {
        RECT sb;
        GetWindowRect(status_, &sb);
        statusH = sb.bottom - sb.top;
    }
    board_ = client;
    board_.bottom = std::max(client.top, client.bottom - statusH);

    const int w = board_.right - board_.left, h = board_.bottom - board_.top;
    const int sx = int(w / (game_->LayoutWidth() * CardRenderer::kArtW) + 1e-6);
    const int sy = int(h / (game_->LayoutHeight() * CardRenderer::kArtH) + 1e-6);
    const int scale = std::clamp(std::min(sx, sy), 1, 8);

    cards_.Configure(scale, settings_.FourColor(), settings_.CardBack());
    game_->Layout(board_, cards_.CardW(), cards_.CardH(), scale);

    if (status_) {
        const UINT dpi = WindowDpi(hwnd_);
        // Game | score (stretches) | best | deal number
        const int best = std::max(MulDiv(520, dpi, 96), int(client.right) - MulDiv(270, dpi, 96));
        int parts[4] = {MulDiv(260, dpi, 96), best, best + MulDiv(130, dpi, 96), -1};
        SendMessageW(status_, SB_SETPARTS, 4, reinterpret_cast<LPARAM>(parts));
    }
}

void App::UpdateStatus() {
    if (!status_ || !game_) return;
    std::wstring deal = L"Deal #" + std::to_wstring(game_->Seed());
    const std::wstring label = game_->Label();   // high scores are per game and variant
    SendMessageW(status_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(label.c_str()));
    int bestScore;
    const std::wstring best = L"Best: " + (settings_.BestScore(label, bestScore) ? FormatScore(label, bestScore) : L"none");
    SendMessageW(status_, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(game_->ScoreText().c_str()));
    SendMessageW(status_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(best.c_str()));
    SendMessageW(status_, SB_SETTEXTW, 3, reinterpret_cast<LPARAM>(deal.c_str()));
}

void App::EnsureBackBuffer(int w, int h) {
    if (backDC_ && w <= backW_ && h <= backH_) return;
    if (backDC_) {
        SelectObject(backDC_, backOld_);
        DeleteDC(backDC_);
        DeleteObject(backBmp_);
    }
    backW_ = std::max(w, backW_);
    backH_ = std::max(h, backH_);
    HDC screen = GetDC(hwnd_);
    backDC_ = CreateCompatibleDC(screen);
    backBmp_ = CreateCompatibleBitmap(screen, backW_, backH_);
    ReleaseDC(hwnd_, screen);
    backOld_ = SelectObject(backDC_, backBmp_);
}

void App::Paint(HDC dc) {
    const int w = board_.right - board_.left, h = board_.bottom - board_.top;
    if (w <= 0 || h <= 0) return;
    EnsureBackBuffer(w, h);

    const COLORREF bg = settings_.Background();
    HBRUSH brush = CreateSolidBrush(bg);
    RECT all{0, 0, w, h};
    FillRect(backDC_, &all, brush);
    DeleteObject(brush);

    const auto& piles = game_->Piles();
    for (int p = 0; p < static_cast<int>(piles.size()); ++p) {
        const Pile& pile = piles[p];
        const int limit = (lifted_.active && lifted_.pile == p) ? lifted_.index : pile.Count();
        if (limit == 0) {
            if (game_->ShowSlot(p)) cards_.DrawSlot(backDC_, pile.SlotRect(), bg, game_->SlotHint(p));
            continue;
        }
        // Unfanned piles only show their top card.
        const int first = pile.fan == Fan::None ? limit - 1 : 0;
        for (int i = first; i < limit; ++i) {
            POINT pos = pile.CardPos(i);
            cards_.DrawCard(backDC_, pile.cards[i], pos.x, pos.y);
        }
    }

    if (sel_.Active() && sel_.index < piles[sel_.pile].Count()) {
        const Pile& pile = piles[sel_.pile];
        RECT r = pile.CardRect(sel_.index);
        RECT last = pile.CardRect(pile.Count() - 1);
        UnionRect(&r, &r, &last);
        const int s = cards_.Scale();
        HPEN pen = CreatePen(PS_SOLID, 2 * s, RGB(0xFF, 0xC8, 0x2E));
        HGDIOBJ oldPen = SelectObject(backDC_, pen);
        HGDIOBJ oldBrush = SelectObject(backDC_, GetStockObject(NULL_BRUSH));
        RoundRect(backDC_, r.left - s, r.top - s, r.right + s, r.bottom + s, 8 * s, 8 * s);
        SelectObject(backDC_, oldBrush);
        SelectObject(backDC_, oldPen);
        DeleteObject(pen);
    }

    if (lifted_.active) {
        const Pile& pile = piles[lifted_.pile];
        const POINT base = pile.CardPos(lifted_.index);
        for (int i = lifted_.index; i < pile.Count(); ++i) {
            POINT pos = pile.CardPos(i);
            cards_.DrawCard(backDC_, pile.cards[i], lifted_.pos.x + pos.x - base.x, lifted_.pos.y + pos.y - base.y);
        }
    }

    BitBlt(dc, board_.left, board_.top, w, h, backDC_, 0, 0, SRCCOPY);
}

// ---- Input ------------------------------------------------------------------

bool App::HitTest(POINT pt, int& pile, int& index) const {
    const auto& piles = game_->Piles();
    for (int p = static_cast<int>(piles.size()) - 1; p >= 0; --p) {
        if (piles[p].Empty() && !game_->ShowSlot(p)) continue;
        int i = piles[p].HitTest(pt);
        if (i != kHitNone) {
            pile = p;
            index = i;
            return true;
        }
    }
    return false;
}

void App::OnLButtonDown(POINT pt) {
    if (Busy() || game_->Ended()) return;
    int pile, index;
    if (!HitTest(pt, pile, index)) {
        ClearSelection();
        return;
    }
    press_.active = true;
    press_.pile = pile;
    press_.index = index;
    press_.start = pt;
    press_.canDrag = index >= 0 && game_->AllowDrag() && game_->CanPickUp(pile, index);
    SetCapture(hwnd_);
}

void App::OnMouseMove(POINT pt) {
    if (!press_.active) return;
    if (!lifted_.active) {
        if (!press_.canDrag) return;
        const int dx = std::abs(pt.x - press_.start.x), dy = std::abs(pt.y - press_.start.y);
        if (dx < GetSystemMetrics(SM_CXDRAG) && dy < GetSystemMetrics(SM_CYDRAG)) return;
        ClearSelection();
        const POINT origin = game_->Piles()[press_.pile].CardPos(press_.index);
        lifted_.active = true;
        lifted_.dragging = true;
        lifted_.pile = press_.pile;
        lifted_.index = press_.index;
        lifted_.grab = {press_.start.x - origin.x, press_.start.y - origin.y};
    }
    lifted_.pos = {pt.x - lifted_.grab.x, pt.y - lifted_.grab.y};
    InvalidateRect(hwnd_, nullptr, FALSE);
}

int App::FindDropTarget() const {
    const auto& piles = game_->Piles();
    const RECT lead{lifted_.pos.x, lifted_.pos.y, lifted_.pos.x + cards_.CardW(), lifted_.pos.y + cards_.CardH()};
    int best = -1, bestArea = 0;
    for (int p = 0; p < static_cast<int>(piles.size()); ++p) {
        if (p == lifted_.pile) continue;
        const Pile& pile = piles[p];
        const RECT r = pile.Empty() ? pile.SlotRect() : pile.CardRect(pile.Count() - 1);
        const int a = Area(lead, r);
        if (a > bestArea && game_->CanDrop(lifted_.pile, lifted_.index, p)) {
            best = p;
            bestArea = a;
        }
    }
    return best;
}

void App::OnLButtonUp(POINT pt) {
    if (!press_.active) return;
    press_.active = false;
    ReleaseCapture();

    if (lifted_.active && lifted_.dragging) {
        lifted_.dragging = false;
        const int target = FindDropTarget();
        if (target >= 0) {
            const int from = lifted_.pile, index = lifted_.index;
            lifted_ = {};
            game_->TryMove(from, index, target);
            AfterChange();
        } else {
            // Illegal drop: slide back home.
            StartSlide(game_->Piles()[lifted_.pile].CardPos(lifted_.index), -1, false, kSnapBackMs);
        }
        return;
    }

    // A plain click.
    int pile, index;
    if (HitTest(pt, pile, index) && pile == press_.pile) HandleClick(pile, index);
}

void App::ClearSelection() {
    if (!sel_.Active()) return;
    sel_ = {};
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::HandleClick(int pile, int index) {
    if (sel_.Active()) {
        const Selection s = sel_;
        ClearSelection();
        if (pile == s.pile && index == s.index) return;   // clicked again: deselect
        if (pile != s.pile && game_->CanDrop(s.pile, s.index, pile)) {
            StartMove(s.pile, s.index, pile, false);
            return;
        }
        // Not a legal destination: treat it as a fresh click below.
    }
    if (game_->Click(pile, index)) {
        AfterChange();
        return;
    }
    if (index >= 0 && game_->CanPickUp(pile, index)) {
        sel_.pile = pile;
        sel_.index = index;
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void App::OnLButtonDblClk(POINT pt) {
    if (Busy() || game_->Ended()) return;
    int pile, index;
    if (!HitTest(pt, pile, index)) return;
    // On the stock a double-click is just a second click.
    if (game_->Piles()[pile].kind == PileKind::Stock) {
        if (game_->Click(pile, index)) AfterChange();
        return;
    }
    const int target = index >= 0 ? game_->DoubleClickTarget(pile, index) : -1;
    if (target >= 0) StartAutoMove(pile, target, false);
    else HandleClick(pile, index);   // no double-click action: treat it as a second click
}

// Capture was taken away mid-press (e.g. alt-tab). Not called for our own
// ReleaseCapture in OnLButtonUp, because press_ is already inactive by then.
void App::CancelDrag() {
    if (!press_.active) return;
    press_.active = false;
    if (lifted_.active && lifted_.dragging) {
        lifted_.dragging = false;
        StartSlide(game_->Piles()[lifted_.pile].CardPos(lifted_.index), -1, false, kSnapBackMs);
    }
}

// ---- Animation --------------------------------------------------------------

void App::StartMove(int from, int index, int to, bool chain) {
    ClearSelection();
    const Pile& src = game_->Piles()[from];
    const Pile& dst = game_->Piles()[to];
    lifted_.active = true;
    lifted_.dragging = false;
    lifted_.pile = from;
    lifted_.index = index;
    lifted_.pos = src.CardPos(lifted_.index);
    StartSlide(dst.CardPos(dst.Count()), to, chain, kAutoMoveMs);
}

void App::StartSlide(POINT to, int target, bool chain, int ms) {
    slide_.active = true;
    slide_.from = lifted_.pos;
    slide_.to = to;
    slide_.start = GetTickCount();
    slide_.ms = ms;
    slide_.target = target;
    slide_.chain = chain;
    SetTimer(hwnd_, kAnimTimer, 10, nullptr);
}

void App::OnTimer() {
    if (!slide_.active) {
        KillTimer(hwnd_, kAnimTimer);
        return;
    }
    const double t = std::min(1.0, double(GetTickCount() - slide_.start) / slide_.ms);
    const double e = 1.0 - (1.0 - t) * (1.0 - t);   // ease-out
    lifted_.pos = {LONG(slide_.from.x + (slide_.to.x - slide_.from.x) * e),
                   LONG(slide_.from.y + (slide_.to.y - slide_.from.y) * e)};
    InvalidateRect(hwnd_, nullptr, FALSE);
    if (t < 1.0) return;

    KillTimer(hwnd_, kAnimTimer);
    slide_.active = false;
    const int from = lifted_.pile, index = lifted_.index, target = slide_.target;
    lifted_ = {};
    if (target < 0) return;   // snap-back finished

    game_->TryMove(from, index, target);
    const bool chain = slide_.chain;
    AfterChange();
    int f, t2;
    if (chain && !Busy() && !game_->Ended() && game_->FindAutoMove(f, t2)) StartAutoMove(f, t2, true);
}

// ---- Dialogs ----------------------------------------------------------------

namespace {
struct SettingsData {
    COLORREF bg, back;
    bool fourColor;
    HBRUSH bgBrush = nullptr, backBrush = nullptr;

    void Refresh(HWND dlg) {
        if (bgBrush) DeleteObject(bgBrush);
        if (backBrush) DeleteObject(backBrush);
        bgBrush = CreateSolidBrush(bg);
        backBrush = CreateSolidBrush(back);
        InvalidateRect(GetDlgItem(dlg, IDC_BG_SWATCH), nullptr, TRUE);
        InvalidateRect(GetDlgItem(dlg, IDC_BACK_SWATCH), nullptr, TRUE);
    }
};

bool PickColor(HWND owner, COLORREF& c) {
    static COLORREF custom[16] = {};
    CHOOSECOLORW cc{sizeof(cc)};
    cc.hwndOwner = owner;
    cc.rgbResult = c;
    cc.lpCustColors = custom;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (!ChooseColorW(&cc)) return false;
    c = cc.rgbResult;
    return true;
}

INT_PTR CALLBACK SettingsProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* d = reinterpret_cast<SettingsData*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG:
        d = reinterpret_cast<SettingsData*>(lp);
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        CheckDlgButton(dlg, IDC_FOURCOLOR, d->fourColor ? BST_CHECKED : BST_UNCHECKED);
        d->Refresh(dlg);
        return TRUE;
    case WM_CTLCOLORSTATIC: {
        const int id = GetDlgCtrlID(reinterpret_cast<HWND>(lp));
        if (id == IDC_BG_SWATCH) return reinterpret_cast<INT_PTR>(d->bgBrush);
        if (id == IDC_BACK_SWATCH) return reinterpret_cast<INT_PTR>(d->backBrush);
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_BG_BUTTON:
        case IDC_BG_SWATCH:
            if (PickColor(dlg, d->bg)) d->Refresh(dlg);
            return TRUE;
        case IDC_BACK_BUTTON:
        case IDC_BACK_SWATCH:
            if (PickColor(dlg, d->back)) d->Refresh(dlg);
            return TRUE;
        case IDC_DEFAULTS:
            d->bg = Settings::kDefaultBackground;
            d->back = Settings::kDefaultCardBack;
            CheckDlgButton(dlg, IDC_FOURCOLOR, BST_UNCHECKED);
            d->Refresh(dlg);
            return TRUE;
        case IDOK:
            d->fourColor = IsDlgButtonChecked(dlg, IDC_FOURCOLOR) == BST_CHECKED;
            EndDialog(dlg, IDOK);
            return TRUE;
        case IDCANCEL:
            EndDialog(dlg, IDCANCEL);
            return TRUE;
        }
        break;
    case WM_DESTROY:
        if (d) {
            DeleteObject(d->bgBrush);
            DeleteObject(d->backBrush);
        }
        break;
    }
    return FALSE;
}

INT_PTR CALLBACK HighScoresProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* settings = reinterpret_cast<Settings*>(GetWindowLongPtrW(dlg, DWLP_USER));
    auto fill = [](HWND dlg, Settings* s) {
        HWND list = GetDlgItem(dlg, IDC_SCORELIST);
        SendMessageW(list, LB_RESETCONTENT, 0, 0);
        auto scores = s->HighScores();
        if (scores.empty()) {
            SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"No high scores yet."));
            return;
        }
        std::sort(scores.begin(), scores.end());
        for (const auto& [label, score] : scores) {
            std::wstring line = label + L"\t" + FormatScore(label, score);
            SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
        }
    };
    switch (msg) {
    case WM_INITDIALOG: {
        settings = reinterpret_cast<Settings*>(lp);
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        int tab = 150;
        SendDlgItemMessageW(dlg, IDC_SCORELIST, LB_SETTABSTOPS, 1, reinterpret_cast<LPARAM>(&tab));
        fill(dlg, settings);
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_SCORERESET:
            if (MessageBoxW(dlg, L"Clear all high scores?", L"High Scores", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                settings->ClearHighScores();
                fill(dlg, settings);
            }
            return TRUE;
        case IDOK:
        case IDCANCEL:
            EndDialog(dlg, 0);
            return TRUE;
        }
        break;
    }
    return FALSE;
}
}

void App::ShowSettings() {
    SettingsData d{settings_.Background(), settings_.CardBack(), settings_.FourColor()};
    if (DialogBoxParamW(inst_, MAKEINTRESOURCEW(IDD_SETTINGS), hwnd_, SettingsProc, reinterpret_cast<LPARAM>(&d)) != IDOK)
        return;
    settings_.SetBackground(d.bg);
    settings_.SetCardBack(d.back);
    settings_.SetFourColor(d.fourColor);
    UpdateLayout();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::ShowHighScores() {
    DialogBoxParamW(inst_, MAKEINTRESOURCEW(IDD_HIGHSCORES), hwnd_, HighScoresProc, reinterpret_cast<LPARAM>(&settings_));
    UpdateStatus();   // Reset may have cleared the best score shown
}

namespace {
INT_PTR CALLBACK DealProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_INITDIALOG: {
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        const HWND edit = GetDlgItem(dlg, IDC_DEAL_NUMBER);
        SendMessageW(edit, EM_LIMITTEXT, 10, 0);   // up to 4294967295
        SetWindowTextW(edit, std::to_wstring(*reinterpret_cast<uint32_t*>(lp)).c_str());
        SendMessageW(edit, EM_SETSEL, 0, -1);
        SetFocus(edit);
        return FALSE;   // focus set by hand
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDOK: {
            wchar_t text[16] = {};
            GetDlgItemTextW(dlg, IDC_DEAL_NUMBER, text, 16);
            const unsigned long long n = wcstoull(text, nullptr, 10);
            if (n < 1 || n > 0xFFFFFFFFull) {
                MessageBoxW(dlg, L"Enter a deal number from 1 to 4294967295.", L"New Game from Deal #", MB_OK | MB_ICONWARNING);
                SetFocus(GetDlgItem(dlg, IDC_DEAL_NUMBER));
                SendDlgItemMessageW(dlg, IDC_DEAL_NUMBER, EM_SETSEL, 0, -1);
                return TRUE;
            }
            *reinterpret_cast<uint32_t*>(GetWindowLongPtrW(dlg, DWLP_USER)) = static_cast<uint32_t>(n);
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        case IDCANCEL:
            EndDialog(dlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}
}

namespace {
struct RulesData {
    Settings* settings;
    int index;   // game shown first
};

void ShowRulesFor(HWND dlg, Settings& settings, int index) {
    // A throwaway instance (never dealt) just to ask for its rules.
    const std::wstring rules = AllGames()[index].create(settings)->Rules();
    std::wstring text;   // the edit control wants \r\n line breaks
    for (wchar_t c : rules) {
        if (c == L'\n') text += L'\r';
        text += c;
    }
    SetDlgItemTextW(dlg, IDC_RULES_TEXT, text.c_str());
}

INT_PTR CALLBACK RulesProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_INITDIALOG: {
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        const auto* d = reinterpret_cast<RulesData*>(lp);
        const HWND list = GetDlgItem(dlg, IDC_RULES_GAMES);
        for (const GameInfo& g : AllGames()) SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(g.name));
        SendMessageW(list, LB_SETCURSEL, d->index, 0);
        ShowRulesFor(dlg, *d->settings, d->index);
        SetFocus(list);
        return FALSE;   // focus set by hand
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_RULES_GAMES:
            if (HIWORD(wp) == LBN_SELCHANGE) {
                const auto* d = reinterpret_cast<RulesData*>(GetWindowLongPtrW(dlg, DWLP_USER));
                const int sel = static_cast<int>(SendDlgItemMessageW(dlg, IDC_RULES_GAMES, LB_GETCURSEL, 0, 0));
                if (sel >= 0) ShowRulesFor(dlg, *d->settings, sel);
            }
            return TRUE;
        case IDOK:
        case IDCANCEL:
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        break;
    }
    return FALSE;
}
}

void App::ShowRules() {
    RulesData d{&settings_, gameIndex_};
    DialogBoxParamW(inst_, MAKEINTRESOURCEW(IDD_RULES), hwnd_, RulesProc, reinterpret_cast<LPARAM>(&d));
}

uint32_t App::AskDealNumber() {
    uint32_t deal = game_->Seed();
    if (DialogBoxParamW(inst_, MAKEINTRESOURCEW(IDD_DEAL), hwnd_, DealProc, reinterpret_cast<LPARAM>(&deal)) != IDOK)
        return 0;
    return deal;
}
