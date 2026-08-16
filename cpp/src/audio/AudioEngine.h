#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
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
    void setRecording(bool shouldRecord);
    void setPlayheadPosition(double timeSecs);
    
    // Audio Item interactions
    void deleteAudioItem(int itemId);
    void moveAudioItem(int itemId, int targetTrackIndex, double newStartTimeSecs);
    void setAudioItemSelection(int itemId, bool isSelected);
    void clearAudioItemSelection();
    void loadAudioFileSynchronous(int trackIndex, double startTimeSecs, const juce::String& filePath);
    
    struct RenderOptions {
        juce::String outputPath;
        double sampleRate { 44100.0 };
        int bitDepth { 24 };
        double startTimeSecs { 0.0 };
        double endTimeSecs { 0.0 };
        bool renderStems { false }; // If true, render each track to a separate file (stem rendering)
        int trackIdToRender { -1 }; // If >= 0, only render this specific track. -1 means Master Mix.
    };
    
    // Offline rendering (must not be called on audio thread)
    // Returns true if successful, false if cancelled or failed
    bool renderOffline(const RenderOptions& options, std::function<bool(float)> progressCallback);
    
    // Project state serialization
    juce::ValueTree serializeProjectState(const std::string& projectDirectory = "");
    void deserializeProjectState(const juce::ValueTree& state, std::function<void(float)> progressCallback = nullptr);
    void clearProject();
    
    bool isEnginePlaying() const { return isPlaying.load(); }
    bool isEngineRecording() const { return isRecording.load(); }
    double getPlayheadTime() const { return playheadTimeSeconds.load(); }
    
    // Looping state
    bool isLoopingEnabled() const { return m_isLooping.load(); }
    void setLooping(bool shouldLoop) { m_isLooping.store(shouldLoop); }
    double getLoopStart() const { return m_loopStartSecs.load(); }
    double getLoopEnd() const { return m_loopEndSecs.load(); }
    void setLoopRegion(double start, double end) {
        if (start > end) std::swap(start, end);
        m_loopStartSecs.store(start);
        m_loopEndSecs.store(end);
    }
    
    // Project dirty state tracking
    bool isProjectDirty() const { return m_isProjectDirty.load(); }
    void markProjectDirty() { m_isProjectDirty = true; m_stateVersion.fetch_add(1, std::memory_order_relaxed); }
    void clearProjectDirty() { m_isProjectDirty = false; }
    
    // Track State Getters (thread safe-ish for UI)
    int getSelectedTrackIndex() const { return selectedTrackIndex.load(); }
    float getTrackPeakL(int trackIndex) const;
    float getTrackPeakR(int trackIndex) const;
    float getTrackPan(int trackIndex) const;
    
    std::vector<Track> getTracksSnapshot() const;
    uint32_t getStateVersion() const { return m_stateVersion.load(std::memory_order_relaxed); }
    
    // Live Recording Getters
    const juce::AudioBuffer<float>* getRecordBuffer(int trackId) const;
    int getRecordSamplesWritten(int trackId) const;
    double getRecordStartTime(int trackId) const;
    double getCurrentSampleRate() const { return currentSampleRate.load(); }
    
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
    
    // Metronome & Tuner
    double getBpm() const { return m_bpm.load(); }
    void setBpm(double bpm) { m_bpm.store(bpm); }
    
    bool isMetronomeEnabled() const { return m_metronomeEnabled.load(); }
    void setMetronomeEnabled(bool enabled) { m_metronomeEnabled.store(enabled); }
    
    float getMetronomeVolume() const { return m_metronomeVolume.load(); }
    void setMetronomeVolume(float vol) { m_metronomeVolume.store(vol); }
    
    int getTimeSigNumerator() const { return m_timeSigNumerator.load(); }
    void setTimeSigNumerator(int num) { m_timeSigNumerator.store(num); }
    
    // Returns the latest samples from the tuner buffer (size up to 4096)
    std::vector<float> getTunerSamples(int count = 2048) const;
    
private:
    void processMessages();

    // Lock-free queue (Single Producer, Single Consumer)
    static constexpr int FIFO_SIZE = 1024;
    juce::AbstractFifo messageFifo { FIFO_SIZE };
    std::array<EngineMessage, FIFO_SIZE> messageBuffer;

    // Engine State
    std::atomic<bool> isPlaying { false };
    std::atomic<bool> isRecording { false };
    std::atomic<double> currentSampleRate { 44100.0 };
    std::atomic<double> playheadTimeSeconds { 0.0 };
    std::atomic<uint32_t> m_stateVersion { 0 };
    
    std::atomic<bool> m_isLooping { false };
    std::atomic<double> m_loopStartSecs { 0.0 };
    std::atomic<double> m_loopEndSecs { 0.0 };
    
    std::atomic<int> selectedTrackIndex { 0 };
    std::atomic<bool> m_isProjectDirty { false };
    std::atomic<int> m_nextItemId{1000};
    
    // Metronome state
    std::atomic<double> m_bpm { 120.0 };
    std::atomic<bool> m_metronomeEnabled { false };
    std::atomic<float> m_metronomeVolume { 0.8f };
    std::atomic<int> m_timeSigNumerator { 4 };
    
    // Metronome Beep State
    std::atomic<int> m_metronomeBeepSamplesRemaining { 0 };
    std::atomic<float> m_metronomeBeepPhase { 0.0f };
    std::atomic<float> m_metronomeBeepFreq { 1000.0f };
    
    // Tuner Buffer (circular)
    static constexpr int TUNER_BUFFER_SIZE = 4096;
    float m_tunerBuffer[TUNER_BUFFER_SIZE] = {0.0f};
    std::atomic<int> m_tunerBufferIndex { 0 };
    
    // Recording buffers
    std::vector<std::unique_ptr<juce::AudioBuffer<float>>> m_recordBuffers;
    std::vector<int> m_recordSamplesWritten;
    std::vector<double> m_recordStartTimes;
    
    // Concurrency for plugins
    std::recursive_mutex m_pluginMutex;
    
    // Tracks
    std::vector<Track> tracks;
    mutable std::recursive_mutex m_trackMutex;
    
    // JUCE specific for VST3 hosting
    juce::AudioPluginFormatManager pluginFormatManager;
    juce::KnownPluginList knownPluginList;
    
    // Audio Formats
    juce::AudioFormatManager audioFormatManager;

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
