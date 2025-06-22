#pragma once

#include "board.h"
#include "move.h"

int alpha_beta(Board& board, int depth, int alpha, int beta);
// 1) depth only, uses a default time limit
Move find_best_move(Board& board, int max_depth);

// 2) depth + time limit in milliseconds
Move find_best_move(Board& board, int max_depth, int time_ms);