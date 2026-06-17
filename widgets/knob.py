import math
from PySide6.QtWidgets import QWidget
from PySide6.QtCore import Qt, QPoint, QPointF, Signal, QRectF
from PySide6.QtGui import QPainter, QColor, QPen, QBrush, QRadialGradient, QFont, QKeySequence

class CustomKnob(QWidget):
    """Custom metallic amp-style knob with flat graphite styling."""
    valueChanged = Signal(float)
    
    def __init__(self, label="", min_val=0.0, max_val=1.0, default_val=0.0, unit="", decimals=1, parent=None):
        super().__init__(parent)
        self.label = label
        self.min_val = min_val
        self.max_val = max_val
        self.value = default_val
        self.default_val = default_val
        self.unit = unit
        self.decimals = decimals
        
        self.setMinimumSize(65, 95)
        self.last_mouse_pos = QPoint()
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)
        self.setMouseTracking(True)
        
        self.is_hovered = False
        self.is_dragging = False
        
        # Professional graphite studio themes
        self.studio_accent = QColor("#888888")  # Professional graphite gray
        self.studio_hover = QColor("#aaaaaa")   # Highlighted gray
        self.dark_bg = QColor("#1e1e1e")
        self.knob_dark = QColor("#2d2d2d")
        self.knob_light = QColor("#505050")
        self.knob_base = QColor("#333333")
        
    def setValue(self, val):
        """Sets the knob value, clamped to limits."""
        val = max(self.min_val, min(self.max_val, val))
        if not math.isclose(self.value, val, abs_tol=1e-5):
            self.value = val
            self.valueChanged.emit(self.value)
            self.update()
            
    def getValue(self):
        return self.value
        
    def enterEvent(self, event):
        self.is_hovered = True
        self.update()
        super().enterEvent(event)
        
    def leaveEvent(self, event):
        self.is_hovered = False
        self.update()
        super().leaveEvent(event)
        
    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.last_mouse_pos = event.pos()
            self.is_dragging = True
            self.setCursor(Qt.CursorShape.BlankCursor)  # Hide cursor during drag
            self.update()
            
    def mouseReleaseEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.is_dragging = False
            self.setCursor(Qt.CursorShape.ArrowCursor)  # Restore cursor
            self.update()
            
    def mouseMoveEvent(self, event):
        if self.is_dragging:
            delta_y = self.last_mouse_pos.y() - event.pos().y()
            self.last_mouse_pos = event.pos()
            
            # Sensitivity modification
            val_range = self.max_val - self.min_val
            step = val_range / 150.0  # 150px vertical drag for full sweep
            
            # Holding Shift makes adjustment much finer (10x sensitivity)
            if event.modifiers() & Qt.KeyboardModifier.ShiftModifier:
                step /= 10.0
                
            self.setValue(self.value + delta_y * step)
            
    def mouseDoubleClickEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.setValue(self.default_val)
            
    def wheelEvent(self, event):
        # Handle scroll wheel events for value adjustment
        angle = event.angleDelta().y()
        val_range = self.max_val - self.min_val
        step = val_range / 40.0  # 40 ticks for full sweep
        
        if event.modifiers() & Qt.KeyboardModifier.ShiftModifier:
            step /= 10.0
            
        if angle > 0:
            self.setValue(self.value + step)
        else:
            self.setValue(self.value - step)
            
    def keyPressEvent(self, event):
        # Support keyboard arrow keys
        val_range = self.max_val - self.min_val
        step = val_range / 100.0  # 100 steps
        
        if event.modifiers() & Qt.KeyboardModifier.ShiftModifier:
            step /= 10.0
            
        if event.key() in (Qt.Key.Key_Up, Qt.Key.Key_Right):
            self.setValue(self.value + step)
        elif event.key() in (Qt.Key.Key_Down, Qt.Key.Key_Left):
            self.setValue(self.value - step)
        else:
            super().keyPressEvent(event)
            
    def paintEvent(self, event):
        painter = QPainter(self)
        try:
            painter.setRenderHint(QPainter.RenderHint.Antialiasing)
            
            w = self.width()
            h = self.height()
            
            # Calculate knob size and boundaries
            knob_size = min(w, h - 35)
            rect = QRectF((w - knob_size) / 2, 5, knob_size, knob_size)
            
            center_x = rect.center().x()
            center_y = rect.center().y()
            radius = knob_size / 2.0
            
            # Draw physical tick markers around the knob face
            painter.setPen(QPen(QColor("#444448"), 1))
            for angle in range(-45, 226, 45):
                angle_rad = math.radians(225 - angle)
                tx1 = center_x + (radius + 2) * math.cos(angle_rad)
                ty1 = center_y - (radius + 2) * math.sin(angle_rad)
                tx2 = center_x + (radius + 4) * math.cos(angle_rad)
                ty2 = center_y - (radius + 4) * math.sin(angle_rad)
                painter.drawLine(QPointF(tx1, ty1), QPointF(tx2, ty2))
                
            # Draw background track arc (charcoal gray)
            pen_bg = QPen(QColor("#2c2c2f"), 2.0)
            pen_bg.setCapStyle(Qt.PenCapStyle.RoundCap)
            painter.setPen(pen_bg)
            painter.drawArc(rect.adjusted(2, 2, -2, -2), int(225 * 16), int(-270 * 16))
            
            # Calculate normalized value [0, 1]
            norm_val = 0.0
            if self.max_val > self.min_val:
                norm_val = (self.value - self.min_val) / (self.max_val - self.min_val)
                
            # Draw active value track (silver/white)
            active_color = self.studio_hover if self.is_dragging else self.studio_accent
            pen_active = QPen(active_color, 2.0)
            pen_active.setCapStyle(Qt.PenCapStyle.RoundCap)
            painter.setPen(pen_active)
            painter.drawArc(rect.adjusted(2, 2, -2, -2), int(225 * 16), int(-270 * 16 * norm_val))
            
            # Draw knob core (metallic dark dial)
            knob_inner_rect = rect.adjusted(4, 4, -4, -4)
            knob_radius = knob_inner_rect.width() / 2.0
            
            # Radial gradient for professional metal appearance
            gradient = QRadialGradient(knob_inner_rect.center(), knob_radius)
            gradient.setColorAt(0.0, self.knob_light)
            gradient.setColorAt(0.7, self.knob_dark)
            gradient.setColorAt(1.0, QColor("#121214"))
            
            painter.setBrush(QBrush(gradient))
            if self.is_hovered or self.hasFocus():
                pen_outline = QPen(self.studio_accent, 1.0)
            else:
                pen_outline = QPen(QColor("#2d2d30"), 0.8)
            painter.setPen(pen_outline)
            painter.drawEllipse(knob_inner_rect)
            
            # Calculate indicator line coordinates
            theta = 225.0 - 270.0 * norm_val
            theta_rad = math.radians(theta)
            
            # Draw visual indicator line on the knob face (light silver/white)
            pointer_r_start = knob_radius * 0.1
            pointer_r_end = knob_radius * 0.9
            
            px1 = center_x + pointer_r_start * math.cos(theta_rad)
            py1 = center_y - pointer_r_start * math.sin(theta_rad)
            px2 = center_x + pointer_r_end * math.cos(theta_rad)
            py2 = center_y - pointer_r_end * math.sin(theta_rad)
            
            indicator_color = QColor("#ffffff") if (self.is_hovered or self.is_dragging) else QColor("#b3b3b3")
            pen_indicator = QPen(indicator_color, 1.5)
            pen_indicator.setCapStyle(Qt.PenCapStyle.RoundCap)
            painter.setPen(pen_indicator)
            painter.drawLine(QPoint(int(px1), int(py1)), QPoint(int(px2), int(py2)))
            
            # Draw Labels
            font_label = QFont("Segoe UI", 8, QFont.Weight.Bold)
            painter.setFont(font_label)
            painter.setPen(QColor("#88888c"))
            
            # Center parameter name (e.g. "DRIVE", "PAN")
            label_y = h - 28
            painter.drawText(0, label_y, w, 13, Qt.AlignmentFlag.AlignCenter, self.label)
            
            # Format the decimal value
            val_str = self.get_value_str()
                
            font_val = QFont("Consolas", 8)
            painter.setFont(font_val)
            painter.setPen(QColor("#ffffff") if self.is_hovered or self.is_dragging else QColor("#88888c"))
            painter.drawText(0, h - 14, w, 13, Qt.AlignmentFlag.AlignCenter, val_str)
        finally:
            painter.end()

    def get_value_str(self) -> str:
        """Override in subclasses to customise the value display label."""
        val_str = f"{self.value:.{self.decimals}f}"
        if self.unit:
            val_str += f" {self.unit}"
        return val_str

    def sizeHint(self):
        from PySide6.QtCore import QSize
        return QSize(80, 100)
