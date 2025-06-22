import tkinter as tk
from tkinter import messagebox
import chess
import os
import pygame
from PIL import Image, ImageTk

simulation_depth = 3 # Engine search depth (display only)

HIGHLIGHT_COLOR = "#5C4033"
CHECKMATE_HIGHLIGHT_COLOR = "#FF0000"

BASE_DIR = os.path.dirname(os.path.abspath(__file__)) # Resource directory

# Determine best resampling filter
try:
    RESAMPLE = Image.Resampling.LANCZOS
except AttributeError:
    try:
        RESAMPLE = Image.LANCZOS
    except AttributeError:
        RESAMPLE = Image.BICUBIC

class ChessUI:
    """Manages the Chess UI and human player interaction."""
    def __init__(self, root):
        self.root = root
        self.root.title("Player vs Player")

        self.root.configure(bg="#2E343A")
        self.root.option_add('*Font', 'Helvetica 11')
        self.root.state('zoomed')

        self.main_container = tk.Frame(root, bg="#2E343A")
        self.main_container.pack(expand=True, fill=tk.BOTH, padx=20, pady=20)

        self.main_container.grid_columnconfigure(0, weight=1)
        self.main_container.grid_rowconfigure(0, weight=1)
        self.main_container.grid_rowconfigure(1, weight=0)
        self.main_container.grid_rowconfigure(2, weight=0)
        self.main_container.grid_rowconfigure(3, weight=0)
        self.main_container.grid_rowconfigure(4, weight=0)
        self.main_container.grid_rowconfigure(5, weight=1)

        self.black_engine_label_board = tk.Label(self.main_container, text="Player Black",
                                                 font='Helvetica 12 italic', fg="#E0E0E0", bg="#2E343A", pady=5)
        self.black_engine_label_board.grid(row=1, column=0, pady=(0, 5))

        self.square_size = 80
        self.canvas = tk.Canvas(self.main_container, width=self.square_size * 8, height=self.square_size * 8, bd=0, highlightthickness=0, bg="#2E343A")
        self.canvas.grid(row=2, column=0)

        self.white_engine_label_board = tk.Label(self.main_container, text="Player White",
                                                 font='Helvetica 12 italic', fg="#E0E0E0", bg="#2E343A", pady=5)
        self.white_engine_label_board.grid(row=3, column=0, pady=(5, 0))

        self.status_var = tk.StringVar()
        tk.Label(self.main_container, textvariable=self.status_var,
                 font='Helvetica 12 bold', fg="#E0E0E0", bg="#2E343A", pady=5).grid(row=4, column=0, pady=(5, 0))

        try:
            pygame.mixer.init()
        except Exception:
            print("Audio init failed. Sounds will not play.")
        self.load_sounds()

        self.board = chess.Board()

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

        self.piece_images = {}
        missing = self.load_piece_images_with_check()
        if missing:
            msg = "The following piece image files failed to load:\n" + "\n".join(missing) + \
                  "\n\nEnsure these files exist in the 'images' folder next to frontend.py, with exact names."
            messagebox.showerror("Missing Piece Images", msg)

        self.canvas.bind("<ButtonPress-1>", self.on_press)
        self.canvas.bind("<B1-Motion>", self.on_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_release)

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
                except pygame.error:
                    return None
            return None

        self.sound_move = try_load("move-self.mp3")
        self.sound_capture = try_load("capture.mp3")
        self.sound_check = try_load("move-check.mp3")
        self.sound_checkmate = try_load("checkmate.mp3")
        self.sound_start = try_load("start.mp3")

    def play_sound(self, sound_obj):
        """Plays a given sound."""
        if sound_obj:
            try:
                sound_obj.play()
            except pygame.error:
                pass

    def load_piece_images_with_check(self):
        """Loads chess piece images, returns list of missing files."""
        mapping = {
            'P': 'white-pawn.png', 'N': 'white-knight.png', 'B': 'white-bishop.png',
            'R': 'white-rook.png', 'Q': 'white-queen.png', 'K': 'white-king.png',
            'p': 'black-pawn.png', 'n': 'black-knight.png', 'b': 'black-bishop.png',
            'r': 'black-rook.png', 'q': 'black-queen.png', 'k': 'black-king.png',
        }
        missing = []
        for symbol, filename in mapping.items():
            path = os.path.join(BASE_DIR, "images", filename)
            if os.path.exists(path):
                try:
                    img = Image.open(path).convert("RGBA")
                    img = img.resize((self.square_size, self.square_size), RESAMPLE)
                    self.piece_images[symbol] = ImageTk.PhotoImage(img)
                except Exception as e:
                    missing.append(f"{filename} (error: {e})")
            else:
                missing.append(f"{filename} (not found at {path})")
        return missing

    def draw_board(self):
        """Draws the board, pieces, highlights, and checkmate."""
        self.canvas.delete("all")

        # Draw squares
        for rank in range(8):
            for file in range(8):
                x1 = file * self.square_size
                y1 = (7 - rank) * self.square_size
                x2 = x1 + self.square_size
                y2 = y1 + self.square_size
                color = "#F0D9B5" if (rank + file) % 2 == 0 else "#B58863"
                self.canvas.create_rectangle(x1, y1, x2, y2, fill=color, outline="")

        # Highlight selected square
        if self.selected_square is not None and not self.dragging:
            f = chess.square_file(self.selected_square)
            r = chess.square_rank(self.selected_square)
            x1 = f * self.square_size
            y1 = (7 - r) * self.square_size
            x2 = x1 + self.square_size
            y2 = y1 + self.square_size
            self.canvas.create_rectangle(x1, y1, x2, y2, outline=HIGHLIGHT_COLOR, width=3)

        # Draw dots for legal destinations
        for dest in self.legal_destinations:
            f = chess.square_file(dest)
            r = chess.square_rank(dest)
            cx = f * self.square_size + self.square_size/2
            cy = (7 - r) * self.square_size + self.square_size/2
            rad = self.square_size * 0.1
            self.canvas.create_oval(cx-rad, cy-rad, cx+rad, cy+rad, fill=HIGHLIGHT_COLOR, outline="")

        # Draw pieces (excluding dragged piece)
        for square in chess.SQUARES:
            if self.dragging and square == self.drag_start:
                continue

            piece = self.board.piece_at(square)
            if piece:
                symbol = piece.symbol()
                img = self.piece_images.get(symbol)
                f = chess.square_file(square)
                r = chess.square_rank(square)
                x = f * self.square_size + self.square_size/2
                y = (7 - r) * self.square_size + self.square_size/2
                if img:
                    self.canvas.create_image(x, y, image=img)
                else: # Fallback to Unicode
                    uni = {'K': '♔', 'Q': '♕', 'R': '♖', 'B': '♗', 'N': '♘', 'P': '♙',
                           'k': '♚', 'q': '♛', 'r': '♜', 'b': '♝', 'n': '♞', 'p': '♟'}.get(symbol, '')
                    if uni:
                        size = int(self.square_size * 0.6)
                        self.canvas.create_text(x, y, text=uni, font=("Arial", size))

        # Draw the dragged piece
        if self.dragging and self.drag_piece:
            x = self.mouse_x
            y = self.mouse_y
            symbol = self.drag_piece.symbol()
            img = self.piece_images.get(symbol)
            if img:
                self.canvas.create_image(x, y, image=img)
            else: # Fallback to Unicode
                uni = {'K': '♔', 'Q': '♕', 'R': '♖', 'B': '♗', 'N': '♘', 'P': '♙',
                       'k': '♚', 'q': '♛', 'r': '♜', 'b': '♝', 'n': '♞', 'p': '♟'}.get(symbol, '')
                if uni:
                    size = int(self.square_size * 0.6)
                    self.canvas.create_text(x, y, text=uni, font=("Arial", size))

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

    def on_press(self, event):
        """Handles mouse button press to select a piece."""
        file = int(event.x // self.square_size)
        rank = 7 - int(event.y // self.square_size)
        if not (0 <= file < 8 and 0 <= rank < 8): return

        sq = chess.square(file, rank)
        piece = self.board.piece_at(sq)

        if piece and piece.color == self.board.turn:
            self.pressed_x = event.x
            self.pressed_y = event.y
            self.drag_start = sq
            self.drag_piece = piece
            self.selected_square = sq
            self.legal_destinations = [m.to_square for m in self.board.legal_moves if m.from_square == sq]
            self.draw_board()

    def on_drag(self, event):
        """Handles mouse drag to move a piece."""
        if not self.drag_piece: return

        dx = abs(event.x - self.pressed_x)
        dy = abs(event.y - self.pressed_y)

        if not self.dragging and (dx >= self.min_drag_distance or dy >= self.min_drag_distance):
            self.dragging = True

        if self.dragging:
            self.mouse_x = event.x
            self.mouse_y = event.y
            self.draw_board()

    def on_release(self, event):
        """Handles mouse button release, completing a move."""
        if self.drag_start is None: return

        x = min(max(event.x, 0), self.square_size * 8 - 1)
        y = min(max(event.y, 0), self.square_size * 8 - 1)
        file = x // self.square_size
        rank = 7 - (y // self.square_size)
        dest_sq = chess.square(file, rank)

        move = None
        for m in self.board.legal_moves:
            if m.from_square == self.drag_start and m.to_square == dest_sq:
                move = m
                break

        was_capture = False
        if move:
            if self.board.is_capture(move):
                was_capture = True
            self.board.push(move)
            if self.board.is_checkmate():
                self.play_sound(self.sound_checkmate)
            else:
                self.play_sound(self.sound_capture if was_capture else self.sound_move)
                if self.board.is_check():
                    self.play_sound(self.sound_check)
        else: # No legal move, re-select if valid
            piece_at_dest = self.board.piece_at(dest_sq)
            if piece_at_dest and piece_at_dest.color == self.board.turn:
                self.selected_square = dest_sq
                self.legal_destinations = [m.to_square for m in self.board.legal_moves if m.from_square == dest_sq]
                self.drag_start = dest_sq
                self.drag_piece = piece_at_dest
                f = chess.square_file(dest_sq)
                r = chess.square_rank(dest_sq)
                self.mouse_x = f * self.square_size + self.square_size / 2
                self.mouse_y = (7 - r) * self.square_size + self.square_size / 2
                self.draw_board()
                self.update_status()
                return

        # Reset drag state and redraw
        self.dragging = False
        self.drag_start = None
        self.drag_piece = None
        self.selected_square = None
        self.legal_destinations = []
        self.draw_board()
        self.update_status()

    def update_status(self):
        """Updates the game status message."""
        if self.board.is_checkmate():
            winner = "Black" if self.board.turn else "White"
            self.status_var.set(f"Checkmate! {winner} wins.")
        elif self.board.is_stalemate():
            self.status_var.set("Stalemate! Draw.")
        elif self.board.is_insufficient_material():
            self.status_var.set("Draw: insufficient material.")
        elif self.board.can_claim_fifty_moves():
            self.status_var.set("Draw: fifty-move rule claimable.")
        elif self.board.can_claim_threefold_repetition():
            self.status_var.set("Draw: threefold repetition claimable.")
        else:
            turn_color = "White" if self.board.turn else "Black"
            self.status_var.set(f"{turn_color} to move.")

if __name__ == "__main__":
    root = tk.Tk()
    ChessUI(root)
    root.mainloop()