import os
from PySide6.QtWidgets import QWidget, QHBoxLayout, QLabel, QFrame, QApplication
from PySide6.QtCore import Qt, QMimeData, QPointF
from PySide6.QtGui import QDrag, QPainter, QColor, QPen, QBrush, QFont

class SignalFlowNode(QFrame):
    """A draggable visual node representing an effect in the signal flow chain."""
    def __init__(self, wrapper, effect_index, flow_widget, parent=None):
        super().__init__(parent)
        self.wrapper = wrapper
        self.effect_index = effect_index
        self.flow_widget = flow_widget
        self.drag_start_pos = None
        
        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.setObjectName("SignalFlowNode")
        self.setCursor(Qt.CursorShape.OpenHandCursor)
        
        self.setup_ui()
        
    def setup_ui(self):
        layout = QHBoxLayout(self)
        layout.setContentsMargins(10, 5, 10, 5)
        layout.setSpacing(0)
        
        # Format the display name (e.g., VST: Archetype -> Archetype, Noise Gate -> Gate)
        name = self.wrapper.name
        if name.startswith("VST: "):
            name = name[5:]
        elif name == "Noise Gate":
            name = "Gate"
        elif name == "Highpass Filter":
            name = "HPF"
        elif name == "LowpassFilter":
            name = "LPF"
            
        self.label = QLabel(name.upper())
        self.label.setFont(QFont("Consolas", 8, QFont.Weight.Bold))
        self.label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(self.label)
        
        # Apply CSS based on status (active/bypassed/selected)
        is_selected = (self.flow_widget.effects_rack.selected_effect_wrapper == self.wrapper)
        
        if not self.wrapper.is_active:
            # Bypassed
            border_style = "1px dashed #3a3a3d"
            text_color = "#55555c"
            bg_color = "#000000"
        elif is_selected:
            # Active and selected
            border_style = "1px solid #ff0033" # Nothing Red highlight border
            text_color = "#ffffff"
            bg_color = "#151518"
        else:
            # Active and not selected
            border_style = "1px solid #333333"
            text_color = "#ffffff"
            bg_color = "#0b0b0c"
            
        self.setStyleSheet(f"""
            SignalFlowNode {{
                background-color: {bg_color};
                border: {border_style};
                border-radius: 4px;
                min-height: 26px;
                max-height: 26px;
            }}
            QLabel {{
                color: {text_color};
                border: none;
                background: transparent;
            }}
        """)

    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.drag_start_pos = event.pos()
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        if not self.drag_start_pos:
            return
        if not (event.buttons() & Qt.MouseButton.LeftButton):
            return
        if (event.pos() - self.drag_start_pos).manhattanLength() < QApplication.startDragDistance():
            return
            
        drag = QDrag(self)
        mime_data = QMimeData()
        # Pass the source effect index as text
        mime_data.setText(str(self.effect_index))
        drag.setMimeData(mime_data)
        
        # Generate pixmap preview of the node
        pixmap = self.grab()
        drag.setPixmap(pixmap)
        drag.setHotSpot(event.pos())
        
        self.setCursor(Qt.CursorShape.ClosedHandCursor)
        drag.exec(Qt.DropAction.MoveAction)
        self.setCursor(Qt.CursorShape.OpenHandCursor)
        
    def mouseReleaseEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton and self.drag_start_pos:
            if (event.pos() - self.drag_start_pos).manhattanLength() < QApplication.startDragDistance():
                # Focus/select this card in the effects rack
                self.flow_widget.select_effect_in_rack(self.effect_index)
        self.drag_start_pos = None
        super().mouseReleaseEvent(event)


class SignalFlowWidget(QWidget):
    """Horizontal Node Graph representing the DAW track's signal chain."""
    def __init__(self, audio_engine, effects_rack, parent=None):
        super().__init__(parent)
        self.audio_engine = audio_engine
        self.effects_rack = effects_rack
        self.selected_track = None
        self.drag_over_index = None  # index where dragged node is hovered over
        
        self.setAcceptDrops(True)
        self.setMinimumHeight(45)
        self.setMaximumHeight(50)
        
        self.setup_ui()
        
    def setup_ui(self):
        self.main_layout = QHBoxLayout(self)
        self.main_layout.setContentsMargins(15, 5, 15, 5)
        self.main_layout.setSpacing(8)
        self.main_layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        self.refresh_flow()
        
    def set_track(self, track):
        self.selected_track = track
        self.refresh_flow()
        
    def refresh_flow(self):
        # Clear layout
        while self.main_layout.count() > 0:
            item = self.main_layout.takeAt(0)
            w = item.widget()
            if w:
                w.deleteLater()
                
        # Draw base styles
        self.setStyleSheet("""
            SignalFlowWidget {
                background-color: #000000;
                border: 1px solid #222225;
                border-radius: 4px;
            }
            QLabel#StaticNode {
                color: #55555c;
                font-family: "Consolas", monospace;
                font-size: 8px;
                font-weight: bold;
                border: 1px solid #222225;
                border-radius: 4px;
                padding: 4px 8px;
                background-color: #060607;
            }
            QLabel#ArrowLabel {
                color: #333335;
                font-family: "Consolas", monospace;
                font-size: 11px;
                font-weight: bold;
            }
            QLabel#EmptyFlowLabel {
                color: #444448;
                font-family: "Consolas", monospace;
                font-size: 9px;
                font-style: italic;
            }
        """)
        
        if not self.selected_track:
            lbl = QLabel("NO TRACK SELECTED")
            lbl.setObjectName("EmptyFlowLabel")
            self.main_layout.addWidget(lbl)
            return
            
        # 1. INPUT static endpoint
        lbl_in = QLabel("INPUT")
        lbl_in.setObjectName("StaticNode")
        self.main_layout.addWidget(lbl_in)
        
        effects = self.selected_track.effects
        if not effects:
            # Draw empty line
            arrow = QLabel("➔")
            arrow.setObjectName("ArrowLabel")
            self.main_layout.addWidget(arrow)
            
            lbl_empty = QLabel("BYPASS / EMPTY RACK")
            lbl_empty.setObjectName("EmptyFlowLabel")
            self.main_layout.addWidget(lbl_empty)
        else:
            # 2. Draw each effect node in sequence
            for idx, wrapper in enumerate(effects):
                arrow = QLabel("➔")
                arrow.setObjectName("ArrowLabel")
                self.main_layout.addWidget(arrow)
                
                node = SignalFlowNode(wrapper, idx, self)
                self.main_layout.addWidget(node)
                
        # 3. OUTPUT static endpoint
        arrow_out = QLabel("➔")
        arrow_out.setObjectName("ArrowLabel")
        self.main_layout.addWidget(arrow_out)
        
        lbl_out = QLabel("OUTPUT")
        lbl_out.setObjectName("StaticNode")
        self.main_layout.addWidget(lbl_out)
        
    def select_effect_in_rack(self, effect_index):
        """Helper to programmatically select the effect card inside EffectsRack."""
        if not self.selected_track or effect_index >= len(self.selected_track.effects):
            return
            
        wrapper = self.selected_track.effects[effect_index]
        
        # Find corresponding EffectCard widget in EffectsRack scroll layout
        target_card = None
        rack_layout = self.effects_rack.scroll_layout
        for i in range(rack_layout.count()):
            item = rack_layout.itemAt(i)
            if item:
                w = item.widget()
                from widgets.effects_rack import EffectCard
                if isinstance(w, EffectCard) and w.wrapper == wrapper:
                    target_card = w
                    break
                    
        if target_card:
            self.effects_rack.select_card(target_card)
            # Re-draw ourselves to update borders/selection highlight
            self.refresh_flow()

    def update_drag_indicator(self, drop_x):
        if not self.selected_track or not self.selected_track.effects:
            self.drag_over_index = None
            self.update()
            return
            
        # Collect only SignalFlowNode widgets
        pills = []
        for i in range(self.main_layout.count()):
            w = self.main_layout.itemAt(i).widget()
            if isinstance(w, SignalFlowNode):
                pills.append(w)
                
        if not pills:
            self.drag_over_index = None
            self.update()
            return
            
        N = len(pills)
        gaps = []
        # Gap 0 is left of pills[0]
        gaps.append(pills[0].geometry().left() - 4)
        # Gaps 1 to N-1
        for i in range(1, N):
            mid_x = (pills[i-1].geometry().right() + pills[i].geometry().left()) / 2
            gaps.append(mid_x)
        # Gap N is right of pills[-1]
        gaps.append(pills[-1].geometry().right() + 4)
        
        # Find closest gap to drop_x
        best_gap = 0
        min_dist = float('inf')
        for idx, gap_x in enumerate(gaps):
            dist = abs(drop_x - gap_x)
            if dist < min_dist:
                min_dist = dist
                best_gap = idx
                
        if self.drag_over_index != best_gap:
            self.drag_over_index = best_gap
            self.update()

    def dragEnterEvent(self, event):
        if event.mimeData().hasText():
            event.acceptProposedAction()
            self.update_drag_indicator(event.position().x())

    def dragMoveEvent(self, event):
        if event.mimeData().hasText():
            event.acceptProposedAction()
            self.update_drag_indicator(event.position().x())

    def dragLeaveEvent(self, event):
        self.drag_over_index = None
        self.update()

    def dropEvent(self, event):
        try:
            source_idx = int(event.mimeData().text())
            target_idx = self.drag_over_index
            self.drag_over_index = None
            self.update()
            
            if target_idx is not None and self.selected_track:
                effects = self.selected_track.effects
                if 0 <= source_idx < len(effects) and 0 <= target_idx <= len(effects):
                    # Perform python list move
                    fx = effects.pop(source_idx)
                    insert_idx = target_idx
                    if source_idx < target_idx:
                        insert_idx -= 1
                    effects.insert(insert_idx, fx)
                    
                    # Update DSP engine
                    self.selected_track.update_pedalboard(self.audio_engine.sample_rate)
                    
                    # Force popups coordinates update or sync active card if we moved the selected one
                    active_card = None
                    # Redraw Effects Rack
                    self.effects_rack.refresh_rack()
                    # Redraw ourselves
                    self.refresh_flow()
            event.acceptProposedAction()
        except Exception as e:
            print("Drop processing error:", e)

    def paintEvent(self, event):
        super().paintEvent(event)
        if self.drag_over_index is not None and self.selected_track and self.selected_track.effects:
            pills = []
            for i in range(self.main_layout.count()):
                w = self.main_layout.itemAt(i).widget()
                if isinstance(w, SignalFlowNode):
                    pills.append(w)
                    
            if pills:
                N = len(pills)
                if self.drag_over_index == 0:
                    x = pills[0].geometry().left() - 5
                elif self.drag_over_index == N:
                    x = pills[-1].geometry().right() + 5
                else:
                    idx = self.drag_over_index
                    x = (pills[idx-1].geometry().right() + pills[idx].geometry().left()) / 2
                    
                try:
                    painter = QPainter(self)
                    painter.setRenderHint(QPainter.RenderHint.Antialiasing)
                    
                    # Glowing red indicator line
                    pen = QPen(QColor("#ff0033"), 2)
                    painter.setPen(pen)
                    
                    h = self.height()
                    painter.drawLine(int(x), 8, int(x), h - 8)
                    
                    # Draw end circles for premium design feel
                    painter.setBrush(QBrush(QColor("#ff0033")))
                    painter.drawEllipse(QPointF(x, 8), 3, 3)
                    painter.drawEllipse(QPointF(x, h - 8), 3, 3)
                finally:
                    painter.end()
