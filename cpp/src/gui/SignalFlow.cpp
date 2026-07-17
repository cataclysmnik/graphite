#include "SignalFlow.h"
#include "../audio/AudioEngine.h"
#include <QPainter>
#include <QPaintEvent>
#include <QColor>
#include <QPen>

namespace gui {

SignalFlow::SignalFlow(dsp::AudioEngine* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName("SignalFlow");
    setMinimumSize(400, 200);
    
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [this](){ 
        m_textScrollOffset += 1.0f;
        if (m_textScrollOffset > 400.0f) m_textScrollOffset = -50.0f;
        update(); 
    });
    m_timer->start(30); // Polling and animating
}

void SignalFlow::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background
    painter.fillRect(event->rect(), QColor("#111111"));
    
    // Draw grid points for aesthetics
    painter.setPen(QPen(QColor("#222225"), 1));
    for (int x = m_panOffset.x() % 20; x < width(); x += 20) {
        for (int y = m_panOffset.y() % 20; y < height(); y += 20) {
            painter.drawPoint(x, y);
        }
    }

    // Calculate all node positions first
    int currentTrackIndex = m_engine ? m_engine->getSelectedTrackIndex() : 0;
    std::vector<std::string> plugins = m_engine ? m_engine->getTrackPluginNames(currentTrackIndex) : std::vector<std::string>();

    int totalGraphWidth = 70 + (plugins.size() * 200) + 70;
    int startY = height() / 2 - 25 + m_panOffset.y();
    int currentX = (width() - totalGraphWidth) / 2 + m_panOffset.x();
    
    m_nodes.clear();
    
    // Adjust offsets size
    while (m_nodeOffsets.size() < plugins.size()) m_nodeOffsets.push_back(QPoint(0, 0));
    while (m_nodeOffsets.size() > plugins.size()) m_nodeOffsets.pop_back();
    
    // Define input node
    QRect inputRect(currentX, startY, 70, 60);
    currentX += 70;
    
    // Pre-calculate plugin nodes
    for (int i = 0; i < plugins.size(); ++i) {
        currentX += 60; // gap
        int nodeX = currentX + m_nodeOffsets[i].x();
        int nodeY = startY + m_nodeOffsets[i].y();
        QRect pRect(nodeX, nodeY, 140, 60);
        m_nodes.push_back({ i, pRect });
        currentX += 140;
    }
    
    currentX += 60;
    QRect outputRect(currentX, startY, 70, 60);
    
    // Draw Wires (connecting centers)
    painter.setPen(QPen(QColor("#ff0033"), 3));
    QPoint prevCenter = inputRect.center();
    
    for (int i = 0; i < plugins.size(); ++i) {
        QPoint currCenter = m_nodes[i].rect.center();
        painter.drawLine(prevCenter, currCenter);
        prevCenter = currCenter;
    }
    painter.drawLine(prevCenter, outputRect.center());
    
    // Setup font for nodes
    QFont font = painter.font();
    font.setPixelSize(14);
    painter.setFont(font);
    
    // Draw Input Node on top
    painter.setBrush(QBrush(QColor("#111111"))); // Match background to hide wire
    painter.setPen(QPen(QColor("#444448"), 2));
    painter.drawRoundedRect(inputRect, 5, 5);
    painter.setPen(QPen(QColor("#ffffff"), 1));
    painter.drawText(inputRect, Qt::AlignCenter, "INPUT");
    
    // Draw Plugin Nodes on top
    for (int i = 0; i < plugins.size(); ++i) {
        QRect pRect = m_nodes[i].rect;
        
        painter.setBrush(QBrush(QColor("#111111"))); // Match background so it's opaque but blends
        painter.setPen(QPen(QColor(m_draggingNodeIndex == i ? "#ffffff" : "#ff0033"), 2));
        painter.drawRoundedRect(pRect, 5, 5);
        painter.setPen(QPen(QColor("#ffffff"), 1));
        
        // Scroll name if it doesn't fit
        QString fullName = QString::fromStdString(plugins[i]);
        QFontMetrics fm(painter.font());
        int textWidth = fm.horizontalAdvance(fullName);
        
        painter.save();
        painter.setClipRect(pRect.adjusted(5, 5, -5, -5));
        
        if (textWidth > pRect.width() - 10) {
            // Ping-pong scroll logic
            int maxScroll = textWidth - pRect.width() + 20;
            if (maxScroll < 0) maxScroll = 10;
            float period = 100.0f; // Adjust for speed
            float phase = fmod(m_textScrollOffset, period * 2);
            int offset = 0;
            if (phase < period) {
                offset = (int)((phase / period) * maxScroll);
            } else {
                offset = (int)(((period * 2 - phase) / period) * maxScroll);
            }
            painter.drawText(pRect.x() + 5 - offset, pRect.y(), textWidth, pRect.height(), Qt::AlignVCenter | Qt::AlignLeft, fullName);
        } else {
            painter.drawText(pRect, Qt::AlignCenter, fullName);
        }
        
        painter.restore();
    }
    
    // Draw Output Node on top
    painter.setBrush(QBrush(QColor("#111111")));
    painter.setPen(QPen(QColor("#444448"), 2));
    painter.drawRoundedRect(outputRect, 5, 5);
    painter.setPen(QPen(QColor("#ffffff"), 1));
    painter.drawText(outputRect, Qt::AlignCenter, "OUT");
}

void SignalFlow::mousePressEvent(QMouseEvent* event)
{
    m_isPanning = true;
    m_dragStartPos = event->pos();
    
    for (const auto& node : m_nodes) {
        if (node.rect.contains(event->pos())) {
            m_draggingNodeIndex = node.pluginIndex;
            m_originalOffset = m_nodeOffsets[m_draggingNodeIndex];
            m_isPanning = false;
            break;
        }
    }
}

void SignalFlow::mouseMoveEvent(QMouseEvent* event)
{
    QPoint delta = event->pos() - m_dragStartPos;
    if (m_draggingNodeIndex != -1) {
        m_nodeOffsets[m_draggingNodeIndex] = m_originalOffset + delta;
        update();
    } else if (m_isPanning) {
        m_panOffset += delta;
        m_dragStartPos = event->pos(); // reset drag start for incremental panning
        update();
    }
}

void SignalFlow::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_draggingNodeIndex != -1 && m_engine) {
        int dropX = m_nodes[m_draggingNodeIndex].rect.center().x();
        int toIndex = m_draggingNodeIndex;
        
        for (int i = 0; i < m_nodes.size(); ++i) {
            if (i == m_draggingNodeIndex) continue;
            // Original X (without drag offset)
            int origCenterX = m_nodes[i].rect.center().x() - m_nodeOffsets[i].x();
            if (dropX < origCenterX) {
                toIndex = i;
                if (m_draggingNodeIndex < i) toIndex--;
                break;
            }
        }
        if (m_nodes.size() > 0 && dropX > (m_nodes.back().rect.center().x() - m_nodeOffsets.back().x()) && m_draggingNodeIndex != m_nodes.size() - 1) {
            toIndex = m_nodes.size() - 1;
        }
        
        if (toIndex != m_draggingNodeIndex) {
            m_engine->movePluginSynchronous(m_engine->getSelectedTrackIndex(), m_draggingNodeIndex, toIndex);
            
            auto tempOffset = m_nodeOffsets[m_draggingNodeIndex];
            m_nodeOffsets.erase(m_nodeOffsets.begin() + m_draggingNodeIndex);
            m_nodeOffsets.insert(m_nodeOffsets.begin() + toIndex, tempOffset);
            m_nodeOffsets[toIndex].setX(0); 
        } else {
            m_nodeOffsets[m_draggingNodeIndex].setX(0);
        }
        
        m_draggingNodeIndex = -1;
        update();
    }
    
    m_isPanning = false;
}

void SignalFlow::mouseDoubleClickEvent(QMouseEvent* event)
{
    for (const auto& node : m_nodes) {
        if (node.rect.contains(event->pos())) {
            if (m_engine) {
                m_engine->openPluginEditor(m_engine->getSelectedTrackIndex(), node.pluginIndex);
            }
            break;
        }
    }
}

} // namespace gui
