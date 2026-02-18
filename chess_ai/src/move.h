#pragma once
#include <cstdint>
#include "board.h"

struct Move {
    uint8_t from = 0;
    uint8_t to = 0;
    bool isCapture = false;
    Piece promotion = Piece::Empty;

    Move() = default;

    Move(uint8_t f, uint8_t t, bool cap, Piece promo = Piece::Empty)
        : from(f), to(t), isCapture(cap), promotion(promo) {}
};