#include "AudioEngine.h"
#include <cmath>

namespace dsp {

AudioEngine::AudioEngine()
{
    // Create a default track
    Track mainTrack;
    mainTrack.id = 0;
    mainTrack.name = "Main";
    tracks.push_back(mainTrack);
}

AudioEngine::~AudioEngine()
{
}

void AudioEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    currentSampleRate = device->getCurrentSampleRate();
    if (currentSampleRate <= 0.0) currentSampleRate = 44100.0;
    
    int bufferSize = device->getCurrentBufferSizeSamples();
    if (bufferSize <= 0) bufferSize = 512;
    
    m_namEffect.setExpectedSetup(currentSampleRate, bufferSize);
}

void AudioEngine::audioDeviceStopped()
{
}

void AudioEngine::audioDeviceIOCallbackWithContext (
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    // 1. Process incoming messages from the Qt thread lock-free
    processMessages();

    // 2. Clear outputs first
    for (int ch = 0; ch < numOutputChannels; ++ch) {
        if (outputChannelData[ch] != nullptr) {
            juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
        }
    }

    // 3. Capture Guitar Input and Process through NAM! (Always running for live monitoring)
    // Find our guitar input channel.
    // Allocate buffer on the stack or use max size. For simplicity, just use a pre-allocated member buffer or process channel 0 directly.
    // Let's use a local fixed size array or process in-place if possible.
    // The max block size is typically <= 2048. Let's use a local array.
    constexpr int MAX_BUFFER = 4096;
    float monoInput[MAX_BUFFER] = {0.0f};
    float namOutput[MAX_BUFFER] = {0.0f};
    
    int samplesToProcess = juce::jmin(numSamples, MAX_BUFFER);

    if (numInputChannels > 0 && samplesToProcess > 0) {
        for (int ch = 0; ch < numInputChannels; ++ch) {
            if (inputChannelData[ch] != nullptr) {
                juce::FloatVectorOperations::add(monoInput, inputChannelData[ch], samplesToProcess);
            }
        }
        
        // Average it down to avoid clipping from summing
        juce::FloatVectorOperations::multiply(monoInput, 1.0f / (float)numInputChannels, samplesToProcess);
    }

    // Send through Neural Amp Modeler DSP!
    m_namEffect.process(monoInput, namOutput, samplesToProcess);

    // Send the NAM-processed mono signal out to both L and R speakers
    for (int ch = 0; ch < numOutputChannels; ++ch) {
        if (outputChannelData[ch] != nullptr) {
            // Copy processed data to output
            juce::FloatVectorOperations::copy(outputChannelData[ch], namOutput, samplesToProcess);
            
            // Apply a basic master gain so we don't blow out speakers
            juce::FloatVectorOperations::multiply(outputChannelData[ch], 0.2f, samplesToProcess);
        }
    }
}

void AudioEngine::sendMessageFromUI(const EngineMessage& msg)
{
    // Write message into the lock-free queue (called from Qt Main Thread)
    int startIndex1, blockSize1, startIndex2, blockSize2;
    messageFifo.prepareToWrite(1, startIndex1, blockSize1, startIndex2, blockSize2);
    
    if (blockSize1 > 0) {
        messageBuffer[(size_t)startIndex1] = msg;
    } else if (blockSize2 > 0) {
        messageBuffer[(size_t)startIndex2] = msg;
    }
    
    messageFifo.finishedWrite(blockSize1 + blockSize2);
}

void AudioEngine::setPlaying(bool shouldPlay)
{
    EngineMessage msg;
    msg.type = shouldPlay ? EngineCommandType::Play : EngineCommandType::Pause;
    sendMessageFromUI(msg);
}

void AudioEngine::loadNamModel(const std::string& path)
{
    // Called from the Qt Main Thread. We load it directly here!
    // The NamEffect now uses a SpinLock to safely swap the fully loaded model
    // into the audio thread without causing audio dropouts or memory allocation on the audio thread.
    m_namEffect.loadModel(path);
}

void AudioEngine::processMessages()
{
    // Read messages from the lock-free queue (called from High-Priority Audio Thread)
    int startIndex1, blockSize1, startIndex2, blockSize2;
    messageFifo.prepareToRead(messageFifo.getNumReady(), startIndex1, blockSize1, startIndex2, blockSize2);
    
    auto processBlock = [this](int startIdx, int size) {
        for (int i = 0; i < size; ++i) {
            const auto& msg = messageBuffer[(size_t)(startIdx + i)];
            
            switch (msg.type) {
                case EngineCommandType::Play:
                    isPlaying.store(true, std::memory_order_relaxed);
                    break;
                case EngineCommandType::Pause:
                case EngineCommandType::Stop:
                    isPlaying.store(false, std::memory_order_relaxed);
                    break;
                case EngineCommandType::SetTrackVolume:
                    if (msg.trackId >= 0 && msg.trackId < tracks.size()) {
                        tracks[msg.trackId].volume = msg.value;
                    }
                    break;
                default:
                    break;
            }
        }
    };

    if (blockSize1 > 0) processBlock(startIndex1, blockSize1);
    if (blockSize2 > 0) processBlock(startIndex2, blockSize2);
    
    messageFifo.finishedRead(blockSize1 + blockSize2);
}

} // namespace dsp
