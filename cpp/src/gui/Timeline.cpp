#include "Timeline.h"
#include "../audio/AudioEngine.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QMenu>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

namespace gui {

// ============================================================================
// TimeRulerWidget
// ============================================================================

TimeRulerWidget::TimeRulerWidget(dsp::AudioEngine* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setFixedHeight(30);
    setMinimumWidth(3600 * 50); // Default to 1 hour at 50px/sec
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
    
    // Draw loop region
    if (m_engine && m_engine->getLoopEnd() > m_engine->getLoopStart()) {
        int loopStartX = m_engine->getLoopStart() * m_pixelsPerSecond;
        int loopEndX = m_engine->getLoopEnd() * m_pixelsPerSecond;
        
        painter.fillRect(loopStartX, 0, loopEndX - loopStartX, h, QColor(51, 119, 255, 60));
        
        painter.setPen(QPen(QColor("#3377ff"), 1));
        painter.drawLine(loopStartX, 0, loopStartX, h);
        painter.drawLine(loopEndX, 0, loopEndX, h);
        
        QPolygon startPoly;
        startPoly << QPoint(loopStartX, 0) << QPoint(loopStartX + 6, 0) << QPoint(loopStartX, 6);
        painter.setBrush(QColor("#3377ff"));
        painter.drawPolygon(startPoly);
        
        QPolygon endPoly;
        endPoly << QPoint(loopEndX, 0) << QPoint(loopEndX - 6, 0) << QPoint(loopEndX, 6);
        painter.drawPolygon(endPoly);
    }
    
    // Draw Ticks (Optimized to exposed rect)
    QRect exposed = event->rect();
    int startX = exposed.left();
    int endX = exposed.right();
    
    double startS = std::max(0.0, (double)startX / m_pixelsPerSecond);
    double endS = (double)endX / m_pixelsPerSecond;
    
    int tickStep = 5; // Draw a label every 5 seconds
    double subStep = 1.0; // Draw a small tick every 1 second
    
    QFont font("Consolas", 8);
    painter.setFont(font);
    
    for (double s = startS; s <= endS + 1.0; s += subStep) {
        int x = s * m_pixelsPerSecond;
        if (x < startX - 50) continue; // Skip off-screen to the left
        
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

void TimeRulerWidget::setPlayheadFromMouse(QMouseEvent* event)
{
    if (!m_engine) return;
    
    // Calculate time from x position directly (event->x() is already in local scrolled coordinates)
    int absoluteX = event->position().x();
    double timeSecs = std::max(0.0, (double)absoluteX / m_pixelsPerSecond);
    
    m_engine->setPlayheadPosition(timeSecs);
}

void TimeRulerWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        int absoluteX = event->position().x();
        double timeSecs = std::max(0.0, (double)absoluteX / m_pixelsPerSecond);
        m_selectionAnchorTime = timeSecs;
        m_isSelectingTime = true;
        
        if (m_engine) {
            // Do not clear the selection, just set the playhead
            m_engine->setPlayheadPosition(timeSecs);
        }
        update();
    }
}

void TimeRulerWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isSelectingTime && (event->buttons() & Qt::LeftButton)) {
        int absoluteX = event->position().x();
        double timeSecs = std::max(0.0, (double)absoluteX / m_pixelsPerSecond);
        
        double start = std::min(m_selectionAnchorTime, timeSecs);
        double end = std::max(m_selectionAnchorTime, timeSecs);
        
        if (m_engine) {
            m_engine->setLoopRegion(start, end);
            m_engine->setPlayheadPosition(timeSecs);
            if (end > start + 0.01) {
                m_engine->setLooping(true);
            }
        }
        update();
        emit timeSelectionChanged(start, end);
    }
}

void TimeRulerWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isSelectingTime = false;
        if (m_engine && m_engine->getLoopEnd() > m_engine->getLoopStart() + 0.01) {
            m_engine->setPlayheadPosition(m_engine->getLoopStart());
        }
        update();
    }
}

void TimeRulerWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_engine) {
        if (m_engine->getLoopEnd() > m_engine->getLoopStart() + 0.01) {
            m_engine->setLoopRegion(0, 0);
            m_engine->setLooping(false);
        } else {
            double maxEndTime = 0.0;
            auto tracks = m_engine->getTracksSnapshot();
            for (const auto& track : tracks) {
                for (const auto& item : track.items) {
                    double end = item.startTimeSecs + item.durationSecs;
                    if (end > maxEndTime) maxEndTime = end;
                }
            }
            if (maxEndTime > 0) {
                m_engine->setLoopRegion(0, maxEndTime);
                m_engine->setLooping(true);
            }
        }
        update();
    }
}


// ============================================================================
// TimelineLanesWidget
// ============================================================================

TimelineLanesWidget::TimelineLanesWidget(dsp::AudioEngine* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setMinimumSize(3600 * 50, 1000); // 1 hour at 50px/sec default
    setFocusPolicy(Qt::StrongFocus); // Accept keyboard events for deletion
    setAcceptDrops(true); // Accept drag and drop for external files
    
    // Timer to update playhead position at ~60fps
    connect(&m_playheadTimer, &QTimer::timeout, this, &TimelineLanesWidget::onPlayheadTimerTick);
    m_playheadTimer.start(16);
}

void TimelineLanesWidget::setZoom(double pixelsPerSecond)
{
    m_pixelsPerSecond = pixelsPerSecond;
    update();
}

void TimelineLanesWidget::setTrackHeight(int height)
{
    m_trackHeight = height;
    update();
}

void TimelineLanesWidget::onPlayheadTimerTick()
{
    if (m_engine) {
        // Even if not playing, we want to update if it moved (e.g. from scrubbing)
        double currentPlayhead = m_engine->getPlayheadTime();
        if (currentPlayhead != m_lastPlayheadTime) {
            m_lastPlayheadTime = currentPlayhead;
            
            // Emit scroll request so container can follow if playing
            if (m_engine->isEnginePlaying()) {
                int playheadX = currentPlayhead * m_pixelsPerSecond;
                emit requestScroll(playheadX);
            }
        }
        update(); // Always unconditionally repaint (e.g. for recording updates, clip modifications)
    }
}

void TimelineLanesWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int w = width();
    int h = height();
    
    // Background (Stark Black)
    painter.fillRect(event->rect(), QColor("#050505"));
    
    // Draw Grid Lines (Dotted) optimized to exposed rect
    QRect exposed = event->rect();
    int startX = exposed.left();
    int endX = exposed.right();
    int startS = std::max(0, (int)(startX / m_pixelsPerSecond));
    int endS = (int)(endX / m_pixelsPerSecond) + 1;
    
    QPen gridPen(QColor("#222222"), 1, Qt::DotLine);
    painter.setPen(gridPen);
    for (int s = startS; s <= endS; s++) {
        int x = s * m_pixelsPerSecond;
        painter.drawLine(x, 0, x, h);
    }
    
    // Draw Track Lanes and Clips dynamically
    if (m_engine) {
        auto tracks = m_engine->getTracksSnapshot();
        
        // Dynamically set height if tracks exist
        int expectedHeight = tracks.size() * (m_trackHeight + 4); // TrackCard + 4px margin
        if (expectedHeight > 0 && expectedHeight != minimumHeight()) {
            setMinimumHeight(expectedHeight);
        }
        
        painter.setPen(QPen(QColor("#222225"), 1));
        
        int yOffset = 0;
        int trackHeight = m_trackHeight;
        int trackMargin = 4;
        
        for (size_t i = 0; i < tracks.size(); ++i) {
            const auto& track = tracks[i];
            
            // Draw lane separator (solid)
            painter.drawLine(0, yOffset + trackHeight + trackMargin/2, w, yOffset + trackHeight + trackMargin/2);
            
            // Draw all clips in this track
            for (const auto& item : track.items) {
                int startX = item.startTimeSecs * m_pixelsPerSecond;
                int itemW = item.durationSecs * m_pixelsPerSecond;
                
                QRect clipRect(startX, yOffset + 10, itemW, trackHeight - 20);
                
                // Nothing Aesthetic: Transparent background, white dotted/solid border
                if (item.isSelected) {
                    painter.fillRect(clipRect, QColor("#333333")); // Lighter background for selection
                    painter.setPen(QPen(QColor("#ffffff"), 2)); // Thicker border
                } else {
                    painter.fillRect(clipRect, QColor("#111111"));
                    painter.setPen(QPen(QColor("#ffffff"), 1));
                }
                
                painter.drawRect(clipRect);
                
                // Draw name or ID placeholder (Monospaced look if possible)
                if (item.isSelected) {
                    painter.setPen(QPen(QColor("#ffffff"), 1));
                } else {
                    painter.setPen(QPen(QColor("#aaaaaa"), 1));
                }
                QFont f = painter.font();
                f.setFamily("Courier"); // Or whatever monospace is available
                f.setPixelSize(11);
                painter.setFont(f);
                painter.drawText(clipRect.adjusted(5, 5, -5, -5), Qt::AlignLeft | Qt::AlignTop, QString("CLIP %1").arg(item.id));
                
                // Draw Waveform if buffer exists
                if (item.buffer != nullptr && item.buffer->getNumSamples() > 0) {
                    painter.setPen(QPen(QColor("#ffffff"), 1)); // White waveform
                    int numSamples = item.buffer->getNumSamples();
                    const float* channelData = item.buffer->getReadPointer(0); // Draw left channel
                    
                    int midY = clipRect.y() + clipRect.height() / 2;
                    float halfHeight = clipRect.height() / 2.0f;
                    
                    for (int x = 0; x < itemW; ++x) {
                        int startSample = (int)(((float)x / itemW) * numSamples);
                        int endSample = (int)(((float)(x + 1) / itemW) * numSamples);
                        endSample = std::min(endSample, numSamples);
                        
                        if (startSample >= numSamples) break;
                        
                        float minAmplitude = 0.0f;
                        float maxAmplitude = 0.0f;
                        
                        // If we are extremely zoomed in, multiple pixels might map to the same sample
                        if (endSample == startSample) {
                            endSample = startSample + 1;
                        }

                        for (int s = startSample; s < endSample; ++s) {
                            float sample = channelData[s];
                            if (sample > maxAmplitude) maxAmplitude = sample;
                            if (sample < minAmplitude) minAmplitude = sample;
                        }
                        
                        int yTop = midY - (int)(maxAmplitude * halfHeight);
                        int yBottom = midY - (int)(minAmplitude * halfHeight);
                        
                        if (yTop == yBottom) {
                            painter.drawPoint(clipRect.x() + x, yTop);
                        } else {
                            painter.drawLine(clipRect.x() + x, yTop, clipRect.x() + x, yBottom);
                        }
                    }
                }
                
                painter.setPen(QPen(QColor("#222225"), 1));
            }
            
            // Draw Live Recording Waveform
            if (m_engine->isEngineRecording() && track.isArmed) {
                auto* recBuffer = m_engine->getRecordBuffer(track.id);
                int samplesWritten = m_engine->getRecordSamplesWritten(track.id);
                double startTime = m_engine->getRecordStartTime(track.id);
                double currentSr = m_engine->getCurrentSampleRate();
                
                if (recBuffer && samplesWritten > 0 && currentSr > 0) {
                    double durationSecs = (double)samplesWritten / currentSr;
                    int startX = startTime * m_pixelsPerSecond;
                    int itemW = durationSecs * m_pixelsPerSecond;
                    itemW = std::max(1, itemW);
                    
                    QRect clipRect(startX, yOffset + 10, itemW, trackHeight - 20);
                    
                    // Draw recording background (darker red)
                    painter.fillRect(clipRect, QColor("#1a0505"));
                    
                    // Draw waveform
                    painter.setPen(QPen(QColor("#ff3333"), 1));
                    const float* channelData = recBuffer->getReadPointer(0);
                    
                    int midY = clipRect.y() + clipRect.height() / 2;
                    float halfHeight = clipRect.height() / 2.0f;
                    
                    for (int x = 0; x < itemW; ++x) {
                        int startSample = (int)(((float)x / itemW) * samplesWritten);
                        int endSample = (int)(((float)(x + 1) / itemW) * samplesWritten);
                        endSample = std::min(endSample, samplesWritten);
                        
                        if (startSample >= samplesWritten) break;
                        
                        float minAmplitude = 0.0f;
                        float maxAmplitude = 0.0f;
                        
                        if (endSample == startSample) endSample = startSample + 1;

                        for (int s = startSample; s < endSample; ++s) {
                            float sample = channelData[s];
                            if (sample > maxAmplitude) maxAmplitude = sample;
                            if (sample < minAmplitude) minAmplitude = sample;
                        }
                        
                        int yTop = midY - (int)(maxAmplitude * halfHeight);
                        int yBottom = midY - (int)(minAmplitude * halfHeight);
                        
                        if (yTop == yBottom) {
                            painter.drawPoint(clipRect.x() + x, yTop);
                        } else {
                            painter.drawLine(clipRect.x() + x, yTop, clipRect.x() + x, yBottom);
                        }
                    }
                    
                    // Draw dotted red border for live recording
                    painter.setPen(QPen(QColor("#ff3333"), 1, Qt::DotLine));
                    painter.drawRect(clipRect);
                    
                    // Recording text
                    painter.setPen(QPen(QColor("#ff3333"), 1));
                    QFont f = painter.font();
                    f.setFamily("Courier");
                    f.setPixelSize(11);
                    painter.setFont(f);
                    painter.drawText(clipRect.adjusted(5, 5, -5, -5), Qt::AlignLeft | Qt::AlignTop, "REC");
                    
                    // Reset brush/pen
                    painter.setPen(QPen(QColor("#222222"), 1, Qt::SolidLine));
                    painter.setBrush(Qt::NoBrush);
                }
            }
            
            yOffset += (trackHeight + trackMargin);
        }
        
        // Draw ghost drag clip for internal drags
        if (m_draggingItemId != -1) {
            for (const auto& track : tracks) {
                for (const auto& item : track.items) {
                    if (item.id == m_draggingItemId) {
                        int ghostX = m_previewStartTime * m_pixelsPerSecond;
                        int itemW = item.durationSecs * m_pixelsPerSecond;
                        int ghostY = m_draggingTrackIndex * (trackHeight + trackMargin) + 10;
                        
                        QRect ghostRect(ghostX, ghostY, itemW, trackHeight - 20);
                        
                        // Transparent ghost effect
                        QColor ghostColor = QColor("#ffffff");
                        ghostColor.setAlpha(80);
                        painter.fillRect(ghostRect, ghostColor);
                        painter.setPen(QPen(QColor("#ffffff"), 1, Qt::DashLine));
                        painter.drawRect(ghostRect);
                        break;
                    }
                }
            }
        } else if (m_isExternalDrag) {
            // Draw ghost clip for external drag
            int ghostX = m_previewStartTime * m_pixelsPerSecond;
            int itemW = 100; // Arbitrary width since we don't know file length yet
            int ghostY = m_draggingTrackIndex * (trackHeight + trackMargin) + 10;
            
            QRect ghostRect(ghostX, ghostY, itemW, trackHeight - 20);
            
            // Transparent ghost effect
            QColor ghostColor = QColor("#00ff00"); // Green for new files
            ghostColor.setAlpha(60);
            painter.fillRect(ghostRect, ghostColor);
            painter.setPen(QPen(QColor("#00ff00"), 1, Qt::DashLine));
            painter.drawRect(ghostRect);
            painter.drawText(ghostRect, Qt::AlignCenter, "Drop Audio Here");
        }
    }
    
    // Draw loop region overlay
    if (m_engine && m_engine->getLoopEnd() > m_engine->getLoopStart()) {
        int loopStartX = m_engine->getLoopStart() * m_pixelsPerSecond;
        int loopEndX = m_engine->getLoopEnd() * m_pixelsPerSecond;
        
        painter.fillRect(loopStartX, 0, loopEndX - loopStartX, h, QColor(255, 255, 255, 10)); // faint white/blue highlight
        
        painter.setPen(QPen(QColor(255, 255, 255, 40), 1, Qt::DashLine));
        painter.drawLine(loopStartX, 0, loopStartX, h);
        painter.drawLine(loopEndX, 0, loopEndX, h);
    }
    
    // Draw Playhead (Nothing Red)
    if (m_engine) {
        double playheadTime = m_engine->getPlayheadTime();
        int playheadX = playheadTime * m_pixelsPerSecond;
        
        painter.setPen(QPen(QColor("#ff3333"), 2));
        painter.drawLine(playheadX, 0, playheadX, h);
        
        // Draw playhead triangle/dot at top
        painter.setBrush(QColor("#ff3333"));
        painter.setPen(Qt::NoPen);
        QPolygon polygon;
        polygon << QPoint(playheadX - 5, 0) << QPoint(playheadX + 5, 0) << QPoint(playheadX, 8);
        painter.drawPolygon(polygon);
    }
}

void TimelineLanesWidget::setPlayheadFromMouse(QMouseEvent* event)
{
    if (!m_engine) return;
    
    // Lanes are already inside a scroll area, so event->x() is the absolute width
    double timeSecs = std::max(0.0, (double)event->x() / m_pixelsPerSecond);
    m_engine->setPlayheadPosition(timeSecs);
}


HitTestResult TimelineLanesWidget::hitTest(const QPoint& pos)
{
    HitTestResult result;
    if (!m_engine) return result;
    
    auto tracks = m_engine->getTracksSnapshot();
    int trackHeight = m_trackHeight;
    int trackMargin = 4;
    int yOffset = 0;
    
    for (size_t i = 0; i < tracks.size(); ++i) {
        if (pos.y() >= yOffset && pos.y() <= yOffset + trackHeight) {
            result.trackIndex = i;
            
            // Check items
            for (const auto& item : tracks[i].items) {
                int startX = item.startTimeSecs * m_pixelsPerSecond;
                int itemW = item.durationSecs * m_pixelsPerSecond;
                QRect clipRect(startX, yOffset + 10, itemW, trackHeight - 20);
                
                if (clipRect.contains(pos)) {
                    result.itemId = item.id;
                    return result; // Found the item
                }
            }
            return result; // Found track, no item
        }
        yOffset += (trackHeight + trackMargin);
    }
    return result;
}

void TimelineLanesWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        HitTestResult hit = hitTest(event->pos());
        
        if (hit.itemId != -1) {
            // Clicked on a clip
            m_engine->clearAudioItemSelection();
            m_engine->setAudioItemSelection(hit.itemId, true);
            
            // Start drag
            m_draggingItemId = hit.itemId;
            m_draggingTrackIndex = hit.trackIndex;
            
            auto tracks = m_engine->getTracksSnapshot();
            for (const auto& item : tracks[hit.trackIndex].items) {
                if (item.id == hit.itemId) {
                    int startX = item.startTimeSecs * m_pixelsPerSecond;
                    m_dragOffsetX = event->x() - startX;
                    m_previewStartTime = item.startTimeSecs;
                    break;
                }
            }
            update();
        } else {
            // Clicked empty space
            m_engine->clearAudioItemSelection();
            setPlayheadFromMouse(event);
        }
    } else if (event->button() == Qt::RightButton) {
        HitTestResult hit = hitTest(event->pos());
        if (hit.itemId != -1) {
            m_engine->clearAudioItemSelection();
            m_engine->setAudioItemSelection(hit.itemId, true);
            update();
        }
    }
}

void TimelineLanesWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        if (m_draggingItemId != -1) {
            // Dragging a clip
            double newStartX = event->x() - m_dragOffsetX;
            m_previewStartTime = std::max(0.0, newStartX / m_pixelsPerSecond);
            
            // Determine new track
            int trackHeight = m_trackHeight;
            int trackMargin = 4;
            int newTrackIndex = event->y() / (trackHeight + trackMargin);
            if (m_engine) {
                size_t numTracks = m_engine->getTracksSnapshot().size();
                newTrackIndex = std::clamp(newTrackIndex, 0, (int)numTracks - 1);
                m_draggingTrackIndex = newTrackIndex;
            }
            update();
        } else {
            // Scrubbing playhead
            setPlayheadFromMouse(event);
        }
    }
}

void TimelineLanesWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_draggingItemId != -1 && m_engine) {
            m_engine->moveAudioItem(m_draggingItemId, m_draggingTrackIndex, m_previewStartTime);
            m_draggingItemId = -1;
            update();
        }
    }
}

void TimelineLanesWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        if (m_engine) {
            auto tracks = m_engine->getTracksSnapshot();
            for (const auto& track : tracks) {
                for (const auto& item : track.items) {
                    if (item.isSelected) {
                        m_engine->deleteAudioItem(item.id);
                    }
                }
            }
            update();
        }
    } else if (event->key() == Qt::Key_S) {
        if (m_engine) {
            double splitTime = m_engine->getPlayheadTime();
            auto tracks = m_engine->getTracksSnapshot();
            for (const auto& track : tracks) {
                for (const auto& item : track.items) {
                    if (item.isSelected && splitTime > item.startTimeSecs && splitTime < (item.startTimeSecs + item.durationSecs)) {
                        dsp::EngineMessage msg;
                        msg.type = dsp::EngineCommandType::SplitAudioItem;
                        msg.itemId = item.id;
                        msg.doubleValue = splitTime;
                        m_engine->sendMessageFromUI(msg);
                    }
                }
            }
            update();
        }
    } else if (event->matches(QKeySequence::Copy)) {
        if (m_engine) {
            m_clipboardItems.clear();
            auto tracks = m_engine->getTracksSnapshot();
            for (size_t t = 0; t < tracks.size(); ++t) {
                for (const auto& item : tracks[t].items) {
                    if (item.isSelected) {
                        ClipboardItem clipItem;
                        clipItem.trackIndex = (int)t;
                        clipItem.item = item;
                        m_clipboardItems.push_back(clipItem);
                    }
                }
            }
        }
    } else if (event->matches(QKeySequence::Paste)) {
        if (m_engine && !m_clipboardItems.empty()) {
            double playheadTime = m_engine->getPlayheadTime();
            double earliestTime = m_clipboardItems[0].item.startTimeSecs;
            for (const auto& clip : m_clipboardItems) {
                if (clip.item.startTimeSecs < earliestTime) earliestTime = clip.item.startTimeSecs;
            }
            
            for (const auto& clip : m_clipboardItems) {
                dsp::AudioItem* newItem = new dsp::AudioItem(clip.item);
                newItem->id = -1;
                newItem->startTimeSecs = playheadTime + (newItem->startTimeSecs - earliestTime);
                newItem->isSelected = true; // Select newly pasted items
                
                dsp::EngineMessage msg;
                msg.type = dsp::EngineCommandType::AddAudioItem;
                msg.trackIndex = clip.trackIndex;
                msg.ptrValue = newItem;
                m_engine->sendMessageFromUI(msg);
            }
            
            // Clear current selection
            m_engine->clearAudioItemSelection();
            update();
        }
    } else if (event->key() == Qt::Key_Home) {
        if (m_engine) {
            m_engine->setPlayheadPosition(0.0);
            update();
        }
    } else if (event->key() == Qt::Key_End) {
        if (m_engine) {
            double maxEndTime = 0.0;
            auto tracks = m_engine->getTracksSnapshot();
            for (const auto& track : tracks) {
                for (const auto& item : track.items) {
                    double end = item.startTimeSecs + item.durationSecs;
                    if (end > maxEndTime) maxEndTime = end;
                }
            }
            m_engine->setPlayheadPosition(maxEndTime);
            update();
        }
    } else {
        QWidget::keyPressEvent(event);
    }
}

void TimelineLanesWidget::contextMenuEvent(QContextMenuEvent* event)
{
    HitTestResult hit = hitTest(event->pos());
    QMenu menu(this);
    
    if (hit.itemId != -1 && m_engine) {
        QAction* copyAction = menu.addAction("Copy Clip (Ctrl+C)");
        QAction* splitAction = menu.addAction("Split Clip at Playhead (S)");
        menu.addSeparator();
        QAction* deleteAction = menu.addAction("Delete Clip (Del)");
        
        QAction* selected = menu.exec(event->globalPos());
        if (selected == deleteAction) {
            m_engine->deleteAudioItem(hit.itemId);
            update();
        } else if (selected == splitAction) {
            double splitTime = m_engine->getPlayheadTime();
            dsp::EngineMessage msg;
            msg.type = dsp::EngineCommandType::SplitAudioItem;
            msg.itemId = hit.itemId;
            msg.doubleValue = splitTime;
            m_engine->sendMessageFromUI(msg);
            update();
        } else if (selected == copyAction) {
            m_clipboardItems.clear();
            auto tracks = m_engine->getTracksSnapshot();
            for (size_t t = 0; t < tracks.size(); ++t) {
                for (const auto& item : tracks[t].items) {
                    if (item.id == hit.itemId) {
                        ClipboardItem clipItem;
                        clipItem.trackIndex = (int)t;
                        clipItem.item = item;
                        m_clipboardItems.push_back(clipItem);
                        break;
                    }
                }
            }
        }
    } else {
        // Background click
        QAction* pasteAction = menu.addAction("Paste (Ctrl+V)");
        pasteAction->setEnabled(!m_clipboardItems.empty());
        
        QAction* selected = menu.exec(event->globalPos());
        if (selected == pasteAction && m_engine && !m_clipboardItems.empty()) {
            double playheadTime = m_engine->getPlayheadTime();
            double earliestTime = m_clipboardItems[0].item.startTimeSecs;
            for (const auto& clip : m_clipboardItems) {
                if (clip.item.startTimeSecs < earliestTime) earliestTime = clip.item.startTimeSecs;
            }
            
            m_engine->clearAudioItemSelection();
            
            for (const auto& clip : m_clipboardItems) {
                dsp::AudioItem* newItem = new dsp::AudioItem(clip.item);
                newItem->id = -1;
                newItem->startTimeSecs = playheadTime + (newItem->startTimeSecs - earliestTime);
                newItem->isSelected = true;
                
                dsp::EngineMessage msg;
                msg.type = dsp::EngineCommandType::AddAudioItem;
                msg.trackIndex = clip.trackIndex;
                msg.ptrValue = newItem;
                m_engine->sendMessageFromUI(msg);
            }
            update();
        }
    }
}

void TimelineLanesWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        m_isExternalDrag = true;
        event->acceptProposedAction();
        update();
    }
}

void TimelineLanesWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasUrls() && m_engine) {
        // Calculate drop target track and time
        int trackHeight = m_trackHeight;
        int trackMargin = 4;
        int trackIndex = event->pos().y() / (trackHeight + trackMargin);
        
        size_t numTracks = m_engine->getTracksSnapshot().size();
        trackIndex = std::clamp(trackIndex, 0, (int)numTracks - 1);
        
        double startTime = std::max(0.0, (double)event->pos().x() / m_pixelsPerSecond);
        
        m_draggingTrackIndex = trackIndex;
        m_previewStartTime = startTime;
        
        event->acceptProposedAction();
        update();
    }
}

void TimelineLanesWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
    m_isExternalDrag = false;
    update();
}

void TimelineLanesWidget::dropEvent(QDropEvent* event)
{
    m_isExternalDrag = false;
    if (!m_engine) {
        update();
        return;
    }
    
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        for (const QUrl& url : urlList) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                
                // Load it using the pre-calculated position
                m_engine->loadAudioFileSynchronous(m_draggingTrackIndex, m_previewStartTime, filePath.toStdString());
                
                break;
            }
        }
        event->acceptProposedAction();
    }
    update();
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
    
    connect(m_lanes, &TimelineLanesWidget::requestScroll, this, &TimelineContainer::onScrollRequested);
}



void TimelineContainer::onScrollRequested(int playheadX)
{
    // Autoscroll logic
    QScrollBar* hBar = horizontalScrollBar();
    int viewportWidth = viewport()->width();
    
    // If playhead approaches the right edge (within 100px), scroll page
    if (playheadX > hBar->value() + viewportWidth - 100) {
        hBar->setValue(playheadX - 100);
    }
}

void TimelineContainer::zoom(double factor, QPoint centerPos)
{
    // Get current zoom
    double currentZoom = m_lanes->property("pixelsPerSecond").toDouble();
    if (currentZoom == 0.0) currentZoom = 50.0;
    
    // Calculate new zoom (clamp between 5px/s and 1000px/s)
    double newZoom = std::clamp(currentZoom * factor, 5.0, 1000.0);
    
    if (newZoom != currentZoom) {
        // Calculate the time at the center position
        int absoluteX = horizontalScrollBar()->value() + centerPos.x();
        double timeAtCenter = absoluteX / currentZoom;
        
        // Update zoom on widgets
        m_ruler->setZoom(newZoom);
        m_lanes->setZoom(newZoom);
        
        // Store property to retrieve later
        m_lanes->setProperty("pixelsPerSecond", newZoom);
        
        // Adjust widths
        int newWidth = 3600 * newZoom; // 1 hour max for now
        m_ruler->setMinimumWidth(newWidth);
        m_lanes->setMinimumWidth(newWidth);
        
        // Restore center position
        int newAbsoluteX = timeAtCenter * newZoom;
        horizontalScrollBar()->setValue(newAbsoluteX - centerPos.x());
    }
}

void TimelineContainer::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom
        double factor = (event->angleDelta().y() > 0) ? 1.2 : 1.0 / 1.2;
        zoom(factor, event->position().toPoint());
        event->accept();
    } else if (event->modifiers() & Qt::ShiftModifier) {
        // Horizontal scroll
        QScrollBar* hBar = horizontalScrollBar();
        hBar->setValue(hBar->value() - event->angleDelta().y());
        event->accept();
    } else {
        // Normal scrolling
        QScrollArea::wheelEvent(event);
    }
}

} // namespace gui
