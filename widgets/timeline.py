import os
import wave
import uuid
import numpy as np
from PySide6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QScrollArea, QFileDialog, QMessageBox, QFrame
)
from PySide6.QtCore import Qt, QRectF, QPointF, Signal, QPoint
from PySide6.QtGui import QPainter, QColor, QPen, QBrush, QFont
from audio_engine import AudioItem

class TimeRulerWidget(QWidget):
    """Time ruler widget displayed above the timeline track lanes."""
    timeClicked = Signal(float)  # Emitted with the selected time in seconds
    zoomChanged = Signal(float)  # Emitted when scrolling to zoom (multiplier)
    
    def __init__(self, audio_engine, parent=None):
        super().__init__(parent)
        self.audio_engine = audio_engine
        self.pixels_per_second = 50.0  # Zoom level
        self.scroll_offset = 0
        self.setMinimumHeight(30)
        self.setMaximumHeight(30)
        
        self.is_scrubbing = False
        
    def set_scroll_offset(self, offset):
        self.scroll_offset = offset
        self.update()
        
    def set_zoom(self, pixels_per_second):
        self.pixels_per_second = pixels_per_second
        self.update()
        
    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.is_scrubbing = True
            self.update_cursor_pos(event.position().x())
            
    def mouseMoveEvent(self, event):
        if self.is_scrubbing:
            self.update_cursor_pos(event.position().x())
            
    def mouseReleaseEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.is_scrubbing = False
            
    def wheelEvent(self, event):
        # Zoom timeline horizontally on wheel scroll (Reaper-style)
        delta = event.angleDelta().y()
        zoom_factor = 1.15 if delta > 0 else 0.85
        self.zoomChanged.emit(zoom_factor)
        
    def update_cursor_pos(self, x):
        abs_x = x + self.scroll_offset
        time_seconds = max(0.0, abs_x / self.pixels_per_second)
        self.timeClicked.emit(time_seconds)
        
    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        w = self.width()
        h = self.height()
        
        # Draw background
        painter.fillRect(0, 0, w, h, QColor("#252526"))
        painter.setPen(QPen(QColor("#333333"), 1))
        painter.drawLine(0, h - 1, w, h - 1)
        
        # Translate painter horizontally to scroll matching the timeline scroll
        painter.translate(-self.scroll_offset, 0)
        
        # Draw ticks and time strings
        total_width = w + self.scroll_offset
        max_seconds = int(total_width / self.pixels_per_second) + 5
        
        # Dynamic tick spacing based on zoom levels
        if self.pixels_per_second < 15.0:
            tick_step = 10  # Draw labels every 10 seconds
            sub_step = 2
        elif self.pixels_per_second < 50.0:
            tick_step = 5   # Every 5 seconds
            sub_step = 1
        elif self.pixels_per_second < 150.0:
            tick_step = 1   # Every second
            sub_step = 0.2
        else:
            tick_step = 0.5  # Every half-second
            sub_step = 0.1
            
        font = QFont("Consolas", 8)
        painter.setFont(font)
        
        # Paint grid markers
        seconds_span = np.arange(0, max_seconds, sub_step)
        for s in seconds_span:
            x_pos = int(s * self.pixels_per_second)
            is_major = abs(s % tick_step) < 1e-5
            
            if is_major:
                painter.setPen(QPen(QColor("#888888"), 1))
                painter.drawLine(x_pos, h - 12, x_pos, h - 2)
                
                # Format time text: M:SS or M:SS.hh
                minutes = int(s // 60)
                secs = s % 60
                if tick_step < 1.0:
                    time_str = f"{minutes}:{secs:05.2f}"
                else:
                    time_str = f"{minutes}:{int(secs):02d}"
                    
                painter.drawText(x_pos + 3, h - 14, time_str)
            else:
                painter.setPen(QPen(QColor("#444444"), 1))
                painter.drawLine(x_pos, h - 6, x_pos, h - 2)
                
        # Draw Playhead Cap (Red Triangle)
        sr = self.audio_engine.sample_rate if self.audio_engine else 44100
        playhead_sec = self.audio_engine.playhead_samples / sr
        playhead_x = int(playhead_sec * self.pixels_per_second)
        
        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(QBrush(QColor("#ff4444")))
        
        points = [
            QPoint(playhead_x - 6, 2),
            QPoint(playhead_x + 6, 2),
            QPoint(playhead_x, 12)
        ]
        painter.drawPolygon(points)


class TimelineLanesWidget(QWidget):
    """Draws track lanes, items, waveforms, and the moving playhead cursor."""
    trackSelected = Signal(int)  # Emitted with track index
    timeClicked = Signal(float)   # Emitted with position in seconds
    zoomChanged = Signal(float)   # Zoom multiplier
    
    def __init__(self, audio_engine, parent=None):
        super().__init__(parent)
        self.audio_engine = audio_engine
        self.pixels_per_second = 50.0
        self.lane_height = 150
        
        self.active_drag_item = None
        self.drag_offset_samples = 0
        self.drag_track = None
        
        self.selected_item = None
        self.selected_track_for_item = None
        
        self.setMouseTracking(True)
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)
        
    def set_zoom(self, pixels_per_second):
        self.pixels_per_second = pixels_per_second
        self.update_geometry()
        
    def update_geometry(self):
        # Calculate maximum track bounds
        tracks_count = len(self.audio_engine.tracks)
        h = max(300, tracks_count * self.lane_height)
        
        # Verify selected track is still valid
        if self.selected_track_for_item and self.selected_track_for_item not in self.audio_engine.tracks:
            self.selected_item = None
            self.selected_track_for_item = None
            
        # Calculate maximum time bound
        max_sec = 300.0  # Default 5 minutes
        for track in self.audio_engine.tracks:
            for item in track.items:
                item_end = (item.start_sample + (item.audio_data.shape[1] if item.audio_data is not None else 0)) / item.sample_rate
                max_sec = max(max_sec, item_end + 15.0)  # Pad with 15s
                
        w = int(max_sec * self.pixels_per_second)
        self.resize(w, h)
        self.update()
        
    def mousePressEvent(self, event):
        self.setFocus()  # Ensure widget gets focus so it can receive key events!
        x = event.position().x()
        y = event.position().y()
        
        track_idx = int(y // self.lane_height)
        if track_idx < len(self.audio_engine.tracks):
            self.trackSelected.emit(track_idx)
            track = self.audio_engine.tracks[track_idx]
            
            # Check if clicked on a track item
            sample_rate = self.audio_engine.sample_rate
            click_sample = int((x / self.pixels_per_second) * sample_rate)
            
            clicked_item = None
            with track.lock:
                for item in track.items:
                    if item.audio_data is None:
                        continue
                    start = item.start_sample
                    end = start + item.audio_data.shape[1]
                    if start <= click_sample <= end:
                        clicked_item = item
                        break
                        
            if clicked_item:
                self.selected_item = clicked_item
                self.selected_track_for_item = track
                # Start dragging item
                self.active_drag_item = clicked_item
                self.drag_track = track
                self.drag_offset_samples = click_sample - clicked_item.start_sample
                self.setCursor(Qt.CursorShape.ClosedHandCursor)
            else:
                self.selected_item = None
                self.selected_track_for_item = None
                # Set playhead position on empty lane
                time_seconds = max(0.0, x / self.pixels_per_second)
                self.timeClicked.emit(time_seconds)
                self.active_drag_item = None
            self.update()
        else:
            self.selected_item = None
            self.selected_track_for_item = None
            self.active_drag_item = None
            self.update()
            
    def mouseMoveEvent(self, event):
        if self.active_drag_item:
            # Handle item move
            x = event.position().x()
            sample_rate = self.audio_engine.sample_rate
            new_start = int((x / self.pixels_per_second) * sample_rate) - self.drag_offset_samples
            new_start = max(0, new_start)
            
            with self.drag_track.lock:
                self.active_drag_item.start_sample = new_start
                
            self.update_geometry()
            
    def mouseReleaseEvent(self, event):
        if self.active_drag_item:
            self.active_drag_item = None
            self.drag_track = None
            self.setCursor(Qt.CursorShape.ArrowCursor)
            
    def mouseDoubleClickEvent(self, event):
        # Double-click empty track lane space to import WAV
        y = event.position().y()
        track_idx = int(y // self.lane_height)
        if track_idx < len(self.audio_engine.tracks):
            track = self.audio_engine.tracks[track_idx]
            
            # Select file dialog
            file_path, _ = QFileDialog.getOpenFileName(
                self,
                "Import Audio File (WAV)",
                "",
                "Audio Files (*.wav)"
            )
            if file_path:
                x = event.position().x()
                sample_rate = self.audio_engine.sample_rate
                start_sample = int((x / self.pixels_per_second) * sample_rate)
                
                # Create and add item
                item = AudioItem(start_sample, sample_rate, file_path=file_path)
                if item.audio_data is not None:
                    # Copy WAV next to project later, or keep path
                    with track.lock:
                        track.items.append(item)
                    track.update_pedalboard(self.audio_engine.sample_rate)
                    self.update_geometry()
                    
    def keyPressEvent(self, event):
        if event.key() == Qt.Key.Key_Delete or event.key() == Qt.Key.Key_Backspace:
            if self.selected_item and self.selected_track_for_item:
                track = self.selected_track_for_item
                item = self.selected_item
                
                with track.lock:
                    if item in track.items:
                        track.items.remove(item)
                
                self.selected_item = None
                self.selected_track_for_item = None
                self.update_geometry()
        else:
            super().keyPressEvent(event)
            
    def wheelEvent(self, event):
        delta = event.angleDelta().y()
        zoom_factor = 1.15 if delta > 0 else 0.85
        self.zoomChanged.emit(zoom_factor)
        
    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        w = self.width()
        h = self.height()
        
        # 1. Alternate Track Lane backgrounds
        tracks = self.audio_engine.tracks
        for idx in range(len(tracks)):
            y_pos = idx * self.lane_height
            bg_color = QColor("#202020") if idx % 2 == 0 else QColor("#242424")
            painter.fillRect(0, y_pos, w, self.lane_height, bg_color)
            
            # Draw lane bottom border
            painter.setPen(QPen(QColor("#333333"), 1))
            painter.drawLine(0, y_pos + self.lane_height - 1, w, y_pos + self.lane_height - 1)
            
        # 2. Draw Items & Waveforms
        for t_idx, track in enumerate(tracks):
            y_center = t_idx * self.lane_height + int(self.lane_height / 2)
            y_top = t_idx * self.lane_height + 5
            draw_h = self.lane_height - 10
            
            with track.lock:
                items_copy = list(track.items)
                
            for item in items_copy:
                if item.audio_data is None:
                    continue
                    
                # Calculate coordinates
                item_len = item.audio_data.shape[1]
                start_x = int((item.start_sample / item.sample_rate) * self.pixels_per_second)
                end_x = int(((item.start_sample + item_len) / item.sample_rate) * self.pixels_per_second)
                item_width = max(2, end_x - start_x)
                
                # Draw clip block background
                clip_rect = QRectF(start_x, y_top, item_width, draw_h)
                if item == self.selected_item:
                    painter.setBrush(QBrush(QColor("#404854")))
                    painter.setPen(QPen(QColor("#ffffff"), 2.0))
                else:
                    painter.setBrush(QBrush(QColor("#333a42")))
                    painter.setPen(QPen(QColor("#556070"), 1.2))
                painter.drawRoundedRect(clip_rect, 4, 4)
                
                # Draw WAV Filename text label
                font = QFont("Segoe UI", 7, QFont.Weight.Bold)
                painter.setFont(font)
                painter.setPen(QColor("#a0b0c0"))
                name_str = os.path.basename(item.file_path) if item.file_path else "Recorded Clip"
                # Clip text to container width
                metrics = painter.fontMetrics()
                elided_str = metrics.elidedText(name_str, Qt.TextElideMode.ElideRight, int(item_width - 10))
                painter.drawText(start_x + 5, y_top + 12, elided_str)
                
                # Draw Waveform Outline
                painter.setPen(QPen(QColor("#c0d0e0"), 1.0))
                
                # Sub-sample waveform for speed
                half_h = int(draw_h / 2.5)
                # Compute min-max for each pixel column
                # Map audio_data channel 0
                ch_data = item.audio_data[0]
                samples_per_pixel = int(item.sample_rate / self.pixels_per_second)
                if samples_per_pixel < 1:
                    samples_per_pixel = 1
                    
                for px in range(item_width):
                    px_start = int(px * samples_per_pixel)
                    px_end = int(px_start + samples_per_pixel)
                    
                    if px_start >= len(ch_data):
                        break
                    chunk = ch_data[px_start:min(px_end, len(ch_data))]
                    if len(chunk) == 0:
                        continue
                        
                    ch_min = np.min(chunk)
                    ch_max = np.max(chunk)
                    
                    line_x = start_x + px
                    line_y_top = y_center + int(ch_min * half_h)
                    line_y_bottom = y_center + int(ch_max * half_h)
                    if line_y_top == line_y_bottom:
                        line_y_bottom += 1
                    painter.drawLine(line_x, line_y_top, line_x, line_y_bottom)
                    
        # 3. Draw Vertical Playhead Cursor Line
        sr = self.audio_engine.sample_rate if self.audio_engine else 44100
        playhead_sec = self.audio_engine.playhead_samples / sr
        playhead_x = int(playhead_sec * self.pixels_per_second)
        
        painter.setPen(QPen(QColor("#ff4444"), 1.2))
        painter.drawLine(playhead_x, 0, playhead_x, h)


class TimelineScrollContainer(QWidget):
    """Integrates the ruler, lanes scrollarea, and coordinates scroll updates."""
    def __init__(self, audio_engine, parent=None):
        super().__init__(parent)
        self.audio_engine = audio_engine
        self.pixels_per_second = 60.0
        
        self.setup_ui()
        
    def setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        
        # 1. Ruler at the top
        self.ruler = TimeRulerWidget(self.audio_engine, self)
        self.ruler.set_zoom(self.pixels_per_second)
        layout.addWidget(self.ruler)
        
        # 2. Scroll area containing the track lanes
        self.scroll_area = QScrollArea(self)
        self.scroll_area.setObjectName("TimelineScrollArea")
        self.scroll_area.setWidgetResizable(False)  # Let Lanes Widget set its size
        self.scroll_area.setFrameShape(QFrame.Shape.NoFrame)
        self.scroll_area.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOn)
        self.scroll_area.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        
        self.lanes = TimelineLanesWidget(self.audio_engine, self.scroll_area)
        self.lanes.set_zoom(self.pixels_per_second)
        self.scroll_area.setWidget(self.lanes)
        layout.addWidget(self.scroll_area)
        
        # 3. Signals connections
        self.scroll_area.horizontalScrollBar().valueChanged.connect(self.ruler.set_scroll_offset)
        
        self.ruler.timeClicked.connect(self.set_playhead_pos)
        self.ruler.zoomChanged.connect(self.zoom_horizontal)
        
        self.lanes.timeClicked.connect(self.set_playhead_pos)
        self.lanes.zoomChanged.connect(self.zoom_horizontal)
        
        self.lanes.update_geometry()
        
        self.setStyleSheet("""
            QScrollArea#TimelineScrollArea {
                background-color: #202020;
            }
        """)
        
    def set_playhead_pos(self, time_seconds):
        sr = self.audio_engine.sample_rate if self.audio_engine else 44100
        self.audio_engine.playhead_samples = int(time_seconds * sr)
        self.ruler.update()
        self.lanes.update()
        
    def zoom_horizontal(self, zoom_factor):
        new_zoom = self.pixels_per_second * zoom_factor
        new_zoom = max(5.0, min(new_zoom, 1200.0))  # Clamp zoom
        self.pixels_per_second = new_zoom
        
        # Propagate zoom to widgets
        self.ruler.set_zoom(new_zoom)
        self.lanes.set_zoom(new_zoom)
        self.lanes.update_geometry()
        
    def update_widgets(self):
        # Force redraw playhead and lanes waveforms
        self.lanes.update()
        self.ruler.update()
        
    def update_track_layout(self):
        # Re-initialize timeline geometry when tracks are added/removed
        self.lanes.update_geometry()
