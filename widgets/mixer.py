import math
from PySide6.QtWidgets import (
    QWidget, QHBoxLayout, QVBoxLayout, QLabel, QSlider,
    QPushButton, QScrollArea, QFrame, QSizePolicy, QSpacerItem
)
from PySide6.QtCore import Qt, QTimer, Signal
from PySide6.QtGui import QPainter, QColor, QPen, QBrush, QLinearGradient, QFont, QRadialGradient

from widgets.knob import CustomKnob
from widgets.level_meter import LevelMeter


class ChannelStrip(QWidget):
    """A single vertical mixer channel strip with fader, pan, mute, solo, and level meter."""

    def __init__(self, track, audio_engine, parent=None):
        super().__init__(parent)
        self.track = track
        self.audio_engine = audio_engine
        self._updating = False  # Guard against feedback loops

        self.setObjectName("ChannelStrip")
        self.setFixedWidth(80)
        self.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Expanding)

        self._build_ui()
        self._apply_style()

    def _build_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(6, 8, 6, 8)
        layout.setSpacing(4)
        layout.setAlignment(Qt.AlignmentFlag.AlignHCenter)

        # ── Track name ──────────────────────────────────────────────
        self.lbl_name = QLabel(self.track.name)
        self.lbl_name.setObjectName("ChannelName")
        self.lbl_name.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.lbl_name.setWordWrap(False)
        font = QFont("Consolas", 8, QFont.Weight.Bold)
        self.lbl_name.setFont(font)
        layout.addWidget(self.lbl_name)

        # ── Level meter ──────────────────────────────────────────────
        self.level_meter = LevelMeter()
        self.level_meter.setMinimumHeight(120)
        self.level_meter.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Expanding)
        layout.addWidget(self.level_meter)

        # ── Pan knob ─────────────────────────────────────────────────
        self.pan_knob = CustomKnob(
            label="PAN",
            min_val=-1.0,
            max_val=1.0,
            default_val=self.track.pan,
            decimals=2,
        )
        self.pan_knob.valueChanged.connect(self._on_pan_changed)
        layout.addWidget(self.pan_knob, alignment=Qt.AlignmentFlag.AlignHCenter)

        # ── Volume fader (vertical) ───────────────────────────────────
        self.fader = QSlider(Qt.Orientation.Vertical)
        self.fader.setObjectName("MixerFader")
        self.fader.setMinimum(-600)   # -60.0 dB
        self.fader.setMaximum(60)     # +6.0  dB
        self.fader.setValue(int(self.track.volume * 10))
        self.fader.setMinimumHeight(90)
        self.fader.setMaximumHeight(120)
        self.fader.valueChanged.connect(self._on_fader_changed)
        layout.addWidget(self.fader, alignment=Qt.AlignmentFlag.AlignHCenter)

        # ── dB label ─────────────────────────────────────────────────
        self.lbl_db = QLabel()
        self.lbl_db.setObjectName("ChannelDb")
        self.lbl_db.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.lbl_db.setFont(QFont("Consolas", 8))
        self._update_db_label(self.track.volume)
        layout.addWidget(self.lbl_db)

        # ── Mute / Solo ──────────────────────────────────────────────
        ms_layout = QHBoxLayout()
        ms_layout.setSpacing(2)
        ms_layout.setContentsMargins(0, 0, 0, 0)

        self.btn_mute = QPushButton("M")
        self.btn_mute.setObjectName("MixerMute")
        self.btn_mute.setCheckable(True)
        self.btn_mute.setChecked(self.track.mute)
        self.btn_mute.setFixedSize(28, 20)
        self.btn_mute.clicked.connect(self._on_mute)
        ms_layout.addWidget(self.btn_mute)

        self.btn_solo = QPushButton("S")
        self.btn_solo.setObjectName("MixerSolo")
        self.btn_solo.setCheckable(True)
        self.btn_solo.setChecked(self.track.solo)
        self.btn_solo.setFixedSize(28, 20)
        self.btn_solo.clicked.connect(self._on_solo)
        ms_layout.addWidget(self.btn_solo)

        layout.addLayout(ms_layout)

    def _on_fader_changed(self, value):
        if self._updating:
            return
        db = value / 10.0
        self.track.volume = db
        self._update_db_label(db)

    def _on_pan_changed(self, value):
        self.track.pan = value

    def _on_mute(self):
        self.track.mute = self.btn_mute.isChecked()

    def _on_solo(self):
        self.track.solo = self.btn_solo.isChecked()

    def _update_db_label(self, db):
        if db <= -60.0:
            self.lbl_db.setText("-∞")
        else:
            self.lbl_db.setText(f"{db:+.1f}")

    def refresh(self):
        """Sync UI state from track model (call when track changes externally)."""
        self._updating = True
        self.lbl_name.setText(self.track.name)
        self.fader.setValue(int(self.track.volume * 10))
        self._update_db_label(self.track.volume)
        self.btn_mute.setChecked(self.track.mute)
        self.btn_solo.setChecked(self.track.solo)
        self._updating = False

    def update_meters(self):
        self.level_meter.set_level(self.track.level_history)

    def _apply_style(self):
        self.setStyleSheet("""
            QWidget#ChannelStrip {
                background-color: #0d0d0f;
                border: 1px solid #1e1e21;
                border-radius: 4px;
            }

            QLabel#ChannelName {
                color: #cccccc;
                font-family: "Consolas", monospace;
                font-size: 9px;
                font-weight: bold;
                letter-spacing: 0.5px;
            }

            QLabel#ChannelDb {
                color: #666669;
                font-family: "Consolas", monospace;
                font-size: 8px;
            }

            QSlider#MixerFader::groove:vertical {
                background: #1a1a1c;
                width: 5px;
                border-radius: 2px;
            }
            QSlider#MixerFader::add-page:vertical {
                background: #ffffff;
                width: 5px;
                border-radius: 2px;
            }
            QSlider#MixerFader::sub-page:vertical {
                background: #1a1a1c;
                width: 5px;
                border-radius: 2px;
            }
            QSlider#MixerFader::handle:vertical {
                background: #e0e0e3;
                border: 1px solid #555558;
                height: 16px;
                width: 28px;
                margin-left: -12px;
                margin-right: -12px;
                border-radius: 3px;
            }
            QSlider#MixerFader::handle:vertical:hover {
                background: #ffffff;
                border-color: #ff0033;
            }

            QPushButton#MixerMute {
                background-color: #111113;
                border: 1px solid #2a2a2d;
                color: #666669;
                font-family: "Consolas", monospace;
                font-size: 8px;
                font-weight: bold;
                border-radius: 3px;
            }
            QPushButton#MixerMute:checked {
                background-color: #ff9900;
                border-color: #ff9900;
                color: #000000;
            }
            QPushButton#MixerMute:hover { border-color: #555558; color: #cccccc; }

            QPushButton#MixerSolo {
                background-color: #111113;
                border: 1px solid #2a2a2d;
                color: #666669;
                font-family: "Consolas", monospace;
                font-size: 8px;
                font-weight: bold;
                border-radius: 3px;
            }
            QPushButton#MixerSolo:checked {
                background-color: #ffffff;
                border-color: #ffffff;
                color: #000000;
            }
            QPushButton#MixerSolo:hover { border-color: #555558; color: #cccccc; }
        """)


class MasterChannelStrip(QWidget):
    """Master output channel strip."""

    def __init__(self, audio_engine, parent=None):
        super().__init__(parent)
        self.audio_engine = audio_engine
        self.setObjectName("MasterStrip")
        self.setFixedWidth(90)
        self.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Expanding)
        self._build_ui()
        self._apply_style()

    def _build_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(6, 8, 6, 8)
        layout.setSpacing(4)
        layout.setAlignment(Qt.AlignmentFlag.AlignHCenter)

        lbl = QLabel("MASTER")
        lbl.setObjectName("MasterLabel")
        lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
        lbl.setFont(QFont("Consolas", 8, QFont.Weight.Bold))
        layout.addWidget(lbl)

        # Two level meters side by side (L/R)
        meters_layout = QHBoxLayout()
        meters_layout.setSpacing(2)
        self.meter_l = LevelMeter()
        self.meter_r = LevelMeter()
        self.meter_l.setMinimumHeight(120)
        self.meter_r.setMinimumHeight(120)
        meters_layout.addWidget(self.meter_l)
        meters_layout.addWidget(self.meter_r)
        layout.addLayout(meters_layout)

        # Master fader
        self.fader = QSlider(Qt.Orientation.Vertical)
        self.fader.setObjectName("MixerFader")
        self.fader.setMinimum(-600)
        self.fader.setMaximum(60)
        self.fader.setValue(int(self.audio_engine.main_volume * 10))
        self.fader.setMinimumHeight(90)
        self.fader.setMaximumHeight(120)
        self.fader.valueChanged.connect(self._on_fader)
        layout.addWidget(self.fader, alignment=Qt.AlignmentFlag.AlignHCenter)

        self.lbl_db = QLabel()
        self.lbl_db.setObjectName("ChannelDb")
        self.lbl_db.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.lbl_db.setFont(QFont("Consolas", 8))
        self._update_db(self.audio_engine.main_volume)
        layout.addWidget(self.lbl_db)

    def _on_fader(self, value):
        db = value / 10.0
        self.audio_engine.main_volume = db
        self._update_db(db)

    def _update_db(self, db):
        if db <= -60.0:
            self.lbl_db.setText("-∞")
        else:
            self.lbl_db.setText(f"{db:+.1f}")

    def update_meters(self, master_level):
        """master_level is a float dB value."""
        self.meter_l.set_level(master_level)
        self.meter_r.set_level(master_level)

    def _apply_style(self):
        self.setStyleSheet("""
            QWidget#MasterStrip {
                background-color: #0d0d0f;
                border: 1px solid #333336;
                border-radius: 4px;
            }
            QLabel#MasterLabel {
                color: #ffffff;
                font-family: "Consolas", monospace;
                font-size: 9px;
                font-weight: bold;
                letter-spacing: 1px;
            }
            QLabel#ChannelDb {
                color: #888888;
                font-family: "Consolas", monospace;
                font-size: 8px;
            }
            QSlider#MixerFader::groove:vertical {
                background: #1a1a1c;
                width: 5px;
                border-radius: 2px;
            }
            QSlider#MixerFader::add-page:vertical {
                background: #ffffff;
                width: 5px;
                border-radius: 2px;
            }
            QSlider#MixerFader::sub-page:vertical {
                background: #1a1a1c;
                width: 5px;
                border-radius: 2px;
            }
            QSlider#MixerFader::handle:vertical {
                background: #ffffff;
                border: 1px solid #888888;
                height: 18px;
                width: 34px;
                margin-left: -15px;
                margin-right: -15px;
                border-radius: 3px;
            }
            QSlider#MixerFader::handle:vertical:hover {
                border-color: #ff0033;
            }
        """)


class MixerWidget(QWidget):
    """Full mixer panel — auto-builds channel strips for every track."""

    def __init__(self, audio_engine, parent=None):
        super().__init__(parent)
        self.audio_engine = audio_engine
        self.strips: list[ChannelStrip] = []

        self.setObjectName("MixerWidget")
        self._build_ui()
        self._apply_style()

        # Poll meters at ~30 fps
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._tick)
        self._timer.start(33)

    def _build_ui(self):
        outer = QHBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.setSpacing(0)

        # Scrollable area for track strips
        self._scroll = QScrollArea()
        self._scroll.setWidgetResizable(True)
        self._scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        self._scroll.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self._scroll.setObjectName("MixerScroll")
        self._scroll.setFrameShape(QFrame.Shape.NoFrame)

        self._strips_container = QWidget()
        self._strips_layout = QHBoxLayout(self._strips_container)
        self._strips_layout.setContentsMargins(8, 8, 8, 8)
        self._strips_layout.setSpacing(4)
        self._strips_layout.setAlignment(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter)

        self._scroll.setWidget(self._strips_container)
        outer.addWidget(self._scroll, 1)

        # Separator
        sep = QFrame()
        sep.setFrameShape(QFrame.Shape.VLine)
        sep.setObjectName("MixerSep")
        sep.setFixedWidth(1)
        outer.addWidget(sep)

        # Master strip (always at right)
        self.master_strip = MasterChannelStrip(self.audio_engine)
        outer.addWidget(self.master_strip)

    def rebuild(self):
        """Rebuild all channel strips from audio_engine.tracks."""
        # Remove old strips
        for strip in self.strips:
            self._strips_layout.removeWidget(strip)
            strip.deleteLater()
        self.strips.clear()

        for track in self.audio_engine.tracks:
            strip = ChannelStrip(track, self.audio_engine)
            self._strips_layout.addWidget(strip)
            self.strips.append(strip)

    def _tick(self):
        """Refresh meters and sync any external name changes."""
        for strip in self.strips:
            strip.update_meters()
            strip.lbl_name.setText(strip.track.name)
        # Master meter — use mean of all active track levels
        if self.audio_engine.tracks:
            levels = [t.level_history for t in self.audio_engine.tracks if not t.mute]
            if levels:
                avg = sum(levels) / len(levels)
                self.master_strip.update_meters(avg)

    def _apply_style(self):
        self.setStyleSheet("""
            QWidget#MixerWidget {
                background-color: #080809;
            }
            QScrollArea#MixerScroll {
                background-color: #080809;
                border: none;
            }
            QWidget#MixerScroll > QWidget {
                background-color: #080809;
            }
            QFrame#MixerSep {
                color: #222225;
                background-color: #222225;
            }
        """)
