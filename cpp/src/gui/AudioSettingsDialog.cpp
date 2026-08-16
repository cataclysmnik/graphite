#include "AudioSettingsDialog.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include "CustomMessageBox.h"
#include <QGroupBox>
#include <QSettings>
#include <QFileDialog>

namespace gui {

AudioSettingsDialog::AudioSettingsDialog(juce::AudioDeviceManager& deviceManager, QWidget* parent)
    : QDialog(parent), m_deviceManager(deviceManager)
{
    setWindowTitle("Graphite — Audio Settings");
    resize(600, 500);
    setObjectName("AudioSettingsDialog");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // ── Custom title bar strip ────────────────────────────────────────────────
    QWidget* titleStrip = new QWidget(this);
    titleStrip->setObjectName("DialogTitleStrip");
    titleStrip->setFixedHeight(32);
    QHBoxLayout* titleLayout = new QHBoxLayout(titleStrip);
    titleLayout->setContentsMargins(12, 0, 0, 0);
    titleLayout->setSpacing(0);

    QLabel* titleLabel = new QLabel("GRAPHITE  ·  AUDIO SETTINGS", titleStrip);
    titleLabel->setObjectName("DialogTitleLabel");
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    QPushButton* closeBtn = new QPushButton("✕", titleStrip);
    closeBtn->setObjectName("DialogCloseBtn");
    closeBtn->setFixedSize(32, 32);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    titleLayout->addWidget(closeBtn);

    outerLayout->addWidget(titleStrip);

    // ── Content area ──────────────────────────────────────────────────────────
    QWidget* content = new QWidget(this);
    outerLayout->addWidget(content, 1);

    QVBoxLayout* mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(12, 10, 12, 12);
    mainLayout->setSpacing(6);

    // Audio System
    QHBoxLayout* systemLayout = new QHBoxLayout();
    systemLayout->addWidget(new QLabel("Audio system:", this));
    m_driverTypeCombo = new QComboBox(this);
    systemLayout->addWidget(m_driverTypeCombo, 1);
    mainLayout->addLayout(systemLayout);

    // Group box for the main device options
    QGroupBox* groupBox = new QGroupBox(this);
    QVBoxLayout* groupLayout = new QVBoxLayout(groupBox);

    // Device
    QHBoxLayout* driverLayout = new QHBoxLayout();
    driverLayout->addWidget(new QLabel("Device:", this));
    m_outputDeviceCombo = new QComboBox(this);
    driverLayout->addWidget(m_outputDeviceCombo, 1);
    groupLayout->addLayout(driverLayout);

    // Enable inputs
    m_enableInputsCheck = new QCheckBox("Enable inputs:", this);
    m_enableInputsCheck->setChecked(true);
    groupLayout->addWidget(m_enableInputsCheck);
    
    QFormLayout* inputForm = new QFormLayout();
    inputForm->setContentsMargins(20, 0, 0, 0); // Indent
    m_inputFirstCombo = new QComboBox(this);
    m_inputLastCombo = new QComboBox(this);
    inputForm->addRow("first", m_inputFirstCombo);
    inputForm->addRow("last", m_inputLastCombo);
    groupLayout->addLayout(inputForm);

    // Output range
    groupLayout->addWidget(new QLabel("Output range:", this));
    QFormLayout* outputForm = new QFormLayout();
    outputForm->setContentsMargins(20, 0, 0, 0); // Indent
    m_outputFirstCombo = new QComboBox(this);
    m_outputLastCombo = new QComboBox(this);
    outputForm->addRow("first", m_outputFirstCombo);
    outputForm->addRow("last", m_outputLastCombo);
    groupLayout->addLayout(outputForm);

    // Request Sample rate & Block size
    QHBoxLayout* reqLayout = new QHBoxLayout();
    m_requestSampleRateCheck = new QCheckBox("Request sample rate:", this);
    m_sampleRateEdit = new QLineEdit("44100", this);
    m_sampleRateEdit->setFixedWidth(60);
    reqLayout->addWidget(m_requestSampleRateCheck);
    reqLayout->addWidget(m_sampleRateEdit);
    
    reqLayout->addSpacing(10);

    m_requestBlockSizeCheck = new QCheckBox("Request block size:", this);
    m_blockSizeEdit = new QLineEdit("256", this);
    m_blockSizeEdit->setFixedWidth(60);
    reqLayout->addWidget(m_requestBlockSizeCheck);
    reqLayout->addWidget(m_blockSizeEdit);
    reqLayout->addStretch(1);
    groupLayout->addLayout(reqLayout);

    // ASIO Config Button
    QHBoxLayout* asioConfLayout = new QHBoxLayout();
    m_asioConfigBtn = new QPushButton("ASIO Configuration...", this);
    asioConfLayout->addWidget(m_asioConfigBtn);
    asioConfLayout->addStretch(1);
    groupLayout->addLayout(asioConfLayout);

    // Pre-zero & Ignore reset
    m_preZeroBuffersCheck = new QCheckBox("Pre-zero output buffers, useful on some hardware (higher CPU use)", this);
    m_preZeroBuffersCheck->setChecked(true);
    groupLayout->addWidget(m_preZeroBuffersCheck);

    m_ignoreAsioResetCheck = new QCheckBox("Ignore ASIO reset messages (needed for some buggy drivers)", this);
    m_ignoreAsioResetCheck->setChecked(true);
    groupLayout->addWidget(m_ignoreAsioResetCheck);

    mainLayout->addWidget(groupBox);

    // Thread priority
    QHBoxLayout* threadLayout = new QHBoxLayout();
    threadLayout->addWidget(new QLabel("Audio thread priority:", this));
    m_threadPriorityCombo = new QComboBox(this);
    m_threadPriorityCombo->addItem("ASIO Default / MMCSS Pro Audio / Time Critical");
    m_threadPriorityCombo->addItem("Highest (Recommended)");
    m_threadPriorityCombo->addItem("Normal");
    threadLayout->addWidget(m_threadPriorityCombo, 1);
    mainLayout->addLayout(threadLayout);

    // Override SR
    m_allowOverrideSampleRateCheck = new QCheckBox("Allow projects to override device sample rate", this);
    m_allowOverrideSampleRateCheck->setChecked(true);
    mainLayout->addWidget(m_allowOverrideSampleRateCheck);

    // General App Settings
    QGroupBox* appGroupBox = new QGroupBox("General Application Settings", this);
    QVBoxLayout* appGroupLayout = new QVBoxLayout(appGroupBox);
    
    QHBoxLayout* startupLayout = new QHBoxLayout();
    startupLayout->addWidget(new QLabel("Startup Action:", this));
    m_startupActionCombo = new QComboBox(this);
    m_startupActionCombo->addItem("Empty Project", 0);
    m_startupActionCombo->addItem("Load Last Project", 1);
    m_startupActionCombo->addItem("Load Template", 2);
    startupLayout->addWidget(m_startupActionCombo, 1);
    appGroupLayout->addLayout(startupLayout);
    
    m_templateContainer = new QWidget(this);
    QHBoxLayout* templateLayout = new QHBoxLayout(m_templateContainer);
    templateLayout->setContentsMargins(0, 0, 0, 0);
    templateLayout->addWidget(new QLabel("Template:", this));
    m_templatePathEdit = new QLineEdit(this);
    templateLayout->addWidget(m_templatePathEdit, 1);
    m_templateBrowseBtn = new QPushButton("Browse...", this);
    templateLayout->addWidget(m_templateBrowseBtn);
    appGroupLayout->addWidget(m_templateContainer);
    
    QSettings settings("Graphite Studio", "Graphite DAW");
    int startupAction = settings.value("StartupAction", 0).toInt();
    int idx = m_startupActionCombo->findData(startupAction);
    m_startupActionCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    m_templatePathEdit->setText(settings.value("TemplatePath", "").toString());
    
    m_templateContainer->setVisible(startupAction == 2);
    
    connect(m_startupActionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        int action = m_startupActionCombo->itemData(index).toInt();
        m_templateContainer->setVisible(action == 2);
    });
    
    connect(m_templateBrowseBtn, &QPushButton::clicked, [this]() {
        QString file = QFileDialog::getOpenFileName(this, "Select Template", "", "Graphite Projects (*.graphite)");
        if (!file.isEmpty()) {
            m_templatePathEdit->setText(file);
        }
    });

    mainLayout->addWidget(appGroupBox);

    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    m_okBtn = new QPushButton("OK", this);
    m_cancelBtn = new QPushButton("Cancel", this);
    m_applyBtn = new QPushButton("Apply", this);
    btnLayout->addWidget(m_okBtn);
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_applyBtn);
    mainLayout->addLayout(btnLayout);

    // Initial population
    populateDriverTypes();

    // Connect signals
    connect(m_driverTypeCombo, &QComboBox::currentIndexChanged, this, &AudioSettingsDialog::onDriverTypeChanged);
    connect(m_outputDeviceCombo, &QComboBox::currentIndexChanged, this, &AudioSettingsDialog::onDeviceChanged);
    connect(m_asioConfigBtn, &QPushButton::clicked, this, &AudioSettingsDialog::onAsioConfigClicked);
    
    connect(m_okBtn, &QPushButton::clicked, this, [this]() {
        onApplyClicked();
        if (result() == QDialog::Accepted) accept();
    });
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_applyBtn, &QPushButton::clicked, this, &AudioSettingsDialog::onApplyClicked);

    // Initialize with current device if any
    if (auto* device = m_deviceManager.getCurrentAudioDevice()) {
        QString typeName = QString::fromStdString(device->getTypeName().toStdString());
        int idx = m_driverTypeCombo->findText(typeName);
        if (idx >= 0) m_driverTypeCombo->setCurrentIndex(idx);
        
        m_sampleRateEdit->setText(QString::number(device->getCurrentSampleRate()));
        m_blockSizeEdit->setText(QString::number(device->getCurrentBufferSizeSamples()));
        
        m_requestSampleRateCheck->setChecked(true);
        m_requestBlockSizeCheck->setChecked(true);
    }
    
    m_isInitializing = false;

    setStyleSheet(R"(
        QDialog#AudioSettingsDialog {
            background-color: #0b0b0c;
            color: #cccccc;
            border: 1px solid #2a2a2d;
        }
        QWidget#DialogTitleStrip {
            background-color: #000000;
            border-bottom: 1px solid #1a1a1c;
        }
        QLabel#DialogTitleLabel {
            color: #ffffff;
            background: transparent;
            font-family: "Consolas", monospace;
            font-size: 10px;
            font-weight: bold;
            letter-spacing: 1.5px;
        }
        QPushButton#DialogCloseBtn {
            background: transparent;
            border: none;
            color: #66666a;
            font-size: 11px;
        }
        QPushButton#DialogCloseBtn:hover {
            background-color: #ff0033;
            color: #ffffff;
        }
        QWidget {
            background-color: #0b0b0c;
            color: #cccccc;
            font-family: "Consolas", monospace;
            font-size: 10px;
        }
        QGroupBox {
            border: 1px solid #2a2a2d;
            margin-top: 8px;
            padding-top: 8px;
            color: #88888c;
        }
        QLabel {
            color: #88888c;
        }
        QComboBox {
            background-color: #111114;
            color: #cccccc;
            border: 1px solid #2a2a2d;
            padding: 3px 6px;
            min-height: 22px;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox QAbstractItemView {
            background-color: #111114;
            color: #cccccc;
            selection-background-color: #1f1f22;
            border: 1px solid #2a2a2d;
        }
        QLineEdit {
            background-color: #111114;
            color: #cccccc;
            border: 1px solid #2a2a2d;
            padding: 3px 6px;
        }
        QCheckBox {
            color: #88888c;
            spacing: 6px;
        }
        QCheckBox::indicator {
            width: 13px;
            height: 13px;
            border: 1px solid #444446;
            background-color: #111114;
        }
        QCheckBox::indicator:checked {
            background-color: #ff0033;
            border-color: #ff0033;
        }
        QPushButton {
            background-color: #111114;
            color: #cccccc;
            border: 1px solid #2a2a2d;
            padding: 5px 14px;
            font-family: "Consolas", monospace;
            font-size: 10px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #1f1f22;
            border-color: #ff0033;
            color: #ffffff;
        }
        QPushButton:pressed {
            background-color: #ff0033;
            color: #ffffff;
        }
    )");
}

AudioSettingsDialog::~AudioSettingsDialog() {}

void AudioSettingsDialog::populateDriverTypes()
{
    m_driverTypeCombo->clear();
    const auto& types = m_deviceManager.getAvailableDeviceTypes();
    for (auto* type : types) {
        m_driverTypeCombo->addItem(QString::fromStdString(type->getTypeName().toStdString()));
    }
    
    if (m_driverTypeCombo->count() > 0) {
        onDriverTypeChanged(m_driverTypeCombo->currentIndex());
    }
}

void AudioSettingsDialog::onDriverTypeChanged(int index)
{
    if (index < 0) return;
    
    QString typeName = m_driverTypeCombo->itemText(index);
    const auto& types = m_deviceManager.getAvailableDeviceTypes();
    for (auto* type : types) {
        if (type->getTypeName().toStdString() == typeName.toStdString()) {
            populateDevices(type);
            break;
        }
    }
}

void AudioSettingsDialog::populateDevices(juce::AudioIODeviceType* type)
{
    m_outputDeviceCombo->clear();
    m_inputFirstCombo->clear();
    m_inputLastCombo->clear();
    m_outputFirstCombo->clear();
    m_outputLastCombo->clear();

    if (!type) return;

    type->scanForDevices();
    
    auto outputNames = type->getDeviceNames(false);
    for (const auto& name : outputNames) {
        m_outputDeviceCombo->addItem(QString::fromStdString(name.toStdString()));
    }
    
    // Pre-select currently active devices if they match the type
    if (auto* device = m_deviceManager.getCurrentAudioDevice()) {
        if (device->getTypeName() == type->getTypeName()) {
            int outIdx = m_outputDeviceCombo->findText(QString::fromStdString(device->getName().toStdString()));
            if (outIdx >= 0) {
                m_outputDeviceCombo->setCurrentIndex(outIdx);
            }
        }
    }
    
    onDeviceChanged();
}

void AudioSettingsDialog::onDeviceChanged()
{
    m_inputFirstCombo->clear();
    m_inputLastCombo->clear();
    m_outputFirstCombo->clear();
    m_outputLastCombo->clear();

    QString typeName = m_driverTypeCombo->currentText();
    QString deviceName = m_outputDeviceCombo->currentText();
    
    juce::AudioIODeviceType* type = nullptr;
    for (auto* t : m_deviceManager.getAvailableDeviceTypes()) {
        if (t->getTypeName().toStdString() == typeName.toStdString()) {
            type = t;
            break;
        }
    }
    
    juce::AudioIODevice* deviceToQuery = nullptr;
    std::unique_ptr<juce::AudioIODevice> tempDevice;
    
    if (type) {
        auto* currentDevice = m_deviceManager.getCurrentAudioDevice();
        if (currentDevice && currentDevice->getTypeName() == type->getTypeName() && currentDevice->getName().toStdString() == deviceName.toStdString()) {
            deviceToQuery = currentDevice;
        } else {
            tempDevice.reset(type->createDevice(juce::String(deviceName.toStdString()), juce::String(deviceName.toStdString())));
            deviceToQuery = tempDevice.get();
        }
    }
    
    if (deviceToQuery) {
        auto inNames = deviceToQuery->getInputChannelNames();
        for (int i = 0; i < inNames.size(); ++i) {
            QString label = QString::number(i + 1) + ": " + QString::fromStdString(inNames[i].toStdString());
            m_inputFirstCombo->addItem(label, i);
            m_inputLastCombo->addItem(label, i);
        }
        
        auto outNames = deviceToQuery->getOutputChannelNames();
        for (int i = 0; i < outNames.size(); ++i) {
            QString label = QString::number(i + 1) + ": " + QString::fromStdString(outNames[i].toStdString());
            m_outputFirstCombo->addItem(label, i);
            m_outputLastCombo->addItem(label, i);
        }
        
        bool loadedFromSetup = false;
        if (m_isInitializing && m_deviceManager.getCurrentAudioDevice() == deviceToQuery) {
            auto setup = m_deviceManager.getAudioDeviceSetup();
            if (!setup.useDefaultInputChannels) {
                int firstIn = setup.inputChannels.findNextSetBit(0);
                int lastIn = setup.inputChannels.getHighestBit();
                if (firstIn >= 0 && firstIn < m_inputFirstCombo->count()) m_inputFirstCombo->setCurrentIndex(firstIn);
                if (lastIn >= 0 && lastIn < m_inputLastCombo->count()) m_inputLastCombo->setCurrentIndex(lastIn);
                loadedFromSetup = true;
            }
            if (!setup.useDefaultOutputChannels) {
                int firstOut = setup.outputChannels.findNextSetBit(0);
                int lastOut = setup.outputChannels.getHighestBit();
                if (firstOut >= 0 && firstOut < m_outputFirstCombo->count()) m_outputFirstCombo->setCurrentIndex(firstOut);
                if (lastOut >= 0 && lastOut < m_outputLastCombo->count()) m_outputLastCombo->setCurrentIndex(lastOut);
                loadedFromSetup = true;
            }
        }
        
        if (!loadedFromSetup) {
            if (m_inputFirstCombo->count() > 0) m_inputFirstCombo->setCurrentIndex(0);
            if (m_inputLastCombo->count() > 0) m_inputLastCombo->setCurrentIndex(m_inputLastCombo->count() - 1);
            if (m_outputFirstCombo->count() > 0) m_outputFirstCombo->setCurrentIndex(0);
            if (m_outputLastCombo->count() > 0) m_outputLastCombo->setCurrentIndex(m_outputLastCombo->count() - 1);
        }
    }
}

void AudioSettingsDialog::onAsioConfigClicked()
{
    if (auto* device = m_deviceManager.getCurrentAudioDevice()) {
        if (device->hasControlPanel()) {
            device->showControlPanel();
        } else {
            CustomMessageBox::information(this, "ASIO Configuration", "This device does not have a control panel.");
        }
    }
}

void AudioSettingsDialog::onApplyClicked()
{
    QString typeName = m_driverTypeCombo->currentText();
    QString outName = m_outputDeviceCombo->currentText();

    int sampleRate = m_requestSampleRateCheck->isChecked() ? m_sampleRateEdit->text().toInt() : 0;
    int bufferSize = m_requestBlockSizeCheck->isChecked() ? m_blockSizeEdit->text().toInt() : 0;

    juce::String jTypeName = juce::String(typeName.toStdString());
    juce::String jOutName = juce::String(outName.toStdString());

    m_deviceManager.setCurrentAudioDeviceType(jTypeName, true);

    juce::AudioDeviceManager::AudioDeviceSetup setup = m_deviceManager.getAudioDeviceSetup();
    setup.outputDeviceName = jOutName;
    setup.inputDeviceName = jOutName; // ASIO typically uses the same device for I/O
    
    if (sampleRate > 0) setup.sampleRate = sampleRate;
    if (bufferSize > 0) setup.bufferSize = bufferSize;

    if (m_enableInputsCheck->isChecked() && m_inputFirstCombo->count() > 0) {
        int firstIn = m_inputFirstCombo->currentData().toInt();
        int lastIn = m_inputLastCombo->currentData().toInt();
        if (firstIn > lastIn) std::swap(firstIn, lastIn);
        
        juce::BigInteger inChannels;
        inChannels.setRange(firstIn, lastIn - firstIn + 1, true);
        setup.inputChannels = inChannels;
        setup.useDefaultInputChannels = false;
    } else {
        setup.useDefaultInputChannels = true;
    }

    if (m_outputFirstCombo->count() > 0) {
        int firstOut = m_outputFirstCombo->currentData().toInt();
        int lastOut = m_outputLastCombo->currentData().toInt();
        if (firstOut > lastOut) std::swap(firstOut, lastOut);
        
        juce::BigInteger outChannels;
        outChannels.setRange(firstOut, lastOut - firstOut + 1, true);
        setup.outputChannels = outChannels;
        setup.useDefaultOutputChannels = false;
    } else {
        setup.useDefaultOutputChannels = true;
    }

    juce::String err = m_deviceManager.setAudioDeviceSetup(setup, true);
    
    QSettings settings("Graphite Studio", "Graphite DAW");
    settings.setValue("StartupAction", m_startupActionCombo->currentData().toInt());
    settings.setValue("TemplatePath", m_templatePathEdit->text());
    
    if (err.isNotEmpty()) {
        CustomMessageBox::critical(this, "Audio Error", QString::fromStdString(err.toStdString()));
        setResult(QDialog::Rejected);
    } else {
        setResult(QDialog::Accepted);
    }
}

void AudioSettingsDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void AudioSettingsDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPos);
        event->accept();
    }
}

} // namespace gui
