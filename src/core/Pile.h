#pragma once
#include "Card.h"
#include <windows.h>
#include <vector>

enum class PileKind { Stock, Waste, Foundation, Tableau, Cell };
enum class Fan { None, Down, Right };

// Hit-test results besides a card index.
constexpr int kHitNone = -2;   // point is not on this pile
constexpr int kHitSlot = -1;   // point is on the empty pile's base slot

// A stack of cards at a screen position. Positions/offsets are in pixels and
// are assigned by the owning Game's Layout().
struct Pile {
    PileKind kind = PileKind::Tableau;
    Fan fan = Fan::None;
    std::vector<Card> cards;

    int x = 0, y = 0;            // top-left of the bottom card
    int cardW = 0, cardH = 0;
    int offFaceDown = 0;         // fan step after a face-down card
    int offFaceUp = 0;           // fan step after a face-up card
    int maxFanned = 0;           // >0: only the top N cards fan (draw-3 waste)
    int maxExtent = 0;           // >0: squeeze the fan so the pile fits in this many px

    bool Empty() const { return cards.empty(); }
    Card& Top() { return cards.back(); }
    const Card& Top() const { return cards.back(); }
    int Count() const { return static_cast<int>(cards.size()); }

    POINT CardPos(int i) const;
    RECT CardRect(int i) const;
    RECT SlotRect() const { return RECT{x, y, x + cardW, y + cardH}; }

    // Topmost card index under pt, kHitSlot for an empty pile's base, or kHitNone.
    int HitTest(POINT pt) const;

private:
    void Steps(int& down, int& up) const;
};
