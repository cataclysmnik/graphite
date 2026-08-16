#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QTimer>
#include <vector>
#include <utility>
#include "../audio/AudioModels.h"

namespace dsp {
    class AudioEngine;
}

namespace gui {

class TimeRulerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimeRulerWidget(dsp::AudioEngine* engine, QWidget* parent = nullptr);
    void setZoom(double pixelsPerSecond);

signals:
    void timeSelectionChanged(double start, double end);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    
private:
    void setPlayheadFromMouse(QMouseEvent* event);
private:
    dsp::AudioEngine* m_engine;
    double m_pixelsPerSecond { 50.0 };
    bool m_isSelectingTime { false };
    bool m_draggedSelection { false };
    double m_selectionAnchorTime { 0.0 };
};


struct HitTestResult {
    int trackIndex = -1;
    int itemId = -1;
    enum class Edge { None, Left, Right };
    Edge edge = Edge::None;
};

class TimelineLanesWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineLanesWidget(dsp::AudioEngine* engine, QWidget* parent = nullptr);

    void setZoom(double pixelsPerSecond);
    void setTrackHeight(int height);
    
signals:
    void requestScroll(int playheadX);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

    // Drag and drop support for external files
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void setPlayheadFromMouse(QMouseEvent* event);
    HitTestResult hitTest(const QPoint& pos);

private slots:
    void onPlayheadTimerTick();

private:
    dsp::AudioEngine* m_engine;
    double m_pixelsPerSecond { 50.0 };
    int m_trackHeight { 100 };
    
    uint32_t m_lastStateVersion { 0xffffffff };
    std::vector<dsp::Track> m_cachedTracks;
    
    QTimer m_playheadTimer;
    
    // Dragging state
    int m_draggingItemId { -1 };
    int m_draggingTrackIndex { -1 };
    double m_dragOffsetX { 0.0 };
    double m_previewStartTime { 0.0 };
    bool m_isExternalDrag { false };
    
    // Cache the playhead position to only update when it moves
    double m_lastPlayheadTime { 0.0 };
    
    struct ClipboardItem {
        int trackIndex;
        dsp::AudioItem item;

        ClipboardItem() = default;
        ClipboardItem(const ClipboardItem&) = default;
        ClipboardItem& operator=(const ClipboardItem&) = default;
        ClipboardItem(ClipboardItem&&) noexcept = default;
        ClipboardItem& operator=(ClipboardItem&&) noexcept = default;
    };
    std::vector<ClipboardItem> m_clipboardItems;
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
    void onScrollRequested(int playheadX);

private:
    TimeRulerWidget* m_ruler;
    TimelineLanesWidget* m_lanes;
};

} // namespace gui
