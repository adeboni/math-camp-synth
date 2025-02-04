import random
import os
import subprocess
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import letter

class Song:
    """Class implementing a song"""
    def __init__(self, path: str, index: int) -> None:
        self.path = path
        self.index = index
        self.played_length_s = 0
        self.length_s = 0
        self.data = None
        self.sr = None
        self.author, self.title = os.path.splitext(os.path.basename(path))[0].split(' - ')

    @property
    def song_id(self) -> tuple[int, int]:
        """Returns the song ID as a tuple"""
        return (ord('A') + self.index // 10, self.index % 10)

    @property
    def song_id_str(self) -> str:
        """Returns the song ID as a string"""
        letter, number = self.song_id
        return f'{chr(letter)}{number}'


class SongHandler:
    """Class implementing a song handler"""
    def __init__(self):
        self.songs = [Song(os.path.join('songs', file), i) for i, file in enumerate(sorted(os.listdir('songs')))]
        print(f'Found {len(self.songs)} songs')

    def get_booklet_letter_limit(self) -> str:
        """Returns the maximum booklet letter"""
        return chr(ord('A') + (len(self.songs) - 1) // 10)


MARGIN_MM = 20
BLACK = (0.0, 0.0, 0.0, 1.0)
WHITE = (0.0, 0.0, 0.0, 0.0)
LIGHT_GREY = (0.0, 0.0, 0.0, 0.3)
BORDERS = [ 
    (0.0, 0.98, 0.98, 0.0),  # red
    (0.0, 0.47, 0.98, 0.01), # orange
    (0.69, 0.23, 0.0, 0.32), # blue
    (0.74, 0.0, 0.63, 0.20), # green
    (0.0, 1.0, 0.0, 0.0)     # pink
] 
FILLS = [
    (0.0, 0.16, 0.12, 0.01), # pink
    (0.0, 0.06, 0.15, 0.01), # yellow
    (0.10, 0.03, 0.0, 0.04), # teal
    (0.10, 0.0, 0.07, 0.04), # green
    (0.0, 0.16, 0.13, 0.01)  # rose
]

def mm_to_pt_abs(mm: float):
    inches = mm / 25.4
    return inches * 72

def mm_to_pt_x(mm: float, apply_margin: bool=True):
    if apply_margin:
        mm += MARGIN_MM
    return mm_to_pt_abs(mm)

def mm_to_pt_y(mm: float, apply_margin: bool=True):
    if apply_margin:
        mm += MARGIN_MM
        mm = 279.4 - mm
    return mm_to_pt_abs(mm)

def draw_fiducial(c: canvas.Canvas, xmm: float, ymm: float) -> None:
    c.line(mm_to_pt_x(xmm - 3), mm_to_pt_y(ymm), mm_to_pt_x(xmm + 3), mm_to_pt_y(ymm))
    c.line(mm_to_pt_x(xmm), mm_to_pt_y(ymm - 3), mm_to_pt_x(xmm), mm_to_pt_y(ymm + 3))

def format_page(c: canvas.Canvas) -> None:
    c.setStrokeColorCMYK(*LIGHT_GREY)
    if c.getPageNumber() % 2 == 0:
        draw_fiducial(c, 0, 0)
        draw_fiducial(c, 142.98 + 2.52, 0)
        draw_fiducial(c, 142.98 + 2.52, 194.12)
        draw_fiducial(c, 0, 194.12)
        c.circle(mm_to_pt_x(6.24), mm_to_pt_y(49.06), mm_to_pt_abs(3))
        c.circle(mm_to_pt_x(6.24), mm_to_pt_y(49.06 + 96), mm_to_pt_abs(3))
    else:
        draw_fiducial(c, -2.52, 0)
        draw_fiducial(c, 142.98, 0)
        draw_fiducial(c, 142.98, 194.12)
        draw_fiducial(c, -2.52, 194.12)
        c.circle(mm_to_pt_x(142.98 - 6.24), mm_to_pt_y(49.06), mm_to_pt_abs(3))
        c.circle(mm_to_pt_x(142.98 - 6.24), mm_to_pt_y(49.06 + 96), mm_to_pt_abs(3))

def add_song(c: canvas.Canvas, i: int, song: Song, assigned_colors: dict[int, tuple[float, float, float, float]]) -> None:
    col, row = divmod(i, 10)
    x, y = 11.8 if c.getPageNumber() % 2 == 0 else 2.18, 2.56

    prev_x_color = assigned_colors.get(10 * col + row - 1, None)
    prev_y_color = assigned_colors.get(10 * (col - 1) + row, None)
    color_index = random.randrange(len(BORDERS))
    while prev_x_color == color_index or prev_y_color == color_index:
        color_index = random.randrange(len(BORDERS))
    assigned_colors[i] = color_index

    c.setStrokeColorCMYK(*BORDERS[color_index])
    c.setFillColorCMYK(*BORDERS[color_index])
    c.rect(mm_to_pt_x(x + col * 65), mm_to_pt_y(y + row * 19), mm_to_pt_abs(64), -mm_to_pt_abs(18), stroke=1, fill=1) 

    c.setStrokeColorCMYK(*FILLS[color_index])
    c.setFillColorCMYK(*FILLS[color_index])
    path = c.beginPath()
    path.moveTo(mm_to_pt_x(x + col * 65 + 1.64), mm_to_pt_y(y + row * 19 + 1.64))
    path.lineTo(mm_to_pt_x(x + col * 65 + 62.36), mm_to_pt_y(y + row * 19 + 1.64))
    path.lineTo(mm_to_pt_x(x + col * 65 + 62.36), mm_to_pt_y(y + row * 19 + 1.64 + 4.61))
    path.lineTo(mm_to_pt_x(x + col * 65 + 64 - (9.95 - 2.3)), mm_to_pt_y(y + row * 19 + 1.64 + 4.61))
    path.lineTo(mm_to_pt_x(x + col * 65 + 64 - 9.95), mm_to_pt_y(y + row * 19 + 1.64 + 4.61 + 5.5/2))
    path.lineTo(mm_to_pt_x(x + col * 65 + 64 - (9.95 - 2.3)), mm_to_pt_y(y + row * 19 + 1.64 + 4.61 + 5.5))
    path.lineTo(mm_to_pt_x(x + col * 65 + 62.36), mm_to_pt_y(y + row * 19 + 1.64 + 4.61 + 5.5))
    path.lineTo(mm_to_pt_x(x + col * 65 + 62.36), mm_to_pt_y(y + row * 19 + 16.36))
    path.lineTo(mm_to_pt_x(x + col * 65 + 1.64), mm_to_pt_y(y + row * 19 + 16.36))
    path.lineTo(mm_to_pt_x(x + col * 65 + 1.64), mm_to_pt_y(y + row * 19 + 1.64 + 4.61 + 5.5))
    path.lineTo(mm_to_pt_x(x + col * 65 + 1.64 + 8.66 - 2.3), mm_to_pt_y(y + row * 19 + 1.64 + 4.61 + 5.5))
    path.lineTo(mm_to_pt_x(x + col * 65 + 1.64 + 8.66), mm_to_pt_y(y + row * 19 + 1.64 + 4.61 + 5.5 / 2))
    path.lineTo(mm_to_pt_x(x + col * 65 + 1.64 + 8.66 - 2.3), mm_to_pt_y(y + row * 19 + 1.64 + 4.61))
    path.lineTo(mm_to_pt_x(x + col * 65 + 1.64), mm_to_pt_y(y + row * 19 + 1.64 + 4.61))
    path.lineTo(mm_to_pt_x(x + col * 65 + 1.64), mm_to_pt_y(y + row * 19 + 1.64))
    c.drawPath(path, stroke=1, fill=1)

    c.setFillColorCMYK(*WHITE)
    c.setFont("Helvetica-Bold", 13.0)
    c.drawCentredString(mm_to_pt_x(x + col * 65 + 4.64), mm_to_pt_y(y + row * 19 + 10.5), song.song_id_str)
    c.setFillColorCMYK(*BLACK)
    c.setFont("Helvetica-Bold", 8.0 if len(song.title) > 20 else 12.0)
    c.drawCentredString(mm_to_pt_x(x + col * 65 + 32), mm_to_pt_y(y + row * 19 + 8), song.title)
    c.setFont("Helvetica", 8.0 if len(song.author) > 20 else 12.0)
    c.drawCentredString(mm_to_pt_x(x + col * 65 + 32), mm_to_pt_y(y + row * 19 + 14), song.author)

if __name__ == '__main__':
    if not os.path.exists("songs"):
        print("Missing 'songs' folder")
        quit()
    
    song_handler = SongHandler()
    assigned_colors = {}

    c = canvas.Canvas("booklet.pdf", pagesize=letter)
    format_page(c)
    for i, song in enumerate(song_handler.songs):
        if i % 20 == 0 and i != 0:
            assigned_colors.clear()
            c.showPage()
            format_page(c)
        add_song(c, i % 20, song, assigned_colors)

    if i < 40:
        song = Song('Artist - Title.mp3', 0)
        for i in range(i + 1, 40):
            if i % 20 == 0 and i != 0:
                assigned_colors.clear()
                c.showPage()
                format_page(c)
            add_song(c, i % 20, song, assigned_colors)

    c.save()

    p = subprocess.Popen("normalize_songs.bat")
    stdout, stderr = p.communicate()
    
    p = subprocess.Popen("create_directory.bat")
    stdout, stderr = p.communicate()