#pragma once

#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QPushButton>

namespace dsp {
    class AudioEngine;
}

namespace gui {

class TrackCard : public QWidget
{
    Q_OBJECT
public:
    explicit TrackCard(int trackIndex, const QString& trackName, dsp::AudioEngine* engine, QWidget* parent = nullptr);

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
    QSlider* m_volSlider;
    
    QPushButton* m_btnMute;
    QPushButton* m_btnSolo;
    QPushButton* m_btnArm;
};

} // namespace gui
