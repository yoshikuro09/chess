#include "eval.h"
#include <algorithm>

using namespace std;

static int materialValue(Piece p) {
    switch (p) {
        case Piece::WP: return 100;
        case Piece::WN: return 320;
        case Piece::WB: return 330;
        case Piece::WR: return 500;
        case Piece::WQ: return 900;
        case Piece::WK: return 0;

        case Piece::BP: return -100;
        case Piece::BN: return -320;
        case Piece::BB: return -330;
        case Piece::BR: return -500;
        case Piece::BQ: return -900;
        case Piece::BK: return 0;

        default: return 0;
    }
}


static int mirrorSq(int sq) {
    int f = sq & 7;
    int r = sq >> 3;
    int mr = 7 - r;
    return (mr << 3) | f;
}

static const int PST_PAWN[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10,-20,-20, 10, 10,  5,
      5, -5,-10,  0,  0,-10, -5,  5,
      0,  0,  0, 20, 20,  0,  0,  0,
      5,  5, 10, 25, 25, 10,  5,  5,
     10, 10, 20, 30, 30, 20, 10, 10,
     50, 50, 50, 50, 50, 50, 50, 50,
      0,  0,  0,  0,  0,  0,  0,  0
};

static const int PST_KNIGHT[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

static int pstValue(Piece p, int sq) {
    switch (p) {
        case Piece::WP: return PST_PAWN[sq];
        case Piece::BP: return -PST_PAWN[mirrorSq(sq)];

        case Piece::WN: return PST_KNIGHT[sq];
        case Piece::BN: return -PST_KNIGHT[mirrorSq(sq)];

        default: return 0;
    }
}

namespace Eval {

int score(const Board& b) {
    int s = 0;

    for (int sq = 0; sq < 64; ++sq) {
        Piece p = b.sq[sq];
        if (p == Piece::Empty) continue;

        s += materialValue(p);
        s += pstValue(p, sq);
    }

    return s; 
}

}
