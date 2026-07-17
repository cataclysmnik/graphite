#pragma once

#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>

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
    void onPanChanged(int value);

public:
    void setSelected(bool selected);
    void setTrackIndex(int newIndex) { m_trackIndex = newIndex; }
    
    bool isArmed() const { return m_btnArm->isChecked(); }
    void setArmed(bool armed) { m_btnArm->setChecked(armed); }

private slots:
    void updateMeters();

private:
    int m_trackIndex;
    dsp::AudioEngine* m_engine;

    QLabel* m_nameLabel;
    
    QPushButton* m_btnMute;
    QPushButton* m_btnSolo;
    QPushButton* m_btnArm;
    
    QProgressBar* m_meterL;
    QProgressBar* m_meterR;
    QTimer* m_meterTimer;
};

} // namespace gui
