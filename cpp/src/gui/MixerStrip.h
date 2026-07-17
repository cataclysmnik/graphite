#pragma once

#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QProgressBar>

namespace dsp {
    class AudioEngine;
}

namespace gui {

class MixerStrip : public QWidget
{
    Q_OBJECT
public:
    explicit MixerStrip(int trackIndex, const QString& trackName, dsp::AudioEngine* engine, QWidget* parent = nullptr);

signals:
    void clicked(int index);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void onMuteToggled(bool checked);
    void onSoloToggled(bool checked);
    void onArmToggled(bool checked);
    void onVolumeChanged(int value);
    void onPanChanged(int value);

public:
    void setSelected(bool selected);

private:
    int m_trackIndex;
    dsp::AudioEngine* m_engine;

    QLabel* m_nameLabel;
    QSlider* m_volFader;
    QSlider* m_panSlider;
    
    QPushButton* m_btnMute;
    QPushButton* m_btnSolo;
    
    QWidget* m_meterPlaceholder;
};

} // namespace gui
