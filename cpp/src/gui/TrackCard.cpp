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
    
    headerLayout->addWidget(m_btnMute);
    headerLayout->addWidget(m_btnSolo);
    headerLayout->addWidget(m_btnArm);
    
    mainLayout->addLayout(headerLayout);

    // No volume slider here anymore
    
    // Meters
    QHBoxLayout* meterLayout = new QHBoxLayout();
    m_meterL = new QProgressBar(this);
    m_meterL->setRange(0, 100);
    m_meterL->setFixedHeight(4);
    m_meterL->setTextVisible(false);
    
    m_meterR = new QProgressBar(this);
    m_meterR->setRange(0, 100);
    m_meterR->setFixedHeight(4);
    m_meterR->setTextVisible(false);
    
    meterLayout->addWidget(m_meterL);
    meterLayout->addWidget(m_meterR);
    mainLayout->addLayout(meterLayout);
    
    // Connect signals
    connect(m_btnMute, &QPushButton::toggled, this, &TrackCard::onMuteToggled);
    connect(m_btnSolo, &QPushButton::toggled, this, &TrackCard::onSoloToggled);
    connect(m_btnArm, &QPushButton::toggled, this, &TrackCard::onArmToggled);
    
    m_meterTimer = new QTimer(this);
    connect(m_meterTimer, &QTimer::timeout, this, &TrackCard::updateMeters);
    m_meterTimer->start(50);
}

void TrackCard::updateMeters()
{
    if (m_engine && m_engine->isEnginePlaying()) {
        float pL = m_engine->getTrackPeakL(m_trackIndex);
        float pR = m_engine->getTrackPeakR(m_trackIndex);
        
        m_meterL->setValue(static_cast<int>(pL * 100.0f));
        m_meterR->setValue(static_cast<int>(pR * 100.0f));
    } else {
        m_meterL->setValue(0);
        m_meterR->setValue(0);
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
        setStyleSheet(styleSheet() + " QWidget#TrackCard { border: 1px solid #ff0033; background-color: #1a1a1c; }");
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
