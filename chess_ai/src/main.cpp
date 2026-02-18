#include <iostream>
#include <vector>
#include <string>
#include "board.h"
#include "move.h"
#include "movegen.h"

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

int main() {
    Board b;
    string fen = "8/4P3/8/8/8/8/8/4k3 w - - 0 1";
    b.setFromFEN(fen);

    cout << b.toString() << "\n";
    cout << "Black in check? " << (b.inCheck(Color::Black) ? "YES" : "NO") << "\n";

    vector<Move> legal;
    MoveGen::generateLegalMoves(b, legal);

    cout << "Legal moves count: " << legal.size() << "\n";
    for (const auto& m : legal) {
        if (m.isCapture)
            cout << sqToAlg(m.from) << "x" << sqToAlg(m.to);
        else
            cout << sqToAlg(m.from) << sqToAlg(m.to);

        if (m.promotion != Piece::Empty) {
            cout << "=" << promoChar(m.promotion);
        }
        cout << "\n";
    }

    return 0;
}
