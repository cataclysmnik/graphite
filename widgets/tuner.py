import math
import numpy as np
from PySide6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QSpinBox, QFrame, QSizePolicy
)
from PySide6.QtCore import Qt, QTimer, QRectF, QPointF
from PySide6.QtGui import QPainter, QColor, QPen, QBrush, QFont, QRadialGradient, QLinearGradient

class TunerDial(QWidget):
    """Custom painted needle dial for cents deviation."""
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.setMinimumHeight(150)
        self.cents = 0.0          # Current target cents deviation (-50 to +50)
        self.smoothed_cents = 0.0 # Low-pass filtered cents for smooth needle movement
        self.has_signal = False
        
        # Timer to update needle physics smooth sweep
        self.physics_timer = QTimer(self)
        self.physics_timer.timeout.connect(self.update_physics)
        self.physics_timer.start(16) # ~60 FPS animation

    def set_cents(self, cents, has_signal):
        self.cents = cents
        self.has_signal = has_signal
        if not has_signal:
            self.cents = 0.0

    def update_physics(self):
        if self.has_signal:
            # Smooth interpolation: 85% old value, 15% new value
            self.smoothed_cents = self.smoothed_cents * 0.85 + self.cents * 0.15
        else:
            # Fall back to center slowly
            self.smoothed_cents = self.smoothed_cents * 0.9 + 0.0 * 0.1
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        w = self.width()
        h = self.height()
        
        # Center of the dial arc is at the bottom-center of the widget
        cx = w / 2.0
        cy = h - 20.0
        
        # Calculate radius based on widget size
        radius = min(w / 2.0 - 30.0, h - 40.0)
        if radius < 50.0:
            radius = 50.0
            
        # Draw background panel glow
        painter.fillRect(0, 0, w, h, QColor("#1e1e1e"))
        
        # Draw target guide arc
        arc_rect = QRectF(cx - radius, cy - radius, radius * 2.0, radius * 2.0)
        
        # Paint color-coded arc segments
        # Cents range from -50 (left) to +50 (right)
        # In angular degrees: flat (-50 cents) is 150 deg, center (0 cents) is 90 deg, sharp (+50 cents) is 30 deg.
        # Draw Flat Arc (Red to Orange)
        pen_flat = QPen(QColor("#a63e3e"), 4)
        painter.setPen(pen_flat)
        painter.drawArc(arc_rect, 110 * 16, 40 * 16) # From -50 to -15 cents (150 to 110 deg)
        
        # Draw Near In-tune Arc (Yellow)
        pen_near = QPen(QColor("#d4a317"), 4)
        painter.setPen(pen_near)
        painter.drawArc(arc_rect, 95 * 16, 15 * 16)  # From -15 to -5 cents (110 to 95 deg)
        painter.drawArc(arc_rect, 70 * 16, 15 * 16)  # From +5 to +15 cents (85 to 70 deg)
        
        # Draw In-tune Arc (Vibrant Green)
        pen_green = QPen(QColor("#2b5a30"), 6)
        painter.setPen(pen_green)
        painter.drawArc(arc_rect, 85 * 16, 10 * 16)  # From -5 to +5 cents (95 to 85 deg)
        
        # Draw Sharp Arc (Orange to Red)
        painter.setPen(pen_flat)
        painter.drawArc(arc_rect, 30 * 16, 40 * 16)  # From +15 to +50 cents (70 to 30 deg)
        
        # Draw Tick Marks
        painter.setPen(QPen(QColor("#555555"), 1))
        font_ticks = QFont("Segoe UI", 7)
        painter.setFont(font_ticks)
        
        for cents_val in range(-50, 51, 10):
            # Calculate angle in radians
            # -50 cents = 150 deg (5*pi/6), 0 cents = 90 deg (pi/2), +50 cents = 30 deg (pi/6)
            angle_deg = 90.0 - (cents_val / 50.0) * 60.0
            angle_rad = math.radians(angle_deg)
            
            # Tick lines
            cos_a = math.cos(angle_rad)
            sin_a = math.sin(angle_rad)
            
            x1 = cx + (radius - 5.0) * cos_a
            y1 = cy - (radius - 5.0) * sin_a
            x2 = cx + (radius + 5.0) * cos_a
            y2 = cy - (radius + 5.0) * sin_a
            
            # Make center tick prominent
            if cents_val == 0:
                painter.setPen(QPen(QColor("#2b5a30"), 2.5))
                painter.drawLine(x1 - cos_a * 2.0, y1 + sin_a * 2.0, x2 + cos_a * 2.0, y2 - sin_a * 2.0)
            else:
                painter.setPen(QPen(QColor("#666666"), 1))
                painter.drawLine(x1, y1, x2, y2)
                
            # Draw tick labels (-50, 0, +50)
            if cents_val in (-50, 0, 50):
                painter.setPen(QColor("#888888"))
                lbl_text = f"+{cents_val}" if cents_val > 0 else str(cents_val)
                if cents_val == 0:
                    lbl_text = "0"
                tx = cx + (radius + 18.0) * cos_a - 10.0
                ty = cy - (radius + 18.0) * sin_a + 5.0
                painter.drawText(int(tx), int(ty), 20, 12, Qt.AlignmentFlag.AlignCenter, lbl_text)
                
        # Draw Center Glow Circle
        radial_grad = QRadialGradient(cx, cy, 30)
        if self.has_signal:
            if abs(self.smoothed_cents) <= 3.0:
                # Perfectly in tune glow (green)
                radial_grad.setColorAt(0, QColor(43, 90, 48, 180))
                radial_grad.setColorAt(1, QColor(43, 90, 48, 0))
            elif abs(self.smoothed_cents) <= 15.0:
                # Slightly out of tune glow (yellow/orange)
                radial_grad.setColorAt(0, QColor(212, 163, 23, 100))
                radial_grad.setColorAt(1, QColor(212, 163, 23, 0))
            else:
                # Far out of tune glow (red)
                radial_grad.setColorAt(0, QColor(166, 62, 62, 100))
                radial_grad.setColorAt(1, QColor(166, 62, 62, 0))
        else:
            # Idle/no signal glow
            radial_grad.setColorAt(0, QColor(80, 80, 80, 50))
            radial_grad.setColorAt(1, QColor(80, 80, 80, 0))
            
        painter.setBrush(QBrush(radial_grad))
        painter.setPen(Qt.PenStyle.NoPen)
        painter.drawEllipse(QPointF(cx, cy), radius * 0.7, radius * 0.7)

        # Draw central note guidelines circle
        painter.setPen(QPen(QColor("#333333"), 1.5))
        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.drawEllipse(QPointF(cx, cy), 16, 16)
        
        # --- Draw the Needle ---
        needle_angle_deg = 90.0 - (self.smoothed_cents / 50.0) * 60.0
        needle_angle_rad = math.radians(needle_angle_deg)
        
        needle_len = radius - 8.0
        nx = cx + needle_len * math.cos(needle_angle_rad)
        ny = cy - needle_len * math.sin(needle_angle_rad)
        
        # Choose needle color based on tuning accuracy
        if self.has_signal:
            if abs(self.smoothed_cents) <= 3.0:
                needle_color = QColor("#4caf50") # In tune green
            elif abs(self.smoothed_cents) <= 15.0:
                needle_color = QColor("#ffeb3b") # Near in tune yellow
            else:
                needle_color = QColor("#f44336") # Out of tune red
        else:
            needle_color = QColor("#555555") # Inactive gray
            
        # Draw needle line
        painter.setPen(QPen(needle_color, 2))
        painter.drawLine(cx, cy, nx, ny)
        
        # Draw needle cap pivot
        painter.setPen(QPen(QColor("#252526"), 2))
        painter.setBrush(QBrush(needle_color))
        painter.drawEllipse(QPointF(cx, cy), 6, 6)


class GuitarTunerWidget(QWidget):
    """Detailed aesthetic built-in guitar tuner widget."""
    def __init__(self, audio_engine, parent=None):
        super().__init__(parent)
        self.audio_engine = audio_engine
        self.ref_a4 = 440.0
        
        self.setup_ui()
        
        # Real-time analysis polling timer
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.process_tuner)
        self.timer.start(50)  # Polling audio at 20 FPS (every 50ms)

    def setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(15, 10, 15, 10)
        layout.setSpacing(10)
        
        # --- Top Toolbar ---
        toolbar = QHBoxLayout()
        toolbar.setSpacing(15)
        
        title = QLabel("GUITAR TUNER")
        title.setFont(QFont("Segoe UI", 11, QFont.Weight.Bold))
        title.setStyleSheet("color: #ffffff; letter-spacing: 1.0px;")
        toolbar.addWidget(title)
        
        toolbar.addStretch()
        
        # A4 reference calibration label
        ref_label = QLabel("A4 Reference:")
        ref_label.setFont(QFont("Segoe UI", 9))
        ref_label.setStyleSheet("color: #b3b3b3;")
        toolbar.addWidget(ref_label)
        
        # Spinbox for calibration input
        self.spin_a4 = QSpinBox()
        self.spin_a4.setObjectName("TunerRefSpinbox")
        self.spin_a4.setRange(400, 480)
        self.spin_a4.setValue(440)
        self.spin_a4.setSuffix(" Hz")
        self.spin_a4.valueChanged.connect(self.on_a4_changed)
        self.spin_a4.setMinimumWidth(85)
        self.spin_a4.setStyleSheet("""
            QSpinBox#TunerRefSpinbox {
                background-color: #252526;
                border: 1px solid #3e3e42;
                border-radius: 3px;
                color: #ffffff;
                padding: 4px 6px;
                font-family: "Segoe UI", sans-serif;
                font-size: 11px;
                font-weight: bold;
            }
        """)
        toolbar.addWidget(self.spin_a4)
        
        layout.addLayout(toolbar)
        
        # --- Body layout (Note display & needle dial) ---
        body_layout = QHBoxLayout()
        body_layout.setSpacing(20)
        
        # Left Panel: Digital Note Display Card
        self.note_card = QFrame()
        self.note_card.setObjectName("NoteCard")
        self.note_card.setMinimumWidth(180)
        self.note_card.setMaximumWidth(220)
        self.note_card.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Expanding)
        self.note_card.setStyleSheet("""
            QFrame#NoteCard {
                background-color: #252526;
                border: 1px solid #333333;
                border-radius: 6px;
            }
        """)
        
        card_layout = QVBoxLayout(self.note_card)
        card_layout.setContentsMargins(10, 15, 10, 15)
        card_layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        card_layout.setSpacing(5)
        
        self.lbl_note = QLabel("--")
        self.lbl_note.setFont(QFont("Consolas", 36, QFont.Weight.Bold))
        self.lbl_note.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.lbl_note.setStyleSheet("color: #555555;") # Inactive gray
        card_layout.addWidget(self.lbl_note)
        
        self.lbl_cents = QLabel("No Signal")
        self.lbl_cents.setFont(QFont("Segoe UI", 10, QFont.Weight.Bold))
        self.lbl_cents.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.lbl_cents.setStyleSheet("color: #888888;")
        card_layout.addWidget(self.lbl_cents)
        
        self.lbl_frequency = QLabel("- Hz")
        self.lbl_frequency.setFont(QFont("Consolas", 9))
        self.lbl_frequency.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.lbl_frequency.setStyleSheet("color: #666666;")
        card_layout.addWidget(self.lbl_frequency)
        
        body_layout.addWidget(self.note_card)
        
        # Right Panel: Dial Widget
        self.dial = TunerDial()
        body_layout.addWidget(self.dial)
        
        layout.addLayout(body_layout)
        
        # General CSS stylesheet
        self.setStyleSheet("""
            QLabel {
                font-family: "Segoe UI", sans-serif;
            }
        """)

    def on_a4_changed(self, val):
        self.ref_a4 = float(val)

    def process_tuner(self):
        """Polls circular buffer, performs autocorrelation, and updates UI."""
        if not self.audio_engine.is_running:
            self.show_inactive()
            return
            
        # Read the latest 2048 samples from tuner circular buffer
        samples = self.audio_engine.tuner_buffer.read_latest(2048)
        if len(samples) < 2048:
            self.show_inactive()
            return
            
        # Calculate RMS signal energy
        rms = np.sqrt(np.mean(samples ** 2))
        if rms < 0.003: # Noise gate threshold
            self.show_inactive()
            return
            
        # Subtract mean to remove any DC offset
        samples = samples - np.mean(samples)
        
        # Compute autocorrelation
        corr = np.correlate(samples, samples, mode='full')
        corr = corr[len(corr)//2:]  # Keep second half
        
        sample_rate = self.audio_engine.sample_rate
        
        # Define search range for open guitar strings & basic chords (65 Hz to 600 Hz)
        min_lag = int(sample_rate / 600)  # ~73 samples
        max_lag = int(sample_rate / 65)   # ~678 samples
        max_lag = min(max_lag, len(corr) - 1)
        
        if min_lag >= max_lag:
            self.show_inactive()
            return
            
        search_area = corr[min_lag:max_lag]
        abs_max = np.max(search_area)
        
        if abs_max <= 0:
            self.show_inactive()
            return
            
        # Find first peak that is significant (above 80% of max peak value) to avoid octave mistakes
        threshold = 0.8 * abs_max
        best_lag = None
        for lag in range(min_lag, max_lag):
            if corr[lag] > corr[lag - 1] and corr[lag] > corr[lag + 1]:
                if corr[lag] > threshold:
                    best_lag = lag
                    break
                    
        if best_lag is None:
            best_lag = np.argmax(search_area) + min_lag
            
        # Apply Parabolic Interpolation for sub-sample lag accuracy
        if best_lag > 0 and best_lag < len(corr) - 1:
            y1 = corr[best_lag - 1]
            y2 = corr[best_lag]
            y3 = corr[best_lag + 1]
            denom = (y1 - 2 * y2 + y3)
            if abs(denom) > 1e-5:
                shift = 0.5 * (y1 - y3) / denom
                best_period = best_lag + shift
            else:
                best_period = best_lag
        else:
            best_period = best_lag
            
        # Convert period to frequency
        freq = sample_rate / best_period
        
        if 65.0 <= freq <= 600.0:
            # Convert frequency to note & cents
            d = 12 * math.log2(freq / self.ref_a4) + 69
            midi_note = round(d)
            cents_dev = (d - midi_note) * 100.0
            
            note_names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
            note_name = note_names[midi_note % 12]
            octave = (midi_note // 12) - 1
            
            note_str = f"{note_name}{octave}"
            
            # Update Digital Card display
            self.lbl_note.setText(note_str)
            self.lbl_frequency.setText(f"{freq:.1f} Hz")
            
            if abs(cents_dev) <= 3.0:
                self.lbl_note.setStyleSheet("color: #4caf50;") # Green
                self.lbl_cents.setText("In Tune")
                self.lbl_cents.setStyleSheet("color: #4caf50;")
            else:
                # Orange/Yellow if close, Red if far
                if abs(cents_dev) <= 15.0:
                    self.lbl_note.setStyleSheet("color: #ffeb3b;") # Yellow
                    self.lbl_cents.setStyleSheet("color: #ffeb3b;")
                else:
                    self.lbl_note.setStyleSheet("color: #ff5722;") # Orange-Red
                    self.lbl_cents.setStyleSheet("color: #ff5722;")
                    
                cents_prefix = "+" if cents_dev > 0 else ""
                self.lbl_cents.setText(f"{cents_prefix}{cents_dev:.0f} cents")
                
            # Update Dial Needle
            self.dial.set_cents(cents_dev, True)
        else:
            self.show_inactive()

    def show_inactive(self):
        self.lbl_note.setText("--")
        self.lbl_note.setStyleSheet("color: #555555;")
        self.lbl_cents.setText("No Signal")
        self.lbl_cents.setStyleSheet("color: #888888;")
        self.lbl_frequency.setText("- Hz")
        self.dial.set_cents(0.0, False)

    def closeEvent(self, event):
        self.timer.stop()
        super().closeEvent(event)
