// main_gui.cpp  (SFML 3.0.2)
// GUI: шахматная доска + Unicode-фигуры + подсветка легальных ходов + игра против ИИ (Search::findBestMoveTimed)

#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <filesystem>
#include <cstdint>
#include <cmath>

#include "board.h"
#include "move.h"
#include "movegen.h"
#include "search.h"

using namespace std;

static char32_t pieceGlyph(Piece p) {
    switch (p) {
        case Piece::WP: return U'\u2659'; // ♙
        case Piece::WN: return U'\u2658'; // ♘
        case Piece::WB: return U'\u2657'; // ♗
        case Piece::WR: return U'\u2656'; // ♖
        case Piece::WQ: return U'\u2655'; // ♕
        case Piece::WK: return U'\u2654'; // ♔

        case Piece::BP: return U'\u265F'; // ♟
        case Piece::BN: return U'\u265E'; // ♞
        case Piece::BB: return U'\u265D'; // ♝
        case Piece::BR: return U'\u265C'; // ♜
        case Piece::BQ: return U'\u265B'; // ♛
        case Piece::BK: return U'\u265A'; // ♚
        default: return 0;
    }
}

static bool isWhitePiece(Piece p) {
    return (p >= Piece::WP && p <= Piece::WK);
}
static bool isBlackPiece(Piece p) {
    return (p >= Piece::BP && p <= Piece::BK);
}

static sf::String glyphToSfString(char32_t g) {
    std::u32string u32;
    u32.push_back(g);
    return sf::String::fromUtf32(u32.begin(), u32.end());
}

static inline int fileOf(int sq) { return sq & 7; }
static inline int rankOf(int sq) { return sq >> 3; }

static sf::Vector2f squareTopLeft(int sq, float tile) {
    int f = fileOf(sq);
    int r = rankOf(sq);
    float x = f * tile;
    float y = (7 - r) * tile;
    return sf::Vector2f(x, y);
}

static int squareFromMouse(sf::Vector2i mouse, float tile) {
    int f = (int)std::floor(mouse.x / tile);
    int y = (int)std::floor(mouse.y / tile);
    if (f < 0 || f > 7 || y < 0 || y > 7) return -1;

    int r = 7 - y;
    return r * 8 + f;
}

static bool sameMoveBasic(const Move& a, const Move& b) {
    return a.from == b.from && a.to == b.to && a.promotion == b.promotion
        && a.isCastling == b.isCastling && a.isEnPassant == b.isEnPassant;
}

static bool tryOpenFont(sf::Font& font, const std::vector<std::string>& paths, std::string& usedPath) {
    namespace fs = std::filesystem;
    for (const auto& p : paths) {
        std::error_code ec;
        if (!fs::exists(p, ec)) continue;
        if (font.openFromFile(p)) {
            usedPath = p;
            return true;
        }
    }
    return false;
}

static Piece autoPromotion(Color side) {
    return (side == Color::White) ? Piece::WQ : Piece::BQ;
}

int main() {
    const float tile = 96.0f;
    const unsigned W = (unsigned)std::lround(tile * 8.0f);
    const unsigned H = (unsigned)std::lround(tile * 8.0f);

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(W, H)), "Chess GUI (SFML 3.0.2)");
    window.setFramerateLimit(60);

    sf::Font font;
    std::string fontPathUsed;
    {

        std::vector<std::string> candidates = {
            "assets/fonts/DejaVuSans.ttf",
            "../assets/fonts/DejaVuSans.ttf",
            "../../assets/fonts/DejaVuSans.ttf",
            "C:/Windows/Fonts/seguisym.ttf",        
            "C:/Windows/Fonts/SegoeUISymbol.ttf"
        };

        if (!tryOpenFont(font, candidates, fontPathUsed)) {
            std::cerr << "Failed to load font.\n";
            std::cerr << "Put a Unicode TTF (e.g. DejaVuSans.ttf) into assets/fonts/ and rebuild/run.\n";
            std::cerr << "Tried paths:\n";
            for (auto& p : candidates) std::cerr << "  " << p << "\n";
            return 1;
        } else {
            std::cout << "Font loaded: " << fontPathUsed << "\n";
        }
    }

    Board b;
    b.setStartPos();

    bool humanIsWhite = true;  

    int aiMaxDepth = 7;
    int aiTimeMs   = 800;

    int selectedSq = -1;
    std::vector<Move> legalMovesCache;
    std::vector<Move> selectedMoves; 

    auto rebuildLegal = [&]() {
        legalMovesCache.clear();
        MoveGen::generateLegalMoves(b, legalMovesCache);
    };

    auto rebuildSelectedMoves = [&]() {
        selectedMoves.clear();
        if (selectedSq < 0) return;
        for (const auto& m : legalMovesCache) {
            if (m.from == (uint8_t)selectedSq) selectedMoves.push_back(m);
        }
    };

    auto isHumanTurn = [&]() -> bool {
        return (b.sideToMove == Color::White) == humanIsWhite;
    };

    auto applyMoveIfLegal = [&](int fromSq, int toSq) -> bool {
        std::vector<Move> candidates;
        for (const auto& m : legalMovesCache) {
            if ((int)m.from == fromSq && (int)m.to == toSq) {
                candidates.push_back(m);
            }
        }
        if (candidates.empty()) return false;

        Move chosen = candidates[0];
        if (candidates.size() > 1) {
            Piece want = autoPromotion(b.sideToMove);
            for (const auto& m : candidates) {
                if (m.promotion == want) { chosen = m; break; }
            }
        }

        Undo u;
        if (!b.makeMove(chosen, u)) return false;
        return true;
    };

    auto checkGameEnd = [&]() -> bool {
        std::vector<Move> lm;
        MoveGen::generateLegalMoves(b, lm);
        if (!lm.empty()) return false;

        Color side = b.sideToMove;
        if (b.inCheck(side)) {
            std::cout << "CHECKMATE! Winner: " << (side == Color::White ? "Black" : "White") << "\n";
        } else {
            std::cout << "STALEMATE!\n";
        }
        return true;
    };

    rebuildLegal();

    while (window.isOpen()) {
        if (!isHumanTurn()) {
            if (checkGameEnd()) {
            } else {
                auto r = Search::findBestMoveTimed(b, aiMaxDepth, aiTimeMs);
                Undo u;
                b.makeMove(r.best, u);

                std::cout << "AI: from=" << (int)r.best.from << " to=" << (int)r.best.to
                          << " score=" << r.score
                          << " nodes=" << r.nodes
                          << " depthDone=" << r.depthDone
                          << " timedOut=" << (r.timedOut ? "YES" : "NO")
                          << "\n";

                selectedSq = -1;
                selectedMoves.clear();
                rebuildLegal();
            }
        }

        while (true) {
            std::optional<sf::Event> ev = window.pollEvent();
            if (!ev.has_value()) break;

            const sf::Event& e = *ev;

            if (e.is<sf::Event::Closed>()) {
                window.close();
            }

            if (e.is<sf::Event::MouseButtonPressed>()) {
                const auto& mb = e.getIf<sf::Event::MouseButtonPressed>();
                if (!mb) continue;

                if (mb->button == sf::Mouse::Button::Left) {
                    if (!isHumanTurn()) continue; 

                    int sq = squareFromMouse(sf::Vector2i(mb->position.x, mb->position.y), tile);
                    if (sq < 0) continue;

                    Piece p = b.sq[sq];

                    if (selectedSq < 0) {
                        bool ok =
                            (b.sideToMove == Color::White && isWhitePiece(p)) ||
                            (b.sideToMove == Color::Black && isBlackPiece(p));

                        if (ok) {
                            selectedSq = sq;
                            rebuildSelectedMoves();
                        }
                    } else {
                        if (applyMoveIfLegal(selectedSq, sq)) {
                            selectedSq = -1;
                            selectedMoves.clear();
                            rebuildLegal();
                            (void)checkGameEnd();
                        } else {
                            bool ok =
                                (b.sideToMove == Color::White && isWhitePiece(p)) ||
                                (b.sideToMove == Color::Black && isBlackPiece(p));

                            if (ok) {
                                selectedSq = sq;
                                rebuildSelectedMoves();
                            } else {
                                selectedSq = -1;
                                selectedMoves.clear();
                            }
                        }
                    }
                }
            }
        }

        window.clear(sf::Color(20, 20, 20));

        sf::RectangleShape cell(sf::Vector2f(tile, tile));
        for (int sq = 0; sq < 64; ++sq) {
            int f = fileOf(sq);
            int r = rankOf(sq);
            bool dark = ((f + r) & 1) != 0;

            auto pos = squareTopLeft(sq, tile);
            cell.setPosition(pos);

            if (dark) cell.setFillColor(sf::Color(118, 150, 86));
            else      cell.setFillColor(sf::Color(238, 238, 210));

            window.draw(cell);
        }

        if (selectedSq >= 0) {
            sf::RectangleShape sel(sf::Vector2f(tile, tile));
            sel.setPosition(squareTopLeft(selectedSq, tile));
            sel.setFillColor(sf::Color(255, 255, 0, 60));
            window.draw(sel);
        }

        for (const auto& m : selectedMoves) {
            int to = (int)m.to;
            auto pos = squareTopLeft(to, tile);

            if (m.isCapture || m.isEnPassant) {
                sf::CircleShape ring(tile * 0.32f);
                ring.setFillColor(sf::Color(0, 0, 0, 0));
                ring.setOutlineThickness(tile * 0.06f);
                ring.setOutlineColor(sf::Color(220, 60, 60, 180));
                ring.setPosition(sf::Vector2f(
                    pos.x + tile * 0.5f - ring.getRadius(),
                    pos.y + tile * 0.5f - ring.getRadius()
                ));
                window.draw(ring);
            } else {
                sf::CircleShape dot(tile * 0.12f);
                dot.setFillColor(sf::Color(0, 0, 0, 120));
                dot.setPosition(sf::Vector2f(
                    pos.x + tile * 0.5f - dot.getRadius(),
                    pos.y + tile * 0.5f - dot.getRadius()
                ));
                window.draw(dot);
            }
        }

        for (int sq = 0; sq < 64; ++sq) {
            Piece p = b.sq[sq];
            char32_t g = pieceGlyph(p);
            if (!g) continue;

            auto pos = squareTopLeft(sq, tile);

            sf::Text t(font);
            t.setString(glyphToSfString(g));
            t.setCharacterSize((unsigned)std::lround(tile * 0.80f));

            if (isWhitePiece(p)) t.setFillColor(sf::Color(245, 245, 245));
            else                 t.setFillColor(sf::Color(25, 25, 25));

            sf::FloatRect bounds = t.getLocalBounds();
            t.setPosition(sf::Vector2f(
                pos.x + tile * 0.5f - (bounds.position.x + bounds.size.x * 0.5f),
                pos.y + tile * 0.5f - (bounds.position.y + bounds.size.y * 0.60f)
            ));

            window.draw(t);
        }

        window.display();
    }

    return 0;
}
