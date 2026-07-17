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
            
            // Emit scroll request so container can follow
            int playheadX = currentPlayhead * m_pixelsPerSecond;
            emit requestScroll(playheadX);
            
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
    
    // Draw Track Lanes and Clips dynamically
    if (m_engine) {
        auto tracks = m_engine->getTracksSnapshot();
        
        // Dynamically set height if tracks exist
        int expectedHeight = tracks.size() * 104; // 100px TrackCard + 4px margin
        if (expectedHeight > 0 && expectedHeight != minimumHeight()) {
            setMinimumHeight(expectedHeight);
        }
        
        painter.setPen(QPen(QColor("#222225"), 1));
        
        int yOffset = 0;
        int trackHeight = 100;
        int trackMargin = 4;
        
        // Base colors for tracks to make them distinct
        QColor trackColors[] = {
            QColor(0, 150, 255),    // Blue
            QColor(255, 100, 0),    // Orange
            QColor(0, 200, 100),    // Green
            QColor(200, 0, 200),    // Purple
            QColor(255, 200, 0)     // Yellow
        };
        
        for (size_t i = 0; i < tracks.size(); ++i) {
            const auto& track = tracks[i];
            
            // Draw lane separator
            painter.drawLine(0, yOffset + trackHeight + trackMargin/2, w, yOffset + trackHeight + trackMargin/2);
            
            // Pick a color for this track
            QColor baseColor = trackColors[i % 5];
            QColor fillColor = baseColor;
            fillColor.setAlpha(100);
            
            // Draw all clips in this track
            for (const auto& item : track.items) {
                int startX = item.startTimeSecs * m_pixelsPerSecond;
                int itemW = item.durationSecs * m_pixelsPerSecond;
                
                QRect clipRect(startX, yOffset + 10, itemW, trackHeight - 20);
                
                // Fill
                painter.fillRect(clipRect, fillColor);
                
                // Border
                painter.setPen(QPen(baseColor, 1));
                painter.drawRect(clipRect);
                
                // Draw name or ID placeholder
                painter.setPen(QPen(QColor("#ffffff"), 1));
                painter.drawText(clipRect.adjusted(5, 5, -5, -5), Qt::AlignLeft | Qt::AlignTop, QString("Item %1").arg(item.id));
                
                // Reset pen for grid/separators
                painter.setPen(QPen(QColor("#222225"), 1));
            }
            
            yOffset += (trackHeight + trackMargin);
        }
    }
    
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
    
    connect(m_lanes, &TimelineLanesWidget::requestScroll, this, &TimelineContainer::onScrollRequested);
}

void TimelineContainer::onHorizontalScroll(int value)
{
    if (m_ruler) {
        m_ruler->setScrollOffset(value);
    }
}

void TimelineContainer::onScrollRequested(int playheadX)
{
    QScrollBar* hBar = horizontalScrollBar();
    if (!hBar) return;
    
    int viewWidth = viewport()->width();
    int currentScroll = hBar->value();
    
    // Auto-scroll (page style) if playhead goes past the right edge
    if (playheadX > currentScroll + viewWidth - 50) {
        hBar->setValue(playheadX - 50);
    }
    // Auto-scroll if user scrolled past the playhead on the left
    else if (playheadX < currentScroll) {
        hBar->setValue(playheadX - 50);
    }
}

} // namespace gui
