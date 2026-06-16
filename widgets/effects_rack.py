import os
from PySide6.QtWidgets import (
    QWidget, QFrame, QVBoxLayout, QHBoxLayout, QLabel,
    QPushButton, QComboBox, QScrollArea, QFileDialog, QSizePolicy, QMessageBox,
    QMenu
)
from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QFont, QColor
from pedalboard import (
    NoiseGate, Distortion, Chorus, Phaser, Delay, Reverb,
    PitchShift, Compressor, LowpassFilter, HighpassFilter, load_plugin
)
from audio_engine import EffectWrapper
from widgets.knob import CustomKnob

def scan_for_vst3s(search_paths):
    vst_list = []
    for path in search_paths:
        if not os.path.exists(path):
            continue
        try:
            for root, dirs, files in os.walk(path):
                # Handle folders ending in .vst3
                vst_dirs = [d for d in dirs if d.lower().endswith('.vst3')]
                for d in vst_dirs:
                    full_path = os.path.join(root, d)
                    name = os.path.splitext(d)[0]
                    vst_list.append((name, full_path))
                
                # Prune walking deeper into .vst3 folders
                dirs[:] = [d for d in dirs if not d.lower().endswith('.vst3')]
                
                for f in files:
                    if f.lower().endswith('.vst3'):
                        full_path = os.path.join(root, f)
                        name = os.path.splitext(f)[0]
                        if not any(item[1] == full_path for item in vst_list):
                            vst_list.append((name, full_path))
        except Exception as e:
            print(f"Error scanning VST path {path}: {e}")
    # Sort alphabetically by name
    vst_list.sort(key=lambda x: x[0].lower())
    return vst_list

# Map of parameter names to their UI configurations
PARAM_METADATA = {
    "threshold_db": {"label": "Threshold", "min": -80.0, "max": 0.0, "default": -40.0, "unit": "dB", "decimals": 1},
    "ratio": {"label": "Ratio", "min": 1.0, "max": 20.0, "default": 4.0, "unit": ":1", "decimals": 1},
    "attack_ms": {"label": "Attack", "min": 0.1, "max": 200.0, "default": 2.0, "unit": "ms", "decimals": 1},
    "release_ms": {"label": "Release", "min": 10.0, "max": 2000.0, "default": 150.0, "unit": "ms", "decimals": 0},
    "drive_db": {"label": "Drive", "min": 0.0, "max": 50.0, "default": 20.0, "unit": "dB", "decimals": 1},
    "rate_hz": {"label": "Rate", "min": 0.05, "max": 10.0, "default": 1.0, "unit": "Hz", "decimals": 2},
    "depth": {"label": "Depth", "min": 0.0, "max": 1.0, "default": 0.5, "unit": "", "decimals": 2},
    "feedback": {"label": "Feedback", "min": 0.0, "max": 0.99, "default": 0.25, "unit": "", "decimals": 2},
    "mix": {"label": "Mix", "min": 0.0, "max": 1.0, "default": 0.5, "unit": "", "decimals": 2},
    "delay_seconds": {"label": "Delay", "min": 0.01, "max": 2.0, "default": 0.3, "unit": "s", "decimals": 2},
    "room_size": {"label": "Room Size", "min": 0.0, "max": 1.0, "default": 0.5, "unit": "", "decimals": 2},
    "damping": {"label": "Damping", "min": 0.0, "max": 1.0, "default": 0.5, "unit": "", "decimals": 2},
    "wet_level": {"label": "Wet Level", "min": 0.0, "max": 1.0, "default": 0.3, "unit": "", "decimals": 2},
    "dry_level": {"label": "Dry Level", "min": 0.0, "max": 1.0, "default": 0.7, "unit": "", "decimals": 2},
    "width": {"label": "Width", "min": 0.0, "max": 1.0, "default": 1.0, "unit": "", "decimals": 2},
    "semitones": {"label": "Pitch", "min": -12.0, "max": 12.0, "default": 0.0, "unit": "st", "decimals": 1},
    "cutoff_frequency_hz": {"label": "Cutoff", "min": 20.0, "max": 20000.0, "default": 1000.0, "unit": "Hz", "decimals": 0},
}

EFFECT_TYPES = {
    "Noise Gate": "NoiseGate",
    "Overdrive": "Distortion",
    "Chorus": "Chorus",
    "Phaser": "Phaser",
    "Delay": "Delay",
    "Reverb": "Reverb",
    "Pitch Shift": "PitchShift",
    "Compressor": "Compressor",
    "Lowpass Filter": "LowpassFilter",
    "Highpass Filter": "HighpassFilter",
}

class EffectCard(QFrame):
    """Visual card representing a single effect in the track rack."""
    effectChanged = Signal()  # Emitted when parameter, bypass, or delete changes
    
    def __init__(self, wrapper, track, parent=None):
        super().__init__(parent)
        self.wrapper = wrapper
        self.track = track
        
        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.setObjectName("EffectCard")
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Preferred)
        
        self.setup_ui()
        
    def setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 8, 10, 8)
        layout.setSpacing(8)
        
        # --- HEADER ROW ---
        header = QHBoxLayout()
        header.setSpacing(6)
        
        # Bypass toggle (power icon)
        self.btn_bypass = QPushButton("⏻")
        self.btn_bypass.setCheckable(True)
        self.btn_bypass.setChecked(not self.wrapper.is_active)  # Checked is bypassed/off
        self.btn_bypass.setObjectName("BypassButton")
        self.btn_bypass.setToolTip("Toggle Bypass")
        self.btn_bypass.clicked.connect(self.on_bypass_toggle)
        header.addWidget(self.btn_bypass)
        
        # Effect Name Label
        self.label_name = QLabel(self.wrapper.name)
        self.label_name.setObjectName("EffectNameLabel")
        header.addWidget(self.label_name)
        
        header.addStretch()
        
        # Delete Button (X)
        self.btn_delete = QPushButton("✕")
        self.btn_delete.setObjectName("DeleteButton")
        self.btn_delete.setToolTip("Remove Effect")
        self.btn_delete.clicked.connect(self.on_delete_clicked)
        header.addWidget(self.btn_delete)
        
        layout.addLayout(header)
        
        # --- BODY ROW (Knobs / Settings) ---
        body = QHBoxLayout()
        body.setSpacing(15)
        
        if self.wrapper.effect_type == "VST3":
            # For VST3, show "Open Editor" button and a path label
            vst_layout = QVBoxLayout()
            vst_layout.setSpacing(4)
            
            vst_path = getattr(self.wrapper, "original_vst_path", None)
            if not vst_path:
                vst_path = getattr(self.wrapper.effect, "path", "")
            filename = os.path.basename(vst_path)
            
            self.lbl_vst_path = QLabel(f"Path: {filename}")
            self.lbl_vst_path.setObjectName("VstPathLabel")
            self.lbl_vst_path.setToolTip(vst_path)
            self.lbl_vst_path.setWordWrap(True)
            vst_layout.addWidget(self.lbl_vst_path)
            
            self.btn_editor = QPushButton("Settings")
            self.btn_editor.setObjectName("VstEditorButton")
            self.btn_editor.clicked.connect(self.open_vst_editor)
            vst_layout.addWidget(self.btn_editor)
            
            body.addLayout(vst_layout)
        else:
            # Render custom knobs for built-in effects
            from project_manager import EFFECT_CLASSES
            if self.wrapper.effect_type in EFFECT_CLASSES:
                _, params = EFFECT_CLASSES[self.wrapper.effect_type]
                for p_name in params:
                    if p_name in PARAM_METADATA:
                        meta = PARAM_METADATA[p_name]
                        curr_val = getattr(self.wrapper.effect, p_name, meta["default"])
                        
                        knob = CustomKnob(
                            label=meta["label"].upper(),
                            min_val=meta["min"],
                            max_val=meta["max"],
                            default_val=meta["default"],
                            unit=meta["unit"],
                            decimals=meta["decimals"]
                        )
                        knob.setValue(curr_val)
                        # Connect knob to real-time parameter setting
                        # Inside lambda, store parameter name dynamically
                        knob.valueChanged.connect(
                            lambda val, name=p_name: self.on_parameter_changed(name, val)
                        )
                        body.addWidget(knob)
                        
        body.addStretch()
        layout.addLayout(body)
        
        # Apply Card Styles
        self.setStyleSheet("""
            EffectCard {
                background-color: #0b0b0c;
                border: 1px solid #222225;
                border-radius: 4px;
            }
            QPushButton#BypassButton {
                font-family: "Consolas", "Courier New", monospace;
                font-weight: bold;
                font-size: 11px;
                border-radius: 4px;
                min-width: 26px;
                min-height: 26px;
                max-width: 26px;
                max-height: 26px;
                color: #000000;
                background-color: #ffffff;
                border: 1px solid #ffffff;
            }
            QPushButton#BypassButton:checked {
                color: #88888c;
                background-color: #0b0b0c;
                border-color: #222225;
            }
            QLabel#EffectNameLabel {
                color: #ffffff;
                font-family: "Consolas", "Courier New", monospace;
                font-size: 11px;
                font-weight: bold;
                text-transform: uppercase;
                letter-spacing: 0.5px;
            }
            QPushButton#DeleteButton {
                color: #88888c;
                font-family: "Consolas", "Courier New", monospace;
                font-weight: bold;
                background: transparent;
                border: none;
                font-size: 12px;
                min-width: 20px;
                min-height: 20px;
            }
            QPushButton#DeleteButton:hover {
                color: #ff0033;
                background-color: rgba(255, 0, 51, 0.1);
                border-radius: 10px;
            }
            QLabel#VstPathLabel {
                color: #888888;
                font-size: 10px;
                font-family: "Segoe UI", sans-serif;
            }
            QPushButton#VstEditorButton {
                background-color: #333333;
                color: #e0e0e0;
                font-weight: bold;
                border: 1px solid #444444;
                border-radius: 3px;
                padding: 6px 12px;
                font-size: 11px;
            }
            QPushButton#VstEditorButton:hover {
                background-color: #444444;
                color: #ffffff;
                border-color: #555555;
            }
        """)
        
    def on_bypass_toggle(self):
        # Wrapper is bypassed if bypass button is checked
        self.wrapper.is_active = not self.btn_bypass.isChecked()
        self.track.update_pedalboard()
        self.effectChanged.emit()
        
    def on_parameter_changed(self, param_name, value):
        # Set parameter directly on the pedalboard effect object
        if hasattr(self.wrapper.effect, param_name):
            setattr(self.wrapper.effect, param_name, value)

    from PySide6.QtCore import Slot
    @Slot(int, int)
    def _resize_vst_dialog(self, w, h):
        if hasattr(self, 'vst_dialog') and self.vst_dialog:
            self.vst_dialog.setFixedSize(w, h)

    def open_vst_editor(self):
        try:
            if not hasattr(self.wrapper.effect, "show_editor"):
                QMessageBox.warning(self, "VST3 Editor", "This plugin does not support a custom editor interface.")
                return
            
            import platform
            if platform.system() != "Windows":
                # Non-Windows: just open directly
                self.wrapper.effect.show_editor()
                return
            
            from PySide6.QtWidgets import QDialog
            from PySide6.QtCore import Qt
            import ctypes
            import ctypes.wintypes
            
            user32 = ctypes.windll.user32
            kernel32 = ctypes.windll.kernel32
            current_pid = kernel32.GetCurrentProcessId()
            
            # Use pointer-safe window style functions to prevent OverflowError
            user32.SetWindowLongPtrW.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p]
            user32.SetWindowLongPtrW.restype = ctypes.c_void_p
            user32.GetWindowLongPtrW.argtypes = [ctypes.c_void_p, ctypes.c_int]
            user32.GetWindowLongPtrW.restype = ctypes.c_void_p
            
            # Create a Qt Dialog to host the VST window
            self.vst_dialog = QDialog(self)
            self.vst_dialog.setWindowTitle(f"{self.wrapper.name} Settings")
            self.vst_dialog.setWindowFlags(Qt.Window | Qt.WindowTitleHint | Qt.WindowCloseButtonHint)
            self.vst_dialog.resize(600, 400) # Initial size, will adapt
            
            # Show the dialog immediately
            self.vst_dialog.show()
            
            import sys
            import os
            sys.path.append(os.path.dirname(os.path.dirname(__file__)))
            from theme_utils import apply_dark_titlebar
            apply_dark_titlebar(self.vst_dialog)

            # Restore previous position if we have one
            if hasattr(self.wrapper, '_last_editor_pos') and self.wrapper._last_editor_pos is not None:
                self.vst_dialog.move(self.wrapper._last_editor_pos)
            
            dialog_hwnd = int(self.vst_dialog.winId())
            vst_hwnd_ref = [None]
            original_style_ref = [None]
            
            # Ensure the dialog closes the VST when its X button is clicked
            def on_close(event):
                # Save the current window position for next time
                self.wrapper._last_editor_pos = self.vst_dialog.pos()
                
                if vst_hwnd_ref[0]:
                    # Extremely important: Reparent the VST back to the desktop BEFORE the dialog is destroyed.
                    # Otherwise, destroying the QDialog destroys the VST HWND, breaking it for future opens!
                    user32.SetParent(vst_hwnd_ref[0], 0)
                    
                    # Restore original styles
                    if original_style_ref[0] is not None:
                        GWL_STYLE = -16
                        user32.SetWindowLongPtrW(vst_hwnd_ref[0], GWL_STYLE, ctypes.c_void_p(original_style_ref[0]))
                    
                    WM_CLOSE = 0x0010
                    user32.PostMessageW(vst_hwnd_ref[0], WM_CLOSE, 0, 0)
                event.accept()
            self.vst_dialog.closeEvent = on_close
            
            # Setup WinEventHook to catch the VST window
            EVENT_OBJECT_CREATE = 0x8000
            EVENT_OBJECT_SHOW = 0x8002
            WINEVENT_OUTOFCONTEXT = 0x0000
            GA_ROOT = 2
            
            WINEVENTPROC = ctypes.WINFUNCTYPE(
                None, ctypes.c_void_p, ctypes.wintypes.DWORD, ctypes.c_void_p,
                ctypes.wintypes.LONG, ctypes.wintypes.LONG,
                ctypes.wintypes.DWORD, ctypes.wintypes.DWORD
            )
            
            hook_ref = [None]
            processed = set()
            
            def _win_event_callback(hHook, event, hwnd, idObject, idChild, tid, time):
                if not hwnd or idObject != 0:
                    return
                
                # Check process
                pid = ctypes.c_ulong()
                user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
                if pid.value != current_pid:
                    return
                
                # Get root window
                root = user32.GetAncestor(hwnd, GA_ROOT)
                if not root:
                    root = hwnd
                    
                if root in processed or root == dialog_hwnd:
                    return
                
                class_buf = ctypes.create_unicode_buffer(256)
                user32.GetClassNameW(root, class_buf, 256)
                if class_buf.value.startswith("Qt"):
                    return
                    
                length = user32.GetWindowTextLengthW(root)
                buf = ctypes.create_unicode_buffer(length + 1)
                user32.GetWindowTextW(root, buf, length + 1)
                if buf.value == "Graphite":
                    return
                
                # Valid VST window found
                processed.add(root)
                vst_hwnd_ref[0] = root
                
                class RECT(ctypes.Structure):
                    _fields_ = [("left", ctypes.c_long), ("top", ctypes.c_long),
                                ("right", ctypes.c_long), ("bottom", ctypes.c_long)]
                
                # Use GetClientRect to get the exact size of the plugin's internal canvas
                rect = RECT()
                user32.GetClientRect(root, ctypes.byref(rect))
                w = rect.right - rect.left
                h = rect.bottom - rect.top
                
                # Strip native borders/popups and add child style
                GWL_STYLE = -16
                WS_POPUP = 0x80000000
                WS_CAPTION = 0x00C00000
                WS_THICKFRAME = 0x00040000
                WS_CHILD = 0x40000000
                
                # Use pointer-safe version to read style
                style_ptr = user32.GetWindowLongPtrW(root, GWL_STYLE)
                # Convert void_p to int
                style = ctypes.cast(style_ptr, ctypes.c_void_p).value
                if style is None:
                    style = 0
                
                original_style_ref[0] = style
                
                style = (style & ~WS_POPUP & ~WS_CAPTION & ~WS_THICKFRAME) | WS_CHILD
                user32.SetWindowLongPtrW(root, GWL_STYLE, ctypes.c_void_p(style))
                
                # Embed inside the QDialog
                user32.SetParent(root, dialog_hwnd)
                
                # Position it perfectly inside the QDialog client area
                SWP_NOZORDER = 0x0004
                SWP_FRAMECHANGED = 0x0020
                SWP_SHOWWINDOW = 0x0040
                user32.SetWindowPos(root, 0, 0, 0, w, h, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW)
                
                # Lock the Qt Dialog strictly to the VST's internal size
                from PySide6.QtCore import QMetaObject, Qt, Q_ARG
                QMetaObject.invokeMethod(self, "_resize_vst_dialog", Qt.QueuedConnection, Q_ARG(int, w), Q_ARG(int, h))
                
                if hook_ref[0]:
                    user32.UnhookWinEvent(hook_ref[0])
                    hook_ref[0] = None

            self._vst_hook_cb = WINEVENTPROC(_win_event_callback)
            hook_ref[0] = user32.SetWinEventHook(
                EVENT_OBJECT_CREATE, EVENT_OBJECT_SHOW,
                None, self._vst_hook_cb, 0, 0, WINEVENT_OUTOFCONTEXT
            )
            
            # show_editor() will block, but its message loop will dispatch our hook and keep the QDialog responsive
            self.wrapper.effect.show_editor()
            
            # Cleanup
            if hook_ref[0]:
                user32.UnhookWinEvent(hook_ref[0])
                hook_ref[0] = None
            self._vst_hook_cb = None
            
            # Close the wrapper dialog if it's still open
            if self.vst_dialog.isVisible():
                self.vst_dialog.close()
                
        except Exception as e:
            QMessageBox.critical(self, "VST3 Editor Error", f"Failed to open editor: {e}")
            
    def on_delete_clicked(self):
        self.track.effects.remove(self.wrapper)
        self.track.update_pedalboard()
        self.effectChanged.emit()


class EffectsRack(QWidget):
    """Rack panel containing list of active effects and add menus."""
    def __init__(self, audio_engine, parent=None):
        super().__init__(parent)
        self.audio_engine = audio_engine
        self.selected_track = None
        
        self.setup_ui()
        
    def setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(10)
        
        # --- TOP PANEL: Add Effect Controls ---
        top_bar = QHBoxLayout()
        top_bar.setSpacing(10)
        
        self.lbl_title = QLabel("Effects Rack")
        self.lbl_title.setObjectName("RackTitle")
        self.lbl_title.setFont(QFont("Inter", 12, QFont.Weight.Bold))
        top_bar.addWidget(self.lbl_title)
        
        top_bar.addStretch()
        
        # Dropdown selector to choose effect
        self.combo_fx = QComboBox()
        self.combo_fx.setObjectName("AddEffectCombo")
        self.combo_fx.addItem("Add Built-in Effect...", None)
        for name in EFFECT_TYPES.keys():
            self.combo_fx.addItem(name, EFFECT_TYPES[name])
        top_bar.addWidget(self.combo_fx)
        
        self.btn_add = QPushButton("Add FX")
        self.btn_add.setObjectName("AddFxButton")
        self.btn_add.clicked.connect(self.on_add_effect)
        top_bar.addWidget(self.btn_add)
        
        # Add VST3 button
        self.btn_add_vst = QPushButton("Load VST3...")
        self.btn_add_vst.setObjectName("AddVstButton")
        self.vst_menu = QMenu(self)
        self.vst_menu.aboutToShow.connect(self.populate_vst_menu)
        self.btn_add_vst.setMenu(self.vst_menu)
        top_bar.addWidget(self.btn_add_vst)
        
        layout.addLayout(top_bar)
        
        # --- MIDDLE PANEL: Scrolling list of cards ---
        self.scroll_area = QScrollArea()
        self.scroll_area.setObjectName("RackScrollArea")
        self.scroll_area.setWidgetResizable(True)
        self.scroll_area.setFrameShape(QFrame.Shape.NoFrame)
        
        self.scroll_widget = QWidget()
        self.scroll_widget.setObjectName("RackScrollWidget")
        self.scroll_layout = QVBoxLayout(self.scroll_widget)
        self.scroll_layout.setContentsMargins(0, 0, 0, 0)
        self.scroll_layout.setSpacing(10)
        self.scroll_layout.addStretch()  # Pin elements to the top
        
        self.scroll_area.setWidget(self.scroll_widget)
        layout.addWidget(self.scroll_area)
        
        # Styling
        self.setStyleSheet("""
            QLabel#RackTitle {
                color: #ffffff;
                font-family: "Consolas", "Courier New", monospace;
            }
            QComboBox#AddEffectCombo {
                background-color: #0b0b0c;
                border: 1px solid #222225;
                border-radius: 4px;
                color: #88888c;
                padding: 5px 10px;
                font-family: "Consolas", "Courier New", monospace;
                font-size: 11px;
                min-width: 150px;
            }
            QPushButton#AddFxButton {
                background-color: #0b0b0c;
                color: #88888c;
                font-family: "Consolas", "Courier New", monospace;
                font-weight: bold;
                border: 1px solid #222225;
                border-radius: 4px;
                padding: 6px 14px;
                font-size: 11px;
            }
            QPushButton#AddFxButton:hover {
                background-color: #1a1a1c;
                color: #ffffff;
                border-color: #444448;
            }
            QPushButton#AddVstButton {
                background-color: #0b0b0c;
                border: 1px solid #222225;
                border-radius: 4px;
                color: #88888c;
                font-family: "Consolas", "Courier New", monospace;
                font-weight: bold;
                padding: 5px 12px;
                font-size: 11px;
            }
            QPushButton#AddVstButton:hover {
                background-color: #1a1a1c;
                color: #ffffff;
                border-color: #444448;
            }
            #RackScrollArea {
                background: transparent;
            }
            #RackScrollWidget {
                background: transparent;
            }
        """)
        
    def set_track(self, track):
        """Sets the active track and redraws cards."""
        self.selected_track = track
        self.refresh_rack()
        
    def refresh_rack(self):
        """Clears and rebuilds the effects list."""
        # Clear existing card widgets (except the spacer stretch)
        while self.scroll_layout.count() > 1:
            child = self.scroll_layout.takeAt(0)
            if child.widget():
                child.widget().deleteLater()
                
        if not self.selected_track:
            self.lbl_title.setText("Effects Rack (No Track Selected)")
            self.combo_fx.setEnabled(False)
            self.btn_add.setEnabled(False)
            self.btn_add_vst.setEnabled(False)
            return
            
        self.lbl_title.setText(f"Effects Rack: {self.selected_track.name}")
        self.combo_fx.setEnabled(True)
        self.btn_add.setEnabled(True)
        self.btn_add_vst.setEnabled(True)
        
        # Insert cards for each effect (above the stretch spacer)
        for wrapper in self.selected_track.effects:
            card = EffectCard(wrapper, self.selected_track)
            card.effectChanged.connect(self.refresh_rack)
            self.scroll_layout.insertWidget(self.scroll_layout.count() - 1, card)
            
    def on_add_effect(self):
        if not self.selected_track:
            return
            
        fx_type = self.combo_fx.currentData()
        if not fx_type:
            return
            
        # Instantiate correct built-in pedalboard class
        effect_obj = None
        try:
            if fx_type == "NoiseGate":
                effect_obj = NoiseGate(threshold_db=-40, ratio=10, attack_ms=1.0, release_ms=100.0)
            elif fx_type == "Distortion":
                effect_obj = Distortion(drive_db=15)
            elif fx_type == "Chorus":
                effect_obj = Chorus(rate_hz=1.0, depth=0.25, feedback=0.0, mix=0.5)
            elif fx_type == "Phaser":
                effect_obj = Phaser(rate_hz=1.0, depth=0.5, feedback=0.0, mix=0.5)
            elif fx_type == "Delay":
                effect_obj = Delay(delay_seconds=0.3, feedback=0.3, mix=0.4)
            elif fx_type == "Reverb":
                effect_obj = Reverb(room_size=0.5, damping=0.5, wet_level=0.33, dry_level=0.4, width=1.0)
            elif fx_type == "PitchShift":
                effect_obj = PitchShift(semitones=0)
            elif fx_type == "Compressor":
                effect_obj = Compressor(threshold_db=-20.0, ratio=4.0, attack_ms=2.0, release_ms=150.0)
            elif fx_type == "LowpassFilter":
                effect_obj = LowpassFilter(cutoff_frequency_hz=1500)
            elif fx_type == "HighpassFilter":
                effect_obj = HighpassFilter(cutoff_frequency_hz=80)
        except Exception as e:
            QMessageBox.critical(self, "Effect Error", f"Could not create effect: {e}")
            return
            
        if effect_obj:
            wrapper = EffectWrapper(effect_obj, self.combo_fx.currentText(), fx_type, is_active=True)
            self.selected_track.effects.append(wrapper)
            self.selected_track.update_pedalboard(self.audio_engine.sample_rate if self.audio_engine else 44100)
            
            # Reset combo box
            self.combo_fx.setCurrentIndex(0)
            self.refresh_rack()
            
    def on_load_vst3(self):
        if not self.selected_track:
            return
            
        file_filter = "VST3 Plugins (*.vst3)"
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            "Select VST3 Plugin File",
            "C:\\Program Files\\Common Files\\VST3",
            file_filter
        )
        
        if file_path:
            self.load_scanned_vst3(file_path)

    def populate_vst_menu(self):
        self.vst_menu.clear()
        self.vst_menu.setStyleSheet("""
            QMenu {
                background-color: #000000;
                color: #88888c;
                border: 1px solid #222225;
                font-family: "Consolas", "Courier New", monospace;
                font-size: 11px;
            }
            QMenu::item {
                padding: 6px 20px;
                background-color: transparent;
            }
            QMenu::item:selected {
                background-color: #ffffff;
                color: #000000;
            }
            QMenu::separator {
                height: 1px;
                background-color: #222225;
                margin: 4px 0px;
            }
        """)
        
        search_paths = getattr(self.audio_engine, "vst_search_paths", ["C:\\Program Files\\Common Files\\VST3"])
        scanned_vsts = scan_for_vst3s(search_paths)
        
        if scanned_vsts:
            for name, path in scanned_vsts:
                action = self.vst_menu.addAction(name)
                # Capture the path in lambda
                action.triggered.connect(lambda checked=False, p=path: self.load_scanned_vst3(p))
            self.vst_menu.addSeparator()
            
        action_browse = self.vst_menu.addAction("Browse for VST3 file...")
        action_browse.triggered.connect(self.on_load_vst3)

    def load_scanned_vst3(self, file_path):
        if not self.selected_track:
            return
            
        was_running = False
        if self.audio_engine:
            was_running = self.audio_engine.is_running
            self.audio_engine.stop_stream()
            
        try:
            # Load plugin using safe isolation loader
            from audio_engine import load_vst_plugin
            vst_obj = load_vst_plugin(file_path)
            filename = os.path.splitext(os.path.basename(file_path))[0]
            
            # Wrap
            wrapper = EffectWrapper(vst_obj, f"VST: {filename}", "VST3", is_active=True)
            wrapper.original_vst_path = file_path
            self.selected_track.effects.append(wrapper)
            self.selected_track.update_pedalboard(self.audio_engine.sample_rate if self.audio_engine else 44100)
            
            self.refresh_rack()
        except Exception as e:
            QMessageBox.critical(
                self,
                "VST3 Error",
                f"Failed to load VST3 plugin at {file_path}.\n\nError details: {e}"
            )
        finally:
            if was_running and self.audio_engine:
                self.audio_engine.start_stream()
