#include "Game.h"

void Game::NewGame(uint32_t seed) {
    seed_ = seed;
    undo_.clear();
    started_ = false;
    ended_ = false;
    score_ = 0;
    for (Pile& p : piles_) p.cards.clear();
    Deal(seed);
}

bool Game::End() {
    if (ended_ || !started_) return false;
    ended_ = true;
    OnEnded();
    return settings_.SubmitScore(Label(), score_);
}

void Game::PushUndo() {
    Snapshot s;
    s.cards.reserve(piles_.size());
    for (const Pile& p : piles_) s.cards.push_back(p.cards);
    s.score = score_;
    s.extra = SaveExtra();
    undo_.push_back(std::move(s));
    started_ = true;
}

bool Game::Undo() {
    if (!CanUndo()) return false;
    Snapshot& s = undo_.back();
    for (size_t i = 0; i < piles_.size(); ++i) piles_[i].cards = std::move(s.cards[i]);
    score_ = s.score;
    LoadExtra(s.extra);
    undo_.pop_back();
    return true;
}

void Game::MoveCards(int from, int index, int to) {
    auto& src = piles_[from].cards;
    auto& dst = piles_[to].cards;
    dst.insert(dst.end(), src.begin() + index, src.end());
    src.erase(src.begin() + index, src.end());
}

bool Game::TryMove(int from, int index, int to) {
    if (ended_ || from == to || !CanDrop(from, index, to)) return false;
    const int count = piles_[from].Count() - index;
    PushUndo();
    MoveCards(from, index, to);
    OnMoved(from, to, count);
    return true;
}

int Game::DoubleClickTarget(int pile, int index) const {
    const Pile& p = piles_[pile];
    if (index != p.Count() - 1 || !p.Top().faceUp) return -1;
    for (int f = 0; f < static_cast<int>(piles_.size()); ++f)
        if (piles_[f].kind == PileKind::Foundation && CanDrop(pile, index, f)) return f;
    return -1;
}

bool Game::FindAutoMove(int& from, int& to) const {
    for (int p = 0; p < static_cast<int>(piles_.size()); ++p) {
        const PileKind k = piles_[p].kind;
        if (piles_[p].Empty() || !(k == PileKind::Waste || k == PileKind::Tableau || k == PileKind::Cell))
            continue;
        int f = DoubleClickTarget(p, piles_[p].Count() - 1);
        if (f >= 0) { from = p; to = f; return true; }
    }
    return false;
}
