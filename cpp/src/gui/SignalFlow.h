#pragma once

#include <QWidget>

#include <QTimer>
#include <QMouseEvent>
#include <vector>
#include <QRect>
#include <QPoint>

namespace dsp {
    class AudioEngine;
}

namespace gui {

struct NodeData {
    int pluginIndex;
    QRect rect;
};

class SignalFlow : public QWidget
{
    Q_OBJECT
public:
    explicit SignalFlow(dsp::AudioEngine* engine, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    dsp::AudioEngine* m_engine;
    QTimer* m_timer;
    
    std::vector<NodeData> m_nodes;
    std::vector<QPoint> m_nodeOffsets; // Persistent Y-offsets for free floating
    
    int m_draggingNodeIndex = -1;
    QPoint m_dragStartPos;
    QPoint m_originalOffset;
    
    // Panning & Scrolling
    QPoint m_panOffset;
    bool m_isPanning = false;
    float m_textScrollOffset = 0.0f;
};

} // namespace gui
