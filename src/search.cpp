// search.cpp
#include "search.h"
#include "eval.h"
#include "movegen.h"
#include <limits>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <future>

const int INF = 100000;
static std::chrono::steady_clock::time_point start_time;
static int time_limit_ms = 20000;
static bool time_up_flag = false;

// Move ordering heuristics
static const int MAX_PLY = 64;
static Move killer_moves[MAX_PLY][2];
static int history_heuristic[64][64];

inline void record_killer(const Move& move, int ply) {
    if (killer_moves[ply][0].from == move.from && killer_moves[ply][0].to == move.to) return;
    if (killer_moves[ply][1].from == move.from && killer_moves[ply][1].to == move.to) return;
    killer_moves[ply][1] = killer_moves[ply][0];
    killer_moves[ply][0] = move;
}

inline void record_history(const Move& move, int depth) {
    history_heuristic[move.from][move.to] += depth * depth;
}

int piece_value_for_mvv(Piece p) {
    switch(p) {
        case WP: case BP: return 100;
        case WN: case BN: return 320;
        case WB: case BB: return 330;
        case WR: case BR: return 500;
        case WQ: case BQ: return 900;
        default: return 0;
    }
}

int score_capture(const Board& board, const Move& move) {
    int victim = piece_value_for_mvv(board.squares[move.to]);
    int attacker = piece_value_for_mvv(board.squares[move.from]);
    return victim * 100 - attacker;
}

struct ScoredMove {
    Move move;
    int score;
};

inline bool is_time_up() {
    auto now = std::chrono::steady_clock::now();
    int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
    return elapsed >= time_limit_ms;
}

int quiescence(Board& board, int alpha, int beta, int ply) {
    // If terminal and checkmate in quiescence, return mate score
    MoveGenerator gen_term(board);
    auto legal_term = gen_term.generate_legal_moves();
    if (legal_term.empty()) {
        if (board.is_king_in_check(board.side_to_move)) {
            return -INF + ply;
        } else {
            return 0;
        }
    }
    int stand_pat = evaluate(board);
    if (stand_pat >= beta) return beta;
    if (alpha < stand_pat) alpha = stand_pat;
    MoveGenerator gen(board);
    auto moves_all = gen.generate_legal_moves();
    std::vector<ScoredMove> captures;
    for (auto& m : moves_all) {
        if (board.squares[m.to] != EMPTY) {
            int sc = score_capture(board, m) + 10000;
            captures.push_back({m, sc});
        }
    }
    std::sort(captures.begin(), captures.end(), [](auto& a, auto& b){ return a.score > b.score; });
    for (auto& sm : captures) {
        if (is_time_up()) break;
        Piece cap, mov;
        board.make_move(sm.move, cap, mov);
        int score = -quiescence(board, -beta, -alpha, ply+1);
        board.unmake_move(sm.move, cap, mov);
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

int alpha_beta(Board& board, int depth, int alpha, int beta, int ply) {
    if (time_up_flag || is_time_up()) {
        time_up_flag = true;
        return evaluate(board);
    }
    if (depth <= 0) return quiescence(board, alpha, beta, ply);
    MoveGenerator gen(board);
    auto moves = gen.generate_legal_moves();
    if (moves.empty()) {
        if (board.is_king_in_check(board.side_to_move)) {
            return -INF + ply; // mate
        } else {
            return 0; // stalemate
        }
    }
    // Move ordering
    std::vector<ScoredMove> scored_moves;
    scored_moves.reserve(moves.size());
    for (auto& m : moves) {
        int sc = 0;
        if (board.squares[m.to] != EMPTY) {
            sc = score_capture(board, m) + 100000;
        } else {
            if (killer_moves[ply][0].from == m.from && killer_moves[ply][0].to == m.to) sc = 90000;
            else if (killer_moves[ply][1].from == m.from && killer_moves[ply][1].to == m.to) sc = 80000;
            else sc = history_heuristic[m.from][m.to];
        }
        scored_moves.push_back({m, sc});
    }
    std::sort(scored_moves.begin(), scored_moves.end(), [](auto& a, auto& b){ return a.score > b.score; });
    int best = -INF;
    for (auto& sm : scored_moves) {
        if (time_up_flag || is_time_up()) { time_up_flag = true; break; }
        Move m = sm.move;
        Piece cap, mov;
        if (!board.make_move(m, cap, mov)) continue;
        int score = -alpha_beta(board, depth-1, -beta, -alpha, ply+1);
        // Adjust mate distance: prefer faster mate
        if (score > INF/2) score -= 1;
        if (score < -INF/2) score += 1;
        board.unmake_move(m, cap, mov);
        if (score > best) best = score;
        if (score > alpha) {
            alpha = score;
            if (board.squares[m.to] == EMPTY) {
                record_killer(m, ply);
                record_history(m, depth);
            }
        }
        if (alpha >= beta) break;
    }
    return best;
}

Move find_best_move(Board& board, int max_depth, int time_ms) {
    start_time = std::chrono::steady_clock::now();
    time_limit_ms = time_ms;
    time_up_flag = false;
    for (int i = 0; i < MAX_PLY; ++i) {
        killer_moves[i][0] = Move(-1,-1);
        killer_moves[i][1] = Move(-1,-1);
    }
    memset(history_heuristic, 0, sizeof(history_heuristic));
    MoveGenerator root_gen(board);
    auto root_moves = root_gen.generate_legal_moves();
    if (root_moves.empty()) return Move();
    Move best_move = root_moves[0];
    int best_score = -INF;
    for (int depth = 1; depth <= max_depth; ++depth) {
        if (is_time_up()) break;
        MoveGenerator gen(board);
        auto moves = gen.generate_legal_moves();
        if (moves.empty()) break;
        int alpha = -INF, beta = INF;
        int current_best_score = -INF;
        Move current_best_move = best_move;
        for (auto& m : moves) {
            if (time_up_flag || is_time_up()) { time_up_flag = true; break; }
            Piece cap, mov;
            if (!board.make_move(m, cap, mov)) continue;
            int score = -alpha_beta(board, depth-1, -INF, INF, 1);
            // Adjust mate distance
            if (score > INF/2) score -= 1;
            if (score < -INF/2) score += 1;
            board.unmake_move(m, cap, mov);
            if (score > current_best_score) {
                current_best_score = score;
                current_best_move = m;
            }
            if (score > alpha) alpha = score;
        }
        if (!time_up_flag) {
            best_score = current_best_score;
            best_move = current_best_move;
        }
    }
    return best_move;
}

Move find_best_move(Board& board, int max_depth) {
    const int DEFAULT_TIME_MS = 20000;
    return find_best_move(board, max_depth, DEFAULT_TIME_MS);
}
