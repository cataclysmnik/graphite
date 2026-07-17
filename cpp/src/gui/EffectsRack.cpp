#include "EffectsRack.h"
#include "../audio/AudioEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QFrame>
#include <QFileDialog>

namespace gui {

EffectsRack::EffectsRack(dsp::AudioEngine* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setObjectName("EffectsRack");
    setStyleSheet(R"(
        QWidget#EffectsRack {
            background-color: #0b0b0c;
            border-right: 1px solid #222225;
        }
        QComboBox {
            background-color: #111111;
            color: #ffffff;
            border: 1px solid #333333;
            border-radius: 4px;
            padding: 4px;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox QAbstractItemView {
            background-color: #111111;
            color: #ffffff;
            selection-background-color: #ff0033;
        }
        .EffectBlock {
            background-color: #111111;
            border: 1px solid #222225;
            border-radius: 4px;
            margin-bottom: 4px;
        }
        .EffectHeader {
            background-color: #1a1a1c;
            border-bottom: 1px solid #222225;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("EFFECTS", this);
    titleLabel->setStyleSheet("color: #88888c; font-weight: bold; font-size: 11px;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    
    m_addEffectCombo = new QComboBox(this);
    populatePluginList();
    headerLayout->addWidget(m_addEffectCombo);
    
    connect(m_addEffectCombo, QOverload<int>::of(&QComboBox::activated), this, &EffectsRack::onAddEffectActivated);
    
    mainLayout->addLayout(headerLayout);

    // Timer to poll for background scan updates
    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &EffectsRack::onPollTimer);
    m_pollTimer->start(1000); // Check every second

    // Scroll Area for Effects
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet("QScrollArea { background: transparent; }");
    
    m_scrollContainer = new QWidget();
    m_scrollLayout = new QVBoxLayout(m_scrollContainer);
    m_scrollLayout->setContentsMargins(0, 0, 0, 0);
    m_scrollLayout->setSpacing(8);
    m_scrollLayout->addStretch();
    
    m_scrollArea->setWidget(m_scrollContainer);
    mainLayout->addWidget(m_scrollArea);
}

void EffectsRack::updateForTrack(int trackIndex)
{
    m_currentTrackIndex = trackIndex;
    
    if (!m_engine) return;
    
    // Clear layout
    QLayoutItem* item;
    while ((item = m_scrollLayout->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    
    std::vector<std::string> plugins = m_engine->getTrackPluginNames(trackIndex);
    for (int i = 0; i < plugins.size(); ++i) {
        QFrame* block = new QFrame();
        block->setProperty("class", "EffectBlock");
        QVBoxLayout* layout = new QVBoxLayout(block);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        
        QWidget* header = new QWidget();
        header->setProperty("class", "EffectHeader");
        header->setFixedHeight(24);
        QHBoxLayout* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(8, 0, 8, 0);
        
        QPushButton* powerBtn = new QPushButton("O");
        powerBtn->setFixedSize(16, 16);
        powerBtn->setCheckable(true);
        powerBtn->setChecked(true);
        powerBtn->setStyleSheet("QPushButton:checked { color: #ff0033; border: none; background: transparent; } QPushButton { color: #88888c; border: none; background: transparent; }");
        
        QLabel* title = new QLabel(QString::fromStdString(plugins[i]));
        title->setStyleSheet("color: #ffffff; font-size: 11px;");
        
        QPushButton* editBtn = new QPushButton("Edit");
        editBtn->setStyleSheet("QPushButton { color: #88888c; font-size: 10px; background: transparent; border: none; } QPushButton:hover { color: #ffffff; }");
        
        connect(editBtn, &QPushButton::clicked, [this, i]() {
            m_engine->openPluginEditor(m_currentTrackIndex, i);
        });
        
        headerLayout->addWidget(powerBtn);
        headerLayout->addWidget(title);
        headerLayout->addStretch();
        headerLayout->addWidget(editBtn);
        
        layout->addWidget(header);
        m_scrollLayout->addWidget(block);
    }
    
    m_scrollLayout->addStretch();
}

void EffectsRack::populatePluginList()
{
    m_addEffectCombo->clear();
    m_addEffectCombo->addItem("+ Add Effect");
    m_addEffectCombo->addItem("Neural Amp Modeler (Built-in)", "NAM_BUILTIN");
    m_addEffectCombo->addItem("Browse VST3 Plugin...");
    m_addEffectCombo->addItem("--- Update Scanned Plugins ---");
    
    if (m_engine) {
        const auto& list = m_engine->getKnownPluginList();
        m_lastKnownPluginCount = list.getNumTypes();
        for (const auto& type : list.getTypes()) {
            juce::String nameStr = type.name.isEmpty() ? type.fileOrIdentifier : type.name;
            m_addEffectCombo->addItem(QString::fromStdString(nameStr.toStdString()), QString::fromStdString(type.fileOrIdentifier.toStdString()));
        }
    }
}

void EffectsRack::onPollTimer()
{
    if (m_engine) {
        const auto& list = m_engine->getKnownPluginList();
        if (list.getNumTypes() != m_lastKnownPluginCount) {
            populatePluginList();
        }
    }
}

void EffectsRack::onAddEffectActivated(int index)
{
    if (index == 0) return; // Placeholder
    
    QString text = m_addEffectCombo->itemText(index);
    QString userData = m_addEffectCombo->itemData(index).toString();

    if (text == "Browse VST3 Plugin...") {
        QString fileName = QFileDialog::getOpenFileName(this, "Select VST3 Plugin", "", "VST3 Plugins (*.vst3)");
        if (!fileName.isEmpty()) {
            m_engine->loadPluginSynchronous(m_currentTrackIndex, juce::String(fileName.toUtf8().constData()));
        }
    }
    else if (text == "--- Update Scanned Plugins ---") {
        m_engine->triggerPluginScan();
    }
    else {
        // Load the selected plugin by its identifier (stored in userData)
        m_engine->loadPluginSynchronous(m_currentTrackIndex, juce::String(userData.toUtf8().constData()));
    }
    
    m_addEffectCombo->setCurrentIndex(0);
}

} // namespace gui
