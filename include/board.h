#pragma once

#include "move.h"
#include "types.h"
#include <string>
#include <vector>

// Forward declaration for MoveGenerator
class MoveGenerator;

// Structure to hold information needed to undo a move
// This struct is now internal to the Board's logic, not passed externally by make/unmake.
struct UndoInfo {
    Color side_to_move;
    bool white_king_castle;
    bool white_queen_castle;
    bool black_king_castle;
    bool black_queen_castle;
    int en_passant_square;
    int halfmove_clock;
    int fullmove_number;
    int en_passant_capture_square; // Specific to en passant to restore the pawn
};

struct Board {
    Piece squares[64];
    Color side_to_move;

    bool white_king_castle;
    bool white_queen_castle;
    bool black_king_castle;
    bool black_queen_castle;

    int en_passant_square; // -1 if none, 0-63 if valid square
    int halfmove_clock;
    int fullmove_number;

    // Internal history stack for undo functionality
    std::vector<UndoInfo> history; 

    Board();

    void load_fen(const std::string& fen);
    std::string print_board() const;

    // Signatures remain unchanged as per your strict requirement
    bool make_move(const Move& move, Piece& captured_piece, Piece& moved_piece);
    void unmake_move(const Move& move, Piece captured_piece, Piece moved_piece);
    bool is_king_in_check(Color color) const;

    bool has_legal_moves();
    bool is_game_over();
    std::string get_result_string();
    bool is_square_attacked(int square, Color attacking_color) const;

    friend class MoveGenerator;
};