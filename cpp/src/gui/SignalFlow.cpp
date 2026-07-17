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
}

void SignalFlow::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background
    painter.fillRect(event->rect(), QColor("#111111"));
    
    // Draw grid points for aesthetics
    painter.setPen(QPen(QColor("#222225"), 1));
    for (int x = 0; x < width(); x += 20) {
        for (int y = 0; y < height(); y += 20) {
            painter.drawPoint(x, y);
        }
    }

    // Draw some placeholder nodes
    int startY = height() / 2 - 25;
    
    // Input Node
    QRect inputRect(20, startY, 60, 50);
    painter.setBrush(QBrush(QColor("#1a1a1c")));
    painter.setPen(QPen(QColor("#444448"), 2));
    painter.drawRoundedRect(inputRect, 5, 5);
    
    painter.setPen(QPen(QColor("#ffffff"), 1));
    painter.drawText(inputRect, Qt::AlignCenter, "INPUT");
    
    // Wire 1
    painter.setPen(QPen(QColor("#ff0033"), 3));
    painter.drawLine(80, startY + 25, 130, startY + 25);
    
    // NAM Node
    QRect namRect(130, startY, 120, 50);
    painter.setBrush(QBrush(QColor("#1a1a1c")));
    painter.setPen(QPen(QColor("#ff0033"), 2));
    painter.drawRoundedRect(namRect, 5, 5);
    
    painter.setPen(QPen(QColor("#ffffff"), 1));
    painter.drawText(namRect, Qt::AlignCenter, "NAM");
    
    // Wire 2
    painter.setPen(QPen(QColor("#ff0033"), 3));
    painter.drawLine(250, startY + 25, 300, startY + 25);
    
    // Output Node
    QRect outputRect(300, startY, 60, 50);
    painter.setBrush(QBrush(QColor("#1a1a1c")));
    painter.setPen(QPen(QColor("#444448"), 2));
    painter.drawRoundedRect(outputRect, 5, 5);
    
    painter.setPen(QPen(QColor("#ffffff"), 1));
    painter.drawText(outputRect, Qt::AlignCenter, "OUT");
}

} // namespace gui
