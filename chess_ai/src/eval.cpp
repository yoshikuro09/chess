#include "eval.h"
#include <algorithm>

using namespace std;

static inline bool isWhite(Piece p) { return p >= Piece::WP && p <= Piece::WK; }
static inline bool isBlack(Piece p) { return p >= Piece::BP && p <= Piece::BK; }

static int pieceValue(Piece p) {
    switch (p) {
        case Piece::WP: case Piece::BP: return 100;
        case Piece::WN: case Piece::BN: return 320;
        case Piece::WB: case Piece::BB: return 330;
        case Piece::WR: case Piece::BR: return 500;
        case Piece::WQ: case Piece::BQ: return 900;
        case Piece::WK: case Piece::BK: return 0;
        default: return 0;
    }
}

static inline int mirror64(int sq) {
    return sq ^ 56;
}


// Пешка: поощряем продвижение и центр
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

// Конь: сильный центр, слабые края
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

// Слон: поощряем диагонали/активность
static const int PST_BISHOP[64] = {
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -20,-10,-10,-10,-10,-10,-10,-20
};

// Ладья: 7-я линия и активность
static const int PST_ROOK[64] = {
     0,  0,  5, 10, 10,  5,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     5, 10, 10, 10, 10, 10, 10,  5,
     0,  0,  0,  5,  5,  0,  0,  0
};

// Ферзь: мягко поощряем активность, но без фанатизма
static const int PST_QUEEN[64] = {
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
     0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20
};

// Король: в миддлгейме — безопасность (края/рокировка)
static const int PST_KING_MG[64] = {
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20
};

// Король: в эндшпиле — в центр
static const int PST_KING_EG[64] = {
   -50,-40,-30,-20,-20,-30,-40,-50,
   -30,-20,-10,  0,  0,-10,-20,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-30,  0,  0,  0,  0,-30,-30,
   -50,-30,-30,-30,-30,-30,-30,-50
};

// Грубая оценка “насколько эндшпиль”
static int endgamePhase(const Board& b) {
    // чем меньше тяжёлых фигур — тем ближе к эндшпилю
    int phase = 0;
    for (int sqi = 0; sqi < 64; ++sqi) {
        Piece p = b.sq[sqi];
        switch (p) {
            case Piece::WQ: case Piece::BQ: phase += 4; break;
            case Piece::WR: case Piece::BR: phase += 2; break;
            case Piece::WB: case Piece::BB: phase += 1; break;
            case Piece::WN: case Piece::BN: phase += 1; break;
            default: break;
        }
    }
    phase = min(24, phase);
    int eg = (24 - phase) * 256 / 24;
    return eg;
}

namespace Eval {

int score(const Board& b) {
    int s = 0;

    int egW = endgamePhase(b);   
    int mgW = 256 - egW;

    for (int sqi = 0; sqi < 64; ++sqi) {
        Piece p = b.sq[sqi];
        if (p == Piece::Empty) continue;

        bool w = isWhite(p);
        int idx = w ? sqi : mirror64(sqi); 

        int base = pieceValue(p);
        int pst = 0;

        switch (p) {
            case Piece::WP: pst = PST_PAWN[idx]; break;
            case Piece::BP: pst = PST_PAWN[idx]; break;

            case Piece::WN: case Piece::BN: pst = PST_KNIGHT[idx]; break;
            case Piece::WB: case Piece::BB: pst = PST_BISHOP[idx]; break;
            case Piece::WR: case Piece::BR: pst = PST_ROOK[idx]; break;
            case Piece::WQ: case Piece::BQ: pst = PST_QUEEN[idx]; break;

            case Piece::WK: {
                int mg = PST_KING_MG[idx];
                int eg = PST_KING_EG[idx];
                pst = (mg * mgW + eg * egW) / 256; 
                break;
            }
            case Piece::BK: {
                int mg = PST_KING_MG[idx];
                int eg = PST_KING_EG[idx];
                pst = (mg * mgW + eg * egW) / 256;
                break;
            }

            default: break;
        }

        int add = base + pst;
        s += w ? add : -add;
    }

    return s; 
}

} 
