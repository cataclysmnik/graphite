#include "MetronomeWidget.h"
#include "../audio/AudioEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <cmath>

namespace gui {

void TapTempo::tap()
{
    auto now = std::chrono::steady_clock::now();
    if (!m_taps.empty()) {
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_taps.back()).count();
        if (diff > 2000) {
            m_taps.clear();
        }
    }
    
    m_taps.push_back(now);
    if (m_taps.size() > 5) {
        m_taps.erase(m_taps.begin());
    }
    
    if (m_taps.size() >= 2) {
        long long totalMs = 0;
        for (size_t i = 1; i < m_taps.size(); ++i) {
            totalMs += std::chrono::duration_cast<std::chrono::milliseconds>(m_taps[i] - m_taps[i-1]).count();
        }
        double avgMs = (double)totalMs / (m_taps.size() - 1);
        double bpm = 60000.0 / avgMs;
        m_lastBpm = std::max(40.0, std::min(240.0, bpm));
    }
}

MetronomePendulum::MetronomePendulum(dsp::AudioEngine* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(140, 180);
    
    connect(&m_timer, &QTimer::timeout, this, [this](){ update(); });
    m_timer.start(16);
}

void MetronomePendulum::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int w = width();
    int h = height();
    
    painter.fillRect(0, 0, w, h, QColor("#000000"));
    
    double cx = w / 2.0;
    double pivot_y = h - 35.0;
    
    QPolygonF body;
    body << QPointF(cx - 15.0, 30.0)
         << QPointF(cx + 15.0, 30.0)
         << QPointF(cx + 55.0, h - 20.0)
         << QPointF(cx - 55.0, h - 20.0);
         
    painter.setBrush(QBrush(QColor("#000000")));
    painter.setPen(QPen(QColor("#222225"), 1.5));
    painter.drawPolygon(body);
    
    QPolygonF facePlate;
    facePlate << QPointF(cx - 10.0, 45.0)
              << QPointF(cx + 10.0, 45.0)
              << QPointF(cx + 38.0, h - 30.0)
              << QPointF(cx - 38.0, h - 30.0);
              
    painter.setBrush(QBrush(QColor("#050505")));
    painter.setPen(QPen(QColor("#222225"), 1));
    painter.drawPolygon(facePlate);
    
    painter.setPen(QPen(QColor("#222225"), 1));
    for (int y_pos = 60; y_pos < (int)(h - 40.0); y_pos += 20) {
        double width_at_y = 15.0 + (y_pos - 45.0) * 0.18;
        painter.drawLine(cx - width_at_y, y_pos, cx + width_at_y, y_pos);
    }
    
    double bpm = m_engine ? m_engine->getBpm() : 120.0;
    double playhead = m_engine ? m_engine->getPlayheadTime() : 0.0;
    bool playActive = m_engine ? (m_engine->isEnginePlaying() || m_engine->isEngineRecording()) : false;
    
    double beatDuration = 60.0 / bpm;
    double beatPhase = std::fmod(playhead / beatDuration, 2.0);
    
    double angleDeg = 0.0;
    if (playActive) {
        angleDeg = 26.0 * std::sin(beatPhase * M_PI);
    }
    double angleRad = angleDeg * M_PI / 180.0;
    
    double rodLen = h - 80.0;
    double rx = cx + rodLen * std::sin(angleRad);
    double ry = pivot_y - rodLen * std::cos(angleRad);
    
    painter.setPen(QPen(QColor("#ffffff"), 2));
    painter.drawLine(cx, pivot_y, rx, ry);
    
    double bpmPercent = (bpm - 40.0) / 200.0;
    double weightDist = 40.0 + (1.0 - bpmPercent) * (rodLen - 60.0);
    
    double wx = cx + weightDist * std::sin(angleRad);
    double wy = pivot_y - weightDist * std::cos(angleRad);
    
    QRectF weightRect(wx - 10.0, wy - 8.0, 20.0, 16.0);
    painter.setBrush(QBrush(QColor("#ffffff")));
    painter.setPen(QPen(QColor("#000000"), 1));
    painter.drawRoundedRect(weightRect, 0, 0);
    
    painter.setBrush(QBrush(QColor("#222225")));
    painter.setPen(QPen(QColor("#333333"), 1));
    painter.drawEllipse(QPointF(cx, pivot_y), 5.0, 5.0);
    
    int timeSigNum = m_engine ? m_engine->getTimeSigNumerator() : 4;
    int beatNumber = (int)(playhead / beatDuration) % timeSigNum;
    double flashDuration = beatDuration * 0.15;
    bool isFlashing = playActive && (std::fmod(playhead, beatDuration) < flashDuration);
    
    double led_y = 15.0;
    QRadialGradient ledGrad(cx, led_y, 10);
    QColor ledColor;
    
    if (isFlashing) {
        if (beatNumber == 0) {
            ledGrad.setColorAt(0, QColor("#ffffff"));
            ledGrad.setColorAt(1, QColor(255, 255, 255, 0));
            ledColor = QColor("#ffffff");
        } else {
            ledGrad.setColorAt(0, QColor("#888888"));
            ledGrad.setColorAt(1, QColor(136, 136, 136, 0));
            ledColor = QColor("#aaaaaa");
        }
    } else {
        ledGrad.setColorAt(0, QColor("#111111"));
        ledGrad.setColorAt(1, QColor(17, 17, 17, 0));
        ledColor = QColor("#222225");
    }
    
    painter.setBrush(QBrush(ledGrad));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(cx, led_y), 12.0, 12.0);
    
    painter.setBrush(QBrush(ledColor));
    painter.drawEllipse(QPointF(cx, led_y), 4.0, 4.0);
}

GuitarMetronomeWidget::GuitarMetronomeWidget(dsp::AudioEngine* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setupUi();
    connect(&m_syncTimer, &QTimer::timeout, this, &GuitarMetronomeWidget::syncValuesToUi);
    m_syncTimer.start(100);
}

void GuitarMetronomeWidget::setupUi()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(15, 10, 15, 10);
    layout->setSpacing(25);
    
    m_pendulum = new MetronomePendulum(m_engine);
    layout->addWidget(m_pendulum);
    
    QVBoxLayout* controlsLayout = new QVBoxLayout();
    controlsLayout->setSpacing(12);
    controlsLayout->setAlignment(Qt::AlignVCenter);
    
    QHBoxLayout* titleRow = new QHBoxLayout();
    QLabel* lblTitle = new QLabel("METRONOME");
    lblTitle->setFont(QFont("Consolas", 11, QFont::Bold));
    lblTitle->setStyleSheet("color: #ffffff; letter-spacing: 1.0px;");
    titleRow->addWidget(lblTitle);
    
    titleRow->addStretch();
    
    m_btnToggle = new QPushButton("ON / OFF");
    m_btnToggle->setCheckable(true);
    m_btnToggle->setChecked(m_engine ? m_engine->isMetronomeEnabled() : false);
    connect(m_btnToggle, &QPushButton::clicked, this, &GuitarMetronomeWidget::onMetronomeToggled);
    updateToggleBtnStyle();
    titleRow->addWidget(m_btnToggle);
    controlsLayout->addLayout(titleRow);
    
    QHBoxLayout* timeSigRow = new QHBoxLayout();
    timeSigRow->setSpacing(8);
    
    QLabel* lblTimeSig = new QLabel("TIME SIG:");
    lblTimeSig->setFont(QFont("Consolas", 9, QFont::Bold));
    lblTimeSig->setStyleSheet("color: #888888;");
    timeSigRow->addWidget(lblTimeSig);
    
    m_comboTimeSig = new QComboBox();
    m_comboTimeSig->addItems({"1/4", "2/4", "3/4", "4/4", "5/4", "6/8", "7/8", "9/8", "12/8"});
    m_comboTimeSig->setStyleSheet(
        "QComboBox { background-color: #000000; border: 1px solid #333333; color: #ffffff; padding: 2px 6px; font-family: 'Consolas', monospace; font-size: 11px; font-weight: bold; }"
        "QComboBox::drop-down { border-left: 1px solid #333333; }"
        "QComboBox QAbstractItemView { background-color: #111111; color: #ffffff; selection-background-color: #333333; }"
    );
    int currentNum = m_engine ? m_engine->getTimeSigNumerator() : 4;
    int idx = m_comboTimeSig->findText(QString("%1/").arg(currentNum), Qt::MatchStartsWith);
    if (idx >= 0) m_comboTimeSig->setCurrentIndex(idx);
    connect(m_comboTimeSig, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GuitarMetronomeWidget::onTimeSigChanged);
    timeSigRow->addWidget(m_comboTimeSig);
    
    timeSigRow->addSpacing(15);
    
    QLabel* lblVol = new QLabel("VOL:");
    lblVol->setFont(QFont("Consolas", 9, QFont::Bold));
    lblVol->setStyleSheet("color: #888888;");
    timeSigRow->addWidget(lblVol);
    
    m_sliderVolume = new QSlider(Qt::Horizontal);
    m_sliderVolume->setRange(0, 100);
    m_sliderVolume->setValue(m_engine ? (int)(m_engine->getMetronomeVolume() * 100) : 80);
    m_sliderVolume->setFixedWidth(60);
    m_sliderVolume->setStyleSheet(
        "QSlider::groove:horizontal { border: 1px solid #222225; height: 6px; background: #000000; }"
        "QSlider::handle:horizontal { background: #ffffff; width: 14px; margin: -4px 0; border-radius: 7px; }"
    );
    connect(m_sliderVolume, &QSlider::valueChanged, this, &GuitarMetronomeWidget::onVolumeSliderChanged);
    timeSigRow->addWidget(m_sliderVolume);
    
    timeSigRow->addStretch();
    
    controlsLayout->addLayout(timeSigRow);
    
    QHBoxLayout* bpmRow = new QHBoxLayout();
    bpmRow->setSpacing(8);
    
    QLabel* lblBpm = new QLabel("BPM:");
    lblBpm->setFont(QFont("Consolas", 9, QFont::Bold));
    lblBpm->setStyleSheet("color: #888888;");
    bpmRow->addWidget(lblBpm);
    
    double initBpm = m_engine ? m_engine->getBpm() : 120.0;
    
    m_spinBpm = new QSpinBox();
    m_spinBpm->setRange(40, 240);
    m_spinBpm->setValue((int)initBpm);
    m_spinBpm->setMinimumWidth(70);
    m_spinBpm->setStyleSheet(
        "QSpinBox { background-color: #000000; border: 1px solid #333333; color: #ffffff; padding: 4px 6px; font-family: 'Consolas', monospace; font-size: 11px; font-weight: bold; }"
        "QSpinBox:focus { border-color: #ffffff; }"
    );
    connect(m_spinBpm, QOverload<int>::of(&QSpinBox::valueChanged), this, &GuitarMetronomeWidget::onBpmSpinChanged);
    bpmRow->addWidget(m_spinBpm);
    
    m_sliderBpm = new QSlider(Qt::Horizontal);
    m_sliderBpm->setRange(40, 240);
    m_sliderBpm->setValue((int)initBpm);
    m_sliderBpm->setStyleSheet(
        "QSlider::groove:horizontal { border: 1px solid #222225; height: 6px; background: #000000; }"
        "QSlider::handle:horizontal { background: #ffffff; width: 14px; margin: -4px 0; border-radius: 7px; }"
    );
    connect(m_sliderBpm, &QSlider::valueChanged, this, &GuitarMetronomeWidget::onBpmSliderChanged);
    bpmRow->addWidget(m_sliderBpm);
    
    m_btnTap = new QPushButton("TAP TEMPO");
    m_btnTap->setStyleSheet(
        "QPushButton { background: #000000; border: 1px solid #333333; color: #ffffff; font-family: 'Consolas', monospace; font-size: 10px; font-weight: bold; padding: 6px; }"
        "QPushButton:hover { background: #111111; border-color: #555555; }"
        "QPushButton:pressed { background: #ffffff; color: #000000; }"
    );
    connect(m_btnTap, &QPushButton::clicked, this, &GuitarMetronomeWidget::onTapClicked);
    controlsLayout->addWidget(m_btnTap);
    
    controlsLayout->addLayout(bpmRow);
    layout->addLayout(controlsLayout);
}

void GuitarMetronomeWidget::updateToggleBtnStyle()
{
    if (m_btnToggle->isChecked()) {
        m_btnToggle->setStyleSheet(
            "QPushButton { background: #ffffff; border: 1px solid #ffffff; color: #000000; font-family: 'Consolas', monospace; font-size: 10px; font-weight: bold; padding: 4px 10px; }"
        );
    } else {
        m_btnToggle->setStyleSheet(
            "QPushButton { background: #000000; border: 1px solid #333333; color: #555555; font-family: 'Consolas', monospace; font-size: 10px; font-weight: bold; padding: 4px 10px; }"
        );
    }
}

void GuitarMetronomeWidget::onMetronomeToggled()
{
    if (m_engine) {
        m_engine->setMetronomeEnabled(m_btnToggle->isChecked());
    }
    updateToggleBtnStyle();
}

void GuitarMetronomeWidget::onBpmSpinChanged(int val)
{
    m_sliderBpm->blockSignals(true);
    m_sliderBpm->setValue(val);
    m_sliderBpm->blockSignals(false);
    
    if (m_engine) m_engine->setBpm((double)val);
}

void GuitarMetronomeWidget::onBpmSliderChanged(int val)
{
    m_spinBpm->blockSignals(true);
    m_spinBpm->setValue(val);
    m_spinBpm->blockSignals(false);
    
    if (m_engine) m_engine->setBpm((double)val);
}

void GuitarMetronomeWidget::onTimeSigChanged(int index)
{
    if (m_engine) {
        QString text = m_comboTimeSig->itemText(index);
        QString numStr = text.split("/").first();
        m_engine->setTimeSigNumerator(numStr.toInt());
    }
}

void GuitarMetronomeWidget::onVolumeSliderChanged(int val)
{
    if (m_engine) m_engine->setMetronomeVolume(val / 100.0f);
}

void GuitarMetronomeWidget::onTapClicked()
{
    m_tapTempo.tap();
    int newBpm = (int)m_tapTempo.getBpm();
    m_spinBpm->setValue(newBpm);
}

void GuitarMetronomeWidget::syncValuesToUi()
{
    if (!m_engine) return;
    
    bool enabled = m_engine->isMetronomeEnabled();
    if (enabled != m_btnToggle->isChecked()) {
        m_btnToggle->setChecked(enabled);
        updateToggleBtnStyle();
    }
    
    int engineBpm = (int)m_engine->getBpm();
    if (engineBpm != m_spinBpm->value() && !m_sliderBpm->isSliderDown()) {
        m_spinBpm->setValue(engineBpm);
    }
}

} // namespace gui
