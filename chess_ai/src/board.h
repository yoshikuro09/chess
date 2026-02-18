#pragma once
#include <array>
#include <string>
#include <cstdint>

enum class Piece : int8_t {
    Empty = 0,
    WP, WN, WB, WR, WQ, WK,
    BP, BN, BB, BR, BQ, BK
};

enum class Color : uint8_t { White, Black };

struct Undo {
    Piece moved = Piece::Empty;
    Piece captured = Piece::Empty;
    int8_t capturedSquare = -1;
    
    bool wasCastling = false;
    int8_t rookFrom = -1;
    int8_t rookTo = -1;
    Piece rookPiece = Piece::Empty;
    
    uint8_t prevCastlingRights = 0;
    int8_t prevEnPassantSquare = -1;
    uint16_t prevHalfmoveClock = 0;
    uint16_t prevFullmoveNumber = 1;
    Color prevSideToMove = Color::White;
};

struct Move;
struct Undo;

class Board {
public:

    std::array<Piece, 64> sq{};

    Color sideToMove = Color::White;

    uint8_t castlingRights = 0;

    int8_t enPassantSquare = -1;

    uint16_t halfmoveClock = 0;
    uint16_t fullmoveNumber = 1;

    Board();
    
    bool makeSimpleMove(const Move& m);
    int squareFromString(const std::string& s) const;
    void setStartPos();
    bool setFromFEN(const std::string& fen);
    int kingSquare(Color side) const;
    bool isSquareAttacked(int square, Color bySide) const;
    bool inCheck(Color side) const;
    bool makeMove(const Move& m, Undo& u);
    void unmakeMove(const Move& m, const Undo& u);

    std::string toString() const;

    static int fileOf(int s) { return s & 7; }
    static int rankOf(int s) { return s >> 3; }
};
