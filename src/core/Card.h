#pragma once
#include <cstdint>

// Suit order matches the rows of res/cards.bmp.
enum class Suit : uint8_t { Clubs = 0, Diamonds = 1, Hearts = 2, Spades = 3 };

constexpr int kAce = 1;
constexpr int kJack = 11;
constexpr int kQueen = 12;
constexpr int kKing = 13;

struct Card {
    Suit suit = Suit::Spades;
    uint8_t rank = kAce;   // 1 (Ace) .. 13 (King)
    bool faceUp = false;

    bool IsRed() const { return suit == Suit::Diamonds || suit == Suit::Hearts; }
    bool IsBlack() const { return !IsRed(); }
    bool SameColor(const Card& o) const { return IsRed() == o.IsRed(); }
};
