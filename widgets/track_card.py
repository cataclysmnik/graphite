from PySide6.QtWidgets import (
    QFrame, QHBoxLayout, QVBoxLayout, QLabel, QSlider,
    QComboBox, QPushButton, QLineEdit, QSizePolicy
)
from PySide6.QtCore import Qt, Signal, QTimer
from PySide6.QtGui import QFont, QColor
from widgets.knob import CustomKnob
from widgets.level_meter import LevelMeter

class TrackCard(QFrame):
    """Card-style Track widget displaying track settings and VU meter."""
    trackSelected = Signal(object)  # Emitted when the card is clicked/selected
    trackRemoved = Signal(object)   # Emitted when delete button is pressed
    
    def __init__(self, track, audio_engine, parent=None):
        super().__init__(parent)
        self.track = track
        self.audio_engine = audio_engine
        self.is_selected = False
        
        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Preferred)
        self.setMinimumHeight(150)
        self.setObjectName("TrackCard")
        
        self.setup_ui()
        self.update_selection_style()
        
        # Setup level meter polling timer
        self.poll_timer = QTimer(self)
        self.poll_timer.timeout.connect(self.update_levels)
        self.poll_timer.start(33)  # Polling level at ~30 FPS
        
    def setup_ui(self):
        # Master horizontal layout for card contents
        main_layout = QHBoxLayout(self)
        main_layout.setContentsMargins(10, 8, 10, 8)
        main_layout.setSpacing(10)
        
        # --- PANEL 1: Track Details & State Buttons ---
        details_layout = QVBoxLayout()
        details_layout.setSpacing(6)
        
        # Editable Track Name
        self.name_edit = QLineEdit(self.track.name)
        self.name_edit.setObjectName("TrackNameEdit")
        self.name_edit.setToolTip("Double-click to rename track")
        self.name_edit.editingFinished.connect(self.on_rename)
        details_layout.addWidget(self.name_edit)
        
        # State Buttons Row: Mute [M], Solo [S], Arm [R]
        btn_layout = QHBoxLayout()
        btn_layout.setSpacing(4)
        
        self.btn_mute = QPushButton("M")
        self.btn_mute.setCheckable(True)
        self.btn_mute.setChecked(self.track.mute)
        self.btn_mute.setObjectName("MuteButton")
        self.btn_mute.setToolTip("Mute Track")
        self.btn_mute.clicked.connect(self.on_mute_clicked)
        
        self.btn_solo = QPushButton("S")
        self.btn_solo.setCheckable(True)
        self.btn_solo.setChecked(self.track.solo)
        self.btn_solo.setObjectName("SoloButton")
        self.btn_solo.setToolTip("Solo Track")
        self.btn_solo.clicked.connect(self.on_solo_clicked)
        
        self.btn_arm = QPushButton("R")
        self.btn_arm.setCheckable(True)
        self.btn_arm.setChecked(self.track.armed)
        self.btn_arm.setObjectName("ArmButton")
        self.btn_arm.setToolTip("Arm Track for Input Monitoring")
        self.btn_arm.clicked.connect(self.on_arm_clicked)
        
        btn_layout.addWidget(self.btn_mute)
        btn_layout.addWidget(self.btn_solo)
        btn_layout.addWidget(self.btn_arm)
        details_layout.addLayout(btn_layout)
        
        # Input Channel Selector
        self.combo_input = QComboBox()
        self.combo_input.setObjectName("InputChannelCombo")
        self.combo_input.setToolTip("Select Track Input Channel")
        self.populate_inputs()
        self.combo_input.currentIndexChanged.connect(self.on_input_changed)
        details_layout.addWidget(self.combo_input)
        
        # Delete track button
        self.btn_delete = QPushButton("Delete Track")
        self.btn_delete.setObjectName("DeleteTrackButton")
        self.btn_delete.clicked.connect(self.on_delete_clicked)
        details_layout.addWidget(self.btn_delete)
        
        main_layout.addLayout(details_layout)
        
        # --- PANEL 2: Panning Knob & FX selector ---
        panning_layout = QVBoxLayout()
        panning_layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        # Custom Pan Knob
        self.pan_knob = CustomKnob(
            label="PAN",
            min_val=-1.0,
            max_val=1.0,
            default_val=self.track.pan,
            decimals=2
        )
        self.pan_knob.valueChanged.connect(self.on_pan_changed)
        panning_layout.addWidget(self.pan_knob)
        
        # FX Select Button
        self.btn_fx = QPushButton("VST / FX")
        self.btn_fx.setObjectName("FxButton")
        self.btn_fx.setToolTip("View/edit effects rack for this track")
        self.btn_fx.clicked.connect(self.on_fx_clicked)
        panning_layout.addWidget(self.btn_fx)
        
        main_layout.addLayout(panning_layout)
        
        # --- PANEL 3: Volume Fader ---
        vol_layout = QVBoxLayout()
        vol_layout.setSpacing(4)
        vol_layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        self.vol_slider = QSlider(Qt.Orientation.Vertical)
        self.vol_slider.setMinimum(-600)  # -60.0 dB
        self.vol_slider.setMaximum(60)    # +6.0 dB
        self.vol_slider.setValue(int(self.track.volume * 10))
        self.vol_slider.setObjectName("VolumeSlider")
        self.vol_slider.valueChanged.connect(self.on_volume_changed)
        vol_layout.addWidget(self.vol_slider)
        
        self.vol_label = QLabel("0.0 dB")
        self.vol_label.setObjectName("VolDbLabel")
        self.vol_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.vol_label.setFont(QFont("Consolas", 8))
        vol_layout.addWidget(self.vol_label)
        self.update_volume_label(self.track.volume)
        
        main_layout.addLayout(vol_layout)
        
        # --- PANEL 4: Custom vertical LED VU Meter ---
        self.level_meter = LevelMeter()
        main_layout.addWidget(self.level_meter)
        
        # Apply CSS stylesheets directly (QSS)
        self.setStyleSheet("""
            TrackCard {
                background-color: #2a2a2a;
                border: 1px solid #333333;
                border-left: 4px solid transparent;
                border-radius: 4px;
            }
            QLineEdit#TrackNameEdit {
                background: transparent;
                color: #ffffff;
                border: none;
                border-bottom: 1px solid transparent;
                font-family: "Segoe UI", sans-serif;
                font-size: 13px;
                font-weight: bold;
                padding-bottom: 2px;
            }
            QLineEdit#TrackNameEdit:focus {
                border-bottom: 1px solid #888888;
                background-color: rgba(255, 255, 255, 0.03);
            }
            QPushButton#MuteButton, QPushButton#SoloButton, QPushButton#ArmButton {
                font-weight: bold;
                border-radius: 3px;
                min-width: 26px;
                min-height: 26px;
                max-width: 26px;
                color: #d4d4d4;
                background-color: #252526;
                border: 1px solid #3e3e42;
            }
            QPushButton#MuteButton:checked {
                background-color: #6b5317;
                color: #ffffff;
                border-color: #6b5317;
            }
            QPushButton#SoloButton:checked {
                background-color: #2b5a30;
                color: #ffffff;
                border-color: #2b5a30;
            }
            QPushButton#ArmButton:checked {
                background-color: #802b2b;
                color: #ffffff;
                border-color: #802b2b;
            }
            QComboBox#InputChannelCombo {
                background-color: #252526;
                border: 1px solid #3e3e42;
                border-radius: 3px;
                color: #d4d4d4;
                padding: 4px;
                font-size: 11px;
            }
            QComboBox#InputChannelCombo QAbstractItemView {
                background-color: #252526;
                color: #d4d4d4;
                selection-background-color: #4a4a4a;
                selection-color: #ffffff;
            }
            QPushButton#DeleteTrackButton {
                background-color: transparent;
                border: 1px solid rgba(166, 62, 62, 0.3);
                border-radius: 3px;
                color: #a63e3e;
                font-size: 10px;
                padding: 3px;
            }
            QPushButton#DeleteTrackButton:hover {
                background-color: rgba(166, 62, 62, 0.1);
                border-color: #a63e3e;
            }
            QPushButton#FxButton {
                background-color: #2d2d2d;
                border: 1px solid #3e3e42;
                border-radius: 3px;
                color: #e0e0e0;
                font-weight: bold;
                font-size: 11px;
                padding: 5px;
            }
            QPushButton#FxButton:hover {
                background-color: #444444;
                color: #ffffff;
                border-color: #555555;
            }
            QSlider#VolumeSlider::groove:vertical {
                background: #18181c;
                width: 6px;
                border-radius: 3px;
            }
            QSlider#VolumeSlider::sub-page:vertical {
                background: #888888;
                width: 6px;
                border-radius: 3px;
            }
            QSlider#VolumeSlider::handle:vertical {
                background: #505050;
                border: 1px solid #666;
                height: 12px;
                margin-left: -4px;
                margin-right: -4px;
                border-radius: 6px;
            }
            QSlider#VolumeSlider::handle:vertical:hover {
                background: #888888;
                border-color: #888888;
            }
            QLabel#VolDbLabel {
                color: #9d9d9d;
                font-size: 10px;
            }
        """)

    def populate_inputs(self):
        """Populates the input selector dropdown filtering by global device settings."""
        self.combo_input.clear()
        
        if self.audio_engine.enable_inputs:
            first_in = self.audio_engine.input_first_channel
            last_in = self.audio_engine.input_last_channel
            for i in range(first_in, last_in + 1):
                self.combo_input.addItem(f"Input Channel {i+1}", i)
                
        # Add Demo Loop option
        self.combo_input.addItem("Guitar Demo Loop", "loop")
        
        # Set current selection
        idx = self.combo_input.findData(self.track.input_channel)
        if idx >= 0:
            self.combo_input.setCurrentIndex(idx)
        else:
            self.combo_input.setCurrentIndex(self.combo_input.count() - 1)  # Default to loop if not found

    def update_levels(self):
        """Updates the LED VU level meter with the current peak dB."""
        self.level_meter.set_level(self.track.level_history)

    def set_selected(self, selected):
        """Sets selected state and updates outline glow."""
        self.is_selected = selected
        self.update_selection_style()
        if selected:
            self.trackSelected.emit(self.track)
            
    def update_selection_style(self):
        """Updates the styling based on selection state."""
        if self.is_selected:
            # Active selected card style (graphite highlight border)
            self.setStyleSheet(self.styleSheet() + """
                TrackCard {
                    border-left: 4px solid #888888;
                    background-color: #333333;
                }
            """)
        else:
            # Inactive card style
            self.setStyleSheet(self.styleSheet() + """
                TrackCard {
                    border-left: 4px solid transparent;
                    background-color: #2a2a2a;
                }
            """)

    def mousePressEvent(self, event):
        """Select track when clicking anywhere on the card."""
        self.set_selected(True)
        super().mousePressEvent(event)
        
    def on_rename(self):
        new_name = self.name_edit.text().strip()
        if new_name:
            self.track.name = new_name
            self.update()
            
    def on_mute_clicked(self):
        self.track.mute = self.btn_mute.isChecked()
        
    def on_solo_clicked(self):
        self.track.solo = self.btn_solo.isChecked()
        
    def on_arm_clicked(self):
        self.track.armed = self.btn_arm.isChecked()
        
    def on_input_changed(self):
        ch_data = self.combo_input.currentData()
        if ch_data is not None:
            self.track.input_channel = ch_data
            
    def on_volume_changed(self):
        val_db = self.vol_slider.value() / 10.0
        self.track.volume = val_db
        self.update_volume_label(val_db)
        
    def update_volume_label(self, val_db):
        if val_db <= -60.0:
            self.vol_label.setText("-inf dB")
        else:
            self.vol_label.setText(f"{val_db:+.1f} dB")
            
    def on_pan_changed(self, value):
        self.track.pan = value
        
    def on_fx_clicked(self):
        self.set_selected(True)
        # We can handle custom actions/notifications for fx rack focus
        
    def on_delete_clicked(self):
        self.trackRemoved.emit(self.track)
