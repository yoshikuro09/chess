#pragma once
#include <cstdint>
#include "board.h"
#include "move.h"

namespace Search {
    struct Result {
        Move best;
        int score = 0;
        uint64_t nodes = 0;
    };
    Result findBestMove(Board& b, int depth);
}
