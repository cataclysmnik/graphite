#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QTimer>

namespace dsp {
    class AudioEngine;
}

namespace gui {

class TimeRulerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimeRulerWidget(dsp::AudioEngine* engine, QWidget* parent = nullptr);
    
    void setScrollOffset(int offset);
    void setZoom(double pixelsPerSecond);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    
private:
    void setPlayheadFromMouse(QMouseEvent* event);
private:
    dsp::AudioEngine* m_engine;
    int m_scrollOffset { 0 };
    double m_pixelsPerSecond { 50.0 };
};


struct HitTestResult {
    int trackIndex = -1;
    int itemId = -1;
};

class TimelineLanesWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineLanesWidget(dsp::AudioEngine* engine, QWidget* parent = nullptr);

    void setZoom(double pixelsPerSecond);
    
signals:
    void requestScroll(int playheadX);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void setPlayheadFromMouse(QMouseEvent* event);
    HitTestResult hitTest(const QPoint& pos);

private slots:
    void onPlayheadTimerTick();

private:
    dsp::AudioEngine* m_engine;
    double m_pixelsPerSecond { 50.0 };
    QTimer m_playheadTimer;
    
    // Dragging state
    int m_draggingItemId { -1 };
    int m_draggingTrackIndex { -1 };
    double m_dragOffsetX { 0.0 };
    double m_previewStartTime { 0.0 };
    
    // Cache the playhead position to only update when it moves
    double m_lastPlayheadTime { 0.0 };
};


class TimelineContainer : public QScrollArea
{
    Q_OBJECT
public:
    explicit TimelineContainer(dsp::AudioEngine* engine, QWidget* parent = nullptr);
    void zoom(double factor, QPoint centerPos);

protected:
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onHorizontalScroll(int value);
    void onScrollRequested(int playheadX);

private:
    TimeRulerWidget* m_ruler;
    TimelineLanesWidget* m_lanes;
};

} // namespace gui
