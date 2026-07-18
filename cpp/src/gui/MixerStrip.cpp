#include "MixerStrip.h"
#include "../audio/AudioEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

namespace gui {

MixerStrip::MixerStrip(int trackIndex, const QString& trackName, dsp::AudioEngine* engine, QWidget* parent)
    : QWidget(parent), m_trackIndex(trackIndex), m_engine(engine)
{
    setFixedWidth(80);
    setObjectName("MixerStrip");
    
    setStyleSheet(R"(
        QWidget#MixerStrip {
            background-color: #111111;
            border-right: 1px solid #222225;
        }
        QPushButton {
            background-color: #222225;
            color: #ffffff;
            border: 1px solid #333333;
            border-radius: 3px;
            padding: 4px;
            font-weight: bold;
        }
        QPushButton:checked {
            background-color: #ff0033;
            border-color: #ff0033;
        }
        QLabel {
            color: #ffffff;
            font-size: 10px;
        }
        /* Vertical Slider */
        QSlider::groove:vertical {
            background: #000000;
            width: 6px;
            border-radius: 3px;
            border: 1px solid #222225;
        }
        QSlider::handle:vertical {
            background: #88888c;
            height: 16px;
            margin: 0 -6px;
            border-radius: 3px;
        }
        QSlider::handle:vertical:hover {
            background: #ffffff;
        }
        /* Horizontal Pan Slider */
        QSlider::groove:horizontal {
            background: #000000;
            height: 4px;
            border-radius: 2px;
            border: 1px solid #222225;
        }
        QSlider::handle:horizontal {
            background: #88888c;
            width: 10px;
            margin: -4px 0;
            border-radius: 2px;
        }
        QSlider::handle:horizontal:hover {
            background: #ffffff;
        QProgressBar {
            background-color: #1a1a1c;
            border: 1px solid #222225;
            border-radius: 2px;
            text-align: center;
        }
        QProgressBar::chunk {
            background-color: #00ff00;
        }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(8);

    // Pan Slider (Top)
    QLabel* panLabel = new QLabel("PAN", this);
    panLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(panLabel);
    
    m_panSlider = new CustomKnob(this);
    m_panSlider->setRange(0, 100);
    m_panSlider->setValue(50);
    m_panSlider->setFixedSize(40, 40);
    mainLayout->addWidget(m_panSlider, 0, Qt::AlignHCenter);
    
    // Middle layout: Level Meter | Fader
    QHBoxLayout* middleLayout = new QHBoxLayout();
    
    // Meters (L and R)
    QVBoxLayout* meterLayout = new QVBoxLayout();
    meterLayout->setSpacing(2);
    
    m_meterL = new LevelMeter(Qt::Vertical, this);
    m_meterL->setFixedWidth(6);
    
    m_meterR = new LevelMeter(Qt::Vertical, this);
    m_meterR->setFixedWidth(6);
    
    QHBoxLayout* lrLayout = new QHBoxLayout();
    lrLayout->setSpacing(1);
    lrLayout->addWidget(m_meterL);
    lrLayout->addWidget(m_meterR);
    meterLayout->addLayout(lrLayout);
    
    // Fader
    m_volFader = new QSlider(Qt::Vertical, this);
    m_volFader->setRange(0, 100);
    m_volFader->setValue(80);
    
    middleLayout->addStretch();
    middleLayout->addLayout(meterLayout);
    middleLayout->addSpacing(8);
    middleLayout->addWidget(m_volFader);
    middleLayout->addStretch();
    
    mainLayout->addLayout(middleLayout, 1); // Give it stretch factor 1
    
    // Mute/Solo
    QHBoxLayout* msLayout = new QHBoxLayout();
    m_btnMute = new QPushButton("M", this);
    m_btnMute->setCheckable(true);
    m_btnMute->setFixedSize(24, 24);
    
    m_btnSolo = new QPushButton("S", this);
    m_btnSolo->setCheckable(true);
    m_btnSolo->setFixedSize(24, 24);
    
    msLayout->addWidget(m_btnMute);
    msLayout->addWidget(m_btnSolo);
    mainLayout->addLayout(msLayout);
    
    // Track Name (Bottom)
    m_nameLabel = new QLabel(trackName, this);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    // Truncate name if too long
    QFontMetrics metrics(m_nameLabel->font());
    QString elidedText = metrics.elidedText(trackName, Qt::ElideRight, 68);
    m_nameLabel->setText(elidedText);
    mainLayout->addWidget(m_nameLabel);
    
    // Connect signals
    connect(m_btnMute, &QPushButton::toggled, this, &MixerStrip::onMuteToggled);
    connect(m_btnSolo, &QPushButton::toggled, this, &MixerStrip::onSoloToggled);
    connect(m_volFader, &QSlider::valueChanged, this, &MixerStrip::onVolumeChanged);
    connect(m_panSlider, &QSlider::valueChanged, this, &MixerStrip::onPanChanged);
    
    m_meterTimer = new QTimer(this);
    connect(m_meterTimer, &QTimer::timeout, this, &MixerStrip::updateMeters);
    m_meterTimer->start(16);
}

void MixerStrip::updateMeters()
{
    if (m_engine && m_engine->isEnginePlaying() && m_trackIndex != -1) {
        float pL = m_engine->getTrackPeakL(m_trackIndex);
        float pR = m_engine->getTrackPeakR(m_trackIndex);
        
        m_meterL->setLevel(pL);
        m_meterR->setLevel(pR);
        
        // Synchronize pan state from engine
        float enginePan = m_engine->getTrackPan(m_trackIndex);
        int panVal = static_cast<int>((enginePan * 50.0f) + 50.0f);
        m_panSlider->blockSignals(true);
        m_panSlider->setValue(panVal);
        m_panSlider->blockSignals(false);
    } else {
        m_meterL->setLevel(0.0f);
        m_meterR->setLevel(0.0f);
    }
}

void MixerStrip::mousePressEvent(QMouseEvent* event)
{
    emit clicked(m_trackIndex);
    QWidget::mousePressEvent(event);
}

void MixerStrip::setSelected(bool selected)
{
    if (selected) {
        setStyleSheet(styleSheet() + " QWidget#MixerStrip { border-right: 1px solid #ff0033; background-color: #1a1a1c; }");
    } else {
        setStyleSheet(styleSheet() + " QWidget#MixerStrip { border-right: 1px solid #222225; background-color: #111111; }");
    }
}

void MixerStrip::onMuteToggled(bool checked)
{
    if (m_engine) {
        dsp::EngineMessage msg;
        msg.type = dsp::EngineCommandType::SetTrackMute;
        msg.trackIndex = m_trackIndex;
        msg.boolValue = checked;
        m_engine->sendMessageFromUI(msg);
    }
}

void MixerStrip::onSoloToggled(bool checked)
{
    if (m_engine) {
        dsp::EngineMessage msg;
        msg.type = dsp::EngineCommandType::SetTrackSolo;
        msg.trackIndex = m_trackIndex;
        msg.boolValue = checked;
        m_engine->sendMessageFromUI(msg);
    }
}

void MixerStrip::onArmToggled(bool checked)
{
    if (m_engine) {
        dsp::EngineMessage msg;
        msg.type = dsp::EngineCommandType::SetTrackArm;
        msg.trackIndex = m_trackIndex;
        msg.boolValue = checked;
        m_engine->sendMessageFromUI(msg);
    }
}

void MixerStrip::onVolumeChanged(int value)
{
    if (m_engine) {
        dsp::EngineMessage msg;
        msg.type = dsp::EngineCommandType::SetTrackVolume;
        msg.trackIndex = m_trackIndex;
        msg.floatValue = value / 100.0f; // Assuming 0-100 range
        m_engine->sendMessageFromUI(msg);
    }
}

void MixerStrip::onPanChanged(int value)
{
    if (m_engine) {
        dsp::EngineMessage msg;
        msg.type = dsp::EngineCommandType::SetTrackPan;
        msg.trackIndex = m_trackIndex;
        msg.floatValue = (value - 50) / 50.0f; // Assuming 0-100 range, where 50 is center
        m_engine->sendMessageFromUI(msg);
    }
}

} // namespace gui
