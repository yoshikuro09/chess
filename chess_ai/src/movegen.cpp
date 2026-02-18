#include "movegen.h"
#include <cstdlib>

using namespace std;

static bool isWhitePiece(Piece p) {
    return p >= Piece::WP && p <= Piece::WK;
}

static bool isBlackPiece(Piece p) {
    return p >= Piece::BP && p <= Piece::BK;
}

void MoveGen::generatePawnPushes(const Board& b, vector<Move>& out) {
    out.clear();

    bool white = (b.sideToMove == Color::White);
    Piece pawn = white ? Piece::WP : Piece::BP;

    int dir = white ? 8 : -8;
    int startRank = white ? 1 : 6;

    for (int from = 0; from < 64; ++from) {
        if (b.sq[from] != pawn) continue;  // Цвет фигуры

        int file = Board::fileOf(from);
        int r = Board::rankOf(from);

        //  Ход на 1 клетку вперед
        int one = from + dir;
        if (one >= 0 && one < 64 && b.sq[one] == Piece::Empty) {
            int toRank = Board::rankOf(one);
            bool promo = white ? (toRank == 7) : (toRank == 0);

            if (promo) {
                out.push_back(Move{ (uint8_t)from, (uint8_t)one, false, white ? Piece::WQ : Piece::BQ, false });
                out.push_back(Move{ (uint8_t)from, (uint8_t)one, false, white ? Piece::WR : Piece::BR, false });
                out.push_back(Move{ (uint8_t)from, (uint8_t)one, false, white ? Piece::WB : Piece::BB, false });
                out.push_back(Move{ (uint8_t)from, (uint8_t)one, false, white ? Piece::WN : Piece::BN, false });
            } else {
                out.push_back(Move{ (uint8_t)from, (uint8_t)one, false, Piece::Empty, false });

                // Ход на 2 клетки со стартовой
                if (r == startRank) {
                    int two = from + 2 * dir;
                    if (two >= 0 && two < 64 && b.sq[two] == Piece::Empty) {
                        out.push_back(Move{ (uint8_t)from, (uint8_t)two, false, Piece::Empty, false });
                    }
                }
            }
        }

        // Взятие влево
        if (file > 0) {
            int capL = from + (white ? 7 : -9);
            if (capL >= 0 && capL < 64) {
                Piece target = b.sq[capL];

                if (target != Piece::Empty) {
                    bool enemy = white ? isBlackPiece(target) : isWhitePiece(target);
                    if (enemy) {
                        int toRank = Board::rankOf(capL);
                        bool promo = white ? (toRank == 7) : (toRank == 0);

                        if (promo) {
                            out.push_back(Move{ (uint8_t)from, (uint8_t)capL, true, white ? Piece::WQ : Piece::BQ, false });
                            out.push_back(Move{ (uint8_t)from, (uint8_t)capL, true, white ? Piece::WR : Piece::BR, false });
                            out.push_back(Move{ (uint8_t)from, (uint8_t)capL, true, white ? Piece::WB : Piece::BB, false });
                            out.push_back(Move{ (uint8_t)from, (uint8_t)capL, true, white ? Piece::WN : Piece::BN, false });
                        } else {
                            out.push_back(Move{ (uint8_t)from, (uint8_t)capL, true, Piece::Empty, false });
                        }
                    }
                } else {
                    // en passant
                    if (b.enPassantSquare == capL) {
                        out.push_back(Move{ (uint8_t)from, (uint8_t)capL, true, Piece::Empty, true });
                    }
                }
            }
        }

        // Взятие вправо
        if (file < 7) {
            int capR = from + (white ? 9 : -7);
            if (capR >= 0 && capR < 64) {
                Piece target = b.sq[capR];

                if (target != Piece::Empty) {
                    bool enemy = white ? isBlackPiece(target) : isWhitePiece(target);
                    if (enemy) {
                        int toRank = Board::rankOf(capR);
                        bool promo = white ? (toRank == 7) : (toRank == 0);

                        if (promo) {
                            out.push_back(Move{ (uint8_t)from, (uint8_t)capR, true, white ? Piece::WQ : Piece::BQ, false });
                            out.push_back(Move{ (uint8_t)from, (uint8_t)capR, true, white ? Piece::WR : Piece::BR, false });
                            out.push_back(Move{ (uint8_t)from, (uint8_t)capR, true, white ? Piece::WB : Piece::BB, false });
                            out.push_back(Move{ (uint8_t)from, (uint8_t)capR, true, white ? Piece::WN : Piece::BN, false });
                        } else {
                            out.push_back(Move{ (uint8_t)from, (uint8_t)capR, true, Piece::Empty, false });
                        }
                    }
                } else {
                    // en passant
                    if (b.enPassantSquare == capR) {
                        out.push_back(Move{ (uint8_t)from, (uint8_t)capR, true, Piece::Empty, true });
                    }
                }
            }
        }
    }
}

void MoveGen::generateKnightMoves(const Board& b, vector<Move>& out) {
    out.clear();

    bool white = (b.sideToMove == Color::White);
    Piece knight = white ? Piece::WN : Piece::BN;

    static const int jumps[8] = { -17, -15, -10, -6, 6, 10, 15, 17 };

    for (int from = 0; from < 64; ++from) {
        if (b.sq[from] != knight) continue;

        int f0 = Board::fileOf(from);
        int r0 = Board::rankOf(from);

        for (int d : jumps) {
            int to = from + d;
            if (to < 0 || to >= 64) continue;

            int df = abs(Board::fileOf(to) - f0);
            int dr = abs(Board::rankOf(to) - r0);
            if (!((df == 1 && dr == 2) || (df == 2 && dr == 1))) continue;

            Piece target = b.sq[to];
            if (target == Piece::Empty) {
                out.push_back(Move{ (uint8_t)from, (uint8_t)to, false, Piece::Empty, false });
            } else {
                bool enemy = white ? isBlackPiece(target) : isWhitePiece(target);
                if (enemy) {
                    out.push_back(Move{ (uint8_t)from, (uint8_t)to, true, Piece::Empty, false });
                }
            }
        }
    }
}

static void addRayBishop(const Board& b, vector<Move>& out, int from, bool white, int df, int dr) {
    int f = Board::fileOf(from);
    int r = Board::rankOf(from);

    while (true) {
        f += df;
        r += dr;
        if (f < 0 || f > 7 || r < 0 || r > 7) break;

        int to = r * 8 + f;
        Piece target = b.sq[to];

        if (target == Piece::Empty) {
            out.push_back(Move{ (uint8_t)from, (uint8_t)to, false, Piece::Empty, false });
            continue;
        }

        // фигура стоп (своя), фигуру берём (противника)
        bool enemy = white ? isBlackPiece(target) : isWhitePiece(target);
        if (enemy) {
            out.push_back(Move{ (uint8_t)from, (uint8_t)to, true, Piece::Empty, false });
        }
        break;
    }
}

void MoveGen::generateBishopMoves(const Board& b, vector<Move>& out) {
    out.clear();

    bool white = (b.sideToMove == Color::White);
    Piece bishop = white ? Piece::WB : Piece::BB;

    for (int from = 0; from < 64; ++from) {
        if (b.sq[from] != bishop) continue;

        // 4 диагонали
        addRayBishop(b, out, from, white, +1, +1);
        addRayBishop(b, out, from, white, -1, +1);
        addRayBishop(b, out, from, white, +1, -1);
        addRayBishop(b, out, from, white, -1, -1);
    }
}

static void addRayRook(const Board& b, vector<Move>& out,
                       int from, bool white, int df, int dr)
{
    int f = Board::fileOf(from);
    int r = Board::rankOf(from);

    while (true)
    {
        f += df;
        r += dr;

        if (f < 0 || f > 7 || r < 0 || r > 7)
            break;

        int to = r * 8 + f;
        Piece target = b.sq[to];

        if (target == Piece::Empty)
        {
            out.push_back(Move{ (uint8_t)from, (uint8_t)to, false, Piece::Empty, false });
            continue;
        }

        bool enemy = white ? isBlackPiece(target) : isWhitePiece(target);

        if (enemy)
        {
            out.push_back(Move{ (uint8_t)from, (uint8_t)to, true, Piece::Empty, false });
        }

        break;
    }
}

void MoveGen::generateRookMoves(const Board& b, vector<Move>& out)
{
    out.clear();

    bool white = (b.sideToMove == Color::White);
    Piece rook = white ? Piece::WR : Piece::BR;

    for (int from = 0; from < 64; ++from)
    {
        if (b.sq[from] != rook) continue;

        // горизонталь вертикаль
        addRayRook(b, out, from, white, 1, 0);
        addRayRook(b, out, from, white, -1, 0);
        addRayRook(b, out, from, white, 0, 1);
        addRayRook(b, out, from, white, 0, -1);
    }
}

void MoveGen::generateQueenMoves(const Board& b, vector<Move>& out)
{
    out.clear();

    bool white = (b.sideToMove == Color::White);
    Piece queen = white ? Piece::WQ : Piece::BQ;

    for (int from = 0; from < 64; ++from)
    {
        if (b.sq[from] != queen) continue;

        addRayBishop(b, out, from, white, +1, +1);
        addRayBishop(b, out, from, white, -1, +1);
        addRayBishop(b, out, from, white, +1, -1);
        addRayBishop(b, out, from, white, -1, -1);

        addRayRook(b, out, from, white, +1, 0);
        addRayRook(b, out, from, white, -1, 0);
        addRayRook(b, out, from, white, 0, +1);
        addRayRook(b, out, from, white, 0, -1);
    }
}

void MoveGen::generateKingMoves(const Board& b, vector<Move>& out) {
    out.clear();

    bool white = (b.sideToMove == Color::White);
    Piece king = white ? Piece::WK : Piece::BK;

    for (int from = 0; from < 64; ++from) {
        if (b.sq[from] != king) continue;

        int f0 = Board::fileOf(from);
        int r0 = Board::rankOf(from);

        for (int dr = -1; dr <= 1; ++dr) {
            for (int df = -1; df <= 1; ++df) {
                if (dr == 0 && df == 0) continue;

                int f = f0 + df;
                int r = r0 + dr;
                if (f < 0 || f > 7 || r < 0 || r > 7) continue;

                int to = r * 8 + f;
                Piece target = b.sq[to];

                if (target == Piece::Empty) {
                    out.push_back(Move{ (uint8_t)from, (uint8_t)to, false, Piece::Empty, false });
                } else {
                    bool enemy = white ? isBlackPiece(target) : isWhitePiece(target);
                    if (enemy) {
                        out.push_back(Move{ (uint8_t)from, (uint8_t)to, true, Piece::Empty, false });
                    }
                }
            }
        }
        if (white) {
            if (from == 4 && (b.castlingRights & 1)) { // K
                if (b.sq[5] == Piece::Empty && b.sq[6] == Piece::Empty) {
                    if (!b.inCheck(Color::White) &&
                        !b.isSquareAttacked(5, Color::Black) &&
                        !b.isSquareAttacked(6, Color::Black)) {
                        out.push_back(Move{ (uint8_t)4, (uint8_t)6, false, Piece::Empty, false, true });
                    }
                }
            }
            if (from == 4 && (b.castlingRights & 2)) { // Q
                if (b.sq[3] == Piece::Empty && b.sq[2] == Piece::Empty && b.sq[1] == Piece::Empty) {
                    if (!b.inCheck(Color::White) &&
                        !b.isSquareAttacked(3, Color::Black) &&
                        !b.isSquareAttacked(2, Color::Black)) {
                        out.push_back(Move{ (uint8_t)4, (uint8_t)2, false, Piece::Empty, false, true });
                    }
                }
            }
        } else {
            if (from == 60 && (b.castlingRights & 4)) { // k
                if (b.sq[61] == Piece::Empty && b.sq[62] == Piece::Empty) {
                    if (!b.inCheck(Color::Black) &&
                        !b.isSquareAttacked(61, Color::White) &&
                        !b.isSquareAttacked(62, Color::White)) {
                        out.push_back(Move{ (uint8_t)60, (uint8_t)62, false, Piece::Empty, false, true });
                    }
                }
            }
            if (from == 60 && (b.castlingRights & 8)) { // q
                if (b.sq[59] == Piece::Empty && b.sq[58] == Piece::Empty && b.sq[57] == Piece::Empty) {
                    if (!b.inCheck(Color::Black) &&
                        !b.isSquareAttacked(59, Color::White) &&
                        !b.isSquareAttacked(58, Color::White)) {
                        out.push_back(Move{ (uint8_t)60, (uint8_t)58, false, Piece::Empty, false, true });
                    }
                }
            }
        }
    }
}

void MoveGen::generateAllPseudoMoves(const Board& b, vector<Move>& out) {
    out.clear();

    vector<Move> tmp;

    generatePawnPushes(b, tmp);
    out.insert(out.end(), tmp.begin(), tmp.end());

    generateKnightMoves(b, tmp);
    out.insert(out.end(), tmp.begin(), tmp.end());

    generateBishopMoves(b, tmp);
    out.insert(out.end(), tmp.begin(), tmp.end());

    generateRookMoves(b, tmp);
    out.insert(out.end(), tmp.begin(), tmp.end());

    generateQueenMoves(b, tmp);
    out.insert(out.end(), tmp.begin(), tmp.end());

    generateKingMoves(b, tmp);
    out.insert(out.end(), tmp.begin(), tmp.end());
}

void MoveGen::generateLegalMoves(Board& b, vector<Move>& out) {
    out.clear();

    vector<Move> pseudo;
    generateAllPseudoMoves(b, pseudo);

    Color us = b.sideToMove;

    for (const auto& m : pseudo) {
        Undo u;
        if (!b.makeMove(m, u)) continue;
        bool illegal = b.inCheck(us);

        b.unmakeMove(m, u);

        if (!illegal) {
            out.push_back(m);
        }
    }
}
