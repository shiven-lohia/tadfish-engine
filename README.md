# Tadfish Engine and GUI

Tadfish is an intermediate-level chess engine written in C++ with a Python-based GUI. It was developed as a coding project to explore core concepts in game tree search, evaluation heuristics, and C++ performance optimization.

---

## Features

* Roughly ~**1700-1800** elo based on testing against Stockfish
* C++ backend for efficient move generation and evaluation
* Python GUI using Tkinter and Pygame for interactive gameplay
* Chess.com-like sound effects and graphical feedback

### Engine Techniques

* **Negamax** framework for simplified recursive game tree search
* **Alpha-Beta Pruning** to eliminate unnecessary branches
* **Piece-Square Tables (PST)** for positional evaluation
* **Precomputed pseudo-legal move generation** for performance
* **Iterative Deepening** to allow time-based search depth
* **Aspiration Windows** to improve Alpha-Beta performance
* **Quiescent Search** to avoid horizon effect on volatile positions
* **Move Ordering** using **MVV-LVA** (Most Valuable Victim - Least Valuable Attacker)

### Game Modes

* Player vs Player
* Player vs Tadfish
* Tadfish vs Stockfish
* Tadfish vs Tadfish

---

## Upcoming Features

* Complete UCI support (Very high prioity)
* Opening book support (high priority)
* Transposition Tables (high priority)
* NNUE based evaluation (lower priority)
* Adjustable difficulty levels
* Game history and PGN export
* Timers for competitive play
* Ability to draw arrows in GUI by right clicking
* Dynamic evaluation bar in GUI

---

## How to Run

### 1. Compile the Engine (C++)

```bash
g++ -Iinclude src/*.cpp main.cpp -o tadfish
```

### 2. Run the GUI (Python)

Make sure you have `python-chess`, `pygame`, and `tkinter` installed. Run the corresponding python file for the gamemode you want to play.

```bash
# Example: Play against Tadfish
python player_vs_tadfish.py
```

---

## Known Drawbacks

* No opening theory
* Only supports one move outputs taking FEN as input, limited UCI support
* No multithreading or parallelism yet
* Lacks time control or search depth adjustments
* Sometimes prone to stalemate in winning positions while playing against itself

---

## Version History

| Version   | Description                                                                                                                                                                        |
| --------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Beta**  | Initial PvP GUI, Chess backend in C++                                                                                                                                              |
| **v1.0**   | Basic engine with evaluation using material value and PST and search using minimax. Improved UI by adding drag moves, legal move highlight, piece sounds, capture/checkmate sounds |
| **v1.0.1** | Added a second GUI for simulating Tadfish vs Tadfish for testing                                                                                                                   |
| **v1.0.2** | Added precompute.h with precomputed pseudo-legal moves for every piece at every square                                                                                             |
| **v1.0.3** | Fixed bugs related to pawn promotion, en passant, and castling                                                                                                                     |
| **v1.1**   | Changed to negamax with alpha-beta pruning; added different endgame PST for pieces like the king                                                                                   |
| **v1.1.1** | Implemented MVV-LVA move ordering to improve alpha-beta pruning efficiency                                                                                                         |
| **v1.2**   | Added Iterative Deepening, Quiescent Search, and Aspiration Windows                                                                                                                |
| **v1.2.1** | Fixed bug with king not switching to endgame PST correctly                                                                                                                         |
| **v1.3**   | Rewrote castling logic to prevent castling through attacked squares                                                                                                                |
| **v1.3.1** | Fixed infinite recursion bug in castling move generation                                                                                                                           |
| **v2.0**   | Complete overhaul of GUI with a dark theme; fixed critical GUI bugs                                                                                                                |
| **v2.1**   | Added GUIs for Player vs Tadfish and Tadfish vs Stockfish                                                                                                                          |
| **v2.1.1** | Updated all GUIs to match new dark-themed design                                                                                                                                   |
| **v2.1.2** | Minor updates to all GUIs                                                                                                                                                          |
| **v2.2**   | Cleaned up code directory; improved code comments                                                                                                                                  |

---

## Project Structure

```
.
├── audio/                  # Sound effects
├── images/                 # GUI graphics
├── include/                # C++ header files
├── src/                    # Engine source files
├── test_engines/           # Optional engines (e.g., Stockfish)
├── player_vs_player.py     # GUI: PvP mode
├── player_vs_tadfish.py    # GUI: Player vs Tadfish
├── tadfish_vs_stockfish.py # GUI: Tadfish vs Stockfish
├── tadfish_vs_tadfish.py   # GUI: Tadfish mirror match
├── main.cpp                # Engine entry point
└── tadfish.exe             # Compiled engine binary
```

## 👨‍💼 Author

Developed by [Shiven Lohia](https://github.com/shiven-lohia)
