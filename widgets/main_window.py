import os
from PySide6.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QLabel,
    QPushButton, QScrollArea, QFileDialog, QSplitter, QSlider,
    QMessageBox, QTabWidget
)
from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QFont, QIcon, QAction, QKeySequence
from audio_engine import AudioEngine
from widgets.track_card import TrackCard
from widgets.effects_rack import EffectsRack
from widgets.level_meter import LevelMeter
from widgets.audio_settings import AudioSettingsDialog
from widgets.timeline import TimelineScrollContainer
from widgets.tuner import GuitarTunerWidget
from widgets.metronome import GuitarMetronomeWidget
import project_manager

class MainWindow(QMainWindow):
    """Core Main Window for the Guitar DAW application."""
    def __init__(self, splash=None):
        super().__init__()
        self.setWindowTitle("Graphite")
        self.setMinimumSize(950, 600)
        self.setObjectName("MainWindow")
        
        if splash:
            splash.set_status("Initializing Audio Engine...", 30)
        # Initialize Audio Engine
        self.audio_engine = AudioEngine()
        
        if splash:
            splash.set_status("Creating default tracks...", 50)
        # Add default track
        self.audio_engine.add_track("Lead Guitar")
        self.audio_engine.add_track("Rhythm Guitar")
        
        self.track_cards = []
        self.selected_track = None
        
        if splash:
            splash.set_status("Building GUI widgets...", 70)
        self.setup_ui()
        
        # Select first track by default
        if self.track_cards:
            self.track_cards[0].set_selected(True)
            
        # Start master meter polling timer
        self.master_timer = QTimer(self)
        self.master_timer.timeout.connect(self.update_master_levels)
        self.master_timer.start(33)
        
        if splash:
            splash.set_status("Starting Audio Stream...", 90)
        # Start the audio stream at startup automatically
        self.audio_engine.start_stream()
        self.update_stream_btn_style()
        
        if splash:
            splash.set_status("Graphite ready!", 100)
        
    def setup_ui(self):
        # Main central widget
        central_widget = QWidget(self)
        central_widget.setObjectName("CentralWidget")
        self.setCentralWidget(central_widget)
        
        main_layout = QVBoxLayout(central_widget)
        main_layout.setContentsMargins(10, 10, 10, 10)
        main_layout.setSpacing(10)
        
        # --- 1. GLOBAL TOP MENU BAR ---
        menu_bar = self.menuBar()
        
        # File Menu
        file_menu = menu_bar.addMenu("File")
        
        self.action_new = QAction("New Project", self)
        self.action_new.setShortcut(QKeySequence("Ctrl+N"))
        self.action_new.triggered.connect(self.on_new_project)
        file_menu.addAction(self.action_new)
        
        self.action_load = QAction("Open Project...", self)
        self.action_load.setShortcut(QKeySequence("Ctrl+O"))
        self.action_load.triggered.connect(self.on_load_project)
        file_menu.addAction(self.action_load)
        
        self.action_save = QAction("Save Project", self)
        self.action_save.setShortcut(QKeySequence("Ctrl+S"))
        self.action_save.triggered.connect(self.on_save_project)
        file_menu.addAction(self.action_save)
        
        self.action_export = QAction("Export Audio...", self)
        self.action_export.setShortcut(QKeySequence("Ctrl+E"))
        self.action_export.triggered.connect(self.on_export_audio)
        file_menu.addAction(self.action_export)
        
        file_menu.addSeparator()
        
        self.action_exit = QAction("Exit", self)
        self.action_exit.setShortcut(QKeySequence("Ctrl+Q"))
        self.action_exit.triggered.connect(self.close)
        file_menu.addAction(self.action_exit)
        
        # Track Menu
        track_menu = menu_bar.addMenu("Track")
        
        self.action_add_track = QAction("Add Track", self)
        self.action_add_track.setShortcut(QKeySequence("Ctrl+T"))
        self.action_add_track.triggered.connect(self.on_add_track)
        track_menu.addAction(self.action_add_track)
        
        # Audio Menu
        audio_menu = menu_bar.addMenu("Audio")
        
        self.action_stream = QAction("Toggle Play / Stop", self)
        self.action_stream.setShortcut(QKeySequence(Qt.Key.Key_Space))
        self.action_stream.triggered.connect(self.toggle_play_stop)
        audio_menu.addAction(self.action_stream)
        
        self.action_pause = QAction("Pause Audio", self)
        self.action_pause.setShortcut(QKeySequence("Ctrl+Space"))
        self.action_pause.triggered.connect(self.on_transport_pause)
        audio_menu.addAction(self.action_pause)
        
        self.action_record = QAction("Record Track", self)
        self.action_record.setShortcut(QKeySequence("R"))
        self.action_record.triggered.connect(self.toggle_record)
        audio_menu.addAction(self.action_record)
        
        self.action_home = QAction("Go to Start", self)
        self.action_home.setShortcut(QKeySequence("Home"))
        self.action_home.triggered.connect(self.on_transport_stop)
        audio_menu.addAction(self.action_home)
        
        self.action_demo = QAction("Guitar Demo Loop", self)
        self.action_demo.setShortcut(QKeySequence("Ctrl+D"))
        self.action_demo.setCheckable(True)
        self.action_demo.setChecked(self.audio_engine.demo_loop_active)
        self.action_demo.triggered.connect(self.toggle_demo_loop)
        audio_menu.addAction(self.action_demo)
        
        audio_menu.addSeparator()
        
        self.action_settings = QAction("Audio Device Settings...", self)
        self.action_settings.setShortcut(QKeySequence("Ctrl+,"))
        self.action_settings.triggered.connect(self.open_settings)
        audio_menu.addAction(self.action_settings)
        
        self.update_demo_btn_style()
        
        # --- 2. MULTI-SPLIT WORKSPACE LAYOUT (Reaper-Style) ---
        main_splitter = QSplitter(Qt.Orientation.Vertical)
        main_splitter.setObjectName("MainVerticalSplitter")
        
        top_workspace = QSplitter(Qt.Orientation.Horizontal)
        top_workspace.setObjectName("TopWorkspaceSplitter")
        
        # Left Panel: Scrollable Track Headers List (TCP)
        self.tracks_scroll = QScrollArea()
        self.tracks_scroll.setObjectName("TracksScrollArea")
        self.tracks_scroll.setWidgetResizable(True)
        self.tracks_scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.tracks_scroll.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        
        self.tracks_container = QWidget()
        self.tracks_container.setObjectName("TracksContainer")
        self.tracks_layout = QVBoxLayout(self.tracks_container)
        self.tracks_layout.setContentsMargins(0, 30, 0, 0)  # Offset 30px top margin to align with Timeline Ruler
        self.tracks_layout.setSpacing(10)
        self.tracks_layout.addStretch()
        
        self.tracks_scroll.setWidget(self.tracks_container)
        top_workspace.addWidget(self.tracks_scroll)
        
        # Right Panel: Waveform Timeline
        self.timeline = TimelineScrollContainer(self.audio_engine, self)
        top_workspace.addWidget(self.timeline)
        
        top_workspace.setSizes([320, 680])
        main_splitter.addWidget(top_workspace)
        
        # Bottom Dock: Tabbed Mixer, Tuner & Metronome Panel
        self.bottom_dock = QTabWidget()
        self.bottom_dock.setObjectName("BottomDockTabs")
        
        self.effects_rack = EffectsRack(self.audio_engine)
        self.bottom_dock.addTab(self.effects_rack, "Effects Rack")
        
        self.tuner_widget = GuitarTunerWidget(self.audio_engine)
        self.bottom_dock.addTab(self.tuner_widget, "Guitar Tuner")
        
        self.metronome_widget = GuitarMetronomeWidget(self.audio_engine)
        self.bottom_dock.addTab(self.metronome_widget, "Metronome")
        
        main_splitter.addWidget(self.bottom_dock)
        
        main_splitter.setSizes([450, 250])
        main_layout.addWidget(main_splitter)
        
        # Synchronize vertical scrolls
        self.tracks_scroll.verticalScrollBar().valueChanged.connect(self.timeline.scroll_area.verticalScrollBar().setValue)
        self.timeline.scroll_area.verticalScrollBar().valueChanged.connect(self.tracks_scroll.verticalScrollBar().setValue)
        
        # Build initial track widgets
        self.refresh_track_cards()
        
        # --- 3. BOTTOM MASTER BAR / STATUS ---
        bottom_bar = QHBoxLayout()
        bottom_bar.setObjectName("BottomBar")
        bottom_bar.setContentsMargins(15, 8, 15, 8)
        bottom_bar.setSpacing(15)
        
        self.lbl_status = QLabel("Audio Engine: RUNNING | Latency: Low Latency Mode")
        self.lbl_status.setObjectName("StatusLabel")
        bottom_bar.addWidget(self.lbl_status)
        
        bottom_bar.addSpacing(20)
        
        bottom_bar.addStretch()
        
        # Master Volume Controls
        lbl_master = QLabel("MASTER VOLUME")
        lbl_master.setObjectName("MasterVolLabel")
        bottom_bar.addWidget(lbl_master)
        
        self.slider_master = QSlider(Qt.Orientation.Horizontal)
        self.slider_master.setMinimum(-600)  # -60 dB
        self.slider_master.setMaximum(60)    # +6 dB
        self.slider_master.setValue(int(self.audio_engine.main_volume * 10))
        self.slider_master.setObjectName("MasterSlider")
        self.slider_master.setMinimumWidth(150)
        self.slider_master.valueChanged.connect(self.on_master_volume_changed)
        bottom_bar.addWidget(self.slider_master)
        
        self.lbl_master_db = QLabel("0.0 dB")
        self.lbl_master_db.setObjectName("MasterDbLabel")
        self.lbl_master_db.setFont(QFont("Consolas", 8))
        bottom_bar.addWidget(self.lbl_master_db)
        self.update_master_volume_label(self.audio_engine.main_volume)
        
        # Master VU meter (placed horizontally or vertically, level meter is vertical)
        self.master_level_meter = LevelMeter()
        self.master_level_meter.setMinimumSize(25, 60)
        self.master_level_meter.setMaximumHeight(65)
        bottom_bar.addWidget(self.master_level_meter)
        
        main_layout.addLayout(bottom_bar)
        
        # --- FLAT GRAPHITE QSS STYLESHEET ---
        self.setStyleSheet("""
            QMainWindow#MainWindow {
                background-color: #1e1e1e;
            }
            QTabWidget::pane {
                border: 1px solid #333333;
                background-color: #1e1e1e;
            }
            QTabBar::tab {
                background-color: #252526;
                color: #888888;
                border: 1px solid #333333;
                border-bottom-color: transparent;
                border-top-left-radius: 4px;
                border-top-right-radius: 4px;
                padding: 6px 15px;
                font-family: "Segoe UI", sans-serif;
                font-size: 11px;
                font-weight: bold;
            }
            QTabBar::tab:selected {
                background-color: #1e1e1e;
                color: #ffffff;
                border-bottom-color: #1e1e1e;
            }
            QTabBar::tab:hover {
                background-color: #2d2d2d;
                color: #ffffff;
            }
            #CentralWidget {
                background-color: #1e1e1e;
            }
            QLabel {
                color: #e0e0e0;
                font-family: "Inter", sans-serif;
            }
            #BottomBar {
                background-color: #252526;
                border: 1px solid #333333;
                border-radius: 6px;
            }
            QPushButton#SettingsButton:hover {
                background-color: rgba(255, 255, 255, 0.05);
                color: #ffffff;
                border-color: #555555;
            }
            QPushButton#TransportButton {
                background-color: #2a2d32;
                color: #e0e0e0;
                border: 1px solid #3e4249;
                border-radius: 4px;
                padding: 4px 12px;
                font-size: 11px;
                font-weight: bold;
            }
            QPushButton#TransportButton:hover {
                background-color: #3e4249;
                color: #ffffff;
            }
            #TracksScrollArea {
                background: transparent;
            }
            QMenuBar {
                background-color: #252526;
                color: #d4d4d4;
                border-bottom: 1px solid #333333;
                font-family: "Segoe UI", sans-serif;
                font-size: 11px;
            }
            QMenuBar::item {
                background-color: transparent;
                padding: 4px 10px;
                color: #d4d4d4;
            }
            QMenuBar::item:selected {
                background-color: #3e3e42;
                color: #ffffff;
            }
            QMenu {
                background-color: #252526;
                color: #d4d4d4;
                border: 1px solid #333333;
                font-family: "Segoe UI", sans-serif;
                font-size: 11px;
            }
            QMenu::item {
                padding: 6px 25px 6px 20px;
                background-color: transparent;
            }
            QMenu::item:selected {
                background-color: #3e3e42;
                color: #ffffff;
            }
            QMenu::separator {
                height: 1px;
                background-color: #333333;
                margin: 4px 0px;
            }
            #TracksScrollArea {
                background: transparent;
            }
            #TracksContainer {
                background: transparent;
            }
            #WorkspaceSplitter::handle {
                background-color: #333333;
                width: 2px;
            }
            QLabel#StatusLabel {
                color: #888888;
                font-size: 11px;
            }
            QLabel#MasterVolLabel {
                color: #b3b3b3;
                font-size: 10px;
                font-weight: bold;
                letter-spacing: 0.5px;
            }
            QLabel#MasterDbLabel {
                color: #888888;
                font-size: 10px;
            }
            QSlider#MasterSlider::groove:horizontal {
                background: #18181c;
                height: 6px;
                border-radius: 3px;
            }
            QSlider#MasterSlider::sub-page:horizontal {
                background: #888888;
                height: 6px;
                border-radius: 3px;
            }
            QSlider#MasterSlider::handle:horizontal {
                background: #505050;
                border: 1px solid #666;
                width: 12px;
                margin-top: -3px;
                margin-bottom: -3px;
                border-radius: 6px;
            }
            QSlider#MasterSlider::handle:horizontal:hover {
                background: #888888;
                border-color: #888888;
            }
        """)

    def refresh_track_cards(self):
        """Clears and rebuilds the vertical track listing UI."""
        # Clean up existing widgets
        for card in self.track_cards:
            self.tracks_layout.removeWidget(card)
            card.deleteLater()
        self.track_cards.clear()
        
        # Build cards for each track in audio engine
        for track in self.audio_engine.tracks:
            card = TrackCard(track, self.audio_engine)
            card.trackSelected.connect(self.on_track_selected)
            card.trackRemoved.connect(self.on_track_removed)
            card.btn_fx.clicked.connect(lambda t=track: self.focus_fx_rack(t))
            
            self.tracks_layout.insertWidget(self.tracks_layout.count() - 1, card)
            self.track_cards.append(card)
            
        # Select first track if available
        if self.track_cards:
            self.track_cards[0].set_selected(True)
        else:
            self.effects_rack.set_track(None)
            self.selected_track = None
            
        if hasattr(self, 'timeline'):
            self.timeline.update_track_layout()

    def on_track_selected(self, track):
        """Deselects other cards and selects this track."""
        self.selected_track = track
        self.audio_engine.selected_track_id = track.track_id if track else None
        for card in self.track_cards:
            if card.track != track:
                card.is_selected = False
                card.update_selection_style()
                
        # Link to effects rack
        self.effects_rack.set_track(track)
        
    def focus_fx_rack(self, track):
        """Forces selecting the track and highlighting the effects rack."""
        self.on_track_selected(track)
        
    def on_add_track(self):
        """Adds a track to audio engine and UI."""
        new_track = self.audio_engine.add_track()
        card = TrackCard(new_track, self.audio_engine)
        card.trackSelected.connect(self.on_track_selected)
        card.trackRemoved.connect(self.on_track_removed)
        card.btn_fx.clicked.connect(lambda t=new_track: self.focus_fx_rack(t))
        
        self.tracks_layout.insertWidget(self.tracks_layout.count() - 1, card)
        self.track_cards.append(card)
        
        # Select it immediately
        card.set_selected(True)
        
        if hasattr(self, 'timeline'):
            self.timeline.update_track_layout()
        
    def on_track_removed(self, track):
        """Deletes a track from engine and UI."""
        # Find card
        target_card = None
        for card in self.track_cards:
            if card.track == track:
                target_card = card
                break
                
        if target_card:
            self.tracks_layout.removeWidget(target_card)
            self.track_cards.remove(target_card)
            target_card.deleteLater()
            
            # Remove from engine
            self.audio_engine.remove_track(track.track_id)
            
            # Reset selection if deleted track was active
            if self.selected_track == track:
                if self.track_cards:
                    self.track_cards[0].set_selected(True)
                else:
                    self.effects_rack.set_track(None)
                    self.selected_track = None
                    self.audio_engine.selected_track_id = None
                    
            if hasattr(self, 'timeline'):
                self.timeline.update_track_layout()

    def toggle_audio_stream(self):
        """Toggles the global sounddevice stream running state."""
        if self.audio_engine.is_running:
            self.audio_engine.stop_stream()
        else:
            self.audio_engine.start_stream()
        self.update_stream_btn_style()

    def update_stream_btn_style(self):
        """Changes stream menu action visual text based on stream state."""
        if hasattr(self, 'action_stream'):
            if self.audio_engine.is_running:
                self.action_stream.setText("Stop Audio")
            else:
                self.action_stream.setText("Start Audio")
        
        if self.audio_engine.is_running:
            self.lbl_status.setText("Audio Engine: RUNNING | Low Latency")
        else:
            self.lbl_status.setText("Audio Engine: STOPPED")
            
        if hasattr(self, 'timeline') and hasattr(self.timeline, 'btn_play_pause'):
            self.update_transport_ui()

    def toggle_demo_loop(self):
        """Toggles real-time pre-computed chord arpeggio demo feedback."""
        active = self.action_demo.isChecked()
        self.audio_engine.demo_loop_active = active
        self.update_demo_btn_style()
        
    def update_demo_btn_style(self):
        """Changes style of demo loop button based on active state."""
        if hasattr(self, 'action_demo'):
            self.action_demo.setChecked(self.audio_engine.demo_loop_active)

    def open_settings(self):
        """Opens audio settings dialog panel."""
        dlg = AudioSettingsDialog(self.audio_engine, self)
        if dlg.exec() == AudioSettingsDialog.DialogCode.Accepted:
            # Refresh inputs in track cards in case device channels changed
            for card in self.track_cards:
                card.populate_inputs()
            self.update_stream_btn_style()

    def on_master_volume_changed(self):
        val_db = self.slider_master.value() / 10.0
        self.audio_engine.main_volume = val_db
        self.update_master_volume_label(val_db)
        
    def update_master_volume_label(self, val_db):
        if val_db <= -60.0:
            self.lbl_master_db.setText("-inf dB")
        else:
            self.lbl_master_db.setText(f"{val_db:+.1f} dB")

    def update_master_levels(self):
        """Polls main engine peak VU dB and updates GUI meter."""
        self.master_level_meter.set_level(self.audio_engine.main_level_history)
        if hasattr(self, 'timeline'):
            self.timeline.update_widgets()

    def on_new_project(self):
        """Clears all tracks and creates a blank project."""
        reply = QMessageBox.question(
            self,
            "New Project",
            "Are you sure you want to clear current session and create a new project?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        if reply == QMessageBox.StandardButton.Yes:
            self.audio_engine.stop_stream()
            with self.audio_engine.lock:
                self.audio_engine.tracks.clear()
                self.audio_engine.main_volume = 0.0
                self.audio_engine.demo_loop_active = False
            self.slider_master.setValue(0)
            self.action_demo.setChecked(False)
            self.update_demo_btn_style()
            self.audio_engine.add_track("Guitar 1")
            self.refresh_track_cards()
            self.audio_engine.start_stream()
            self.update_stream_btn_style()
            if hasattr(self, 'timeline'):
                self.timeline.update_track_layout()

    def on_save_project(self):
        """Saves current state to JSON file dialog."""
        file_path, _ = QFileDialog.getSaveFileName(
            self,
            "Save Graphite Session",
            "",
            "Graphite DAW Project (*.graphite)"
        )
        if file_path:
            # Add extension if not typed
            if not file_path.endswith(".graphite"):
                file_path += ".graphite"
            success = project_manager.save_project(file_path, self.audio_engine)
            if success:
                QMessageBox.information(self, "Project Saved", f"Successfully saved session to:\n{os.path.basename(file_path)}")
            else:
                QMessageBox.critical(self, "Save Error", "Failed to save project file.")
 
    def on_export_audio(self):
        """Opens the export settings dialog to render timeline mixdown to file."""
        from widgets.export_dialog import ExportDialog
        dlg = ExportDialog(self.audio_engine, self)
        dlg.exec()
 
    def on_load_project(self):
        """Loads session file and rebuilds cards UI."""
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            "Open Graphite Session",
            "",
            "Graphite DAW Project (*.graphite *.gtrp);;Graphite Project (*.graphite);;Legacy Cyberamp Project (*.gtrp)"
        )
        if file_path:
            success = project_manager.load_project(file_path, self.audio_engine)
            if success:
                # Rebuild cards and GUI state
                self.refresh_track_cards()
                self.slider_master.setValue(int(self.audio_engine.main_volume * 10))
                self.update_master_volume_label(self.audio_engine.main_volume)
                self.action_demo.setChecked(self.audio_engine.demo_loop_active)
                self.update_demo_btn_style()
                self.update_stream_btn_style()
                
                # Check dropdowns inputs in track cards
                for card in self.track_cards:
                    card.populate_inputs()
                    
                if hasattr(self, 'timeline'):
                    self.timeline.update_track_layout()
                    
                QMessageBox.information(self, "Project Loaded", f"Successfully loaded session:\n{os.path.basename(file_path)}")
            else:
                QMessageBox.critical(self, "Load Error", "Failed to parse or restore project file. Some VST3s may have failed loading.")

    def closeEvent(self, event):
        """Release audio stream explicitly when app terminates."""
        self.audio_engine.stop_stream()
        try:
            from audio_engine import clean_temp_vsts
            clean_temp_vsts()
        except Exception:
            pass
        super().closeEvent(event)

    def toggle_play_pause(self):
        if self.audio_engine.play_state == "playing":
            self.on_transport_pause()
        else:
            self.on_transport_play()

    def toggle_play_stop(self):
        if self.audio_engine.play_state == "playing":
            self.on_transport_stop()
        else:
            self.on_transport_play()

    def toggle_record(self):
        if self.audio_engine.play_state == "recording":
            self.on_transport_stop()
        else:
            self.on_transport_record()

    def on_transport_play(self):
        self.audio_engine.start_playback()
        self.update_transport_ui()
        self.update_stream_btn_style()
        
    def on_transport_pause(self):
        self.audio_engine.pause_playback()
        self.update_transport_ui()
        self.update_stream_btn_style()
        if hasattr(self, 'timeline'):
            self.timeline.update_widgets()
            self.timeline.update_track_layout()
        
    def on_transport_stop(self):
        self.audio_engine.stop_playback()
        self.update_transport_ui()
        self.update_stream_btn_style()
        if hasattr(self, 'timeline'):
            self.timeline.update_widgets()
            self.timeline.update_track_layout()
        
    def on_transport_record(self):
        self.audio_engine.start_recording()
        self.update_transport_ui()
        self.update_stream_btn_style()

    def update_transport_ui(self):
        state = self.audio_engine.play_state
        if not hasattr(self, 'timeline') or not hasattr(self.timeline, 'btn_play_pause'):
            return
            
        self.timeline.btn_play_pause.setStyleSheet("")
        self.timeline.btn_record.setStyleSheet("")
        
        if state == "playing":
            self.timeline.btn_play_pause.setIcon(self.timeline.icon_pause)
            self.timeline.btn_play_pause.setToolTip("Pause")
            self.timeline.btn_play_pause.setStyleSheet("background-color: #2b5a30;")
        else:
            self.timeline.btn_play_pause.setIcon(self.timeline.icon_play)
            self.timeline.btn_play_pause.setToolTip("Play")
            if state == "paused":
                self.timeline.btn_play_pause.setStyleSheet("background-color: #6b5317;")
                
        if state == "recording":
            self.timeline.btn_record.setStyleSheet("background-color: #802b2b;")
