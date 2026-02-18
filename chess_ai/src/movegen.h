#pragma once
#include <vector>
#include "board.h"
#include "move.h"

class MoveGen {
public:
    static void generatePawnPushes(const Board& b, std::vector<Move>& out);
    static void generateKnightMoves(const Board& b, std::vector<Move>& out);
    static void generateBishopMoves(const Board& b, std::vector<Move>& out);
    static void generateRookMoves(const Board& b, std::vector<Move>& out);
    static void generateQueenMoves(const Board& b, std::vector<Move>& out);
    static void generateKingMoves(const Board& b, std::vector<Move>& out);
    static void generateAllPseudoMoves(const Board& b, std::vector<Move>& out);
    static void generateLegalMoves(Board& b, std::vector<Move>& out);
};
