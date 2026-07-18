#include "LevelMeter.h"
#include <QPainter>
#include <QPainterPath>
#include <algorithm>

namespace gui {

LevelMeter::LevelMeter(Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent), m_orientation(orientation)
{
    m_decayTimer = new QTimer(this);
    connect(m_decayTimer, &QTimer::timeout, this, &LevelMeter::decay);
    m_decayTimer->start(33); // 30fps decay
}

void LevelMeter::setLevel(float peak)
{
    // Attack and release smoothing for the visual bar to reduce noise/jitter
    if (peak > m_currentPeak) {
        m_currentPeak += 0.6f * (peak - m_currentPeak); // fast attack
    } else {
        m_currentPeak += 0.2f * (peak - m_currentPeak); // smooth release
    }
    
    if (peak >= m_peakHold) {
        m_peakHold = peak;
    }
    
    // threshold for clipping: 1.0 (0dBFS)
    if (peak >= 1.0f) {
        m_clipActive = true;
        m_clipFrames = 30; // hold clip light for ~1 second
    }
    
    update();
}

void LevelMeter::decay()
{
    if (m_peakHold > 0.0f) {
        m_peakHold -= 0.02f; // simple linear decay
        if (m_peakHold < 0.0f) m_peakHold = 0.0f;
    }
    
    if (m_clipFrames > 0) {
        m_clipFrames--;
        if (m_clipFrames <= 0) {
            m_clipActive = false;
        }
    }
    
    update();
}

void LevelMeter::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    
    int w = width();
    int h = height();
    
    // Draw background
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#111111"));
    painter.drawRect(0, 0, w, h);
    
    // Draw border
    painter.setPen(QPen(QColor("#222225"), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(0, 0, w - 1, h - 1);
    
    // Normalize current peak for display (e.g. logarithmic mapping can be done here, 
    // but for simplicity we'll assume engine gives us a linear-ish peak [0, 1] for visual,
    // or we clamp it). The engine gives us a float peak value (0.0 to 1.0+).
    float displayPeak = std::min(1.0f, std::max(0.0f, m_currentPeak));
    
    if (m_orientation == Qt::Horizontal) {
        int meterWidth = w - 2;
        int activeWidth = static_cast<int>(meterWidth * displayPeak);
        
        // Draw active white bar
        if (activeWidth > 0) {
            painter.fillRect(1, 1, activeWidth, h - 2, QColor("#ffffff"));
        }
        
        // Draw clip block at the very end
        if (m_clipActive) {
            painter.fillRect(w - 4, 1, 3, h - 2, QColor("#ff0033"));
        }
    } else {
        int meterHeight = h - 2;
        int activeHeight = static_cast<int>(meterHeight * displayPeak);
        
        // Draw active white bar (from bottom up)
        if (activeHeight > 0) {
            painter.fillRect(1, h - 1 - activeHeight, w - 2, activeHeight, QColor("#ffffff"));
        }
        
        // Draw clip block at the very top
        if (m_clipActive) {
            painter.fillRect(1, 1, w - 2, 3, QColor("#ff0033"));
        }
    }
}

} // namespace gui
