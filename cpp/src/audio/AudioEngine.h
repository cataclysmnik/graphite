#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include "AudioModels.h"
#include "NamEffect.h"

namespace dsp {

class AudioEngine : public juce::AudioIODeviceCallback {
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
    void loadNamModel(const std::string& path);

private:
    void processMessages();

    // Lock-free queue (Single Producer, Single Consumer)
    static constexpr int FIFO_SIZE = 1024;
    juce::AbstractFifo messageFifo { FIFO_SIZE };
    std::array<EngineMessage, FIFO_SIZE> messageBuffer;

    // Engine State
    std::atomic<bool> isPlaying { false };
    std::atomic<double> currentSampleRate { 44100.0 };
    
    // Tracks
    std::vector<Track> tracks;

    // Neural Amp Modeler DSP
    NamEffect m_namEffect;

    // Temporary basic test oscillator state
    double currentPhase = 0.0;
    double phaseDelta = 0.0;
};

} // namespace dsp
