#include "perft.h"
#include "movegen.h"
#include "move.h"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

static string sqToAlg(int s) {
    char file = char('a' + (s & 7));
    char rank = char('1' + (s >> 3));
    return string() + file + rank;
}

static char promoChar(Piece p) {
    switch (p) {
        case Piece::WQ: case Piece::BQ: return 'Q';
        case Piece::WR: case Piece::BR: return 'R';
        case Piece::WB: case Piece::BB: return 'B';
        case Piece::WN: case Piece::BN: return 'N';
        default: return '\0';
    }
}

static string moveToStr(const Move& m) {
    if (m.isCastling) {
        if (m.from == 4 && m.to == 6)  return "O-O";
        if (m.from == 4 && m.to == 2)  return "O-O-O";
        if (m.from == 60 && m.to == 62) return "O-O";
        if (m.from == 60 && m.to == 58) return "O-O-O";
        return "O-O(?)";
    }

    string s;
    if (m.isCapture) s = sqToAlg(m.from) + "x" + sqToAlg(m.to);
    else             s = sqToAlg(m.from) + sqToAlg(m.to);

    if (m.promotion != Piece::Empty) {
        s += "=";
        s += promoChar(m.promotion);
    }
    if (m.isEnPassant) s += " e.p.";
    return s;
}

namespace Perft {

uint64_t run(Board& b, int depth) {
    if (depth <= 0) return 1;

    vector<Move> moves;
    MoveGen::generateLegalMoves(b, moves);

    if (depth == 1) return (uint64_t)moves.size();

    uint64_t nodes = 0;
    Color us = b.sideToMove;

    for (const auto& m : moves) {
        Undo u;
        if (!b.makeMove(m, u)) continue;

        nodes += run(b, depth - 1);

        b.unmakeMove(m, u);
    }
    return nodes;
}

void divide(Board& b, int depth) {
    vector<Move> moves;
    MoveGen::generateLegalMoves(b, moves);

    uint64_t total = 0;

    for (const auto& m : moves) {
        Undo u;
        if (!b.makeMove(m, u)) continue;

        uint64_t cnt = run(b, depth - 1);
        total += cnt;

        b.unmakeMove(m, u);

        cout << moveToStr(m) << ": " << cnt << "\n";
    }

    cout << "Total: " << total << "\n";
}

} 
