#include "Timeline.h"
#include "../audio/AudioEngine.h"
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QScrollBar>

namespace gui {

// ============================================================================
// TimeRulerWidget
// ============================================================================

TimeRulerWidget::TimeRulerWidget(dsp::AudioEngine* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setFixedHeight(30);
    setMinimumWidth(5000); // Allow scrolling
}

void TimeRulerWidget::setScrollOffset(int offset)
{
    m_scrollOffset = offset;
    update();
}

void TimeRulerWidget::setZoom(double pixelsPerSecond)
{
    m_pixelsPerSecond = pixelsPerSecond;
    update();
}

void TimeRulerWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int w = width();
    int h = height();
    
    // Background
    painter.fillRect(0, 0, w, h, QColor("#0b0b0c"));
    
    // Bottom border
    painter.setPen(QPen(QColor("#222225"), 1));
    painter.drawLine(0, h - 1, w, h - 1);
    
    // Translate painter horizontally for scrolling
    painter.translate(-m_scrollOffset, 0);
    
    // Draw Ticks
    int visibleWidth = w + m_scrollOffset;
    int maxSeconds = (visibleWidth / m_pixelsPerSecond) + 5;
    
    int tickStep = 5; // Draw a label every 5 seconds
    double subStep = 1.0; // Draw a small tick every 1 second
    
    QFont font("Consolas", 8);
    painter.setFont(font);
    
    for (double s = 0.0; s <= maxSeconds; s += subStep) {
        int x = s * m_pixelsPerSecond;
        if (x < m_scrollOffset - 50) continue; // Skip off-screen to the left
        
        bool isMajor = (std::fmod(s, tickStep) == 0.0);
        
        if (isMajor) {
            painter.setPen(QPen(QColor("#88888c"), 1));
            painter.drawLine(x, h - 10, x, h);
            
            // Format time string MM:SS
            int mins = (int)s / 60;
            int secs = (int)s % 60;
            QString timeStr = QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
            
            painter.setPen(QPen(QColor("#ffffff"), 1));
            painter.drawText(x + 3, h - 12, timeStr);
        } else {
            painter.setPen(QPen(QColor("#444448"), 1));
            painter.drawLine(x, h - 5, x, h);
        }
    }
}


// ============================================================================
// TimelineLanesWidget
// ============================================================================

TimelineLanesWidget::TimelineLanesWidget(dsp::AudioEngine* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setMinimumSize(5000, 1000); // Allow scrolling
    
    // Timer to update playhead position at ~60fps
    connect(&m_playheadTimer, &QTimer::timeout, this, &TimelineLanesWidget::onPlayheadTimerTick);
    m_playheadTimer.start(16);
}

void TimelineLanesWidget::setZoom(double pixelsPerSecond)
{
    m_pixelsPerSecond = pixelsPerSecond;
    update();
}

void TimelineLanesWidget::onPlayheadTimerTick()
{
    if (m_engine && m_engine->isEnginePlaying()) {
        double currentPlayhead = m_engine->getPlayheadTime();
        if (currentPlayhead != m_lastPlayheadTime) {
            m_lastPlayheadTime = currentPlayhead;
            update(); // Repaint the playhead
        }
    }
}

void TimelineLanesWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int w = width();
    int h = height();
    
    // Background
    painter.fillRect(event->rect(), QColor("#111111"));
    
    // Draw Grid Lines (simplified)
    painter.setPen(QPen(QColor("#222225"), 1));
    int maxSeconds = w / m_pixelsPerSecond;
    for (int s = 0; s <= maxSeconds; s++) {
        int x = s * m_pixelsPerSecond;
        painter.drawLine(x, 0, x, h);
    }
    
    // Draw Track Lane separators (placeholder height = 100px)
    painter.setPen(QPen(QColor("#222225"), 1));
    for (int y = 100; y < h; y += 100) {
        painter.drawLine(0, y, w, y);
    }
    
    // Draw Placeholder Clips
    // Just a couple colored blocks to represent waveforms
    painter.fillRect(QRect(m_pixelsPerSecond * 2, 10, m_pixelsPerSecond * 5, 80), QColor(0, 150, 255, 100)); // Track 1
    painter.setPen(QPen(QColor(0, 150, 255), 1));
    painter.drawRect(QRect(m_pixelsPerSecond * 2, 10, m_pixelsPerSecond * 5, 80));
    
    painter.fillRect(QRect(m_pixelsPerSecond * 4, 110, m_pixelsPerSecond * 3, 80), QColor(255, 100, 0, 100)); // Track 2
    painter.setPen(QPen(QColor(255, 100, 0), 1));
    painter.drawRect(QRect(m_pixelsPerSecond * 4, 110, m_pixelsPerSecond * 3, 80));
    
    // Draw Playhead
    if (m_engine) {
        double playheadTime = m_engine->getPlayheadTime();
        int playheadX = playheadTime * m_pixelsPerSecond;
        
        painter.setPen(QPen(QColor("#ff0033"), 2));
        painter.drawLine(playheadX, 0, playheadX, h);
    }
}


// ============================================================================
// TimelineContainer
// ============================================================================

TimelineContainer::TimelineContainer(dsp::AudioEngine* engine, QWidget* parent)
    : QScrollArea(parent)
{
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    
    QWidget* container = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    m_ruler = new TimeRulerWidget(engine, container);
    layout->addWidget(m_ruler);
    
    m_lanes = new TimelineLanesWidget(engine, container);
    layout->addWidget(m_lanes);
    
    setWidget(container);
    
    // Sync the horizontal scrollbar with the Ruler widget so it renders the ticks correctly
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &TimelineContainer::onHorizontalScroll);
}

void TimelineContainer::onHorizontalScroll(int value)
{
    if (m_ruler) {
        m_ruler->setScrollOffset(value);
    }
}

} // namespace gui
