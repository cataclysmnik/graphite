#include "TrackCard.h"
#include "../audio/AudioEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>

namespace gui {

TrackCard::TrackCard(int trackIndex, const QString& trackName, dsp::AudioEngine* engine, QWidget* parent)
    : QWidget(parent), m_trackIndex(trackIndex), m_engine(engine)
{
    setFixedHeight(100);
    setObjectName("TrackCard");
    
    // Style specific to TrackCard
    setStyleSheet(R"(
        QWidget#TrackCard {
            background-color: #0b0b0c;
            border: 1px solid #222225;
            border-radius: 4px;
            margin-bottom: 4px;
        }
        QPushButton {
            min-width: 24px;
            padding: 2px 4px;
        }
        QPushButton:checked {
            background-color: #ff0033;
            border-color: #ff0033;
            color: #ffffff;
        }
        QSlider::groove:horizontal {
            border: 1px solid #222225;
            height: 4px;
            background: #000000;
        }
        QSlider::handle:horizontal {
            background: #88888c;
            width: 10px;
            margin: -4px 0;
            border-radius: 2px;
        }
        QSlider::handle:horizontal:hover {
            background: #ffffff;
        }
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
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Header (Name + Mute/Solo/Arm)
    QHBoxLayout* headerLayout = new QHBoxLayout();
    m_nameLabel = new QLabel(trackName, this);
    m_nameLabel->setStyleSheet("font-weight: bold; color: #ffffff;");
    headerLayout->addWidget(m_nameLabel);
    
    headerLayout->addStretch();
    
    m_btnMute = new QPushButton("M", this);
    m_btnMute->setCheckable(true);
    m_btnSolo = new QPushButton("S", this);
    m_btnSolo->setCheckable(true);
    m_btnArm = new QPushButton(QChar(0x25CF), this); // Record circle
    m_btnArm->setCheckable(true);
    
    m_panDial = new CustomKnob(this);
    m_panDial->setRange(0, 100);
    m_panDial->setValue(50);
    m_panDial->setFixedSize(30, 30);
    m_panDial->setToolTip("Pan");
    
    headerLayout->addWidget(m_btnMute);
    headerLayout->addWidget(m_btnSolo);
    headerLayout->addWidget(m_btnArm);
    headerLayout->addWidget(m_panDial);
    
    mainLayout->addLayout(headerLayout);

    // No volume slider here anymore
    
    // Meters
    QVBoxLayout* meterLayout = new QVBoxLayout();
    meterLayout->setSpacing(2);
    m_meterL = new LevelMeter(Qt::Horizontal, this);
    m_meterL->setFixedHeight(6);
    
    m_meterR = new LevelMeter(Qt::Horizontal, this);
    m_meterR->setFixedHeight(6);
    
    meterLayout->addWidget(m_meterL);
    meterLayout->addWidget(m_meterR);
    mainLayout->addLayout(meterLayout);
    
    // Connect signals
    connect(m_panDial, &QDial::valueChanged, this, &TrackCard::onPanChanged);
    connect(m_btnMute, &QPushButton::toggled, this, &TrackCard::onMuteToggled);
    connect(m_btnSolo, &QPushButton::toggled, this, &TrackCard::onSoloToggled);
    connect(m_btnArm, &QPushButton::toggled, this, &TrackCard::onArmToggled);
    
    m_meterTimer = new QTimer(this);
    connect(m_meterTimer, &QTimer::timeout, this, &TrackCard::updateMeters);
    m_meterTimer->start(16);
}

void TrackCard::updateMeters()
{
    if (m_engine && m_engine->isEnginePlaying()) {
        float pL = m_engine->getTrackPeakL(m_trackIndex);
        float pR = m_engine->getTrackPeakR(m_trackIndex);
        
        m_meterL->setLevel(pL);
        m_meterR->setLevel(pR);
        
        // Synchronize pan state from engine
        float enginePan = m_engine->getTrackPan(m_trackIndex); // -1.0 to 1.0
        int panVal = static_cast<int>((enginePan * 50.0f) + 50.0f);
        m_panDial->blockSignals(true);
        m_panDial->setValue(panVal);
        m_panDial->blockSignals(false);
    } else {
        m_meterL->setLevel(0.0f);
        m_meterR->setLevel(0.0f);
    }
}

void TrackCard::mousePressEvent(QMouseEvent* event)
{
    emit clicked(m_trackIndex);
    event->ignore(); // allow QListWidget to process drag
}

void TrackCard::setSelected(bool selected)
{
    if (selected) {
        setStyleSheet(styleSheet() + " QWidget#TrackCard { border: 2px solid #00ff00; background-color: #1a1a1c; }");
    } else {
        setStyleSheet(styleSheet() + " QWidget#TrackCard { border: none; border-bottom: 1px solid #222225; background-color: #111111; }");
    }
}

void TrackCard::onMuteToggled(bool checked)
{
    if (m_engine) {
        dsp::EngineMessage msg;
        msg.type = dsp::EngineCommandType::SetTrackMute;
        msg.trackIndex = m_trackIndex;
        msg.boolValue = checked;
        m_engine->sendMessageFromUI(msg);
    }
}

void TrackCard::onSoloToggled(bool checked)
{
    if (m_engine) {
        dsp::EngineMessage msg;
        msg.type = dsp::EngineCommandType::SetTrackSolo;
        msg.trackIndex = m_trackIndex;
        msg.boolValue = checked;
        m_engine->sendMessageFromUI(msg);
    }
}

void TrackCard::onArmToggled(bool checked)
{
    if (m_engine) {
        dsp::EngineMessage msg;
        msg.type = dsp::EngineCommandType::SetTrackArm;
        msg.trackIndex = m_trackIndex;
        msg.boolValue = checked;
        m_engine->sendMessageFromUI(msg);
    }
}

void TrackCard::onPanChanged(int value)
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
