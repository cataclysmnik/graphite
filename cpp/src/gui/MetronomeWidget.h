#pragma once

#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QTimer>
#include <vector>
#include <chrono>

namespace dsp {
    class AudioEngine;
}

namespace gui {

class MetronomePendulum : public QWidget
{
    Q_OBJECT
public:
    explicit MetronomePendulum(dsp::AudioEngine* engine, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    dsp::AudioEngine* m_engine;
    QTimer m_timer;
};

class TapTempo
{
public:
    void tap();
    double getBpm() const { return m_lastBpm; }
    
private:
    std::vector<std::chrono::time_point<std::chrono::steady_clock>> m_taps;
    double m_lastBpm { 120.0 };
};

class GuitarMetronomeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GuitarMetronomeWidget(dsp::AudioEngine* engine, QWidget* parent = nullptr);

private slots:
    void syncValuesToUi();
    void onMetronomeToggled();
    void onBpmSpinChanged(int val);
    void onBpmSliderChanged(int val);
    void onTapClicked();
    void onTimeSigChanged(int index);
    void onVolumeSliderChanged(int val);

private:
    void setupUi();
    void updateToggleBtnStyle();

    dsp::AudioEngine* m_engine;
    TapTempo m_tapTempo;
    QTimer m_syncTimer;
    
    MetronomePendulum* m_pendulum;
    QPushButton* m_btnToggle;
    QSpinBox* m_spinBpm;
    QSlider* m_sliderBpm;
    QPushButton* m_btnTap;
    QComboBox* m_comboTimeSig;
    QSlider* m_sliderVolume;
};

} // namespace gui
