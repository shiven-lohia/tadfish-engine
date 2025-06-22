#include "movegen.h"
#include "board.h"
#include "precomputed_moves.h"
#include <cassert>
using namespace std;

MoveGenerator::MoveGenerator(Board& b) : board(b) {}

std::vector<Move> MoveGenerator::generate_legal_moves() {
    std::vector<Move> pseudo = generate_pseudo_legal_moves();
    std::vector<Move> legal;

    for (const Move& m : pseudo) {
        Piece captured, moved;
        Color us = board.side_to_move;

        if (!board.make_move(m, captured, moved)) continue;

        if (!board.is_king_in_check(us)) {
            legal.push_back(m);
        }

        board.unmake_move(m, captured, moved);
    }

    return legal;
}

std::vector<Move> MoveGenerator::generate_pseudo_legal_moves() {
    std::vector<Move> moves;

    generate_pawn_moves(moves);
    generate_knight_moves(moves);
    generate_bishop_moves(moves);
    generate_rook_moves(moves);
    generate_queen_moves(moves);
    generate_king_moves(moves);
    generate_castling_moves(moves);

    return moves;
}

void MoveGenerator::generate_pawn_moves(std::vector<Move>& moves) {
    int direction = board.side_to_move == WHITE ? 8 : -8;
    int start_rank = board.side_to_move == WHITE ? 1 : 6;
    int promotion_rank = board.side_to_move == WHITE ? 7 : 0;

    Piece our_pawn = board.side_to_move == WHITE ? WP : BP;
    int their_min = board.side_to_move == WHITE ? BP : WP;
    int their_max = board.side_to_move == WHITE ? BK : WK;
    Color us = board.side_to_move;

    for (int sq = 0; sq < 64; ++sq) {
        if (board.squares[sq] != our_pawn)
            continue;

        int file = sq % 8;
        int rank = sq / 8;

        int one_step = sq + direction;
        if (one_step >= 0 && one_step < 64 && board.squares[one_step] == EMPTY) {
            int dest_rank = one_step / 8;
            if (dest_rank == promotion_rank) {
                moves.emplace_back(sq, one_step, us == WHITE ? WQ : BQ);
                moves.emplace_back(sq, one_step, us == WHITE ? WR : BR);
                moves.emplace_back(sq, one_step, us == WHITE ? WB : BB);
                moves.emplace_back(sq, one_step, us == WHITE ? WN : BN);
            } else {
                moves.emplace_back(sq, one_step);
            }

            if (rank == start_rank) {
                int two_step = sq + 2 * direction;
                if (board.squares[two_step] == EMPTY)
                    moves.emplace_back(sq, two_step);
            }
        }

        int capture_left = (file > 0) ? sq + direction - 1 : -1;
        int capture_right = (file < 7) ? sq + direction + 1 : -1;

        for (int target : {capture_left, capture_right}) {
            if (target < 0 || target >= 64) continue;

            Piece tp = board.squares[target];
            if (tp >= their_min && tp <= their_max) {
                int dest_rank = target / 8;
                if (dest_rank == promotion_rank) {
                    moves.emplace_back(sq, target, us == WHITE ? WQ : BQ);
                    moves.emplace_back(sq, target, us == WHITE ? WR : BR);
                    moves.emplace_back(sq, target, us == WHITE ? WB : BB);
                    moves.emplace_back(sq, target, us == WHITE ? WN : BN);
                } else {
                    moves.emplace_back(sq, target);
                }
            }
        }
    }
}

void MoveGenerator::generate_knight_moves(std::vector<Move>& moves) {
    for (int sq = 0; sq < 64; ++sq) {
        Piece p = board.squares[sq];
        if (p == EMPTY || (board.side_to_move == WHITE && p >= BP) || (board.side_to_move == BLACK && p <= WK))
            continue;
        if ((p != WN && p != BN))
            continue;

        for (int i = 0; i < 8 && KNIGHT_MOVES[sq][i] != -1; ++i) {
            int target = KNIGHT_MOVES[sq][i];
            Piece tp = board.squares[target];
            if (tp == EMPTY || ((board.side_to_move == WHITE && tp >= BP) || (board.side_to_move == BLACK && tp <= WK))) {
                moves.emplace_back(sq, target);
            }
        }
    }
}

void MoveGenerator::generate_bishop_moves(std::vector<Move>& moves) {
    for (int sq = 0; sq < 64; ++sq) {
        Piece p = board.squares[sq];
        if (p == EMPTY || (board.side_to_move == WHITE && p >= BP) || (board.side_to_move == BLACK && p <= WK))
            continue;
        if ((p != WB && p != BB) && (p != WQ && p != BQ))
            continue;

        for (int d = 0; d < 4; ++d) {
            for (int i = 0; i < 7 && BISHOP_MOVES[sq][d][i] != -1; ++i) {
                int target = BISHOP_MOVES[sq][d][i];
                Piece tp = board.squares[target];
                if (tp == EMPTY) {
                    moves.emplace_back(sq, target);
                } else {
                    if ((board.side_to_move == WHITE && tp >= BP) || (board.side_to_move == BLACK && tp <= WK)) {
                        moves.emplace_back(sq, target);
                    }
                    break;
                }
            }
        }
    }
}

void MoveGenerator::generate_rook_moves(std::vector<Move>& moves) {
    for (int sq = 0; sq < 64; ++sq) {
        Piece p = board.squares[sq];
        if (p == EMPTY || (board.side_to_move == WHITE && p >= BP) || (board.side_to_move == BLACK && p <= WK))
            continue;
        if ((p != WR && p != BR) && (p != WQ && p != BQ))
            continue;

        for (int d = 0; d < 4; ++d) {
            for (int i = 0; i < 7 && ROOK_MOVES[sq][d][i] != -1; ++i) {
                int target = ROOK_MOVES[sq][d][i];
                Piece tp = board.squares[target];
                if (tp == EMPTY) {
                    moves.emplace_back(sq, target);
                } else {
                    if ((board.side_to_move == WHITE && tp >= BP) || (board.side_to_move == BLACK && tp <= WK)) {
                        moves.emplace_back(sq, target);
                    }
                    break;
                }
            }
        }
    }
}

void MoveGenerator::generate_queen_moves(std::vector<Move>& moves) {
    generate_bishop_moves(moves);
    generate_rook_moves(moves);
}

void MoveGenerator::generate_king_moves(std::vector<Move>& moves) {
    static const int offsets[8] = {8, -8, 1, -1, 9, -9, 7, -7};

    for (int sq = 0; sq < 64; ++sq) {
        Piece p = board.squares[sq];
        if (p == EMPTY || (board.side_to_move == WHITE && p >= BP) || (board.side_to_move == BLACK && p <= WK))
            continue;
        if ((p != WK && p != BK))
            continue;

        int rank = sq / 8, file = sq % 8;
        for (int offset : offsets) {
            int target = sq + offset;
            if (target < 0 || target >= 64) continue;
            int trank = target / 8;
            int tfile = target % 8;
            if (abs(trank - rank) > 1 || abs(tfile - file) > 1) continue;

            Piece tp = board.squares[target];
            if (tp == EMPTY || ((board.side_to_move == WHITE && tp >= BP) || (board.side_to_move == BLACK && tp <= WK))) {
                moves.emplace_back(sq, target);
            }
        }
    }
}

void MoveGenerator::generate_castling_moves(std::vector<Move>& moves) {
    Color us = board.side_to_move;
    Color opp = (us == WHITE ? BLACK : WHITE);

    if (us == WHITE) {
        // King and rook must be on their squares:
        if (board.white_king_castle
            && board.squares[E1] == WK
            && board.squares[H1] == WR
            && board.squares[F1] == EMPTY
            && board.squares[G1] == EMPTY
            && !board.is_square_attacked(E1, opp)
            && !board.is_square_attacked(F1, opp)
            && !board.is_square_attacked(G1, opp))
        {
            moves.emplace_back(E1, G1);
        }
        if (board.white_queen_castle
            && board.squares[E1] == WK
            && board.squares[A1] == WR
            && board.squares[D1] == EMPTY
            && board.squares[C1] == EMPTY
            && board.squares[B1] == EMPTY
            && !board.is_square_attacked(E1, opp)
            && !board.is_square_attacked(D1, opp)
            && !board.is_square_attacked(C1, opp))
        {
            moves.emplace_back(E1, C1);
        }
    } else {
        if (board.black_king_castle
            && board.squares[E8] == BK
            && board.squares[H8] == BR
            && board.squares[F8] == EMPTY
            && board.squares[G8] == EMPTY
            && !board.is_square_attacked(E8, opp)
            && !board.is_square_attacked(F8, opp)
            && !board.is_square_attacked(G8, opp))
        {
            moves.emplace_back(E8, G8);
        }
        if (board.black_queen_castle
            && board.squares[E8] == BK
            && board.squares[A8] == BR
            && board.squares[D8] == EMPTY
            && board.squares[C8] == EMPTY
            && board.squares[B8] == EMPTY
            && !board.is_square_attacked(E8, opp)
            && !board.is_square_attacked(D8, opp)
            && !board.is_square_attacked(C8, opp))
        {
            moves.emplace_back(E8, C8);
        }
    }
}


std::vector<Move> MoveGenerator::generate_pseudo_legal_attack_moves() {
    std::vector<Move> moves;

    generate_pawn_moves(moves);
    generate_knight_moves(moves);
    generate_bishop_moves(moves);
    generate_rook_moves(moves);
    generate_queen_moves(moves);
    generate_king_moves(moves);
    // !DO NOT CALL generate_castling_moves(moves) here! (stack overflow)

    return moves;
}