import math
import time
import numpy as np
from PySide6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QSlider, QComboBox,
    QPushButton, QSpinBox, QFrame, QSizePolicy
)
from PySide6.QtCore import Qt, QTimer, QPoint, QRectF, QPointF
from PySide6.QtGui import QPainter, QColor, QPen, QBrush, QPolygonF, QFont, QRadialGradient

class MetronomePendulum(QWidget):
    """Custom painted mechanical metronome pendulum widget."""
    def __init__(self, audio_engine, parent=None):
        super().__init__(parent)
        self.audio_engine = audio_engine
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.setMinimumSize(140, 180)
        
        # 60 FPS repaint timer for smooth pendulum swing
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update)
        self.timer.start(16)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        w = self.width()
        h = self.height()
        
        # Fill background
        painter.fillRect(0, 0, w, h, QColor("#1e1e1e"))
        
        # Core geometry
        cx = w / 2.0
        # Pivot point of the pendulum is near the bottom of the body
        pivot_y = h - 35.0
        
        # 1. Draw Metronome body (Wooden/Metal Pyramid)
        body = QPolygonF([
            QPointF(cx - 15.0, 30.0),
            QPointF(cx + 15.0, 30.0),
            QPointF(cx + 55.0, h - 20.0),
            QPointF(cx - 55.0, h - 20.0)
        ])
        
        painter.setBrush(QBrush(QColor("#252526")))
        painter.setPen(QPen(QColor("#3c3f41"), 1.5))
        painter.drawPolygon(body)
        
        # Draw inner face plate (lighter gray)
        face_plate = QPolygonF([
            QPointF(cx - 10.0, 45.0),
            QPointF(cx + 10.0, 45.0),
            QPointF(cx + 38.0, h - 30.0),
            QPointF(cx - 38.0, h - 30.0)
        ])
        painter.setBrush(QBrush(QColor("#2d2d30")))
        painter.setPen(QPen(QColor("#202020"), 1))
        painter.drawPolygon(face_plate)
        
        # Draw tempo markings / guidelines on the face plate
        painter.setPen(QPen(QColor("#404040"), 1))
        for y_pos in range(int(45.0 + 15.0), int(h - 40.0), 20):
            # Guidelines lines
            width_at_y = 15.0 + (y_pos - 45.0) * 0.18
            painter.drawLine(cx - width_at_y, y_pos, cx + width_at_y, y_pos)
            
        # 2. Calculate Pendulum Swing Phase
        # Find exact playhead sample alignment from audio engine
        sr = self.audio_engine.sample_rate if self.audio_engine else 44100
        bpm = self.audio_engine.bpm
        playhead = self.audio_engine.playhead_samples
        play_active = self.audio_engine.play_state in ("playing", "recording")
        
        samples_per_beat = (60.0 / bpm) * sr
        
        # Pendulum cycle is 2 beats (left, then right)
        beat_phase = (playhead / samples_per_beat) % 2.0
        
        # Calculate angle of pendulum swing (in degrees)
        if play_active:
            # Swing angle uses sine wave mapped between -26 and +26 degrees
            angle_deg = 26.0 * math.sin(beat_phase * math.pi)
        else:
            angle_deg = 0.0 # Rest straight up
            
        angle_rad = math.radians(angle_deg)
        
        # 3. Draw the Steel Pendulum Rod
        rod_len = h - 80.0
        # The rod extends upwards from the pivot
        rx = cx + rod_len * math.sin(angle_rad)
        ry = pivot_y - rod_len * math.cos(angle_rad)
        
        painter.setPen(QPen(QColor("#a0a0a0"), 2))
        painter.drawLine(cx, pivot_y, rx, ry)
        
        # Draw the sliding weight block on the rod
        # Height of the weight changes based on BPM (faster BPM = lower weight, slower = higher weight)
        # Standard metronome physics: BPM range 40 - 240
        bpm_percent = (bpm - 40.0) / (240.0 - 40.0) # 0.0 to 1.0
        weight_dist = 40.0 + (1.0 - bpm_percent) * (rod_len - 60.0)
        
        wx = cx + weight_dist * math.sin(angle_rad)
        wy = pivot_y - weight_dist * math.cos(angle_rad)
        
        # Draw rectangular weight
        weight_rect = QRectF(wx - 10.0, wy - 8.0, 20.0, 16.0)
        painter.setBrush(QBrush(QColor("#a63e3e"))) # Accent red weight
        painter.setPen(QPen(QColor("#ffffff"), 1))
        painter.drawRoundedRect(weight_rect, 2, 2)
        
        # Draw pivot cap at base
        painter.setBrush(QBrush(QColor("#505050")))
        painter.setPen(QPen(QColor("#202020"), 1))
        painter.drawEllipse(QPointF(cx, pivot_y), 5, 5)
        
        # 4. Draw Beat Flashing LED
        # Determine if currently flashing (first 10% of a beat)
        beat_number = int((playhead / samples_per_beat) % self.audio_engine.time_sig_numerator)
        flash_duration = min(4410, int(samples_per_beat * 0.15))
        is_flashing = play_active and (playhead % samples_per_beat < flash_duration)
        
        led_y = 15.0
        led_grad = QRadialGradient(cx, led_y, 10)
        
        if is_flashing:
            if beat_number == 0:
                # Accented beat 1 (vibrant green)
                led_grad.setColorAt(0, QColor("#4caf50"))
                led_grad.setColorAt(1, QColor(76, 175, 80, 0))
                led_color = QColor("#81c784")
            else:
                # Unaccented beats (glowing amber)
                led_grad.setColorAt(0, QColor("#ffc107"))
                led_grad.setColorAt(1, QColor(255, 193, 7, 0))
                led_color = QColor("#ffe082")
        else:
            # Idle LED (dim gray)
            led_grad.setColorAt(0, QColor("#333333"))
            led_grad.setColorAt(1, QColor(51, 51, 51, 0))
            led_color = QColor("#444444")
            
        painter.setBrush(QBrush(led_grad))
        painter.setPen(Qt.PenStyle.NoPen)
        painter.drawEllipse(QPointF(cx, led_y), 12, 12)
        
        painter.setBrush(QBrush(led_color))
        painter.drawEllipse(QPointF(cx, led_y), 4, 4)


class TapTempo:
    """Tracks time between taps to calculate average BPM."""
    def __init__(self):
        self.taps = []
        
    def tap(self):
        now = time.time()
        # If the last tap was more than 2.0 seconds ago, reset
        if self.taps and (now - self.taps[-1] > 2.0):
            self.taps = []
            
        self.taps.append(now)
        self.taps = self.taps[-5:] # Keep last 5 taps
        
        if len(self.taps) < 2:
            return None
            
        intervals = [self.taps[i] - self.taps[i-1] for i in range(1, len(self.taps))]
        avg_interval = sum(intervals) / len(intervals)
        
        bpm = 60.0 / avg_interval
        return min(240.0, max(40.0, bpm))


class GuitarMetronomeWidget(QWidget):
    """Metronome configuration interface with visual pendulum swing."""
    def __init__(self, audio_engine, parent=None):
        super().__init__(parent)
        self.audio_engine = audio_engine
        self.tap_tempo = TapTempo()
        
        self.setup_ui()
        
        # UI sync timer to match slider states when modified elsewhere
        self.sync_timer = QTimer(self)
        self.sync_timer.timeout.connect(self.sync_values_to_ui)
        self.sync_timer.start(100) # sync 10 times a sec

    def setup_ui(self):
        layout = QHBoxLayout(self)
        layout.setContentsMargins(15, 10, 15, 10)
        layout.setSpacing(25)
        
        # --- Left Panel: Pendulum Visualizer ---
        self.pendulum = MetronomePendulum(self.audio_engine)
        layout.addWidget(self.pendulum)
        
        # --- Right Panel: Controls layout ---
        controls_layout = QVBoxLayout()
        controls_layout.setSpacing(12)
        controls_layout.setAlignment(Qt.AlignmentFlag.AlignVCenter)
        
        # Metronome Enable and Title Row
        title_row = QHBoxLayout()
        self.lbl_title = QLabel("METRONOME")
        self.lbl_title.setFont(QFont("Segoe UI", 11, QFont.Weight.Bold))
        self.lbl_title.setStyleSheet("color: #ffffff; letter-spacing: 1.0px;")
        title_row.addWidget(self.lbl_title)
        
        title_row.addStretch()
        
        self.btn_toggle = QPushButton("ON / OFF")
        self.btn_toggle.setCheckable(True)
        self.btn_toggle.setChecked(self.audio_engine.metronome_enabled)
        self.btn_toggle.setObjectName("MetronomeToggleBtn")
        self.btn_toggle.clicked.connect(self.on_metronome_toggled)
        self.update_toggle_btn_style()
        title_row.addWidget(self.btn_toggle)
        controls_layout.addLayout(title_row)
        
        # Tempo (BPM) Adjustment controls
        bpm_row = QHBoxLayout()
        bpm_row.setSpacing(8)
        
        self.lbl_bpm = QLabel("BPM:")
        self.lbl_bpm.setFont(QFont("Segoe UI", 9, QFont.Weight.Bold))
        self.lbl_bpm.setStyleSheet("color: #b3b3b3;")
        bpm_row.addWidget(self.lbl_bpm)
        
        self.spin_bpm = QSpinBox()
        self.spin_bpm.setObjectName("BpmSpinbox")
        self.spin_bpm.setRange(40, 240)
        self.spin_bpm.setValue(int(self.audio_engine.bpm))
        self.spin_bpm.valueChanged.connect(self.on_bpm_spin_changed)
        self.spin_bpm.setMinimumWidth(70)
        bpm_row.addWidget(self.spin_bpm)
        
        self.slider_bpm = QSlider(Qt.Orientation.Horizontal)
        self.slider_bpm.setObjectName("BpmSlider")
        self.slider_bpm.setRange(40, 240)
        self.slider_bpm.setValue(int(self.audio_engine.bpm))
        self.slider_bpm.valueChanged.connect(self.on_bpm_slider_changed)
        bpm_row.addWidget(self.slider_bpm)
        
        self.btn_tap = QPushButton("TAP TEMPO")
        self.btn_tap.setObjectName("TapTempoBtn")
        self.btn_tap.clicked.connect(self.on_tap_clicked)
        bpm_row.addWidget(self.btn_tap)
        
        controls_layout.addLayout(bpm_row)
        
        # Time Signature Row
        time_sig_row = QHBoxLayout()
        time_sig_row.setSpacing(10)
        
        self.lbl_time_sig = QLabel("Time Signature:")
        self.lbl_time_sig.setFont(QFont("Segoe UI", 9, QFont.Weight.Bold))
        self.lbl_time_sig.setStyleSheet("color: #b3b3b3;")
        time_sig_row.addWidget(self.lbl_time_sig)
        
        self.combo_time_sig = QComboBox()
        self.combo_time_sig.setObjectName("TimeSigCombo")
        self.combo_time_sig.addItem("4 / 4", (4, 4))
        self.combo_time_sig.addItem("3 / 4", (3, 4))
        self.combo_time_sig.addItem("2 / 4", (2, 4))
        self.combo_time_sig.addItem("6 / 8", (6, 8))
        self.combo_time_sig.currentIndexChanged.connect(self.on_time_sig_changed)
        time_sig_row.addWidget(self.combo_time_sig)
        
        time_sig_row.addStretch()
        controls_layout.addLayout(time_sig_row)
        
        # Volume control row
        vol_row = QHBoxLayout()
        vol_row.setSpacing(10)
        
        self.lbl_vol = QLabel("Volume:")
        self.lbl_vol.setFont(QFont("Segoe UI", 9, QFont.Weight.Bold))
        self.lbl_vol.setStyleSheet("color: #b3b3b3;")
        vol_row.addWidget(self.lbl_vol)
        
        self.slider_vol = QSlider(Qt.Orientation.Horizontal)
        self.slider_vol.setObjectName("MetronomeVolSlider")
        self.slider_vol.setRange(-400, 0) # -40 dB to 0 dB
        self.slider_vol.setValue(int(self.audio_engine.metronome_volume_db * 10))
        self.slider_vol.valueChanged.connect(self.on_volume_changed)
        self.slider_vol.setMinimumWidth(150)
        vol_row.addWidget(self.slider_vol)
        
        self.lbl_vol_db = QLabel(f"{self.audio_engine.metronome_volume_db:+.1f} dB")
        self.lbl_vol_db.setFont(QFont("Consolas", 8))
        self.lbl_vol_db.setStyleSheet("color: #888888; min-width: 50px;")
        vol_row.addWidget(self.lbl_vol_db)
        
        vol_row.addStretch()
        controls_layout.addLayout(vol_row)
        
        layout.addLayout(controls_layout)
        layout.setStretch(1, 2)
        
        # Global stylesheet styling
        self.setStyleSheet("""
            QPushButton#MetronomeToggleBtn {
                background-color: #3e4249;
                border: 1px solid #555555;
                border-radius: 4px;
                color: #e0e0e0;
                font-weight: bold;
                padding: 5px 15px;
                font-size: 11px;
            }
            QPushButton#MetronomeToggleBtn:checked {
                background-color: #2b5a30;
                border-color: #43a047;
                color: #ffffff;
            }
            QPushButton#TapTempoBtn {
                background-color: #2d2d2d;
                border: 1px solid #3e3e42;
                border-radius: 3px;
                color: #ffffff;
                font-weight: bold;
                font-size: 10px;
                padding: 5px 10px;
            }
            QPushButton#TapTempoBtn:hover {
                background-color: #444444;
            }
            QSpinBox#BpmSpinbox {
                background-color: #252526;
                border: 1px solid #3e3e42;
                border-radius: 3px;
                color: #ffffff;
                padding: 4px 6px;
                font-family: "Segoe UI", sans-serif;
                font-size: 11px;
                font-weight: bold;
            }
            QComboBox#TimeSigCombo {
                background-color: #252526;
                border: 1px solid #3e3e42;
                border-radius: 3px;
                color: #ffffff;
                padding: 4px 10px;
                font-size: 11px;
                font-weight: bold;
                min-width: 90px;
            }
            QSlider#BpmSlider::groove:horizontal, QSlider#MetronomeVolSlider::groove:horizontal {
                background: #18181c;
                height: 6px;
                border-radius: 3px;
            }
            QSlider#BpmSlider::sub-page:horizontal, QSlider#MetronomeVolSlider::sub-page:horizontal {
                background: #888888;
                height: 6px;
                border-radius: 3px;
            }
            QSlider#BpmSlider::handle:horizontal, QSlider#MetronomeVolSlider::handle:horizontal {
                background: #505050;
                border: 1px solid #666;
                width: 12px;
                margin-top: -3px;
                margin-bottom: -3px;
                border-radius: 6px;
            }
            QLabel {
                font-family: "Segoe UI", sans-serif;
            }
        """)

    def on_metronome_toggled(self):
        enabled = self.btn_toggle.isChecked()
        self.audio_engine.metronome_enabled = enabled
        self.update_toggle_btn_style()

    def update_toggle_btn_style(self):
        if self.btn_toggle.isChecked():
            self.btn_toggle.setText("METRONOME ON")
        else:
            self.btn_toggle.setText("METRONOME OFF")

    def on_bpm_spin_changed(self, val):
        self.audio_engine.bpm = float(val)
        if self.slider_bpm.value() != val:
            self.slider_bpm.setValue(val)

    def on_bpm_slider_changed(self, val):
        self.audio_engine.bpm = float(val)
        if self.spin_bpm.value() != val:
            self.spin_bpm.setValue(val)

    def on_tap_clicked(self):
        tapped_bpm = self.tap_tempo.tap()
        if tapped_bpm:
            bpm_rounded = int(round(tapped_bpm))
            self.spin_bpm.setValue(bpm_rounded)
            self.audio_engine.bpm = float(bpm_rounded)

    def on_time_sig_changed(self):
        sig = self.combo_time_sig.currentData()
        if sig:
            num, den = sig
            self.audio_engine.time_sig_numerator = num
            self.audio_engine.time_sig_denominator = den

    def on_volume_changed(self, val):
        val_db = val / 10.0
        self.audio_engine.metronome_volume_db = val_db
        self.lbl_vol_db.setText(f"{val_db:+.1f} dB")

    def sync_values_to_ui(self):
        """Timer ticks to sync UI with state changes from tap tempo or settings."""
        engine_bpm = int(self.audio_engine.bpm)
        if self.spin_bpm.value() != engine_bpm:
            self.spin_bpm.setValue(engine_bpm)
        if self.slider_bpm.value() != engine_bpm:
            self.slider_bpm.setValue(engine_bpm)
            
        enabled = self.audio_engine.metronome_enabled
        if self.btn_toggle.isChecked() != enabled:
            self.btn_toggle.setChecked(enabled)
            self.update_toggle_btn_style()
