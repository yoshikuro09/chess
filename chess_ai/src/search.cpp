// search.cpp
#include "search.h"
#include "movegen.h"
#include "eval.h"

#include <vector>
#include <limits>
#include <algorithm>
#include <utility>
#include <chrono>
#include <cstring>  

using namespace std;

static const int INF  = 1'000'000'000;
static const int MATE = 1'000'000;

static inline bool isMateScore(int s) {
    return (s > MATE - 10000) || (s < -MATE + 10000);
}

static inline int toTTScore(int s, int ply) {
    if (s > MATE - 10000) return s + ply;
    if (s < -MATE + 10000) return s - ply;
    return s;
}

static inline int fromTTScore(int s, int ply) {
    if (s > MATE - 10000) return s - ply;
    if (s < -MATE + 10000) return s + ply;
    return s;
}

enum TTFlag { TT_EXACT, TT_LOWER, TT_UPPER };

struct TTEntry {
    uint64_t key = 0;
    int depth = -1;
    int score = 0;
    TTFlag flag = TT_EXACT;
    Move best;
};

static const int TT_SIZE = 1 << 20;
static TTEntry TT[TT_SIZE];

static inline TTEntry* probeTT(uint64_t key) {
    return &TT[key & (TT_SIZE - 1)];
}

static inline void storeTT(uint64_t key, int depth, int score,
                           TTFlag flag, const Move& best)
{
    TTEntry* e = probeTT(key);

    if (depth >= e->depth) {
        e->key = key;
        e->depth = depth;
        e->score = score;
        e->flag = flag;
        e->best = best;
    }
}

struct SearchState {
    chrono::steady_clock::time_point deadline; // дедлайн времени
    bool stop = false;                         // флаг стоп
};

static inline bool timeUp(const SearchState& st) {
    return chrono::steady_clock::now() >= st.deadline; // проверка времени
}

static const int MAX_PLY = 128;

static Move killers[MAX_PLY][2];        // killer ходы
static int  historyTable[2][64][64];    // history таблица

static inline int sideIndex(Color c) { return (c == Color::White) ? 0 : 1; }

static inline bool sameMoveFull(const Move& a, const Move& b) {
    return a.from == b.from &&
           a.to == b.to &&
           a.promotion == b.promotion &&
           a.isCastling == b.isCastling &&
           a.isEnPassant == b.isEnPassant;
}

static inline bool isQuiet(const Move& m) {
    return !m.isCapture && !m.isEnPassant &&
           (m.promotion == Piece::Empty) &&
           !m.isCastling;
}

static inline void clearHeuristics() {
    memset(killers, 0, sizeof(killers));       // очистка killers
    memset(historyTable, 0, sizeof(historyTable)); // очистка history
}
static int absPieceValue(Piece p) {
    switch (p) {
        case Piece::WP: case Piece::BP: return 100;
        case Piece::WN: case Piece::BN: return 320;
        case Piece::WB: case Piece::BB: return 330;
        case Piece::WR: case Piece::BR: return 500;
        case Piece::WQ: case Piece::BQ: return 900;
        case Piece::WK: case Piece::BK: return 20000;
        default: return 0;
    }
}

static int promoValue(Piece p) {
    switch (p) {
        case Piece::WQ: case Piece::BQ: return 900;
        case Piece::WR: case Piece::BR: return 500;
        case Piece::WB: case Piece::BB: return 330;
        case Piece::WN: case Piece::BN: return 320;
        default: return 0;
    }
}

static bool isTactical(const Move& m) {
    return m.isCapture || m.isEnPassant || (m.promotion != Piece::Empty); 
}

static int moveScore(Board& b, const Move& m, const Move* ttMove, int ply) {

    if (ttMove && sameMoveFull(m, *ttMove))
        return 2'000'000'000;

    int s = 0;

    if (m.promotion != Piece::Empty) {
        s += 1'000'000;              // приоритет промоции
        s += promoValue(m.promotion);
    }

    if (m.isCapture || m.isEnPassant) {
        s += 900'000;                // приоритет взятия

        Piece victim = Piece::Empty;
        if (m.isEnPassant)
            victim = (b.sideToMove == Color::White) ? Piece::BP : Piece::WP;
        else
            victim = b.sq[m.to];

        Piece attacker = b.sq[m.from];
        s += 10 * absPieceValue(victim) - absPieceValue(attacker); 
        return s;
    }

    // killer ходы
    if (isQuiet(m) && ply < MAX_PLY) {
        if (sameMoveFull(m, killers[ply][0])) return 800'000;
        if (sameMoveFull(m, killers[ply][1])) return 790'000;
    }

    // history
    if (isQuiet(m)) {
        int si = sideIndex(b.sideToMove);
        s += historyTable[si][m.from][m.to];
    }

    return s;
}

static void orderMoves(Board& b, vector<Move>& moves, const Move* ttMove, int ply) {
    vector<pair<int, Move>> scored;
    scored.reserve(moves.size());

    for (const auto& m : moves)
        scored.push_back({ moveScore(b, m, ttMove, ply), m });

    stable_sort(scored.begin(), scored.end(),
        [](const pair<int, Move>& a, const pair<int, Move>& b) {
            return a.first > b.first;
        });

    for (size_t i = 0; i < moves.size(); ++i)
        moves[i] = scored[i].second;
}

static int quiescence(Board& b, int alpha, int beta, int ply,
                      uint64_t& nodes, SearchState& st)
{
    nodes++;                               // счет узлов
    if (st.stop) return 0;
    if (timeUp(st)) { st.stop = true; return 0; } // проверка таймера

    int stand = Eval::score(b);             // стат оценка
    stand = (b.sideToMove == Color::White) ? stand : -stand;

    if (stand >= beta) return beta;         // beta отсечение
    if (stand > alpha) alpha = stand;       // alpha обновление

    vector<Move> pseudo;
    MoveGen::generateAllPseudoMoves(b, pseudo);

    vector<Move> tact;
    for (const auto& m : pseudo)
        if (isTactical(m)) tact.push_back(m);   // только тактика

    orderMoves(b, tact, nullptr, ply);

    for (const auto& m : tact) {

        Undo u;
        if (!b.makeMove(m, u)) continue;

        int score = -quiescence(b, -beta, -alpha, ply + 1, nodes, st);

        b.unmakeMove(m, u);

        if (score >= beta) return beta;     // отсечение
        if (score > alpha) alpha = score;   // лучший
    }

    return alpha;
}

static int negamax(Board& b, int depth, int alpha, int beta, int ply,
                   uint64_t& nodes, SearchState& st)
{
    nodes++;                                    // счет узлов
    if (st.stop) return 0;
    if (timeUp(st)) { st.stop = true; return 0; } // таймер

    uint64_t key = b.computeHash();
    TTEntry* tte = probeTT(key);

    int alphaOrig = alpha;

    // TT проверка
    if (tte->key == key && tte->depth >= depth) {
        int ttScore = fromTTScore(tte->score, ply);

        if (tte->flag == TT_EXACT) return ttScore;
        if (tte->flag == TT_LOWER && ttScore >= beta) return ttScore;
        if (tte->flag == TT_UPPER && ttScore <= alpha) return ttScore;
    }

    vector<Move> legal;
    MoveGen::generateLegalMoves(b, legal);       // легальные

    if (legal.empty()) {
        Color us = b.sideToMove;
        if (b.inCheck(us)) return -MATE + ply;   // мат
        return 0;                                // пат
    }

    if (depth == 0)
        return quiescence(b, alpha, beta, ply, nodes, st); // qsearch

    const Move* ttMove = (tte->key == key) ? &tte->best : nullptr;

    orderMoves(b, legal, ttMove, ply);           // ordering

    int bestScore = -INF;
    Move bestMove = legal[0];

    for (const auto& m : legal) {

        Undo u;
        if (!b.makeMove(m, u)) continue;

        int score = -negamax(b, depth - 1, -beta, -alpha, ply + 1, nodes, st);

        b.unmakeMove(m, u);

        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }

        if (score > alpha) alpha = score;

        // beta cutoff
        if (alpha >= beta) {

            // killer + history обновление
            if (isQuiet(m) && ply < MAX_PLY) {

                if (!sameMoveFull(m, killers[ply][0])) {
                    killers[ply][1] = killers[ply][0];
                    killers[ply][0] = m;
                }

                int si = sideIndex(b.sideToMove);
                historyTable[si][m.from][m.to] += depth * depth;
            }

            break;
        }
    }

    // TT запись
    TTFlag flag = TT_EXACT;
    if (bestScore <= alphaOrig) flag = TT_UPPER;
    else if (bestScore >= beta) flag = TT_LOWER;

    storeTT(key, depth, toTTScore(bestScore, ply), flag, bestMove);

    return bestScore;
}

namespace Search {

Result findBestMove(Board& b, int depth) {

    clearHeuristics(); // очистка таблиц

    Result res;
    res.nodes = 0;

    vector<Move> legal;
    MoveGen::generateLegalMoves(b, legal);

    if (legal.empty()) {
        res.score = 0;
        res.depthDone = 0;
        return res;
    }

    orderMoves(b, legal, nullptr, 0);

    int bestScore = -INF;
    Move bestMove = legal[0];

    int alpha = -INF;
    int beta  = INF;

    SearchState st;
    st.deadline = chrono::steady_clock::time_point::max(); // без таймера

    for (const auto& m : legal) {

        Undo u;
        if (!b.makeMove(m, u)) continue;

        int score = -negamax(b, depth - 1, -beta, -alpha, 1, res.nodes, st);

        b.unmakeMove(m, u);

        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }
        if (score > alpha) alpha = score;
    }

    res.best = bestMove;
    res.score = bestScore;
    res.depthDone = depth;
    res.timedOut = false;
    return res;
}

Result findBestMoveTimed(Board& b, int maxDepth, int timeMs) {

    clearHeuristics(); // очистка таблиц

    Result res;
    res.nodes = 0;

    vector<Move> legalRoot;
    MoveGen::generateLegalMoves(b, legalRoot);

    if (legalRoot.empty()) {
        res.score = 0;
        res.depthDone = 0;
        res.timedOut = false;
        return res;
    }

    SearchState st;
    st.deadline = chrono::steady_clock::now() + chrono::milliseconds(timeMs);

    Move bestMove = legalRoot[0];
    Move pvMove   = bestMove;
    int bestScore = -INF;
    int depthDone = 0;

    for (int depth = 1; depth <= maxDepth; ++depth) {

        if (timeUp(st)) { st.stop = true; break; }

        vector<Move> legal = legalRoot;

        orderMoves(b, legal, &pvMove, 0);

        int alpha = -INF;
        int beta  = INF;

        Move iterBest = legal[0];
        int iterBestScore = -INF;

        for (const auto& m : legal) {

            Undo u;
            if (!b.makeMove(m, u)) continue;

            int score = -negamax(b, depth - 1, -beta, -alpha, 1, res.nodes, st);

            b.unmakeMove(m, u);

            if (score > iterBestScore) {
                iterBestScore = score;
                iterBest = m;
            }
            if (score > alpha) alpha = score;
        }

        if (!st.stop) {
            bestMove  = iterBest;
            pvMove    = iterBest;
            bestScore = iterBestScore;
            depthDone = depth;
        } else break;
    }

    res.best = bestMove;
    res.score = bestScore;
    res.depthDone = depthDone;
    res.timedOut = (depthDone < maxDepth);
    return res;
}

}
