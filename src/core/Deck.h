#pragma once
#include "Card.h"
#include <cstdint>
#include <initializer_list>
#include <random>
#include <vector>

// A shuffled source of cards. Games deal from it into their piles.
class Deck {
public:
    // Standard 52-card deck, repeated `copies` times.
    explicit Deck(int copies = 1);
    // Custom suit mix, e.g. Spider one-suit: Deck(8, {Suit::Spades}) = 8 x 13 spades.
    Deck(int suitSets, std::initializer_list<Suit> suits);

    // Adds jokers, alternating red and black. Call before Shuffle.
    void AddJokers(int count);
    void Shuffle(uint32_t seed);
    Card Draw(bool faceUp = false);

    bool Empty() const { return cards_.empty(); }
    size_t Size() const { return cards_.size(); }

private:
    std::vector<Card> cards_;
};
