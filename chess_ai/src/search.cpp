#include "search.h"
#include "movegen.h"
#include "eval.h"
#include <vector>
#include <limits>

using namespace std;

static const int INF = 1'000'000'000;
static const int MATE = 1'000'000; 

static int negamax(Board& b, int depth, int alpha, int beta, uint64_t& nodes) {
    nodes++;

    vector<Move> moves;
    MoveGen::generateLegalMoves(b, moves);

    if (moves.empty()) {
        Color us = b.sideToMove;
        if (b.inCheck(us)) {
            return -MATE + (10 - depth);
        } else {
            return 0;
        }
    }

    if (depth == 0) {
        int s = Eval::score(b);
        return (b.sideToMove == Color::White) ? s : -s;
    }

    int best = -INF;

    for (const auto& m : moves) {
        Undo u;
        if (!b.makeMove(m, u)) continue;

        int score = -negamax(b, depth - 1, -beta, -alpha, nodes);

        b.unmakeMove(m, u);

        if (score > best) best = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) break; 
    }

    return best;
}

namespace Search {

Result findBestMove(Board& b, int depth) {
    Result res;
    res.nodes = 0;

    vector<Move> moves;
    MoveGen::generateLegalMoves(b, moves);

    if (moves.empty()) {
        res.score = 0;
        return res;
    }

    int bestScore = -INF;
    Move bestMove = moves[0];

    int alpha = -INF;
    int beta = INF;

    for (const auto& m : moves) {
        Undo u;
        if (!b.makeMove(m, u)) continue;

        int score = -negamax(b, depth - 1, -beta, -alpha, res.nodes);

        b.unmakeMove(m, u);

        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }
        if (score > alpha) alpha = score;
    }

    res.best = bestMove;
    res.score = bestScore;
    return res;
}

}
