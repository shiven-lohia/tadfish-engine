#pragma once
#include "types.h"

struct Move {
    int from;
    int to;
    Piece promotion = EMPTY;
    bool is_en_passant_capture;

    Move() : from(-1), to(-1), promotion(EMPTY), is_en_passant_capture(false) {}
    Move(int f, int t, Piece promo = EMPTY, bool is_ep_cap = false) 
        : from(f), to(t), promotion(promo), is_en_passant_capture(is_ep_cap) {}
};