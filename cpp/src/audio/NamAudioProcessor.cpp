#include "NamAudioProcessor.h"

namespace dsp {

NamAudioProcessor::NamAudioProcessor() 
    : juce::AudioProcessor(BusesProperties()
                           .withInput("Input", juce::AudioChannelSet::stereo(), true)
                           .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

NamAudioProcessor::~NamAudioProcessor()
{
}

void NamAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    m_namEffect.setExpectedSetup(sampleRate, samplesPerBlock);
}

void NamAudioProcessor::releaseResources()
{
}

void NamAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    
    int numSamples = buffer.getNumSamples();
    
    // Process left channel through NAM
    if (buffer.getNumChannels() > 0) {
        float* leftChannel = buffer.getWritePointer(0);
        m_namEffect.process(leftChannel, leftChannel, numSamples);
        
        // Copy to right channel if stereo
        if (buffer.getNumChannels() > 1) {
            float* rightChannel = buffer.getWritePointer(1);
            juce::FloatVectorOperations::copy(rightChannel, leftChannel, numSamples);
        }
    }
}

void NamAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    // Normally wouldn't be called, just clear it
    buffer.clear();
}

const juce::String NamAudioProcessor::getName() const
{
    return "Neural Amp Modeler";
}

bool NamAudioProcessor::acceptsMidi() const { return false; }
bool NamAudioProcessor::producesMidi() const { return false; }
double NamAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int NamAudioProcessor::getNumPrograms() { return 1; }
int NamAudioProcessor::getCurrentProgram() { return 0; }
void NamAudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused(index); }
const juce::String NamAudioProcessor::getProgramName (int index) { juce::ignoreUnused(index); return {}; }
void NamAudioProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

void NamAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save model path
    juce::String path(m_modelPath);
    destData.append(path.toRawUTF8(), path.getNumBytesAsUTF8());
}

void NamAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::String path = juce::String::fromUTF8((const char*)data, sizeInBytes);
    loadModel(path.toStdString());
}

bool NamAudioProcessor::loadModel(const std::string& path)
{
    m_modelPath = path;
    return m_namEffect.loadModel(path);
}

} // namespace dsp
