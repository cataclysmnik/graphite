#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>
#include "../audio/AudioEngine.h"

class ExportDialog : public QDialog
{
    Q_OBJECT
public:
    ExportDialog(dsp::AudioEngine* engine, QWidget* parent = nullptr);
    
    dsp::AudioEngine::RenderOptions getRenderOptions() const;

private slots:
    void browseFile();
    
private:
    dsp::AudioEngine* m_engine;
    
    QLineEdit* m_filePathEdit;
    QPushButton* m_browseBtn;
    
    QComboBox* m_sampleRateCombo;
    QComboBox* m_bitDepthCombo;
    
    QRadioButton* m_boundsEntireProjectBtn;
    QRadioButton* m_boundsTimeSelectionBtn;
    
    QComboBox* m_sourceCombo;
};
