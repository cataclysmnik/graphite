#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <array>
#include "AudioModels.h"
#include "NamEffect.h"

namespace dsp {

class AudioEngine : public juce::AudioIODeviceCallback, public juce::Timer {
public:
    AudioEngine();
    ~AudioEngine() override;

    // JUCE AudioIODeviceCallback overrides
    void audioDeviceIOCallbackWithContext (
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    // GUI interactions (called from Qt Main Thread)
    void sendMessageFromUI(const EngineMessage& msg);
    void setPlaying(bool shouldPlay);
    bool isEnginePlaying() const { return isPlaying.load(); }
    double getPlayheadTime() const { return playheadTimeSeconds.load(); }
    
    // Track State Getters (thread safe-ish for UI)
    int getSelectedTrackIndex() const { return selectedTrackIndex.load(); }
    float getTrackPeakL(int trackIndex) const;
    float getTrackPeakR(int trackIndex) const;
    
    // Plugin Management getters for UI
    const juce::KnownPluginList& getKnownPluginList() const { return knownPluginList; }
    void triggerPluginScan() { scanForPlugins(); }
    
    // UI can call this (carefully) to display plugin names in the effects rack
    std::vector<std::string> getTrackPluginNames(int trackIndex) const;
    void openPluginEditor(int trackIndex, int pluginIndex);
    
    // Call this on the UI thread to instantiate VST3s safely before sending to audio thread
    void loadPluginSynchronous(int trackIndex, const juce::String& identifierOrPath);
    void movePluginSynchronous(int trackIndex, int fromIndex, int toIndex);
    void deletePluginSynchronous(int trackIndex, int pluginIndex);
    
    // Track management
    void moveTrackSynchronous(int fromIndex, int toIndex);
    
private:
    void processMessages();

    // Lock-free queue (Single Producer, Single Consumer)
    static constexpr int FIFO_SIZE = 1024;
    juce::AbstractFifo messageFifo { FIFO_SIZE };
    std::array<EngineMessage, FIFO_SIZE> messageBuffer;

    // Engine State
    std::atomic<bool> isPlaying { false };
    std::atomic<double> currentSampleRate { 44100.0 };
    std::atomic<double> playheadTimeSeconds { 0.0 };
    std::atomic<int> selectedTrackIndex { 0 };
    
    // Concurrency for plugins
    std::mutex m_pluginMutex;
    
    // Tracks
    std::vector<Track> tracks;
    std::mutex m_trackMutex;
    
    // JUCE specific for VST3 hostingagement
    juce::AudioPluginFormatManager pluginFormatManager;
    juce::KnownPluginList knownPluginList;

    // Load/Save plugin list
    void loadKnownPlugins();
    void saveKnownPlugins();
    void scanForPlugins(); // Triggers async scan
    
    // juce::Timer callback for incremental scanning on the main thread
    void timerCallback() override;

private:
    std::unique_ptr<juce::PluginDirectoryScanner> m_activeScanner;

    // Temporary basic test oscillator state
    double currentPhase = 0.0;
    double phaseDelta = 0.0;
};

} // namespace dsp
