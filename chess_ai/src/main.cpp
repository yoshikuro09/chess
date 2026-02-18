#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "board.h"
#include "move.h"
#include "movegen.h"
#include "search.h"

using namespace std;

static string sqToAlg(int s) {
    char file = char('a' + (s & 7));
    char rank = char('1' + (s >> 3));
    return string() + file + rank;
}

static int algToSq(const string& a) {
    if (a.size() != 2) return -1;
    char f = a[0], r = a[1];
    if (f < 'a' || f > 'h') return -1;
    if (r < '1' || r > '8') return -1;
    int file = f - 'a';
    int rank = r - '1';
    return rank * 8 + file;
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

static Piece parsePromoChar(char c, bool white) {
    c = (char)toupper((unsigned char)c);

    if (white) {
        if (c == 'Q') return Piece::WQ;
        if (c == 'R') return Piece::WR;
        if (c == 'B') return Piece::WB;
        if (c == 'N') return Piece::WN;
    } else {
        if (c == 'Q') return Piece::BQ;
        if (c == 'R') return Piece::BR;
        if (c == 'B') return Piece::BB;
        if (c == 'N') return Piece::BN;
    }

    return Piece::Empty;
}

static bool parseUserMove(const Board& b, const string& input, Move& out) {

    string s = input;

    while (!s.empty() && isspace((unsigned char)s.back())) s.pop_back();
    size_t i = 0;
    while (i < s.size() && isspace((unsigned char)s[i])) i++;
    s = s.substr(i);

    if (s == "O-O" || s == "o-o") {
        if (b.sideToMove == Color::White)
            out = Move(4, 6, false, Piece::Empty, false, true);
        else
            out = Move(60, 62, false, Piece::Empty, false, true);
        return true;
    }

    if (s == "O-O-O" || s == "o-o-o") {
        if (b.sideToMove == Color::White)
            out = Move(4, 2, false, Piece::Empty, false, true);
        else
            out = Move(60, 58, false, Piece::Empty, false, true);
        return true;
    }

    s.erase(remove(s.begin(), s.end(), 'x'), s.end());

    Piece promo = Piece::Empty;

    auto eqPos = s.find('=');
    if (eqPos != string::npos) {
        if (eqPos + 1 >= s.size()) return false;

        bool white = (b.sideToMove == Color::White);
        promo = parsePromoChar(s[eqPos + 1], white);

        if (promo == Piece::Empty) return false;

        s = s.substr(0, eqPos);
    }

    if (s.size() != 4) return false;

    int from = algToSq(s.substr(0, 2));
    int to   = algToSq(s.substr(2, 2));

    if (from < 0 || to < 0) return false;

    out = Move((uint8_t)from, (uint8_t)to, false, promo, false, false);

    return true;
}

static bool sameMoveIgnoringFlags(const Move& a, const Move& b) {
    return a.from == b.from &&
           a.to == b.to &&
           a.promotion == b.promotion;
}

static void printGameState(Board& b) {
    cout << b.toString() << "\n";
    cout << "Turn: "
         << (b.sideToMove == Color::White ? "White" : "Black")
         << "\n";
}

static bool checkEnd(Board& b) {

    vector<Move> legal;
    MoveGen::generateLegalMoves(b, legal);

    if (!legal.empty()) return false;

    Color side = b.sideToMove;

    if (b.inCheck(side)) {
        cout << "CHECKMATE! Winner: "
             << (side == Color::White ? "Black" : "White")
             << "\n";
    } else {
        cout << "STALEMATE! Draw.\n";
    }

    return true;
}

int main() {

    Board b;
    b.setStartPos();

    bool humanIsWhite = true;

    while (true) {

        printGameState(b);

        if (checkEnd(b)) break;

        bool humanTurn =
            (b.sideToMove == Color::White) == humanIsWhite;

        if (humanTurn) {

            vector<Move> legal;
            MoveGen::generateLegalMoves(b, legal);

            cout << "Enter move (e2e4, e7e8=Q, O-O, O-O-O)\n";
            cout << "Type 'moves' or 'quit'\n> ";

            string inp;
            getline(cin, inp);

            if (inp == "quit") break;

            if (inp == "moves") {
                for (auto& m : legal)
                    cout << moveToStr(m) << "\n";
                cout << "\n";
                continue;
            }

            Move um;

            if (!parseUserMove(b, inp, um)) {
                cout << "Bad format\n\n";
                continue;
            }

            bool found = false;
            Move chosen;

            for (const auto& m : legal) {
                if (sameMoveIgnoringFlags(m, um)) {
                    chosen = m;
                    found = true;
                    break;
                }
            }

            if (!found) {
                cout << "Illegal move\n\n";
                continue;
            }

            Undo u;
            b.makeMove(chosen, u);

        } else {

            cout << "AI thinking...\n";

            int maxDepth = 6;
            int timeMs   = 800;

            auto r =
                Search::findBestMoveTimed(b, maxDepth, timeMs);

            cout << "AI plays: "
                 << moveToStr(r.best)
                 << " (score=" << r.score
                 << ", nodes=" << r.nodes
                 << ", depth=" << r.depthDone
                 << ")\n\n";

            Undo u;
            b.makeMove(r.best, u);
        }
    }

    return 0;
}
