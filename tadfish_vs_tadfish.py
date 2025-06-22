import tkinter as tk
from tkinter import messagebox
import chess
import os
import subprocess
import pygame
from PIL import Image, ImageTk

simulation_depth = 5  # Engine search depth

HIGHLIGHT_COLOR = "#5C4033"
CHECKMATE_HIGHLIGHT_COLOR = "#FF0000"

BASE_DIR = os.path.dirname(os.path.abspath(__file__)) # Resource directory
ENGINE_PATH = os.path.join(BASE_DIR, "tadfish.exe") # Chess engine executable path

# Determine PIL resampling filter
try:
    RESAMPLE = Image.Resampling.LANCZOS
except AttributeError:
    try:
        RESAMPLE = Image.LANCZOS
    except AttributeError:
        RESAMPLE = Image.BICUBIC

class ChessUI:
    """Manages the Chess UI and engine simulation."""
    def __init__(self, root):
        self.root = root
        self.root.title("Tadfish vs Tadfish")
        self.root.configure(bg="#2E343A")
        self.root.option_add('*Font', 'Helvetica 11')
        self.root.state('zoomed')

        # Main container for layout
        self.main_container = tk.Frame(root, bg="#2E343A")
        self.main_container.pack(expand=True, fill=tk.BOTH, padx=20, pady=20)
        self.main_container.grid_columnconfigure(0, weight=1)
        self.main_container.grid_columnconfigure(1, weight=1)
        self.main_container.grid_rowconfigure(0, weight=1)

        # Left panel for chessboard
        self.left_panel_frame = tk.Frame(self.main_container, bg="#2E343A")
        self.left_panel_frame.grid(row=0, column=0, sticky="nsew", padx=(0, 10))
        self.left_panel_frame.grid_rowconfigure([0, 4], weight=1)
        self.left_panel_frame.grid_columnconfigure([0, 2], weight=1)

        # Black engine label
        self.black_engine_label_board = tk.Label(self.left_panel_frame, text="Tadfish Black", font='Helvetica 12 italic', fg="#E0E0E0", bg="#2E343A", pady=5)
        self.black_engine_label_board.grid(row=1, column=1, pady=(0, 5))

        # Chessboard canvas
        self.square_size = 80
        self.canvas = tk.Canvas(self.left_panel_frame, width=self.square_size * 8, height=self.square_size * 8, bd=0, highlightthickness=0, bg="#2E343A")
        self.canvas.grid(row=2, column=1)

        # White engine label
        self.white_engine_label_board = tk.Label(self.left_panel_frame, text="Tadfish White", font='Helvetica 12 italic', fg="#E0E0E0", bg="#2E343A", pady=5)
        self.white_engine_label_board.grid(row=3, column=1, pady=(5, 0))

        # Right panel for controls
        self.right_panel_frame = tk.Frame(self.main_container, bg="#3C3F41", bd=2, relief=tk.GROOVE)
        self.right_panel_frame.grid(row=0, column=1, sticky="ew", padx=(10, 10), ipady=150)
        self.right_panel_frame.grid_rowconfigure([0, 2], weight=1)
        self.right_panel_frame.grid_columnconfigure([0, 2], weight=1)

        # Inner control frame
        self.inner_control_frame = tk.Frame(self.right_panel_frame, bg="#3C3F41", padx=20, pady=0)
        self.inner_control_frame.grid(row=1, column=1)

        # UI elements
        tk.Label(self.inner_control_frame, text="Tadfish vs Tadfish", font='Helvetica 20 bold', fg="#F0F0F0", bg="#3C3F41", pady=5).pack(pady=(0, 5))
        tk.Label(self.inner_control_frame, text="Tadfish v2.2", font='Helvetica 16', fg="#B0B0B0", bg="#3C3F41", pady=5).pack(pady=(5, 0))
        
        self.status_var = tk.StringVar()
        tk.Label(self.inner_control_frame, textvariable=self.status_var, font='Helvetica 12 bold', fg="#E0E0E0", bg="#3C3F41", pady=5).pack(pady=(0, 10))

        # Depth display
        depth_display_container = tk.Frame(self.inner_control_frame, bg="#3C3F41")
        depth_display_container.pack(pady=10)
        tk.Label(depth_display_container, text="Depth:", fg="#E0E0E0", bg="#3C3F41", font='Helvetica 11').pack(side=tk.LEFT, padx=(0,0))
        self.depth_var = tk.StringVar(value=str(simulation_depth))
        tk.Label(depth_display_container, textvariable=self.depth_var, bg="#3C3F41", fg="white", font='Helvetica 11', width=4, anchor="center").pack(side=tk.LEFT, padx=(0, 0))

        # Simulation control buttons
        button_frame = tk.Frame(self.inner_control_frame, bg="#3C3F41")
        button_frame.pack(pady=(20, 0))
        self.sim_button = tk.Button(button_frame, text="Start Simulation", command=self.start_simulation, bg="#4CAF50", fg="white", activebackground="#60B863", activeforeground="white", relief=tk.RAISED, bd=3, padx=15, pady=8, font='Helvetica 11 bold', cursor="hand2")
        self.sim_button.pack(side=tk.LEFT, padx=(0, 10))
        self.stop_button = tk.Button(button_frame, text="Stop Simulation", command=self.stop_simulation, state=tk.DISABLED, bg="#F44336", fg="white", activebackground="#FF5C52", activeforeground="white", relief=tk.RAISED, bd=3, padx=15, pady=8, font='Helvetica 11 bold', cursor="hand2")
        self.stop_button.pack(side=tk.LEFT, padx=(10, 0))

        # Initialize Pygame mixer and load sounds
        try:
            pygame.mixer.init()
        except Exception as e:
            print(f"Audio initialization failed: {e}. Sounds will not play.")
        self.load_sounds()

        self.board = chess.Board() # Chess board state
        self.simulating = False # Simulation active flag
        self.simulation_ply = 0 # Current ply in simulation
        self.max_plies = 100 # Max plies for simulation
        self.after_id = None # For `after` method cancellation

        self.piece_images = {} # Dictionary to store piece images
        self.load_piece_images_with_check()

        # Placeholder mouse event bindings (not used for engine vs. engine)
        self.canvas.bind("<ButtonPress-1>", self.on_press)
        self.canvas.bind("<B1-Motion>", self.on_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_release)

        self.draw_board() # Initial board draw
        self.update_status() # Initial status update
        self.play_sound(self.sound_start) # Play start sound

    def load_sounds(self):
        """Loads sound files for game events."""
        def try_load(name):
            path = os.path.join(BASE_DIR, "audio", name)
            if os.path.exists(path):
                try: return pygame.mixer.Sound(path)
                except pygame.error as e: print(f"Failed to load sound {name}: {e}"); return None
            else: print(f"Sound file not found: {path}"); return None

        self.sound_move = try_load("move-self.mp3")
        self.sound_capture = try_load("capture.mp3")
        self.sound_check = try_load("move-check.mp3")
        self.sound_checkmate = try_load("checkmate.mp3")
        self.sound_start = try_load("start.mp3")

    def play_sound(self, sound):
        """Plays a given sound."""
        if sound:
            try: sound.play()
            except pygame.error as e: print(f"Error playing sound: {e}")

    def load_piece_images_with_check(self):
        """Loads chess piece images."""
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
                except Exception as e: print(f"Failed to load image {filename}: {e}")
            else: print(f"Image file not found: {path}")

    def draw_board(self):
        """Draws the board and pieces, highlighting checkmate."""
        self.canvas.delete("all")
        
        # Draw squares
        for rank in range(8):
            for file in range(8):
                x1, y1 = file * self.square_size, (7 - rank) * self.square_size
                color = "#F0D9B5" if (rank + file) % 2 == 0 else "#B58863"
                self.canvas.create_rectangle(x1, y1, x1 + self.square_size, y1 + self.square_size, fill=color, outline="")

        # Draw pieces
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
                f, r = chess.square_file(sq), chess.square_rank(sq)
                x1, y1 = f * self.square_size + 2, (7 - r) * self.square_size + 2
                x2, y2 = x1 + self.square_size - 4, y1 + self.square_size - 4
                self.canvas.create_rectangle(x1, y1, x2, y2, outline=CHECKMATE_HIGHLIGHT_COLOR, width=4)

    def update_status(self):
        """Updates the game status message."""
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
        """Starts the engine simulation."""
        if not os.path.exists(ENGINE_PATH):
            messagebox.showerror("Engine Not Found", f"Chess engine executable not found at: {ENGINE_PATH}\nPlease ensure 'tadfish.exe' (or your engine) is in the same directory.")
            return
        
        self.board = chess.Board()
        self.max_plies = 50 * 2 # 50 moves each side
        self.simulation_ply = 0
        self.simulating = True
        self.sim_button.config(state=tk.DISABLED)
        self.stop_button.config(state=tk.NORMAL)
        
        self.draw_board()
        self.update_status()
        self.play_sound(self.sound_start)
        self.after_id = self.root.after(300, self.simulation_step)

    def stop_simulation(self):
        """Stops the engine simulation."""
        self.simulating = False
        self.sim_button.config(state=tk.NORMAL)
        self.stop_button.config(state=tk.DISABLED)
        if self.after_id:
            self.root.after_cancel(self.after_id)
            self.after_id = None
        print("Simulation stopped by user or game end.")

    def simulation_step(self):
        """Performs one simulation step (engine move)."""
        if not self.simulating or self.board.is_game_over() or self.simulation_ply >= self.max_plies:
            self.update_status()
            self.stop_simulation()
            if self.simulation_ply >= self.max_plies:
                messagebox.showinfo("Simulation Complete", "Reached maximum number of moves for simulation. Game is a draw.")
            return

        fen = self.board.fen()
        try:
            result = subprocess.run(
                [ENGINE_PATH, fen, str(simulation_depth)],
                capture_output=True, text=True, check=True, timeout=30
            )
            
            output_parts = result.stdout.strip().split()
            if not output_parts: raise ValueError("Engine returned empty output.")
            
            move_str = output_parts[0]
            eval_score = 0
            if len(output_parts) > 1:
                try:
                    eval_score = int(output_parts[1])
                    if self.board.turn == chess.BLACK: eval_score *= -1
                except ValueError: print("Warning: Could not parse evaluation score from engine output.")
            
            print(f"Engine ({'White' if self.board.turn else 'Black'}) move: {move_str}, Eval: {eval_score} cp")
            move = chess.Move.from_uci(move_str)

        except FileNotFoundError: messagebox.showerror("Engine Error", f"Engine executable not found at {ENGINE_PATH}.\nPlease ensure it exists and is executable."); self.stop_simulation(); return
        except subprocess.CalledProcessError as e: messagebox.showerror("Engine Error", f"Engine returned an error: {e.stderr.strip()}.\nCheck engine path and arguments."); self.stop_simulation(); return
        except subprocess.TimeoutExpired: messagebox.showerror("Engine Error", "Engine did not respond in time."); self.stop_simulation(); return
        except ValueError as e: messagebox.showerror("Engine Output Error", f"Failed to parse engine output: {e}.\nOutput: '{result.stdout.strip()}'"); self.stop_simulation(); return
        except Exception as e: messagebox.showerror("Unexpected Engine Error", f"An unexpected error occurred: {e}"); self.stop_simulation(); return

        if move in self.board.legal_moves:
            is_capture = self.board.is_capture(move)
            gives_check = self.board.gives_check(move)
            
            self.board.push(move)
            self.simulation_ply += 1

            self.draw_board()
            self.update_status()
            
            # Play appropriate sound
            if self.board.is_checkmate(): self.play_sound(self.sound_checkmate)
            elif gives_check: self.play_sound(self.sound_check)
            elif is_capture: self.play_sound(self.sound_capture)
            else: self.play_sound(self.sound_move)

            self.after_id = self.root.after(300, self.simulation_step)
        else:
            messagebox.showerror("Invalid Move", f"Engine returned an illegal move: {move_str} for current board state.\nSimulation stopped.")
            self.stop_simulation()

    def on_press(self, event): """Placeholder for mouse press event."""; pass
    def on_drag(self, event): """Placeholder for mouse drag event."""; pass
    def on_release(self, event): """Placeholder for mouse release event."""; pass

if __name__ == "__main__":
    root = tk.Tk()
    ChessUI(root)
    root.mainloop()