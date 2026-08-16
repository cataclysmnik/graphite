#include "TunerWidget.h"
#include "../audio/AudioEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <cmath>
#include <algorithm>

namespace gui {

TunerDial::TunerDial(QWidget* parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumHeight(150);
    
    connect(&m_physicsTimer, &QTimer::timeout, this, &TunerDial::updatePhysics);
    m_physicsTimer.start(16);
}

void TunerDial::setCents(double cents, bool hasSignal)
{
    m_cents = cents;
    m_hasSignal = hasSignal;
    if (!hasSignal) {
        m_cents = 0.0;
    }
}

void TunerDial::updatePhysics()
{
    if (m_hasSignal) {
        m_smoothedCents = m_smoothedCents * 0.85 + m_cents * 0.15;
    } else {
        m_smoothedCents = m_smoothedCents * 0.9 + 0.0 * 0.1;
    }
    update();
}

void TunerDial::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int w = width();
    int h = height();
    
    double cx = w / 2.0;
    double cy = h - 20.0;
    
    double radius = std::min(w / 2.0 - 30.0, h - 40.0);
    if (radius < 50.0) radius = 50.0;
    
    painter.fillRect(0, 0, w, h, QColor("#000000"));
    
    QRectF arcRect(cx - radius, cy - radius, radius * 2.0, radius * 2.0);
    QPen penArc(QColor("#333333"), 1.5);
    painter.setPen(penArc);
    painter.drawArc(arcRect, 30 * 16, 120 * 16);
    
    painter.setPen(QPen(QColor("#333333"), 1));
    QFont fontTicks("Consolas", 8);
    painter.setFont(fontTicks);
    
    for (int centsVal = -50; centsVal <= 50; centsVal += 10) {
        double angleDeg = 90.0 - (centsVal / 50.0) * 60.0;
        double angleRad = angleDeg * M_PI / 180.0;
        
        double cosA = std::cos(angleRad);
        double sinA = std::sin(angleRad);
        
        double x1 = cx + (radius - 5.0) * cosA;
        double y1 = cy - (radius - 5.0) * sinA;
        double x2 = cx + (radius + 5.0) * cosA;
        double y2 = cy - (radius + 5.0) * sinA;
        
        if (centsVal == 0) {
            painter.setPen(QPen(QColor("#ff0033"), 2.5));
            painter.drawLine(x1 - cosA * 2.0, y1 + sinA * 2.0, x2 + cosA * 2.0, y2 - sinA * 2.0);
        } else {
            painter.setPen(QPen(QColor("#555555"), 1));
            painter.drawLine(x1, y1, x2, y2);
        }
        
        if (centsVal == -50 || centsVal == 0 || centsVal == 50) {
            if (centsVal == 0) {
                painter.setPen(QColor("#ff0033"));
            } else {
                painter.setPen(QColor("#888888"));
            }
            QString lblText = centsVal > 0 ? QString("+%1").arg(centsVal) : QString::number(centsVal);
            if (centsVal == 0) lblText = "0";
            
            double tx = cx + (radius + 18.0) * cosA - 10.0;
            double ty = cy - (radius + 18.0) * sinA + 5.0;
            painter.drawText(tx, ty, 20, 12, Qt::AlignCenter, lblText);
        }
    }
    
    QRadialGradient radialGrad(cx, cy, 30);
    if (m_hasSignal) {
        if (std::abs(m_smoothedCents) <= 3.0) {
            radialGrad.setColorAt(0, QColor(255, 0, 51, 80));
            radialGrad.setColorAt(1, QColor(255, 0, 51, 0));
        } else {
            radialGrad.setColorAt(0, QColor(255, 255, 255, 30));
            radialGrad.setColorAt(1, QColor(255, 255, 255, 0));
        }
    } else {
        radialGrad.setColorAt(0, QColor(50, 50, 50, 10));
        radialGrad.setColorAt(1, QColor(50, 50, 50, 0));
    }
    painter.setBrush(QBrush(radialGrad));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(cx, cy), radius * 0.7, radius * 0.7);
    
    painter.setPen(QPen(QColor("#222225"), 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(cx, cy), 16, 16);
    
    double needleAngleDeg = 90.0 - (m_smoothedCents / 50.0) * 60.0;
    double needleAngleRad = needleAngleDeg * M_PI / 180.0;
    
    double needleLen = radius - 8.0;
    double nx = cx + needleLen * std::cos(needleAngleRad);
    double ny = cy - needleLen * std::sin(needleAngleRad);
    
    QColor needleColor = m_hasSignal ? (std::abs(m_smoothedCents) <= 3.0 ? QColor("#ff0033") : QColor("#ffffff")) : QColor("#333333");
    
    painter.setPen(QPen(needleColor, 2));
    painter.drawLine(cx, cy, nx, ny);
    
    painter.setPen(QPen(QColor("#000000"), 2));
    painter.setBrush(QBrush(needleColor));
    painter.drawEllipse(QPointF(cx, cy), 6, 6);
}

GuitarTunerWidget::GuitarTunerWidget(dsp::AudioEngine* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setupUi();
    connect(&m_timer, &QTimer::timeout, this, &GuitarTunerWidget::processTuner);
    m_timer.start(50);
}

void GuitarTunerWidget::setupUi()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 10, 15, 10);
    layout->setSpacing(10);
    
    QHBoxLayout* toolbar = new QHBoxLayout();
    toolbar->setSpacing(15);
    
    QLabel* title = new QLabel("GUITAR TUNER");
    title->setFont(QFont("Consolas", 11, QFont::Bold));
    title->setStyleSheet("color: #ffffff; letter-spacing: 1.0px;");
    toolbar->addWidget(title);
    
    toolbar->addStretch();
    
    QLabel* refLabel = new QLabel("A4 Reference:");
    refLabel->setFont(QFont("Consolas", 9));
    refLabel->setStyleSheet("color: #888888;");
    toolbar->addWidget(refLabel);
    
    m_spinA4 = new QSpinBox();
    m_spinA4->setRange(400, 480);
    m_spinA4->setValue(440);
    m_spinA4->setSuffix(" Hz");
    m_spinA4->setMinimumWidth(85);
    m_spinA4->setStyleSheet(
        "QSpinBox { background-color: #000000; border: 1px solid #333333; color: #ffffff; padding: 4px 6px; font-family: 'Consolas', monospace; font-size: 11px; font-weight: bold; }"
        "QSpinBox:focus { border-color: #ffffff; }"
    );
    connect(m_spinA4, QOverload<int>::of(&QSpinBox::valueChanged), this, &GuitarTunerWidget::onA4Changed);
    toolbar->addWidget(m_spinA4);
    
    layout->addLayout(toolbar);
    
    QHBoxLayout* bodyLayout = new QHBoxLayout();
    bodyLayout->setSpacing(20);
    
    m_noteCard = new QFrame();
    m_noteCard->setMinimumWidth(180);
    m_noteCard->setMaximumWidth(220);
    m_noteCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_noteCard->setStyleSheet("QFrame { background-color: #000000; border: 1px solid #222225; }");
    
    QVBoxLayout* cardLayout = new QVBoxLayout(m_noteCard);
    cardLayout->setContentsMargins(10, 15, 10, 15);
    cardLayout->setAlignment(Qt::AlignCenter);
    cardLayout->setSpacing(5);
    
    m_lblNote = new QLabel("--");
    m_lblNote->setFont(QFont("Consolas", 36, QFont::Bold));
    m_lblNote->setAlignment(Qt::AlignCenter);
    m_lblNote->setStyleSheet("color: #555555;");
    cardLayout->addWidget(m_lblNote);
    
    m_lblCents = new QLabel("NO SIGNAL");
    m_lblCents->setFont(QFont("Consolas", 10, QFont::Bold));
    m_lblCents->setAlignment(Qt::AlignCenter);
    m_lblCents->setStyleSheet("color: #555555;");
    cardLayout->addWidget(m_lblCents);
    
    m_lblFrequency = new QLabel("- Hz");
    m_lblFrequency->setFont(QFont("Consolas", 9));
    m_lblFrequency->setAlignment(Qt::AlignCenter);
    m_lblFrequency->setStyleSheet("color: #555555;");
    cardLayout->addWidget(m_lblFrequency);
    
    bodyLayout->addWidget(m_noteCard);
    
    m_dial = new TunerDial();
    bodyLayout->addWidget(m_dial);
    
    layout->addLayout(bodyLayout);
    
    setStyleSheet("QLabel { font-family: 'Consolas', monospace; color: #ffffff; }");
}

void GuitarTunerWidget::onA4Changed(int val)
{
    m_refA4 = val;
}

void GuitarTunerWidget::showInactive()
{
    m_lblNote->setText("--");
    m_lblNote->setStyleSheet("color: #555555;");
    m_lblCents->setText("NO SIGNAL");
    m_lblCents->setStyleSheet("color: #555555;");
    m_lblFrequency->setText("- Hz");
    m_lblFrequency->setStyleSheet("color: #555555;");
    m_dial->setCents(0.0, false);
}

void GuitarTunerWidget::processTuner()
{
    if (!m_engine) {
        showInactive();
        return;
    }
    
    auto samples = m_engine->getTunerSamples(2048);
    if (samples.size() < 2048) {
        showInactive();
        return;
    }
    
    double sumSq = 0.0;
    double sum = 0.0;
    for (float s : samples) {
        sumSq += s * s;
        sum += s;
    }
    double mean = sum / samples.size();
    
    double sumSqMeanRemoved = 0.0;
    for (float& s : samples) {
        s -= mean;
        sumSqMeanRemoved += s * s;
    }
    
    double rms = std::sqrt(sumSqMeanRemoved / samples.size());
    if (rms < 0.003) {
        showInactive();
        return;
    }
    
    int n = samples.size();
    int minLag = m_engine->getCurrentSampleRate() / 1500.0; 
    int maxLag = m_engine->getCurrentSampleRate() / 40.0;   
    
    if (maxLag >= n) maxLag = n - 1;
    
    std::vector<double> diffs(maxLag + 1, 0.0);
    for (int tau = minLag; tau <= maxLag; ++tau) {
        double d = 0.0;
        for (int i = 0; i < n - tau; ++i) {
            double delta = samples[i] - samples[i + tau];
            d += delta * delta;
        }
        diffs[tau] = d;
    }
    
    int bestTau = -1;
    double minDiff = 1e9;
    for (int tau = minLag; tau <= maxLag; ++tau) {
        if (diffs[tau] < minDiff) {
            minDiff = diffs[tau];
            bestTau = tau;
        }
    }
    
    if (bestTau > 0) {
        double freq = m_engine->getCurrentSampleRate() / bestTau;
        
        if (bestTau > minLag && bestTau < maxLag) {
            double s0 = diffs[bestTau - 1];
            double s1 = diffs[bestTau];
            double s2 = diffs[bestTau + 1];
            double denom = 2.0 * (s0 - 2.0 * s1 + s2);
            if (denom != 0.0) {
                double peakOffset = (s0 - s2) / denom;
                if (std::abs(peakOffset) < 1.0) {
                    freq = m_engine->getCurrentSampleRate() / (bestTau + peakOffset);
                }
            }
        }
        
        if (freq > 0) {
            double pitch = 69.0 + 12.0 * std::log2(freq / m_refA4);
            int midiNote = std::round(pitch);
            double cents = (pitch - midiNote) * 100.0;
            
            const char* notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
            QString noteName = notes[midiNote % 12];
            int octave = (midiNote / 12) - 1;
            
            m_lblNote->setText(QString("%1%2").arg(noteName).arg(octave));
            m_lblNote->setStyleSheet("color: #ffffff;");
            
            m_lblFrequency->setText(QString::number(freq, 'f', 1) + " Hz");
            m_lblFrequency->setStyleSheet("color: #aaaaaa;");
            
            if (std::abs(cents) <= 3.0) {
                m_lblCents->setText(QString("IN TUNE"));
                m_lblCents->setStyleSheet("color: #ff0033;");
            } else {
                QString sign = cents > 0 ? "+" : "";
                m_lblCents->setText(QString("%1%2 CENTS").arg(sign).arg(cents, 0, 'f', 1));
                m_lblCents->setStyleSheet("color: #ffffff;");
            }
            
            m_dial->setCents(cents, true);
            return;
        }
    }
    
    showInactive();
}

} // namespace gui
