#include "eval.h"
#include "pst.h"
#include "board.h"
#include "types.h"
#include <array>
#include <cmath>

const int PAWN_VALUE   = 100;
const int KNIGHT_VALUE = 320;
const int BISHOP_VALUE = 330;
const int ROOK_VALUE   = 500;
const int QUEEN_VALUE  = 900;

// Helper: Detect color of a piece
Color piece_color(Piece p) {
    if (p >= WP && p <= WK) return WHITE;
    if (p >= BP && p <= BK) return BLACK;
    return WHITE; // default fallback
}

// Helper: Get piece material value
int piece_value(Piece p) {
    switch (p) {
        case WP: case BP: return PAWN_VALUE;
        case WN: case BN: return KNIGHT_VALUE;
        case WB: case BB: return BISHOP_VALUE;
        case WR: case BR: return ROOK_VALUE;
        case WQ: case BQ: return QUEEN_VALUE;
        default: return 0;
    }
}

// Simple mobility estimator: count attacks (pseudo-mobility)
int mobility_score(const Board& board, Color side) {
    static const int knight_offsets[8] = { 17, 15, 10, 6, -17, -15, -10, -6 };
    static const int bishop_dirs[4] = { 9, 7, -9, -7 };
    static const int rook_dirs[4] = { 8, -8, 1, -1 };
    static const int queen_dirs[8] = { 8, -8, 1, -1, 9, 7, -9, -7 };

    int score = 0;

    for (int sq = 0; sq < 64; ++sq) {
        Piece p = board.squares[sq];
        if (p == EMPTY || piece_color(p) != side) continue;

        switch (p) {
            case WN: case BN:
                for (int off : knight_offsets) {
                    int t = sq + off;
                    if (t >= 0 && t < 64 && board.squares[t] == EMPTY)
                        score += 4;
                }
                break;
            case WB: case BB:
                for (int dir : bishop_dirs) {
                    int t = sq + dir;
                    while (t >= 0 && t < 64 && board.squares[t] == EMPTY) {
                        score += 2;
                        t += dir;
                    }
                }
                break;
            case WR: case BR:
                for (int dir : rook_dirs) {
                    int t = sq + dir;
                    while (t >= 0 && t < 64 && board.squares[t] == EMPTY) {
                        score += 2;
                        t += dir;
                    }
                }
                break;
            case WQ: case BQ:
                for (int dir : queen_dirs) {
                    int t = sq + dir;
                    while (t >= 0 && t < 64 && board.squares[t] == EMPTY) {
                        score += 1;
                        t += dir;
                    }
                }
                break;
            case WK: case BK:
                score += 0; // King mobility not counted
                break;
            default:
                break;
        }
    }
    return score;
}

// Check if file is open/semi-open for rook bonus
int rook_file_bonus(const Board& board, int file, Color color) {
    bool has_own_pawn = false, has_enemy_pawn = false;
    for (int rank = 0; rank < 8; ++rank) {
        Piece p = board.squares[rank * 8 + file];
        if (p == EMPTY) continue;
        if ((p == WP && color == WHITE) || (p == BP && color == BLACK)) has_own_pawn = true;
        if ((p == WP && color == BLACK) || (p == BP && color == WHITE)) has_enemy_pawn = true;
    }

    if (!has_own_pawn && !has_enemy_pawn) return 15;       // open file
    if (!has_own_pawn && has_enemy_pawn) return 10;        // semi-open
    return 0;
}

// Pawn structure: doubled pawn penalty, isolated penalty
int pawn_structure_penalty(const Board& board, Color side) {
    std::array<int, 8> file_counts = {0};

    for (int sq = 0; sq < 64; ++sq) {
        Piece p = board.squares[sq];
        if ((side == WHITE && p == WP) || (side == BLACK && p == BP)) {
            int file = sq % 8;
            file_counts[file]++;
        }
    }

    int penalty = 0;
    for (int f = 0; f < 8; ++f) {
        if (file_counts[f] > 1) penalty -= 10 * (file_counts[f] - 1); // doubled
        if (file_counts[f] > 0 && (f == 0 || f == 7 || (file_counts[f - 1] == 0 && file_counts[f + 1] == 0)))
            penalty -= 15; // isolated
    }

    return penalty;
}

int evaluate(const Board& board) {
    int score = 0;

    for (int sq = 0; sq < 64; ++sq) {
        Piece p = board.squares[sq];
        if (p == EMPTY) continue;

        Color color = piece_color(p);
        int base = piece_value(p);

        // PST bonus: use pawn_table, knight_table, etc.
        int pst_bonus = 0;
        int mirrored_sq = (color == WHITE) ? sq : ((7 - (sq / 8)) * 8 + (sq % 8));
        switch (p) {
            case WP: case BP: pst_bonus = pawn_table[mirrored_sq]; break;
            case WN: case BN: pst_bonus = knight_table[mirrored_sq]; break;
            case WB: case BB: pst_bonus = bishop_table[mirrored_sq]; break;
            case WR: case BR: pst_bonus = rook_table[mirrored_sq]; break;
            case WQ: case BQ: pst_bonus = queen_table[mirrored_sq]; break;
            case WK: case BK: pst_bonus = king_table[mirrored_sq]; break;
            default: break;
        }

        int piece_score = base + pst_bonus;
        score += (color == WHITE) ? piece_score : -piece_score;

        // Rook open/semi-open file
        if (p == WR || p == BR) {
            int file = sq % 8;
            int bonus = rook_file_bonus(board, file, color);
            score += (color == WHITE) ? bonus : -bonus;
        }
    }

    // Add pawn structure and mobility
    score += pawn_structure_penalty(board, WHITE);
    score -= pawn_structure_penalty(board, BLACK);
    score += mobility_score(board, WHITE);
    score -= mobility_score(board, BLACK);

    // avoiding repitition
    score += board.halfmove_clock%2;

    // Flip if black to move
    return (board.side_to_move == WHITE) ? score : -score;
}
