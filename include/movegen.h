#pragma once

#include "board.h"
#include <vector>

class MoveGenerator {
public:
    explicit MoveGenerator(Board& b);  // Note: not const

    std::vector<Move> generate_legal_moves();
    std::vector<Move> generate_pseudo_legal_moves();
    std::vector<Move> generate_pseudo_legal_attack_moves();

private:
    Board& board;

    void generate_pawn_moves(std::vector<Move>& moves);
    void generate_knight_moves(std::vector<Move>& moves);
    void generate_bishop_moves(std::vector<Move>& moves);
    void generate_rook_moves(std::vector<Move>& moves);
    void generate_queen_moves(std::vector<Move>& moves);
    void generate_king_moves(std::vector<Move>& moves);
    void generate_castling_moves(std::vector<Move>& moves);
};
