import tkinter as tk
from tkinter import messagebox
import chess
import os
import subprocess
import pygame
from PIL import Image, ImageTk

# Set this variable to control engine search depth globally
simulation_depth = 5

# Highlight colors for selected squares and checkmate
HIGHLIGHT_COLOR = "#5C4033"
CHECKMATE_HIGHLIGHT_COLOR = "#FF0000"

# Determine the base directory for resources
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# Paths to the chess engine executables
ENGINE_PATH = os.path.join(BASE_DIR, "tadfish.exe")
STOCKFISH_PATH = os.path.join(BASE_DIR, "test_engines", "stockfish.exe")

# Determine the appropriate resampling filter for PIL Image.resize
try:
    RESAMPLE = Image.Resampling.LANCZOS
except AttributeError:
    try:
        RESAMPLE = Image.LANCZOS
    except AttributeError:
        RESAMPLE = Image.BICUBIC

class ChessUI:
    """
    Manages the Chess UI, game logic, and engine simulation.
    """
    def __init__(self, root):
        self.root = root
        self.root.title("Tadfish VS Stockfish")

        # --- UI Enhancements Start ---
        self.root.configure(bg="#2E343A")
        self.root.option_add('*Font', 'Helvetica 11')
        self.root.state('zoomed')

        self.main_container = tk.Frame(root, bg="#2E343A")
        self.main_container.pack(expand=True, fill=tk.BOTH, padx=20, pady=20)

        self.main_container.grid_columnconfigure(0, weight=1)
        self.main_container.grid_columnconfigure(1, weight=1)
        self.main_container.grid_rowconfigure(0, weight=1)

        self.left_panel_frame = tk.Frame(self.main_container, bg="#2E343A")
        self.left_panel_frame.grid(row=0, column=0, sticky="nsew", padx=(0, 10))

        self.left_panel_frame.grid_rowconfigure(0, weight=1)
        self.left_panel_frame.grid_rowconfigure(1, weight=0)
        self.left_panel_frame.grid_rowconfigure(2, weight=0)
        self.left_panel_frame.grid_rowconfigure(3, weight=0)
        self.left_panel_frame.grid_rowconfigure(4, weight=1)
        self.left_panel_frame.grid_columnconfigure(0, weight=1)
        self.left_panel_frame.grid_columnconfigure(1, weight=0)
        self.left_panel_frame.grid_columnconfigure(2, weight=1)

        self.black_engine_label_board = tk.Label(self.left_panel_frame, text="Stockfish",
                                                 font='Helvetica 12 italic', fg="#E0E0E0", bg="#2E343A", pady=5)
        self.black_engine_label_board.grid(row=1, column=1, pady=(0, 5))

        self.square_size = 80
        self.canvas = tk.Canvas(self.left_panel_frame, width=self.square_size * 8, height=self.square_size * 8, bd=0, highlightthickness=0, bg="#2E343A")
        self.canvas.grid(row=2, column=1)

        self.white_engine_label_board = tk.Label(self.left_panel_frame, text="Tadfish",
                                                 font='Helvetica 12 italic', fg="#E0E0E0", bg="#2E343A", pady=5)
        self.white_engine_label_board.grid(row=3, column=1, pady=(5, 0))

        self.right_panel_frame = tk.Frame(self.main_container, bg="#3C3F41", bd=2, relief=tk.GROOVE)
        self.right_panel_frame.grid(row=0, column=1, sticky="ew", padx=(10, 10), ipady=150)

        self.right_panel_frame.grid_rowconfigure(0, weight=1)
        self.right_panel_frame.grid_rowconfigure(1, weight=0)
        self.right_panel_frame.grid_rowconfigure(2, weight=1)
        self.right_panel_frame.grid_columnconfigure(0, weight=1)
        self.right_panel_frame.grid_columnconfigure(1, weight=0)
        self.right_panel_frame.grid_columnconfigure(2, weight=1)

        self.inner_control_frame = tk.Frame(self.right_panel_frame, bg="#3C3F41", padx=20, pady=0)
        self.inner_control_frame.grid(row=1, column=1)

        tk.Label(self.inner_control_frame, text="Tadfish vs Stockfish",
                 font='Helvetica 20 bold', fg="#F0F0F0", bg="#3C3F41", pady=5).pack(pady=(0, 5))

        tk.Label(self.inner_control_frame, text="Tadfish v2.2",
                 font='Helvetica 16', fg="#B0B0B0", bg="#3C3F41", pady=5).pack(pady=(5, 0))

        self.status_var = tk.StringVar()
        tk.Label(self.inner_control_frame, textvariable=self.status_var,
                 font='Helvetica 12 bold', fg="#E0E0E0", bg="#3C3F41", pady=5).pack(pady=(0, 10))

        depth_display_container = tk.Frame(self.inner_control_frame, bg="#3C3F41")
        depth_display_container.pack(pady=10)

        tk.Label(depth_display_container, text="Depth:", fg="#E0E0E0", bg="#3C3F41", font='Helvetica 11').pack(side=tk.LEFT, padx=(0,0))
        self.depth_var = tk.StringVar(value=str(simulation_depth))
        tk.Label(depth_display_container, textvariable=self.depth_var, bg="#3C3F41", fg="white", font='Helvetica 11', width=4, anchor="center").pack(side=tk.LEFT, padx=(0, 0))

        button_frame = tk.Frame(self.inner_control_frame, bg="#3C3F41")
        button_frame.pack(pady=(20, 0))

        self.sim_button = tk.Button(button_frame, text="Start Simulation", command=self.start_simulation,
                                     bg="#4CAF50", fg="white", activebackground="#60B863", activeforeground="white",
                                     relief=tk.RAISED, bd=3, padx=15, pady=8, font='Helvetica 11 bold', cursor="hand2")
        self.sim_button.pack(side=tk.LEFT, padx=(0, 10))

        self.stop_button = tk.Button(button_frame, text="Stop Simulation", command=self.stop_simulation, state=tk.DISABLED,
                                     bg="#F44336", fg="white", activebackground="#FF5C52", activeforeground="white",
                                     relief=tk.RAISED, bd=3, padx=15, pady=8, font='Helvetica 11 bold', cursor="hand2")
        self.stop_button.pack(side=tk.LEFT, padx=(10, 0))

        # --- UI Enhancements End ---

        # Initialize Pygame mixer for sounds
        try:
            pygame.mixer.init()
        except Exception as e:
            print(f"Audio initialization failed: {e}. Sounds will not play.")
        self.load_sounds()

        self.board = chess.Board()

        # Game state variables (some are placeholders for human interaction, not used in engine vs. engine)
        self.selected_square = None
        self.legal_destinations = []
        self.dragging = False
        self.drag_start = None
        self.drag_piece = None
        self.mouse_x = 0
        self.mouse_y = 0
        self.min_drag_distance = 5
        self.pressed_x = 0
        self.pressed_y = 0
        self.simulating = False
        self.simulation_ply = 0
        self.max_plies = 100
        self.after_id = None

        self.piece_images = {}
        self.load_piece_images_with_check()

        # Bind mouse events (placeholders)
        self.canvas.bind("<ButtonPress-1>", self.on_press)
        self.canvas.bind("<B1-Motion>", self.on_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_release)

        # Initial drawing and status update
        self.draw_board()
        self.update_status()
        self.play_sound(self.sound_start)

    def load_sounds(self):
        """Loads sound files for game events."""
        def try_load(name):
            path = os.path.join(BASE_DIR, "audio", name)
            if os.path.exists(path):
                try:
                    return pygame.mixer.Sound(path)
                except pygame.error as e:
                    print(f"Failed to load sound {name}: {e}")
                    return None
            else:
                print(f"Sound file not found: {path}")
                return None

        self.sound_move = try_load("move-self.mp3")
        self.sound_capture = try_load("capture.mp3")
        self.sound_check = try_load("move-check.mp3")
        self.sound_checkmate = try_load("checkmate.mp3")
        self.sound_start = try_load("start.mp3")

    def play_sound(self, sound):
        """Plays a given sound."""
        if sound:
            try:
                sound.play()
            except pygame.error as e:
                print(f"Error playing sound: {e}")

    def load_piece_images_with_check(self):
        """Loads chess piece images, handles missing files."""
        mapping = {
            'P': 'white-pawn.png', 'N': 'white-knight.png', 'B': 'white-bishop.png',
            'R': 'white-rook.png', 'Q': 'white-queen.png', 'K': 'white-king.png',
            'p': 'black-pawn.png', 'n': 'black-knight.png', 'b': 'black-bishop.png',
            'r': 'black-rook.png', 'q': 'black-queen.png', 'k': 'black-king.png',
        }
        for symbol, filename in mapping.items():
            path = os.path.join(BASE_DIR, "images", filename)
            if os.path.exists(path):
                try:
                    img = Image.open(path).convert("RGBA").resize((self.square_size, self.square_size), RESAMPLE)
                    self.piece_images[symbol] = ImageTk.PhotoImage(img)
                except Exception as e:
                    print(f"Failed to load image {filename}: {e}")
            else:
                print(f"Image file not found: {path}")

    def draw_board(self):
        """Draws the chess board, pieces, and highlights."""
        self.canvas.delete("all")

        # Draw the squares
        for rank in range(8):
            for file in range(8):
                x1 = file * self.square_size
                y1 = (7 - rank) * self.square_size
                color = "#F0D9B5" if (rank + file) % 2 == 0 else "#B58863"
                self.canvas.create_rectangle(x1, y1, x1 + self.square_size, y1 + self.square_size, fill=color, outline="")

        # Draw the pieces
        for square in chess.SQUARES:
            piece = self.board.piece_at(square)
            if piece:
                symbol = piece.symbol()
                img = self.piece_images.get(symbol)
                if img:
                    x = chess.square_file(square) * self.square_size + self.square_size / 2
                    y = (7 - chess.square_rank(square)) * self.square_size + self.square_size / 2
                    self.canvas.create_image(x, y, image=img)

        # Highlight king if in checkmate
        if self.board.is_checkmate():
            sq = self.board.king(self.board.turn)
            if sq is not None:
                f = chess.square_file(sq)
                r = chess.square_rank(sq)
                x1 = f * self.square_size + 2
                y1 = (7 - r) * self.square_size + 2
                x2 = x1 + self.square_size - 4
                y2 = y1 + self.square_size - 4
                self.canvas.create_rectangle(x1, y1, x2, y2, outline=CHECKMATE_HIGHLIGHT_COLOR, width=4)

    def update_status(self):
        """Updates the status message displayed to the user."""
        if self.board.is_checkmate():
            winner = "Black" if self.board.turn else "White"
            self.status_var.set(f"Checkmate! {winner} wins.")
            self.play_sound(self.sound_checkmate)
            self.stop_simulation()
        elif self.board.is_stalemate():
            self.status_var.set("Stalemate!")
            self.stop_simulation()
        elif self.board.is_insufficient_material():
            self.status_var.set("Draw by insufficient material!")
            self.stop_simulation()
        elif self.board.is_fivefold_repetition():
            self.status_var.set("Draw by fivefold repetition!")
            self.stop_simulation()
        elif self.board.is_seventyfive_moves():
            self.status_var.set("Draw by seventy-five move rule!")
            self.stop_simulation()
        else:
            self.status_var.set(f"{'White' if self.board.turn == chess.WHITE else 'Black'}'s Turn")

    def start_simulation(self):
        """Starts the engine vs. engine simulation."""
        if not os.path.exists(ENGINE_PATH):
            messagebox.showerror("Engine Not Found", f"Tadfish engine executable not found at: {ENGINE_PATH}\n"
                                                     "Please ensure 'tadfish.exe' is in the same directory.")
            return
        if not os.path.exists(STOCKFISH_PATH):
            messagebox.showerror("Engine Not Found", f"Stockfish engine executable not found at: {STOCKFISH_PATH}\n"
                                                     "Please ensure 'stockfish.exe' is in the 'test_engines' subdirectory.")
            return

        moves_each_side = 50 # Fixed value
        global simulation_depth

        self.board = chess.Board()
        self.max_plies = moves_each_side * 2
        self.simulation_ply = 0
        self.simulating = True
        self.sim_button.config(state=tk.DISABLED)
        self.stop_button.config(state=tk.NORMAL)

        self.draw_board()
        self.update_status()
        self.play_sound(self.sound_start)

        self.after_id = self.root.after(300, self.simulation_step)

    def stop_simulation(self):
        """Stops the ongoing engine simulation."""
        self.simulating = False
        self.sim_button.config(state=tk.NORMAL)
        self.stop_button.config(state=tk.DISABLED)
        if self.after_id:
            self.root.after_cancel(self.after_id)
            self.after_id = None
        print("Simulation stopped by user or game end.")

    def simulation_step(self):
        """Performs one step of the simulation (one engine move)."""
        if not self.simulating:
            print("Simulation aborted.")
            return

        if self.board.is_game_over():
            self.update_status()
            self.stop_simulation()
            return

        if self.simulation_ply >= self.max_plies:
            messagebox.showinfo("Simulation Complete", "Reached maximum number of moves for simulation. Game is a draw.")
            self.stop_simulation()
            return

        fen = self.board.fen()
        engine_to_use = ""

        if self.board.turn == chess.WHITE:
            engine_to_use = ENGINE_PATH # Tadfish (your engine) plays as White
        else:
            engine_to_use = STOCKFISH_PATH # Stockfish plays as Black

        move_str = ""

        try:
            if engine_to_use == STOCKFISH_PATH:
                process = subprocess.Popen(
                    [engine_to_use],
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True
                )
                process.stdin.write(f"position fen {fen}\n")
                process.stdin.write(f"go depth {simulation_depth}\n")
                process.stdin.flush()

                while True:
                    line = process.stdout.readline().strip()
                    if not line:
                        break
                    if line.startswith("bestmove"):
                        move_str = line.split(" ")[1]
                        break
                process.terminate()
            else: # For Tadfish (your engine)
                result = subprocess.run(
                    [engine_to_use, fen, str(simulation_depth)],
                    capture_output=True,
                    text=True,
                    check=True,
                    timeout=30
                )
                output_parts = result.stdout.strip().split()
                if not output_parts:
                    raise ValueError("Engine returned empty output.")
                move_str = output_parts[0]

            print(f"Engine ({'White' if self.board.turn == chess.WHITE else 'Black'}) move: {move_str}")
            move = chess.Move.from_uci(move_str)

        except FileNotFoundError:
            messagebox.showerror("Engine Error", f"Engine executable not found at {engine_to_use}.\n"
                                                 "Please ensure it exists and is executable.")
            self.stop_simulation()
            return
        except subprocess.CalledProcessError as e:
            messagebox.showerror("Engine Error", f"Engine returned an error: {e.stderr.strip()}.\n"
                                                 "Check engine path and arguments.")
            self.stop_simulation()
            return
        except subprocess.TimeoutExpired:
            messagebox.showerror("Engine Error", "Engine did not respond in time.")
            self.stop_simulation()
            return
        except ValueError as e:
            messagebox.showerror("Engine Output Error", f"Failed to parse engine output: {e}.\n"
                                                         f"Output: '{result.stdout.strip() if 'result' in locals() else 'N/A'}'")
            self.stop_simulation()
            return
        except Exception as e:
            messagebox.showerror("Unexpected Engine Error", f"An unexpected error occurred: {e}")
            self.stop_simulation()
            return

        if move in self.board.legal_moves:
            is_capture = self.board.is_capture(move)
            gives_check = self.board.gives_check(move)

            self.board.push(move)
            self.simulation_ply += 1

            self.draw_board()
            self.update_status()

            if self.board.is_checkmate():
                self.play_sound(self.sound_checkmate)
            elif gives_check:
                self.play_sound(self.sound_check)
            elif is_capture:
                self.play_sound(self.sound_capture)
            else:
                self.play_sound(self.sound_move)

            self.after_id = self.root.after(300, self.simulation_step)
        else:
            messagebox.showerror("Invalid Move", f"Engine returned an illegal move: {move_str} for current board state.\n"
                                                 "Simulation stopped.")
            self.stop_simulation()

    # Placeholder methods for human interaction (not used in engine vs. engine simulation)
    def on_press(self, event):
        pass

    def on_drag(self, event):
        pass

    def on_release(self, event):
        pass

if __name__ == "__main__":
    root = tk.Tk()
    ChessUI(root)
    root.mainloop()