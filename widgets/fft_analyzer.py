import math
import numpy as np
from PySide6.QtWidgets import QWidget, QSizePolicy
from PySide6.QtCore import Qt, QTimer, QPointF, QRectF
from PySide6.QtGui import QPainter, QColor, QPen, QBrush, QFont, QLinearGradient, QPainterPath

class FftAnalyzerWidget(QWidget):
    """Real-Time FFT Spectrum Analyzer with glowing red waveforms and logarithmic mapping."""
    def __init__(self, audio_engine, parent=None):
        super().__init__(parent)
        self.audio_engine = audio_engine
        
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.setMinimumHeight(150)
        
        # Exponential smoothing variables
        self.smooth_db = None
        self.decay = 0.65  # Smoothing factor (decay rate for peak fall)
        
        # Timer to trigger repaint at ~33 FPS
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update)
        self.timer.start(30)
        
        # Grid parameters
        self.min_freq = 20.0
        self.max_freq = 20000.0
        self.min_db = -60.0
        self.max_db = 10.0
        
    def freq_to_x(self, freq, w):
        if freq < self.min_freq:
            freq = self.min_freq
        if freq > self.max_freq:
            freq = self.max_freq
        return w * math.log(freq / self.min_freq) / math.log(self.max_freq / self.min_freq)
        
    def db_to_y(self, db, h):
        if db < self.min_db:
            db = self.min_db
        if db > self.max_db:
            db = self.max_db
        return h * (self.max_db - db) / (self.max_db - self.min_db)

    def paintEvent(self, event):
        painter = QPainter(self)
        try:
            painter.setRenderHint(QPainter.RenderHint.Antialiasing)
            
            w = self.width()
            h = self.height()
            
            # 1. Fill background (pure black for theme contrast)
            painter.fillRect(0, 0, w, h, QColor("#000000"))
            
            # Draw thin border around the analyzer widget
            border_pen = QPen(QColor("#222225"), 1)
            painter.setPen(border_pen)
            painter.drawRect(0, 0, w - 1, h - 1)
            
            # 2. Draw Grids
            self.draw_grids(painter, w, h)
            
            # 3. Read audio buffer and compute FFT
            fft_db = None
            samples = []
            
            if self.audio_engine and self.audio_engine.is_running:
                # Retrieve latest 2048 samples from fft_buffer
                samples = self.audio_engine.fft_buffer.read_latest(2048)
                
            if len(samples) >= 512:
                # Subtract mean to remove DC offset
                samples = samples - np.mean(samples)
                
                # Apply Hanning window
                window = np.hanning(len(samples))
                windowed = samples * window
                
                # Compute real FFT
                fft_vals = np.fft.rfft(windowed)
                # Normalize magnitude relative to window sum (Hanning window sum is N/2)
                fft_mag = np.abs(fft_vals) / (len(samples) / 2.0)
                # Convert to dB
                fft_db = 20.0 * np.log10(fft_mag + 1e-6)
            else:
                # Default silent state: -60 dB across the spectrum
                fft_db = np.full(513, -60.0)
                
            # 4. Smooth FFT values over time
            if self.smooth_db is None or len(self.smooth_db) != len(fft_db):
                self.smooth_db = fft_db.copy()
            else:
                # Exponential moving average smoothing
                self.smooth_db = self.smooth_db * self.decay + fft_db * (1.0 - self.decay)
                
            # 5. Draw the FFT Waveform Curve
            self.draw_spectrum_curve(painter, w, h)
        finally:
            painter.end()
            
    def draw_grids(self, painter, w, h):
        # Grid font
        font = QFont("Consolas", 8)
        painter.setFont(font)
        
        # Logarithmic Frequency vertical grid lines
        freqs = [
            (50, "50 Hz"), (100, "100 Hz"), (200, "200 Hz"), (500, "500 Hz"),
            (1000, "1 kHz"), (2000, "2 kHz"), (5000, "5 kHz"), (10000, "10 kHz"), (20000, "20 kHz")
        ]
        
        grid_pen = QPen(QColor("#151518"), 1, Qt.PenStyle.DashLine)
        label_color = QColor("#55555c")
        
        for f, lbl in freqs:
            x = self.freq_to_x(f, w)
            # Draw line
            painter.setPen(grid_pen)
            painter.drawLine(int(x), 0, int(x), h)
            
            # Draw label (skip first/last slightly to avoid clip, draw rotated or just simple horizontal text)
            painter.setPen(label_color)
            lbl_rect = QRectF(x - 30, h - 18, 60, 15)
            painter.drawText(lbl_rect, Qt.AlignmentFlag.AlignCenter, lbl)
            
        # Decibel horizontal grid lines
        dbs = [0.0, -6.0, -12.0, -18.0, -24.0, -36.0, -48.0, -60.0]
        
        for db in dbs:
            y = self.db_to_y(db, h)
            # Draw line
            painter.setPen(grid_pen)
            painter.drawLine(0, int(y), w, int(y))
            
            # Draw label on the right
            painter.setPen(label_color)
            lbl_text = f"{int(db)} dB" if db != 0 else "0 dB"
            painter.drawText(w - 45, int(y) - 2, lbl_text)

    def draw_spectrum_curve(self, painter, w, h):
        if self.smooth_db is None:
            return
            
        N = len(self.smooth_db)
        sample_rate = self.audio_engine.sample_rate if self.audio_engine else 44100
        
        # We will build two paths: 
        # 1. fill_path: closed path for gradient filling
        # 2. line_path: open path for drawing the glowing top line
        fill_path = QPainterPath()
        line_path = QPainterPath()
        
        # Starting point
        start_freq = self.min_freq
        start_bin = start_freq * (N - 1) / (sample_rate / 2.0)
        start_bin_clamped = max(0.0, min(N - 1, start_bin))
        start_val = self.interpolate_db(start_bin_clamped)
        start_y = self.db_to_y(start_val, h)
        
        fill_path.moveTo(0, h)
        fill_path.lineTo(0, start_y)
        line_path.moveTo(0, start_y)
        
        # Sample points horizontally across pixels
        # For performance, we step by 2 pixels
        step = 2
        for x in range(step, w + 1, step):
            # Map x coordinate to logarithmic frequency
            t = x / w
            freq = self.min_freq * math.pow(self.max_freq / self.min_freq, t)
            
            # Find the bin index float
            bin_idx = freq * (N - 1) / (sample_rate / 2.0)
            bin_idx = max(0.0, min(N - 1, bin_idx))
            
            db_val = self.interpolate_db(bin_idx)
            y = self.db_to_y(db_val, h)
            
            fill_path.lineTo(x, y)
            line_path.lineTo(x, y)
            
        # Close fill path
        fill_path.lineTo(w, h)
        fill_path.close()
        
        # Draw Gradient Fill
        grad = QLinearGradient(0, 0, 0, h)
        # Red transparent gradient for Nothing brand look
        grad.setColorAt(0.0, QColor(255, 0, 51, 60))
        grad.setColorAt(0.7, QColor(255, 0, 51, 20))
        grad.setColorAt(1.0, QColor(255, 0, 51, 0))
        
        painter.setBrush(QBrush(grad))
        painter.setPen(Qt.PenStyle.NoPen)
        painter.drawPath(fill_path)
        
        # Draw Glowing Waveform Outline
        outline_pen = QPen(QColor("#ff0033"), 1.8)
        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.setPen(outline_pen)
        painter.drawPath(line_path)

    def interpolate_db(self, bin_idx):
        idx_low = int(math.floor(bin_idx))
        idx_high = int(math.ceil(bin_idx))
        
        # Handle boundary
        if idx_high >= len(self.smooth_db):
            return self.smooth_db[len(self.smooth_db) - 1]
            
        weight = bin_idx - idx_low
        val_low = self.smooth_db[idx_low]
        val_high = self.smooth_db[idx_high]
        
        return (1.0 - weight) * val_low + weight * val_high

    def closeEvent(self, event):
        self.timer.stop()
        super().closeEvent(event)
