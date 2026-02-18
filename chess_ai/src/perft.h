#pragma once
#include <cstdint>
#include "board.h"

namespace Perft {
    uint64_t run(Board& b, int depth);
    void divide(Board& b, int depth);
}
