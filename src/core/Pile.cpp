#include "Pile.h"
#include <algorithm>

void Pile::Steps(int& down, int& up) const {
    down = offFaceDown;
    up = offFaceUp;
    if (fan == Fan::None || maxExtent <= 0 || cards.size() < 2) return;

    // Total fan length with the normal offsets; squeeze proportionally if too long.
    int length = 0;
    for (size_t i = 0; i + 1 < cards.size(); ++i)
        length += cards[i].faceUp ? up : down;
    const int avail = maxExtent - (fan == Fan::Down ? cardH : cardW);
    if (length <= avail || length == 0) return;

    const double k = std::max(0.0, double(avail) / length);
    down = std::max(2, int(down * k));
    up = std::max(3, int(up * k));
}

POINT Pile::CardPos(int i) const {
    POINT p{x, y};
    if (fan == Fan::None || i <= 0) return p;

    int down, up;
    Steps(down, up);
    const int first = maxFanned > 0 ? std::max(0, Count() - maxFanned) : 0;
    int offset = 0;
    for (int k = first; k < i; ++k)
        offset += cards[k].faceUp ? up : down;
    if (fan == Fan::Down) p.y += offset;
    else p.x += offset;
    return p;
}

RECT Pile::CardRect(int i) const {
    POINT p = CardPos(i);
    return RECT{p.x, p.y, p.x + cardW, p.y + cardH};
}

int Pile::HitTest(POINT pt) const {
    for (int i = Count() - 1; i >= 0; --i) {
        RECT r = CardRect(i);
        if (PtInRect(&r, pt)) return i;
    }
    if (cards.empty()) {
        RECT r = SlotRect();
        if (PtInRect(&r, pt)) return kHitSlot;
    }
    return kHitNone;
}
