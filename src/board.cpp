#include "board.h"
#include "move.h"
#include "movegen.h" // Assuming movegen.h defines MoveGenerator
#include <sstream>
#include <iostream>
#include <cctype>

using namespace std;

Board::Board() {
    side_to_move = WHITE;
    white_king_castle = white_queen_castle = true;
    black_king_castle = black_queen_castle = true;
    en_passant_square = -1;
    halfmove_clock = 0;
    fullmove_number = 1;
    // history is default-constructed (empty)

    for (int i = 0; i < 64; ++i)
        squares[i] = EMPTY;
}

void Board::load_fen(const string& fen) {
    // Reset board state
    for (int i = 0; i < 64; ++i) squares[i] = EMPTY;
    side_to_move = WHITE;
    white_king_castle = white_queen_castle = true;
    black_king_castle = black_queen_castle = true;
    en_passant_square = -1;
    halfmove_clock = 0;
    fullmove_number = 1;
    history.clear(); // Clear history on FEN load

    istringstream iss(fen);
    string board_part, turn, castling, en_passant, halfmove, fullmove;
    if (!(iss >> board_part >> turn >> castling >> en_passant >> halfmove >> fullmove)) {
        cerr << "ERROR: Invalid FEN: " << fen << endl;
        return;
    }
#ifdef DEBUG
    cerr << "DEBUG load_fen input: \"" << fen << "\"" << endl;
    cerr << "   parts: board_part=\"" << board_part
             << "\" turn=\"" << turn
             << "\" castling=\"" << castling
             << "\" en_passant=\"" << en_passant
             << "\" halfmove=\"" << halfmove
             << "\" fullmove=\"" << fullmove << "\"" << endl;
#endif

    int rank = 0, file = 0;
    // Parse piece placement
    for (char c : board_part) {
        if (c == '/') {
            rank++;
            file = 0;
            continue;
        }
        if (isdigit(c)) {
            file += c - '0';
        } else {
            if (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
                int index = (7 - rank) * 8 + file;
                Piece p = EMPTY;
                bool isWhite = isupper(c);
                switch (tolower(c)) {
                    case 'p': p = isWhite ? WP : BP; break;
                    case 'n': p = isWhite ? WN : BN; break;
                    case 'b': p = isWhite ? WB : BB; break;
                    case 'r': p = isWhite ? WR : BR; break;
                    case 'q': p = isWhite ? WQ : BQ; break;
                    case 'k': p = isWhite ? WK : BK; break;
                    default: p = EMPTY; break;
                }
                squares[index] = p;
            }
            file++;
        }
    }

    // Side to move
    side_to_move = (turn == "w" ? WHITE : BLACK);

    // Castling rights
    white_king_castle    = (castling.find('K') != string::npos);
    white_queen_castle = (castling.find('Q') != string::npos);
    black_king_castle    = (castling.find('k') != string::npos);
    black_queen_castle = (castling.find('q') != string::npos);

    // En passant square
    if (en_passant != "-") {
        if (en_passant.size() == 2) {
            int f = en_passant[0] - 'a';
            int r = en_passant[1] - '1';
            if (f >= 0 && f < 8 && r >= 0 && r < 8) {
                en_passant_square = r * 8 + f;
            } else {
                en_passant_square = -1;
            }
        } else {
            en_passant_square = -1;
        }
    } else {
        en_passant_square = -1;
    }

    // Halfmove and fullmove clocks
    try {
        halfmove_clock = stoi(halfmove);
    } catch (...) {
        halfmove_clock = 0;
    }
    try {
        fullmove_number = stoi(fullmove);
    } catch (...) {
        fullmove_number = 1;
    }
}

string Board::print_board() const {
    stringstream ss;
    for (int rank = 7; rank >= 0; --rank) {
        ss << rank + 1 << " ";
        for (int file = 0; file < 8; ++file) {
            int sq = rank * 8 + file;
            char symbol = '.';
            switch (squares[sq]) {
                case WP: symbol = 'P'; break;
                case WN: symbol = 'N'; break;
                case WB: symbol = 'B'; break;
                case WR: symbol = 'R'; break;
                case WQ: symbol = 'Q'; break;
                case WK: symbol = 'K'; break;
                case BP: symbol = 'p'; break;
                case BN: symbol = 'n'; break;
                case BB: symbol = 'b'; break;
                case BR: symbol = 'r'; break;
                case BQ: symbol = 'q'; break;
                case BK: symbol = 'k'; break;
                case EMPTY: symbol = '.'; break;
            }
            ss << symbol << " ";
        }
        ss << "\n";
    }
    ss << "  a b c d e f g h\n";
    return ss.str();
}

bool Board::make_move(const Move& move, Piece& captured_piece, Piece& moved_piece) {
    // 1. Save current state to history for undo
    UndoInfo undo_info;
    undo_info.side_to_move = side_to_move;
    undo_info.en_passant_square = en_passant_square;
    undo_info.white_king_castle = white_king_castle;
    undo_info.white_queen_castle = white_queen_castle;
    undo_info.black_king_castle = black_king_castle;
    undo_info.black_queen_castle = black_queen_castle;
    undo_info.halfmove_clock = halfmove_clock;
    undo_info.fullmove_number = fullmove_number;
    undo_info.en_passant_capture_square = -1; // Default, will be set if it's an EP capture

    int from = move.from;
    int to   = move.to;

    // Ensure valid squares
    if (from < 0 || from >= 64 || to < 0 || to >= 64) return false;

    // Get the piece that is moving
    moved_piece = squares[from];
    captured_piece = squares[to]; // Assume target square is captured piece by default

    // 2. Handle specific move types before updating main board squares
    // Handle EN PASSANT capture
    // An en passant capture is detected if a pawn moves diagonally to an empty square,
    // and that empty square is the current en_passant_square target.
    // The captured pawn is NOT on the 'to' square, but behind it.
    bool is_en_passant_capture = false;
    if ((moved_piece == WP && from/8 == 4 && to == en_passant_square) ||
        (moved_piece == BP && from/8 == 3 && to == en_passant_square))
    {
        if (squares[to] == EMPTY) { // Confirm it's a move to an empty square
            is_en_passant_capture = true;
            if (side_to_move == WHITE) {
                captured_piece = BP; // White captures Black pawn
                squares[to - 8] = EMPTY; // Remove the captured pawn from its square
                undo_info.en_passant_capture_square = to - 8; // Store its original square for undo
            } else {
                captured_piece = WP; // Black captures White pawn
                squares[to + 8] = EMPTY; // Remove the captured pawn
                undo_info.en_passant_capture_square = to + 8; // Store its original square for undo
            }
        }
    }

    // 3. Update board state based on the move
    // Update halfmove clock
    if (captured_piece != EMPTY || moved_piece == WP || moved_piece == BP) {
        halfmove_clock = 0;
    } else {
        halfmove_clock++;
    }

    // Move piece (or promote)
    if (move.promotion != EMPTY) {
        squares[to] = move.promotion;
    } else {
        squares[to] = moved_piece;
    }
    squares[from] = EMPTY;

    // Update en passant square for next turn
    // A new en passant square is set ONLY if a pawn moves two squares forward
    if (moved_piece == WP && from/8 == 1 && to/8 == 3) {
        en_passant_square = from + 8; // Square behind the white pawn
    } else if (moved_piece == BP && from/8 == 6 && to/8 == 4) {
        en_passant_square = from - 8; // Square behind the black pawn
    } else {
        en_passant_square = -1; // No en passant target otherwise
    }

    // Update castling rights
    // If King moves, revoke all castling rights for that side
    if (moved_piece == WK) {
        white_king_castle = false;
        white_queen_castle = false;
    } else if (moved_piece == BK) {
        black_king_castle = false;
        black_queen_castle = false;
    }
    // If a rook moves from its original square, revoke that specific castling right
    // Also revoke if a rook is captured on its original square
    if (from == H1 || to == H1) white_king_castle = false;
    if (from == A1 || to == A1) white_queen_castle = false;
    if (from == H8 || to == H8) black_king_castle = false;
    if (from == A8 || to == A8) black_queen_castle = false;


    // Handle CASTLING move (king moves two squares)
    if (moved_piece == WK && from == E1) {
        if (to == G1) { // White King-side
            squares[F1] = WR; // Move rook
            squares[H1] = EMPTY;
        } else if (to == C1) { // White Queen-side
            squares[D1] = WR; // Move rook
            squares[A1] = EMPTY;
        }
    } else if (moved_piece == BK && from == E8) {
        if (to == G8) { // Black King-side
            squares[F8] = BR; // Move rook
            squares[H8] = EMPTY;
        } else if (to == C8) { // Black Queen-side
            squares[D8] = BR; // Move rook
            squares[A8] = EMPTY;
        }
    }

    // Update fullmove number (only after Black's move)
    if (side_to_move == BLACK) {
        fullmove_number++;
    }

    // Flip side to move
    side_to_move = (side_to_move == WHITE ? BLACK : WHITE);

    // Push the saved state onto the history stack
    history.push_back(undo_info);

    return true; // Move was made successfully
}

void Board::unmake_move(const Move& move, Piece captured_piece, Piece moved_piece) {
    if (history.empty()) {
        cerr << "ERROR: Cannot unmake move, history is empty." << endl;
        return;
    }

    // 1. Pop the last saved state from history
    UndoInfo undo_info = history.back();
    history.pop_back();

    // 2. Restore all board state variables from undo_info
    side_to_move = undo_info.side_to_move;
    en_passant_square = undo_info.en_passant_square;
    white_king_castle = undo_info.white_king_castle;
    white_queen_castle = undo_info.white_queen_castle;
    black_king_castle = undo_info.black_king_castle;
    black_queen_castle = undo_info.black_queen_castle;
    halfmove_clock = undo_info.halfmove_clock;
    fullmove_number = undo_info.fullmove_number;

    int from = move.from;
    int to   = move.to;

    // Restore pieces to their positions BEFORE the move
    // This will restore the moved piece to 'from' and captured piece to 'to'
    squares[from] = moved_piece; // The piece that moved goes back to 'from'
    squares[to]   = captured_piece; // The captured piece (or EMPTY) goes back to 'to'

    if (moved_piece == WK && from == E1) {
        if (to == G1) { // King-side castling
            squares[H1] = WR; // Restore rook to H1
            squares[F1] = EMPTY; // Empty rook's landing square (F1)
        } else if (to == C1) { // Queen-side castling
            squares[A1] = WR; // Restore rook to A1
            squares[D1] = EMPTY; // Empty rook's landing square (D1)
        }
    } else if (moved_piece == BK && from == E8) {
        if (to == G8) { // King-side castling
            squares[H8] = BR;
            squares[F8] = EMPTY;
        } else if (to == C8) { // Queen-side castling
            squares[A8] = BR;
            squares[D8] = EMPTY;
        }
    }

    // Undo EN PASSANT capture
    // If it was an en passant capture, the captured pawn was NOT on the 'to' square.
    // It was on the square saved in undo_info.en_passant_capture_square.
    // The 'to' square where the capturing pawn landed should be made EMPTY.
    if (undo_info.en_passant_capture_square != -1) { // This means it was an en passant capture
        // The square where the capturing pawn landed ('to') should now be empty
        squares[to] = EMPTY;
        // Restore the captured pawn to its actual original square
        if (moved_piece == WP) { // White pawn captured black pawn
            squares[undo_info.en_passant_capture_square] = BP;
        } else { // Black pawn captured white pawn
            squares[undo_info.en_passant_capture_square] = WP;
        }
    }

    if (move.promotion != EMPTY) {
        squares[from] = (side_to_move == WHITE ? WP : BP); // Restore the pawn to its 'from' square
        squares[to] = captured_piece; // Restore captured piece (or EMPTY) to 'to' square
    }
}

bool Board::is_king_in_check(Color color) const {
    Piece king_piece = (color == WHITE ? WK : BK);
    int king_sq = -1;
    for (int i = 0; i < 64; ++i) {
        if (squares[i] == king_piece) {
            king_sq = i;
            break;
        }
    }
    if (king_sq < 0) return false; // Should not happen in a valid game

    // Create a temporary board for move generation
    // This copy is necessary because MoveGenerator needs a non-const Board reference
    // to potentially generate pseudo-legal moves (which might temporarily modify board state for checks).
    Board temp_board_for_check_gen = *this;
    temp_board_for_check_gen.side_to_move = (color == WHITE ? BLACK : WHITE);

    MoveGenerator gen(temp_board_for_check_gen);
    auto moves = gen.generate_pseudo_legal_moves();

    for (auto& m : moves) {
        // If any of the opponent's pseudo-legal moves attacks the king square
        if (m.to == king_sq) return true;
    }
    return false;
}

bool Board::has_legal_moves() {
    MoveGenerator gen(*this);
    auto moves = gen.generate_legal_moves(); // This function will filter out moves that leave king in check
    return !moves.empty();
}

bool Board::is_game_over() {
    // Check for 50-move rule (100 half-moves)
    if (halfmove_clock >= 100) return true;

    // Check for checkmate or stalemate
    return !has_legal_moves();
}

string Board::get_result_string() {
    if (halfmove_clock >= 100) return "1/2-1/2 (50-move rule)"; // Draw by 50-move rule

    if (has_legal_moves()) return "*"; // Game not over

    if (is_king_in_check(side_to_move)) {
        return (side_to_move == WHITE ? "0-1 (Black wins by checkmate)" : "1-0 (White wins by checkmate)");
    } else {
        return "1/2-1/2 (Stalemate)";
    }
}

bool Board::is_square_attacked(int square, Color attacker) const {
    int rank = square / 8;
    int file = square % 8;

    // Direction offsets
    const int knight_offsets[] = {-17, -15, -10, -6, 6, 10, 15, 17};
    const int king_offsets[] = {-9, -8, -7, -1, 1, 7, 8, 9};

    // Pawn attacks
    if (attacker == WHITE) {
        if (square >= 9 && squares[square - 9] == WP && (square % 8 != 0)) return true;
        if (square >= 7 && squares[square - 7] == WP && (square % 8 != 7)) return true;
    } else {
        if (square <= 55 && squares[square + 7] == BP && (square % 8 != 0)) return true;
        if (square <= 55 && squares[square + 9] == BP && (square % 8 != 7)) return true;
    }

    // Knight attacks
    for (int offset : knight_offsets) {
        int to = square + offset;
        if (to < 0 || to >= 64) continue;
        Piece p = squares[to];
        if ((attacker == WHITE && p == WN) || (attacker == BLACK && p == BN)) return true;
    }

    // King attacks
    for (int offset : king_offsets) {
        int to = square + offset;
        if (to < 0 || to >= 64) continue;
        int dr = abs((to / 8) - rank);
        int df = abs((to % 8) - file);
        if (dr > 1 || df > 1) continue;
        Piece p = squares[to];
        if ((attacker == WHITE && p == WK) || (attacker == BLACK && p == BK)) return true;
    }

    // Sliding pieces: Rooks / Queens (horizontal + vertical)
    const int rook_dirs[] = {-8, -1, 1, 8};
    for (int dir : rook_dirs) {
        int to = square;
        while (true) {
            to += dir;
            if (to < 0 || to >= 64) break;
            int r = to / 8, f = to % 8;
            if (abs(r - rank) > 7 || abs(f - file) > 7) break;
            Piece p = squares[to];
            if (p == EMPTY) continue;
            if (attacker == WHITE && (p == WR || p == WQ)) return true;
            if (attacker == BLACK && (p == BR || p == BQ)) return true;
            break;
        }
    }

    // Sliding pieces: Bishops / Queens (diagonals)
    const int bishop_dirs[] = {-9, -7, 7, 9};
    for (int dir : bishop_dirs) {
        int to = square;
        while (true) {
            to += dir;
            if (to < 0 || to >= 64) break;
            int r = to / 8, f = to % 8;
            if (abs(r - rank) > 7 || abs(f - file) > 7 || abs(r - rank) != abs(f - file)) break;
            Piece p = squares[to];
            if (p == EMPTY) continue;
            if (attacker == WHITE && (p == WB || p == WQ)) return true;
            if (attacker == BLACK && (p == BB || p == BQ)) return true;
            break;
        }
    }

    return false;
}

