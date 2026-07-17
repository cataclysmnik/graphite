#pragma once

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <juce_audio_devices/juce_audio_devices.h>

namespace gui {

class AudioSettingsDialog : public QDialog {
public:
    explicit AudioSettingsDialog(juce::AudioDeviceManager& deviceManager, QWidget* parent = nullptr);
    ~AudioSettingsDialog() override;

private:
    void populateDriverTypes();
    void onDriverTypeChanged(int index);
    void populateDevices(juce::AudioIODeviceType* type);
    void onDeviceChanged();
    void onApplyClicked();
    void onAsioConfigClicked();

    juce::AudioDeviceManager& m_deviceManager;

    QComboBox* m_driverTypeCombo;
    QComboBox* m_outputDeviceCombo;
    
    QCheckBox* m_enableInputsCheck;
    QComboBox* m_inputFirstCombo;
    QComboBox* m_inputLastCombo;
    
    QComboBox* m_outputFirstCombo;
    QComboBox* m_outputLastCombo;
    
    QCheckBox* m_requestSampleRateCheck;
    QLineEdit* m_sampleRateEdit;
    
    QCheckBox* m_requestBlockSizeCheck;
    QLineEdit* m_blockSizeEdit;
    
    QPushButton* m_asioConfigBtn;
    
    QCheckBox* m_preZeroBuffersCheck;
    QCheckBox* m_ignoreAsioResetCheck;
    
    QComboBox* m_threadPriorityCombo;
    QCheckBox* m_allowOverrideSampleRateCheck;

    QPushButton* m_okBtn;
    QPushButton* m_cancelBtn;
    QPushButton* m_applyBtn;
};

} // namespace gui
