#include "CardRenderer.h"
#include <algorithm>

namespace {
// Sheet layout (see tools/make_cardsheet.ps1).
constexpr int kBackCol = 13, kBackRow = 0;
constexpr int kEmptyCol = 13, kEmptyRow = 1;
constexpr int kJokerCol = 13, kRedJokerRow = 2, kBlackJokerRow = 3;

// Kenney palette, as 0x00RRGGBB.
constexpr uint32_t kKey = 0xFF00FF;        // transparent
constexpr uint32_t kInk = 0x4D5766;        // outline + black suits
constexpr uint32_t kRed = 0xF52C4E;
constexpr uint32_t kBackTint = 0xC7D7EC;   // light blue on the card back

// Four-color deck replacements.
constexpr uint32_t kClubGreen = 0x1E9E4A;
constexpr uint32_t kDiamondBlue = 0x2F6FDB;

uint32_t ToRGB(COLORREF c) { return (GetRValue(c) << 16) | (GetGValue(c) << 8) | GetBValue(c); }
COLORREF Darken(COLORREF c, double k) {
    return RGB(int(GetRValue(c) * k), int(GetGValue(c) * k), int(GetBValue(c) * k));
}
}

CardRenderer::~CardRenderer() {
    FreeScaled();
    if (hintFont_) DeleteObject(hintFont_);
}

bool CardRenderer::Load(HINSTANCE inst, int resourceId) {
    HBITMAP bmp = static_cast<HBITMAP>(LoadImageW(inst, MAKEINTRESOURCEW(resourceId), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (!bmp) return false;

    BITMAP bm{};
    GetObjectW(bmp, sizeof(bm), &bm);
    baseW_ = bm.bmWidth;
    baseH_ = bm.bmHeight;
    base_.assign(size_t(baseW_) * baseH_, 0);

    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = baseW_;
    bi.bmiHeader.biHeight = -baseH_;   // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC screen = GetDC(nullptr);
    int lines = GetDIBits(screen, bmp, 0, baseH_, base_.data(), &bi, DIB_RGB_COLORS);
    ReleaseDC(nullptr, screen);
    DeleteObject(bmp);
    for (auto& px : base_) px &= 0xFFFFFF;
    return lines == baseH_;
}

void CardRenderer::FreeScaled() {
    if (memDC_) {
        SelectObject(memDC_, oldBmp_);
        DeleteDC(memDC_);
        memDC_ = nullptr;
    }
    if (scaled_) {
        DeleteObject(scaled_);
        scaled_ = nullptr;
    }
}

void CardRenderer::Configure(int scale, bool fourColor, COLORREF backColor) {
    if (scale < 1) scale = 1;
    if (scale == scale_ && fourColor == fourColor_ && backColor == back_ && memDC_) return;
    scale_ = scale;
    fourColor_ = fourColor;
    back_ = backColor;

    // 1. Recolor a copy of the base sheet.
    std::vector<uint32_t> px = base_;
    auto at = [&](int col, int row, int x, int y) -> uint32_t& {
        return px[size_t(row * kArtH + y) * baseW_ + col * kArtW + x];
    };
    auto emptyAt = [&](int x, int y) {
        return base_[size_t(kEmptyRow * kArtH + y) * baseW_ + kEmptyCol * kArtW + x];
    };

    const uint32_t tint = ToRGB(backColor);
    for (int y = 0; y < kArtH; ++y) {
        // Only tint inside the card outline (the corners hold stray light-blue pixels).
        int first = kArtW, last = -1;
        for (int x = 0; x < kArtW; ++x)
            if (emptyAt(x, y) == kInk) { first = std::min(first, x); last = x; }
        for (int x = first + 1; x < last; ++x) {
            uint32_t& p = at(kBackCol, kBackRow, x, y);
            if (p != kBackTint) continue;
            // Pixels touching the outline are corner rounding, not the panel.
            p = (x == first + 1 || x == last - 1) ? 0xFFFFFF : tint;
        }
    }

    if (fourColor) {
        for (int col = 0; col < 13; ++col) {
            for (int y = 0; y < kArtH; ++y) {
                for (int x = 0; x < kArtW; ++x) {
                    // Clubs: ink pixels that aren't part of the card outline.
                    uint32_t& c = at(col, static_cast<int>(Suit::Clubs), x, y);
                    if (c == kInk && emptyAt(x, y) != kInk) c = kClubGreen;
                    uint32_t& d = at(col, static_cast<int>(Suit::Diamonds), x, y);
                    if (d == kRed) d = kDiamondBlue;
                }
            }
        }
    }

    // 2. Nearest-neighbour upscale into a DIB section.
    FreeScaled();
    const int w = baseW_ * scale, h = baseH_ * scale;
    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC screen = GetDC(nullptr);
    scaled_ = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    memDC_ = CreateCompatibleDC(screen);
    ReleaseDC(nullptr, screen);
    oldBmp_ = SelectObject(memDC_, scaled_);

    auto* out = static_cast<uint32_t*>(bits);
    for (int y = 0; y < h; ++y) {
        const uint32_t* srcRow = &px[size_t(y / scale) * baseW_];
        uint32_t* dstRow = out + size_t(y) * w;
        for (int x = 0; x < w; ++x) dstRow[x] = srcRow[x / scale];
    }
    GdiFlush();

    // Font for slot hints, sized to the card.
    if (hintFont_) DeleteObject(hintFont_);
    hintFont_ = CreateFontW(-CardH() / 3, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
                            L"Segoe UI Symbol");
}

void CardRenderer::DrawCell(HDC dc, int col, int row, int x, int y) const {
    const int w = CardW(), h = CardH();
    TransparentBlt(dc, x, y, w, h, memDC_, col * w, row * h, w, h, RGB(0xFF, 0x00, 0xFF));
}

void CardRenderer::DrawCard(HDC dc, const Card& card, int x, int y) const {
    if (!card.faceUp) DrawCell(dc, kBackCol, kBackRow, x, y);
    else if (card.IsJoker()) DrawCell(dc, kJokerCol, card.IsRed() ? kRedJokerRow : kBlackJokerRow, x, y);
    else DrawCell(dc, card.rank - 1, static_cast<int>(card.suit), x, y);
}

void CardRenderer::DrawSlot(HDC dc, const RECT& r, COLORREF background, const std::wstring& hint) const {
    const COLORREF line = Darken(background, 0.65);
    HPEN pen = CreatePen(PS_SOLID, scale_, line);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    const int inset = scale_ / 2;
    const int round = 6 * scale_;
    RoundRect(dc, r.left + inset, r.top + inset, r.right - inset, r.bottom - inset, round, round);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(pen);

    if (!hint.empty()) {
        HGDIOBJ oldFont = SelectObject(dc, hintFont_);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, line);
        RECT tr = r;
        DrawTextW(dc, hint.c_str(), -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, oldFont);
    }
}
