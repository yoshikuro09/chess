#include "board.h"
#include <sstream>
#include <vector>
#include <cstdlib> 
#include "move.h"

using namespace std;

static Piece pieceFromChar(char c) {
    switch (c) {
        case 'P': return Piece::WP; case 'N': return Piece::WN; case 'B': return Piece::WB;
        case 'R': return Piece::WR; case 'Q': return Piece::WQ; case 'K': return Piece::WK;
        case 'p': return Piece::BP; case 'n': return Piece::BN; case 'b': return Piece::BB;
        case 'r': return Piece::BR; case 'q': return Piece::BQ; case 'k': return Piece::BK;
        default:  return Piece::Empty;
    }
}

static char charFromPiece(Piece p) {
    switch (p) {
        case Piece::WP: return 'P'; case Piece::WN: return 'N'; case Piece::WB: return 'B';
        case Piece::WR: return 'R'; case Piece::WQ: return 'Q'; case Piece::WK: return 'K';
        case Piece::BP: return 'p'; case Piece::BN: return 'n'; case Piece::BB: return 'b';
        case Piece::BR: return 'r'; case Piece::BQ: return 'q'; case Piece::BK: return 'k';
        default:        return '.';
    }
}

static int squareFromAlg(const string& s) {
    if (s.size() != 2) return -1;
    char f = s[0], r = s[1];
    if (f < 'a' || f > 'h') return -1;
    if (r < '1' || r > '8') return -1;
    int file = f - 'a';
    int rank = r - '1';
    return rank * 8 + file; 
}

static vector<string> splitWS(const string& str) {
    istringstream in(str);
    vector<string> parts;
    string tok;
    while (in >> tok) parts.push_back(tok);
    return parts;
}

Board::Board() { setStartPos(); }

void Board::setStartPos() {
    setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

bool Board::setFromFEN(const string& fen) {
    auto parts = splitWS(fen);
    if (parts.size() < 4) return false;

    const string& placement = parts[0];
    const string& stm = parts[1];
    const string& castling = parts[2];
    const string& ep = parts[3];

    sq.fill(Piece::Empty);
    castlingRights = 0;
    enPassantSquare = -1;
    halfmoveClock = 0;
    fullmoveNumber = 1;

    int rank = 7;
    int file = 0;
    for (char c : placement) {
        if (c == '/') {
            rank--;
            file = 0;
            continue;
        }
        if (rank < 0) return false;

        if (c >= '1' && c <= '8') {
            file += (c - '0');
            if (file > 8) return false;
            continue;
        }

        Piece p = pieceFromChar(c);
        if (p == Piece::Empty) return false;
        if (file >= 8) return false;

        int sqIndex = rank * 8 + file;
        sq[sqIndex] = p;
        file++;
    }
    if (rank != 0 && rank != -1) {
        // Защита от кривой проэкции доски
    }

    if (stm == "w") sideToMove = Color::White;
    else if (stm == "b") sideToMove = Color::Black;
    else return false;

    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
                case 'K': castlingRights |= 1; break;
                case 'Q': castlingRights |= 2; break;
                case 'k': castlingRights |= 4; break;
                case 'q': castlingRights |= 8; break;
                default: return false;
            }
        }
    }

    if (ep != "-") {
        int s = squareFromAlg(ep);
        if (s < 0) return false;
        enPassantSquare = (int8_t)s;
    }

    if (parts.size() >= 5) {
        halfmoveClock = (uint16_t)stoi(parts[4]);
    }
    if (parts.size() >= 6) {
        fullmoveNumber = (uint16_t)stoi(parts[5]);
        if (fullmoveNumber == 0) fullmoveNumber = 1;
    }

    return true;
}

string Board::toString() const {
    ostringstream out;
    for (int r = 7; r >= 0; --r) {
        out << (r + 1) << " ";
        for (int f = 0; f < 8; ++f) {
            out << charFromPiece(sq[r * 8 + f]) << " ";
        }
        out << "\n";
    }
    out << "  a b c d e f g h\n";
    out << "Side: " << (sideToMove == Color::White ? "White" : "Black") << "\n";

    out << "Castling: ";
    if (castlingRights == 0) out << "-";
    else {
        if (castlingRights & 1) out << "K";
        if (castlingRights & 2) out << "Q";
        if (castlingRights & 4) out << "k";
        if (castlingRights & 8) out << "q";
    }
    out << "\n";

    out << "EP: " << (enPassantSquare >= 0 ? to_string(enPassantSquare) : string("-")) << "\n";
    out << "Halfmove: " << halfmoveClock << " Fullmove: " << fullmoveNumber << "\n";
    return out.str();
}

int Board::squareFromString(const string& s) const {
    if (s.size() != 2) return -1;
    char f = s[0], r = s[1];
    if (f < 'a' || f > 'h') return -1;
    if (r < '1' || r > '8') return -1;
    int file = f - 'a';
    int rank = r - '1';
    return rank * 8 + file;
}

bool Board::makeSimpleMove(const Move& m) {
    if (m.from >= 64 || m.to >= 64) return false;
    if (sq[m.from] == Piece::Empty) return false;

    sq[m.to] = sq[m.from];
    sq[m.from] = Piece::Empty;

    sideToMove = (sideToMove == Color::White)
                    ? Color::Black
                    : Color::White;

    return true;
}

bool Board::makeMove(const Move& m, Undo& u) {
    if (m.from >= 64 || m.to >= 64) return false;
    if (sq[m.from] == Piece::Empty) return false;

    u.moved = sq[m.from];
    u.prevCastlingRights = castlingRights;
    u.prevEnPassantSquare = enPassantSquare;
    u.prevHalfmoveClock = halfmoveClock;
    u.prevFullmoveNumber = fullmoveNumber;
    u.prevSideToMove = sideToMove;

    u.captured = Piece::Empty;
    u.capturedSquare = -1;

    u.wasCastling = false;
    u.rookFrom = -1;
    u.rookTo = -1;
    u.rookPiece = Piece::Empty;

    Piece movedBefore = sq[m.from];

    enPassantSquare = -1;

    // Обработка взятия
    if (m.isEnPassant) {
        int dir = (sideToMove == Color::White) ? 8 : -8;
        int capSq = (int)m.to - dir;
        if (capSq < 0 || capSq >= 64) return false;

        u.capturedSquare = (int8_t)capSq;
        u.captured = sq[capSq];
        sq[capSq] = Piece::Empty;
    } else {
        if (sq[m.to] != Piece::Empty) {
            u.capturedSquare = (int8_t)m.to;
            u.captured = sq[m.to];
        }
    }

    // Переносим фигуру
    sq[m.to] = sq[m.from];
    sq[m.from] = Piece::Empty;

    if (m.isCastling) {
        u.wasCastling = true;

        if (m.to == 6) {          
            u.rookFrom = 7; u.rookTo = 5;
        } else if (m.to == 2) {  
            u.rookFrom = 0; u.rookTo = 3;
        } else if (m.to == 62) { 
            u.rookFrom = 63; u.rookTo = 61;
        } else if (m.to == 58) {  
            u.rookFrom = 56; u.rookTo = 59;
        } else {
            return false;
        }

        u.rookPiece = sq[u.rookFrom];
        sq[u.rookTo] = sq[u.rookFrom];
        sq[u.rookFrom] = Piece::Empty;
    }

    if (m.promotion != Piece::Empty) {
        sq[m.to] = m.promotion;
    }

    if (movedBefore == Piece::WP || movedBefore == Piece::BP) {
        int diff = (int)m.to - (int)m.from;
        if (diff == 16 || diff == -16) {
            enPassantSquare = (int8_t)((int)m.from + diff / 2);
        }
    }

    if (movedBefore == Piece::WK) castlingRights &= ~uint8_t(1 | 2);
    if (movedBefore == Piece::BK) castlingRights &= ~uint8_t(4 | 8);

    if (movedBefore == Piece::WR) {
        if (m.from == 7) castlingRights &= ~uint8_t(1); // K
        if (m.from == 0) castlingRights &= ~uint8_t(2); // Q
    }
    if (movedBefore == Piece::BR) {
        if (m.from == 63) castlingRights &= ~uint8_t(4); // k
        if (m.from == 56) castlingRights &= ~uint8_t(8); // q
    }

    if (u.captured != Piece::Empty && u.capturedSquare >= 0) {
        if (u.captured == Piece::WR) {
            if (u.capturedSquare == 7) castlingRights &= ~uint8_t(1);
            if (u.capturedSquare == 0) castlingRights &= ~uint8_t(2);
        }
        if (u.captured == Piece::BR) {
            if (u.capturedSquare == 63) castlingRights &= ~uint8_t(4);
            if (u.capturedSquare == 56) castlingRights &= ~uint8_t(8);
        }
    }

    // если было взятие или ход пешкой — сброс
    Piece moved = u.moved; 
    bool pawnMoved = (moved == Piece::WP || moved == Piece::BP);
    bool wasCapture = (u.captured != Piece::Empty);
    halfmoveClock = (pawnMoved || wasCapture) ? 0 : (uint16_t)(halfmoveClock + 1);

    if (sideToMove == Color::Black) {
        fullmoveNumber = (uint16_t)(fullmoveNumber + 1);
    }

    // смена стороны
    sideToMove = (sideToMove == Color::White) ? Color::Black : Color::White;

    return true;
}

void Board::unmakeMove(const Move& m, const Undo& u) {
    sq[m.from] = u.moved;

    sq[m.to] = Piece::Empty;

    if (u.wasCastling) {
        sq[u.rookFrom] = u.rookPiece;
        sq[u.rookTo] = Piece::Empty;
    }

    if (u.capturedSquare >= 0) {
        sq[u.capturedSquare] = u.captured;
    }

    castlingRights = u.prevCastlingRights;
    enPassantSquare = u.prevEnPassantSquare;
    halfmoveClock = u.prevHalfmoveClock;
    fullmoveNumber = u.prevFullmoveNumber;
    sideToMove = u.prevSideToMove;
}


static bool isWhitePiece(Piece p) { return p >= Piece::WP && p <= Piece::WK; }
static bool isBlackPiece(Piece p) { return p >= Piece::BP && p <= Piece::BK; }

int Board::kingSquare(Color side) const {
    Piece k = (side == Color::White) ? Piece::WK : Piece::BK;
    for (int i = 0; i < 64; ++i) {
        if (sq[i] == k) return i;
    }
    return -1;
}

bool Board::inCheck(Color side) const {
    int ks = kingSquare(side);
    if (ks < 0) return false; 
    Color enemy = (side == Color::White) ? Color::Black : Color::White;
    return isSquareAttacked(ks, enemy);
}

bool Board::isSquareAttacked(int s, Color bySide) const {
    if (s < 0 || s >= 64) return false;

    bool byWhite = (bySide == Color::White);
    int f0 = fileOf(s);
    int r0 = rankOf(s);

    //  Ход Пешек
    if (byWhite) {
        if (f0 > 0) {
            int from = s - 9;
            if (from >= 0 && sq[from] == Piece::WP) return true;
        }
        if (f0 < 7) {
            int from = s - 7;
            if (from >= 0 && sq[from] == Piece::WP) return true;
        }
    } else {
        if (f0 > 0) {
            int from = s + 7;
            if (from < 64 && sq[from] == Piece::BP) return true;
        }
        if (f0 < 7) {
            int from = s + 9;
            if (from < 64 && sq[from] == Piece::BP) return true;
        }
    }

    // Ход Коня
    static const int kJumps[8] = { -17, -15, -10, -6, 6, 10, 15, 17 };
    Piece atkKnight = byWhite ? Piece::WN : Piece::BN;

    for (int d : kJumps) {
        int from = s + d;
        if (from < 0 || from >= 64) continue;

        int df = abs(fileOf(from) - f0);
        int dr = abs(rankOf(from) - r0);
        if (!((df == 1 && dr == 2) || (df == 2 && dr == 1))) continue;

        if (sq[from] == atkKnight) return true;
    }

    // Ход Короля
    Piece atkKing = byWhite ? Piece::WK : Piece::BK;
    for (int dr = -1; dr <= 1; ++dr) {
        for (int df = -1; df <= 1; ++df) {
            if (dr == 0 && df == 0) continue;
            int f = f0 + df;
            int r = r0 + dr;
            if (f < 0 || f > 7 || r < 0 || r > 7) continue;
            int from = r * 8 + f;
            if (sq[from] == atkKing) return true;
        }
    }

    // Ход Слона, Ферзя
    auto diagRay = [&](int df, int dr) -> bool {
        int f = f0, r = r0;
        while (true) {
            f += df; r += dr;
            if (f < 0 || f > 7 || r < 0 || r > 7) return false;
            int from = r * 8 + f;
            Piece p = sq[from];
            if (p == Piece::Empty) continue;
            if (byWhite) return (p == Piece::WB || p == Piece::WQ);
            else        return (p == Piece::BB || p == Piece::BQ);
        }
    };

    if (diagRay(+1, +1)) return true;
    if (diagRay(-1, +1)) return true;
    if (diagRay(+1, -1)) return true;
    if (diagRay(-1, -1)) return true;

    // Ход Ладьи, Ферзя
    auto lineRay = [&](int df, int dr) -> bool {
        int f = f0, r = r0;
        while (true) {
            f += df; r += dr;
            if (f < 0 || f > 7 || r < 0 || r > 7) return false;
            int from = r * 8 + f;
            Piece p = sq[from];
            if (p == Piece::Empty) continue;
            if (byWhite) return (p == Piece::WR || p == Piece::WQ);
            else        return (p == Piece::BR || p == Piece::BQ);
        }
    };

    if (lineRay(+1, 0)) return true;
    if (lineRay(-1, 0)) return true;
    if (lineRay(0, +1)) return true;
    if (lineRay(0, -1)) return true;

    return false;
}
