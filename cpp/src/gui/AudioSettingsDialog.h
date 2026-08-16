#pragma once

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <juce_audio_devices/juce_audio_devices.h>
#include <QPoint>
#include <QMouseEvent>

namespace gui {

class AudioSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit AudioSettingsDialog(juce::AudioDeviceManager& deviceManager, QWidget* parent = nullptr);
    ~AudioSettingsDialog() override;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
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
    
    QComboBox* m_startupActionCombo;
    QWidget* m_templateContainer;
    QLineEdit* m_templatePathEdit;
    QPushButton* m_templateBrowseBtn;

    QPushButton* m_okBtn;
    QPushButton* m_cancelBtn;
    QPushButton* m_applyBtn;
    
    bool m_isInitializing = true;
    QPoint m_dragPos;
};

} // namespace gui
