#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QTabWidget>
#include <memory>
#include <vector>
#include <QPushButton>
#include <QButtonGroup>

namespace dsp {
    class AudioEngine;
}

namespace juce {
    class AudioDeviceManager;
}

namespace gui {

class CustomTitleBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    enum class ArmMode {
        Standard,
        Union,
        Exclusive
    };

    explicit MainWindow(dsp::AudioEngine* engine, juce::AudioDeviceManager* deviceManager, QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
#ifdef _WIN32
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#endif

private slots:
    void selectTrack(int index);
    void openAudioSettings();
    void reorderTracks(int fromIndex, int toIndex);
    void togglePlayback();
    void toggleRecording();
    void zoomIn();
    void zoomOut();

private:
    void setupUi();
    void setupMenus();
    void enforceDarkImmersiveMode();

    dsp::AudioEngine* m_engine;
    juce::AudioDeviceManager* m_deviceManager;
    
    // Core Layout Widgets
    CustomTitleBar* m_titleBar;
    QSplitter* m_mainSplitter;
    QSplitter* m_topWorkspace;
    QWidget* m_tcpPanel;
    QTabWidget* m_bottomDock;
    
    // Track references for selection syncing
    std::vector<class TrackCard*> m_trackCards;
    std::vector<class MixerStrip*> m_mixerStrips;
    class EffectsRack* m_effectsRack;
    class TimelineContainer* m_timeline;
    
    int m_selectedTrackIndex = 0;
    
    QButtonGroup* m_armModeGroup;
    QPushButton* m_btnPlayPause;
    QPushButton* m_btnRecord;
    
    bool m_isRecording = false;
    bool m_isPlaying = false;
    
    ArmMode m_armMode = ArmMode::Standard;
};

} // namespace gui
