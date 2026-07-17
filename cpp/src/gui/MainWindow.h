#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QTabWidget>

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
    explicit MainWindow(dsp::AudioEngine* engine, juce::AudioDeviceManager* deviceManager, QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
#ifdef _WIN32
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#endif

private slots:
    void selectTrack(int index);

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
    
    int m_selectedTrackIndex = 0;
};

} // namespace gui
