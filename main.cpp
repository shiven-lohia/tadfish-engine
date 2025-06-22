#include "board.h"
#include "search.h"
#include "move.h"
#include "movegen.h"
#include "eval.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

// Convert 0–63 square index to UCI coordinate
static std::string square_to_coord(int sq) {
    int file = sq % 8;
    int rank = sq / 8;
    return std::string{char('a' + file), char('1' + rank)};
}

// Convert Move to UCI string
static std::string move_to_uci(const Move& m) {
    std::string u = square_to_coord(m.from) + square_to_coord(m.to);
    if (m.promotion != EMPTY) {
        char promo_char = 'q';
        switch (m.promotion) {
            case WN: case BN: promo_char = 'n'; break;
            case WB: case BB: promo_char = 'b'; break;
            case WR: case BR: promo_char = 'r'; break;
            case WQ: case BQ: promo_char = 'q'; break;
            default: break;
        }
        u += promo_char;
    }
    return u;
}

int main(int argc, char* argv[]) {
    if (argc >= 2) {
        std::string fen = argv[1];
        int depth = 1;
        if (argc >= 3) {
            try {
                depth = std::stoi(argv[2]);
            } catch (...) {
                depth = 5;
            }
        }

        Board board;
        board.load_fen(fen);

        // Generate legal moves
        MoveGenerator gen(board);
        auto legal = gen.generate_legal_moves();
        std::cerr << "Legal moves (" << legal.size() << "): ";
        for (const auto& m : legal) {
            std::cerr << move_to_uci(m) << " ";
        }
        std::cerr << "\n";

        // Find best move
        Move best = find_best_move(board, depth);
        std::cerr << "Best move: " << move_to_uci(best) << "\n";

        // Apply the best move to get evaluation after the move
        Piece captured, moved;
        if (!board.make_move(best, captured, moved)) {
            std::cout << move_to_uci(best) << " 0\n";
            return 0;
        }

        int eval_score = evaluate(board);  // White-positive centipawn score
        board.unmake_move(best, captured, moved);

        std::cout << move_to_uci(best) << " " << eval_score << "\n";
        return 0;
    }

    std::cerr << "Usage: chess.exe \"<FEN>\" <depth>\n";
    return 1;
}
