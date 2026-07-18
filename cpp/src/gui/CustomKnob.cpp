#include "CustomKnob.h"
#include <QPainter>
#include <QPainterPath>
#include <cmath>

namespace gui {

CustomKnob::CustomKnob(QWidget* parent)
    : QDial(parent)
{
    setMouseTracking(true);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void CustomKnob::enterEvent(QEnterEvent* event)
{
    m_isHovered = true;
    update();
    QDial::enterEvent(event);
}
#else
void CustomKnob::enterEvent(QEvent* event)
{
    m_isHovered = true;
    update();
    QDial::enterEvent(event);
}
#endif

void CustomKnob::leaveEvent(QEvent* event)
{
    m_isHovered = false;
    update();
    QDial::leaveEvent(event);
}

void CustomKnob::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int size = std::min(w, h);
    QRect rect((w - size) / 2, (h - size) / 2, size, size);
    
    // Geometry
    double radius = size / 2.0;
    QPointF center = rect.center();
    
    // Normalized value 0.0 to 1.0
    double valRange = std::max(1, maximum() - minimum());
    double normVal = (value() - minimum()) / valRange;
    
    // Angle math (typically 225 deg to -45 deg)
    double startAngle = 225.0;
    double endAngle = -45.0;
    double angleRange = startAngle - endAngle; // 270 degrees
    double currentAngle = startAngle - (normVal * angleRange);
    
    // Draw background track (flat dark grey)
    painter.setPen(QPen(QColor("#222225"), 3, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(rect.adjusted(3, 3, -3, -3), startAngle * 16, -angleRange * 16);
    
    // Draw active track (white or red if hovered)
    QColor accentColor = m_isHovered || isSliderDown() ? QColor("#ff0033") : QColor("#ffffff");
    painter.setPen(QPen(accentColor, 3, Qt::SolidLine, Qt::RoundCap));
    
    // Draw from center for pan knobs
    double centerAngle = 90.0;
    double sweepAngle = currentAngle - centerAngle;
    painter.drawArc(rect.adjusted(3, 3, -3, -3), centerAngle * 16, sweepAngle * 16);
    
    // Draw knob base (black)
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#000000"));
    painter.drawEllipse(rect.adjusted(6, 6, -6, -6));
    
    // Outline of knob base
    painter.setPen(QPen(QColor("#333333"), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(rect.adjusted(6, 6, -6, -6));
    
    // Draw Indicator dot/line
    double theta = currentAngle * M_PI / 180.0;
    double indicatorRadius = radius - 8;
    QPointF indicatorPos(center.x() + indicatorRadius * std::cos(theta),
                         center.y() - indicatorRadius * std::sin(theta));
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(accentColor);
    painter.drawEllipse(indicatorPos, 2.5, 2.5);
}

} // namespace gui
