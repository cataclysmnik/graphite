#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "NamEffect.h"

namespace dsp {

class NamAudioProcessor : public juce::AudioProcessor {
public:
    NamAudioProcessor();
    ~NamAudioProcessor() override;

    // --- AudioProcessor methods ---
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    // Non-standard API for loading a model path (called manually when instantiated)
    bool loadModel(const std::string& path);
    const std::string& getModelPath() const { return m_modelPath; }

    bool hasEditor() const override { return false; } // For now, no editor
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

private:
    NamEffect m_namEffect;
    std::string m_modelPath;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NamAudioProcessor)
};

} // namespace dsp
