#include "search.h"
#include "movegen.h"
#include "eval.h"
#include <vector>
#include <limits>
#include <algorithm>
#include <utility>
#include <chrono>

using namespace std;

static const int INF  = 1'000'000'000;
static const int MATE = 1'000'000;

struct SearchState {
    chrono::steady_clock::time_point deadline; // дедлайн времени
    bool stop = false;                         // флаг стоп
};

static inline bool timeUp(const SearchState& st) {
    return chrono::steady_clock::now() >= st.deadline; // проверка времени
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
    return m.isCapture || m.isEnPassant || (m.promotion != Piece::Empty); // тактический ход
}

static int moveScore(Board& b, const Move& m) {
    int s = 0;

    if (m.promotion != Piece::Empty) {
        s += 1'000'000;              // приоритет промоции
        s += promoValue(m.promotion);
    }

    if (m.isCapture || m.isEnPassant) {
        s += 500'000;                // приоритет взятия

        Piece victim = Piece::Empty;
        if (m.isEnPassant) {
            victim = (b.sideToMove == Color::White) ? Piece::BP : Piece::WP;
        } else {
            victim = b.sq[m.to];
        }

        Piece attacker = b.sq[m.from];
        s += 10 * absPieceValue(victim) - absPieceValue(attacker); // MVV-LVA
    }

    {
        int to = m.to;
        int f = to & 7;
        int r = to >> 3;
        if ((f >= 2 && f <= 5) && (r >= 2 && r <= 5)) s += 50; // центр бонус
    }

    {
        Undo u;
        Color enemy = (b.sideToMove == Color::White) ? Color::Black : Color::White;
        if (b.makeMove(m, u)) {
            if (b.inCheck(enemy)) s += 300'000; // шах бонус
            b.unmakeMove(m, u);
        }
    }

    return s;
}

static void orderMoves(Board& b, vector<Move>& moves) {
    vector<pair<int, Move>> scored;
    scored.reserve(moves.size());

    for (const auto& m : moves) {
        scored.push_back({ moveScore(b, m), m }); // оценка ходов
    }

    stable_sort(scored.begin(), scored.end(),
        [](const pair<int, Move>& a, const pair<int, Move>& b) {
            return a.first > b.first; // сортировка
        }
    );

    for (size_t i = 0; i < moves.size(); ++i) moves[i] = scored[i].second; // копирование
}

static int quiescence(Board& b, int alpha, int beta, uint64_t& nodes, SearchState& st) {
    nodes++;                               // счет узлов
    if (st.stop) return 0;
    if (timeUp(st)) { st.stop = true; return 0; } // проверка таймера

    vector<Move> legalNow;
    MoveGen::generateLegalMoves(b, legalNow);

    if (legalNow.empty()) {                 // терминал
        Color us = b.sideToMove;
        if (b.inCheck(us)) return -MATE;    // мат
        return 0;                           // пат
    }

    int stand = Eval::score(b);             // стат оценка
    stand = (b.sideToMove == Color::White) ? stand : -stand;

    if (stand >= beta) return beta;         // beta отсечение
    if (stand > alpha) alpha = stand;       // alpha обновление

    vector<Move> pseudo;
    MoveGen::generateAllPseudoMoves(b, pseudo); // псевдоходы

    vector<Move> tact;
    tact.reserve(pseudo.size());

    for (const auto& m : pseudo)
        if (isTactical(m)) tact.push_back(m);   // только тактика

    vector<Move> legalTact;
    legalTact.reserve(tact.size());

    Color us = b.sideToMove;

    for (const auto& m : tact) {
        if (st.stop) break;
        if (timeUp(st)) { st.stop = true; break; }

        Undo u;
        if (!b.makeMove(m, u)) continue;

        bool illegal = b.inCheck(us);
        b.unmakeMove(m, u);

        if (!illegal) legalTact.push_back(m);   // легальный
    }

    orderMoves(b, legalTact);                   // сортировка

    for (const auto& m : legalTact) {
        if (st.stop) break;
        if (timeUp(st)) { st.stop = true; break; }

        Undo u;
        if (!b.makeMove(m, u)) continue;

        int score = -quiescence(b, -beta, -alpha, nodes, st); // рекурсия

        b.unmakeMove(m, u);

        if (st.stop) break;

        if (score >= beta) return beta;         // отсечение
        if (score > alpha) alpha = score;       // лучший
    }

    return alpha;
}

static int negamax(Board& b, int depth, int alpha, int beta, int ply,
                   uint64_t& nodes, SearchState& st)
{
    nodes++;                                    // счет узлов
    if (st.stop) return 0;
    if (timeUp(st)) { st.stop = true; return 0; } // таймер

    vector<Move> legal;
    MoveGen::generateLegalMoves(b, legal);       // легальные

    if (legal.empty()) {                         // терминал
        Color us = b.sideToMove;
        if (b.inCheck(us)) return -MATE + ply;   // мат
        return 0;                                // пат
    }

    if (depth == 0) {
        return quiescence(b, alpha, beta, nodes, st); // переход qsearch
    }

    orderMoves(b, legal);                        // ordering

    int best = -INF;

    for (const auto& m : legal) {
        if (st.stop) break;
        if (timeUp(st)) { st.stop = true; break; }

        Undo u;
        if (!b.makeMove(m, u)) continue;

        int score = -negamax(b, depth - 1, -beta, -alpha, ply + 1, nodes, st); // рекурсия

        b.unmakeMove(m, u);

        if (st.stop) break;

        if (score > best) best = score;
        if (score > alpha) alpha = score;

        if (alpha >= beta) break;                // alpha-beta
    }

    return best;
}

namespace Search {

Result findBestMove(Board& b, int depth) {
    Result res;
    res.nodes = 0;

    vector<Move> legal;
    MoveGen::generateLegalMoves(b, legal);       // генерация

    if (legal.empty()) {
        res.score = 0;
        res.depthDone = 0;
        return res;
    }

    orderMoves(b, legal);                        // сортировка

    int bestScore = -INF;
    Move bestMove = legal[0];

    int alpha = -INF;
    int beta  = INF;

    SearchState st;
    st.deadline = chrono::steady_clock::time_point::max(); // без таймера

    for (const auto& m : legal) {
        Undo u;
        if (!b.makeMove(m, u)) continue;

        int score = -negamax(b, depth - 1, -beta, -alpha, 1, res.nodes, st); // поиск

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
    Result res;
    res.nodes = 0;

    vector<Move> legalRoot;
    MoveGen::generateLegalMoves(b, legalRoot);   // корневые

    if (legalRoot.empty()) {
        res.score = 0;
        res.depthDone = 0;
        res.timedOut = false;
        return res;
    }

    SearchState st;
    st.deadline = chrono::steady_clock::now() + chrono::milliseconds(timeMs); // старт таймера

    Move bestMove = legalRoot[0];
    int bestScore = -INF;
    int depthDone = 0;

    for (int depth = 1; depth <= maxDepth; ++depth) { // iterative deepening

        if (timeUp(st)) { st.stop = true; break; }

        vector<Move> legal = legalRoot;
        orderMoves(b, legal);

        int alpha = -INF;
        int beta  = INF;

        Move iterBest = legal[0];
        int iterBestScore = -INF;

        for (const auto& m : legal) {

            if (st.stop) break;
            if (timeUp(st)) { st.stop = true; break; }

            Undo u;
            if (!b.makeMove(m, u)) continue;

            int score = -negamax(b, depth - 1, -beta, -alpha, 1, res.nodes, st);

            b.unmakeMove(m, u);

            if (st.stop) break;

            if (score > iterBestScore) {
                iterBestScore = score;
                iterBest = m;
            }
            if (score > alpha) alpha = score;
        }

        if (!st.stop) {
            bestMove = iterBest;         // лучший ход
            bestScore = iterBestScore;
            depthDone = depth;           // глубина готова
        } else {
            break;                       // время вышло
        }
    }

    res.best = bestMove;
    res.score = bestScore;
    res.depthDone = depthDone;
    res.timedOut = (depthDone < maxDepth); // таймаут флаг
    return res;
}

}
