#pragma once
#include "Card.h"
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

// Draws cards from the pixel-art sheet (res/cards.bmp, built by
// tools/make_cardsheet.ps1). The sheet is recolored (4-color deck, card-back
// tint) and scaled up by an integer factor with nearest-neighbour sampling so
// the pixel art stays crisp.
class CardRenderer {
public:
    static constexpr int kArtW = 71;   // card size in art pixels
    static constexpr int kArtH = 96;

    ~CardRenderer();

    bool Load(HINSTANCE inst, int resourceId);
    // Rebuild the scaled sheet. Cheap; call when scale or appearance changes.
    void Configure(int scale, bool fourColor, COLORREF backColor);

    int Scale() const { return scale_; }
    int CardW() const { return kArtW * scale_; }
    int CardH() const { return kArtH * scale_; }

    void DrawCard(HDC dc, const Card& card, int x, int y) const;
    // Outline for an empty pile, plus an optional glyph in the middle.
    void DrawSlot(HDC dc, const RECT& r, COLORREF background, const std::wstring& hint) const;

private:
    void DrawCell(HDC dc, int col, int row, int x, int y) const;
    void FreeScaled();

    std::vector<uint32_t> base_;   // unscaled sheet, 0x00RRGGBB, top-down
    int baseW_ = 0, baseH_ = 0;

    HDC memDC_ = nullptr;
    HBITMAP scaled_ = nullptr;
    HGDIOBJ oldBmp_ = nullptr;
    HFONT hintFont_ = nullptr;
    int scale_ = 0;
    bool fourColor_ = false;
    COLORREF back_ = CLR_INVALID;
};
