#include "Deck.h"
#include <algorithm>

Deck::Deck(int copies)
    : Deck(copies, {Suit::Clubs, Suit::Diamonds, Suit::Hearts, Suit::Spades}) {}

Deck::Deck(int suitSets, std::initializer_list<Suit> suits) {
    for (int i = 0; i < suitSets; ++i)
        for (Suit s : suits)
            for (int r = kAce; r <= kKing; ++r)
                cards_.push_back(Card{s, static_cast<uint8_t>(r), false});
}

void Deck::Shuffle(uint32_t seed) {
    std::mt19937 rng(seed);
    std::shuffle(cards_.begin(), cards_.end(), rng);
}

Card Deck::Draw(bool faceUp) {
    Card c = cards_.back();
    cards_.pop_back();
    c.faceUp = faceUp;
    return c;
}
