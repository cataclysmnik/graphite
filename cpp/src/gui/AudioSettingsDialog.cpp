#include "AudioSettingsDialog.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QGroupBox>

namespace gui {

AudioSettingsDialog::AudioSettingsDialog(juce::AudioDeviceManager& deviceManager, QWidget* parent)
    : QDialog(parent), m_deviceManager(deviceManager)
{
    setWindowTitle("REAPER Preferences");
    resize(600, 500);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Audio device settings label
    QLabel* headerLabel = new QLabel("Audio device settings", this);
    mainLayout->addWidget(headerLabel);

    // Audio System
    QHBoxLayout* systemLayout = new QHBoxLayout();
    systemLayout->addWidget(new QLabel("Audio system:", this));
    m_driverTypeCombo = new QComboBox(this);
    systemLayout->addWidget(m_driverTypeCombo, 1);
    mainLayout->addLayout(systemLayout);

    // Group box for the main options (to match the gray box in reaper)
    QGroupBox* groupBox = new QGroupBox(this);
    QVBoxLayout* groupLayout = new QVBoxLayout(groupBox);

    // ASIO Driver
    QHBoxLayout* driverLayout = new QHBoxLayout();
    driverLayout->addWidget(new QLabel("ASIO Driver:", this));
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
    m_deviceManager.setCurrentAudioDeviceType(juce::String(typeName.toStdString()), true);
    
    if (auto* device = m_deviceManager.getCurrentAudioDevice()) {
        auto inNames = device->getInputChannelNames();
        for (int i = 0; i < inNames.size(); ++i) {
            QString label = QString::number(i + 1) + ": " + QString::fromStdString(inNames[i].toStdString());
            m_inputFirstCombo->addItem(label, i);
            m_inputLastCombo->addItem(label, i);
        }
        
        auto outNames = device->getOutputChannelNames();
        for (int i = 0; i < outNames.size(); ++i) {
            QString label = QString::number(i + 1) + ": " + QString::fromStdString(outNames[i].toStdString());
            m_outputFirstCombo->addItem(label, i);
            m_outputLastCombo->addItem(label, i);
        }
        
        if (m_inputFirstCombo->count() > 0) m_inputFirstCombo->setCurrentIndex(0);
        if (m_inputLastCombo->count() > 0) m_inputLastCombo->setCurrentIndex(m_inputLastCombo->count() - 1);
        
        if (m_outputFirstCombo->count() > 0) m_outputFirstCombo->setCurrentIndex(0);
        if (m_outputLastCombo->count() > 0) m_outputLastCombo->setCurrentIndex(m_outputLastCombo->count() - 1);
    }
}

void AudioSettingsDialog::onAsioConfigClicked()
{
    if (auto* device = m_deviceManager.getCurrentAudioDevice()) {
        if (device->hasControlPanel()) {
            device->showControlPanel();
        } else {
            QMessageBox::information(this, "ASIO Configuration", "This device does not have a control panel.");
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
    
    if (err.isNotEmpty()) {
        QMessageBox::critical(this, "Audio Error", QString::fromStdString(err.toStdString()));
        setResult(QDialog::Rejected);
    } else {
        setResult(QDialog::Accepted);
    }
}

} // namespace gui
