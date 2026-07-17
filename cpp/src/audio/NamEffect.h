#pragma once

#include <memory>
#include <string>
#include <vector>
#include <juce_core/juce_core.h>
#include <NAM/dsp.h>

namespace dsp {

class NamEffect {
public:
    NamEffect();
    ~NamEffect();

    // Must be called from the Audio Thread (wait, loading shouldn't block, 
    // but for this phase we'll load it on the main thread before starting audio, 
    // or use a unique_ptr swap in the lock-free queue in the future).
    bool loadModel(const std::string& modelPath);

    // Audio thread processing
    // Processes the input buffer and writes to the output buffer.
    void process(float* inputBuffer, float* outputBuffer, int numSamples);

    // Set the expected sample rate and buffer size
    void setExpectedSetup(double sampleRate, int maxBufferSize);

private:
    juce::SpinLock m_spinLock;
    std::unique_ptr<nam::DSP> m_namDSP;
    double m_sampleRate { 44100.0 };
    int m_maxBufferSize { 512 };
};

} // namespace dsp
