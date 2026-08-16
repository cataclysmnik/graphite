#include "ExportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFileDialog>
#include <QGroupBox>
#include <QStandardPaths>
#include "StyleTheme.h"

ExportDialog::ExportDialog(dsp::AudioEngine* engine, QWidget* parent)
    : QDialog(parent), m_engine(engine)
{
    setWindowTitle("Render to File");
    setMinimumWidth(450);
    setStyleSheet(gui::StyleTheme::getGlobalStyleSheet());
    
    auto* mainLayout = new QVBoxLayout(this);
    
    // --- File Settings ---
    auto* fileGroup = new QGroupBox("Output Settings");
    auto* fileLayout = new QGridLayout(fileGroup);
    
    m_filePathEdit = new QLineEdit();
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    m_filePathEdit->setText(defaultDir + "/GraphiteRender.wav");
    
    m_browseBtn = new QPushButton("Browse...");
    connect(m_browseBtn, &QPushButton::clicked, this, &ExportDialog::browseFile);
    
    fileLayout->addWidget(new QLabel("File Name:"), 0, 0);
    fileLayout->addWidget(m_filePathEdit, 0, 1);
    fileLayout->addWidget(m_browseBtn, 0, 2);
    
    m_sourceCombo = new QComboBox();
    m_sourceCombo->addItem("Master mix", -1);
    auto tracks = m_engine->getTracksSnapshot();
    for (const auto& t : tracks) {
        QString trackName = QString::fromStdString(t.name);
        if (trackName.isEmpty()) trackName = QString("Track %1").arg(t.id + 1);
        m_sourceCombo->addItem(trackName, t.id);
    }
    fileLayout->addWidget(new QLabel("Source:"), 1, 0);
    fileLayout->addWidget(m_sourceCombo, 1, 1, 1, 2);
    
    mainLayout->addWidget(fileGroup);
    
    // --- Bounds Settings ---
    auto* boundsGroup = new QGroupBox("Bounds");
    auto* boundsLayout = new QVBoxLayout(boundsGroup);
    
    m_boundsEntireProjectBtn = new QRadioButton("Entire project");
    m_boundsTimeSelectionBtn = new QRadioButton("Time selection");
    
    m_boundsEntireProjectBtn->setChecked(true);
    if (m_engine->getLoopEnd() <= m_engine->getLoopStart() + 0.01) {
        m_boundsTimeSelectionBtn->setEnabled(false);
    } else {
        m_boundsTimeSelectionBtn->setChecked(true);
    }
    
    boundsLayout->addWidget(m_boundsEntireProjectBtn);
    boundsLayout->addWidget(m_boundsTimeSelectionBtn);
    
    mainLayout->addWidget(boundsGroup);
    
    // --- Format Settings ---
    auto* formatGroup = new QGroupBox("Format");
    auto* formatLayout = new QGridLayout(formatGroup);
    
    m_sampleRateCombo = new QComboBox();
    m_sampleRateCombo->addItem("44100", 44100.0);
    m_sampleRateCombo->addItem("48000", 48000.0);
    m_sampleRateCombo->addItem("88200", 88200.0);
    m_sampleRateCombo->addItem("96000", 96000.0);
    m_sampleRateCombo->setCurrentIndex(0);
    
    m_bitDepthCombo = new QComboBox();
    m_bitDepthCombo->addItem("16 bit PCM", 16);
    m_bitDepthCombo->addItem("24 bit PCM", 24);
    m_bitDepthCombo->addItem("32 bit FP", 32);
    m_bitDepthCombo->setCurrentIndex(1); // 24 bit default
    
    formatLayout->addWidget(new QLabel("Sample rate:"), 0, 0);
    formatLayout->addWidget(m_sampleRateCombo, 0, 1);
    
    formatLayout->addWidget(new QLabel("WAV bit depth:"), 1, 0);
    formatLayout->addWidget(m_bitDepthCombo, 1, 1);
    
    mainLayout->addWidget(formatGroup);
    
    // --- Buttons ---
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* cancelBtn = new QPushButton("Cancel");
    auto* renderBtn = new QPushButton("Render 1 file...");
    
    renderBtn->setDefault(true);
    
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(renderBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(renderBtn);
    
    mainLayout->addLayout(btnLayout);
}

void ExportDialog::browseFile()
{
    QString path = QFileDialog::getSaveFileName(this, "Save Render File", m_filePathEdit->text(), "WAV Files (*.wav)");
    if (!path.isEmpty()) {
        m_filePathEdit->setText(path);
    }
}

dsp::AudioEngine::RenderOptions ExportDialog::getRenderOptions() const
{
    dsp::AudioEngine::RenderOptions opt;
    opt.outputPath = m_filePathEdit->text().toStdString();
    opt.sampleRate = m_sampleRateCombo->currentData().toDouble();
    opt.bitDepth = m_bitDepthCombo->currentData().toInt();
    opt.renderStems = false; // obsolete with trackIdToRender
    opt.trackIdToRender = m_sourceCombo->currentData().toInt();
    
    if (m_boundsTimeSelectionBtn->isChecked()) {
        opt.startTimeSecs = m_engine->getLoopStart();
        opt.endTimeSecs = m_engine->getLoopEnd();
    } else {
        // Entire project (or entire selected track)
        opt.startTimeSecs = 0.0;
        double maxTime = 0.0;
        auto tracks = m_engine->getTracksSnapshot();
        for (const auto& t : tracks) {
            if (opt.trackIdToRender >= 0 && t.id != opt.trackIdToRender) continue;
            
            for (const auto& i : t.items) {
                if (i.startTimeSecs + i.durationSecs > maxTime) {
                    maxTime = i.startTimeSecs + i.durationSecs;
                }
            }
        }
        opt.endTimeSecs = maxTime + 1.0; // 1 second tail
    }
    
    return opt;
}
