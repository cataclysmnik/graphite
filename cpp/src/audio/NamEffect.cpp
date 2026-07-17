#include "NamEffect.h"
#include <NAM/get_dsp.h>
#include <iostream>
#include <filesystem>

namespace dsp {

NamEffect::NamEffect()
{
}

NamEffect::~NamEffect()
{
}

bool NamEffect::loadModel(const std::string& modelPath)
{
    try {
        auto newDSP = nam::get_dsp(std::filesystem::path(modelPath));
        if (newDSP) {
            // Setup expected sample rate and buffer size to avoid assertion failures!
            newDSP->ResetAndPrewarm(m_sampleRate, m_maxBufferSize);
            
            std::unique_ptr<nam::DSP> oldDSP;
            {
                juce::SpinLock::ScopedLockType lock(m_spinLock);
                oldDSP = std::move(m_namDSP);
                m_namDSP = std::move(newDSP);
            }
            // oldDSP is safely deleted here outside the lock, on the UI thread!
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to load NAM model: " << e.what() << "\n";
    }
    return false;
}

void NamEffect::setExpectedSetup(double sampleRate, int maxBufferSize)
{
    m_sampleRate = sampleRate;
    m_maxBufferSize = 4096; // Hardcode to 4096 to guarantee we never overflow NAM's pre-warmed buffers
    
    juce::SpinLock::ScopedLockType lock(m_spinLock);
    if (m_namDSP) {
        m_namDSP->ResetAndPrewarm(m_sampleRate, m_maxBufferSize);
    }
}

void NamEffect::process(float* inputBuffer, float* outputBuffer, int numSamples)
{
    juce::SpinLock::ScopedTryLockType lock(m_spinLock);
    
    // If the spinlock is locked (meaning the UI thread is swapping the model),
    // we simply bypass NAM processing for this tiny block to avoid blocking the audio thread!
    if (lock.isLocked() && m_namDSP) {
        float* inputs[1] = { inputBuffer };
        float* outputs[1] = { outputBuffer };
        
        m_namDSP->process(inputs, outputs, numSamples);
    } else {
        // Bypass directly if no DSP loaded or if currently swapping
        if (inputBuffer != outputBuffer) {
            std::copy(inputBuffer, inputBuffer + numSamples, outputBuffer);
        }
    }
}

} // namespace dsp
