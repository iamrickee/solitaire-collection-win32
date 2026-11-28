#pragma once
#include "Yukon.h"

// Russian Solitaire: Yukon, but columns build down in suit instead of
// alternating colours. Same deal, moves and scoring.
class RussianSolitaire : public Yukon {
public:
    using Yukon::Yukon;

    std::wstring Name() const override { return L"Russian Solitaire"; }
    std::wstring Rules() const override;   // in Yukon.cpp

protected:
    bool Builds(const Card& c, const Card& onto) const override { return c.suit == onto.suit; }
};
