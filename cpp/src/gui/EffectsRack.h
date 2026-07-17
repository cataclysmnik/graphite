#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QComboBox>
#include <QTimer>
#include <QVBoxLayout>

namespace dsp {
    class AudioEngine;
}

namespace gui {

class EffectsRack : public QWidget
{
    Q_OBJECT
public:
    explicit EffectsRack(dsp::AudioEngine* engine, QWidget* parent = nullptr);
    void updateForTrack(int trackIndex);
    void populatePluginList();
    int getCurrentTrackIndex() const { return m_currentTrackIndex; }
    void openAddEffectCombo();

private slots:
    void onAddEffectActivated(int index);
    void onPollTimer();

private:
    dsp::AudioEngine* m_engine;
    class EffectsListWidget* m_listWidget;
    ::QComboBox* m_addEffectCombo;
    int m_currentTrackIndex = 0;
    QTimer* m_pollTimer;
    int m_lastKnownPluginCount = -1;
};

} // namespace gui
