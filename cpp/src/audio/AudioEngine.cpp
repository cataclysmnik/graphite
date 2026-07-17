#include "AudioEngine.h"
#include <cmath>
#include <iostream>
#include <juce_audio_processors/juce_audio_processors.h>
#include "NamAudioProcessor.h"

namespace dsp {

void AudioEngine::loadKnownPlugins()
{
    juce::File appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("GuitarDaw");
    juce::File xmlFile = appDataDir.getChildFile("KnownPlugins.xml");
    
    if (xmlFile.existsAsFile()) {
        if (auto xml = juce::XmlDocument::parse(xmlFile)) {
            knownPluginList.recreateFromXml(*xml);
        }
    }
}

void AudioEngine::saveKnownPlugins()
{
    juce::File appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("GuitarDaw");
    if (!appDataDir.exists()) appDataDir.createDirectory();
    
    juce::File xmlFile = appDataDir.getChildFile("KnownPlugins.xml");
    if (auto xml = knownPluginList.createXml()) {
        xml->writeTo(xmlFile, juce::XmlElement::TextFormat());
    }
}

void AudioEngine::scanForPlugins()
{
    if (m_activeScanner != nullptr) return; // Already scanning
    
    auto* vst3Format = pluginFormatManager.getFormat(0);
    if (!vst3Format) return;

    juce::FileSearchPath searchPath = vst3Format->getDefaultLocationsToSearch();
    
    m_activeScanner = std::make_unique<juce::PluginDirectoryScanner>(
        knownPluginList,
        *vst3Format,
        searchPath,
        true, // recursive search
        juce::File(), // deadlock detector file
        true // allowAsync
    );

    // Start timer to pump the scanner
    startTimer(10); // 10ms intervals
}

void AudioEngine::timerCallback()
{
    if (m_activeScanner) {
        juce::String nameBeingScanned;
        bool keepScanning = m_activeScanner->scanNextFile(true, nameBeingScanned);
        
        if (!keepScanning) {
            // Scan finished!
            m_activeScanner.reset();
            stopTimer();
            saveKnownPlugins();
        }
    }
}


AudioEngine::AudioEngine()
{
    // Setup VST3 format
    pluginFormatManager.addDefaultFormats();

    // Load previously scanned plugins from XML
    loadKnownPlugins();
    
    // If we have no plugins, trigger an initial scan
    if (knownPluginList.getNumTypes() == 0) {
        scanForPlugins();
    }

    // Create default tracks
    for (int i = 0; i < 4; ++i) {
        Track t;
        t.id = i;
        if (i == 0) t.name = "Lead Guitar";
        else if (i == 1) t.name = "Rhythm Guitar";
        else if (i == 2) t.name = "Bass";
        else if (i == 3) t.name = "Drums";
        tracks.push_back(std::move(t));
    }
}

AudioEngine::~AudioEngine()
{
}

void AudioEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    if (device != nullptr) {
        currentSampleRate.store(device->getCurrentSampleRate());
        
        // Prepare all loaded plugins
        for (auto& track : tracks) {
            for (auto& plugin : track.plugins) {
                if (plugin != nullptr) {
                    plugin->prepareToPlay(currentSampleRate.load(), device->getCurrentBufferSizeSamples());
                }
            }
        }
    }
    
    double sr = currentSampleRate.load();
    if (sr <= 0.0) sr = 44100.0;
    
    int bufferSize = device->getCurrentBufferSizeSamples();
    if (bufferSize <= 0) bufferSize = 512;
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

    constexpr int MAX_BUFFER = 4096;
    float monoInput[MAX_BUFFER] = {0.0f};
    
    int samplesToProcess = juce::jmin(numSamples, MAX_BUFFER);

    // Capture mono hardware input (e.g. guitar)
    if (numInputChannels > 0 && samplesToProcess > 0) {
        for (int ch = 0; ch < numInputChannels; ++ch) {
            if (inputChannelData[ch] != nullptr) {
                juce::FloatVectorOperations::add(monoInput, inputChannelData[ch], samplesToProcess);
            }
        }
        juce::FloatVectorOperations::multiply(monoInput, 1.0f / (float)numInputChannels, samplesToProcess);
    }

    // Process each track
    for (auto& track : tracks) {
        if (track.isMuted) continue;

        // A temporary buffer for this track's stereo signal
        float trackLeft[MAX_BUFFER] = {0.0f};
        float trackRight[MAX_BUFFER] = {0.0f};
        float* trackChannels[2] = { trackLeft, trackRight };
        
        // Wrap it in an AudioBuffer for JUCE plugins
        juce::AudioBuffer<float> trackBuffer(trackChannels, 2, samplesToProcess);
        
        // If track is armed, feed it the live input
        if (track.isArmed && samplesToProcess > 0) {
            juce::FloatVectorOperations::copy(trackLeft, monoInput, samplesToProcess);
            juce::FloatVectorOperations::copy(trackRight, monoInput, samplesToProcess);
        }
        
        // --- VST3 Plugin Processing ---
        juce::MidiBuffer midiMessages; // empty for now
        for (auto& plugin : track.plugins) {
            if (plugin != nullptr && !plugin->isSuspended()) {
                // Prepare if necessary
                if (plugin->getSampleRate() != currentSampleRate.load()) {
                    plugin->prepareToPlay(currentSampleRate.load(), samplesToProcess);
                }
                plugin->processBlock(trackBuffer, midiMessages);
            }
        }
        
        // Apply track volume and pan
        float vol = track.volume;
        // Simple equal power panning rule: 
        // pan is -1 (left) to 1 (right)
        float panVal = (track.pan + 1.0f) * 0.5f; // mapped 0 to 1
        float gainL = vol * cosf(panVal * 1.570796f); // pi/2
        float gainR = vol * sinf(panVal * 1.570796f);
        
        juce::FloatVectorOperations::multiply(trackLeft, gainL, samplesToProcess);
        juce::FloatVectorOperations::multiply(trackRight, gainR, samplesToProcess);
        
        // Sum into master output
        if (numOutputChannels >= 2) {
            juce::FloatVectorOperations::add(outputChannelData[0], trackLeft, samplesToProcess);
            juce::FloatVectorOperations::add(outputChannelData[1], trackRight, samplesToProcess);
        } else if (numOutputChannels == 1) {
            // Mono output device fallback
            juce::FloatVectorOperations::add(outputChannelData[0], trackLeft, samplesToProcess);
            juce::FloatVectorOperations::add(outputChannelData[0], trackRight, samplesToProcess);
        }
    }
    
    // Update playhead time if playing
    if (isPlaying.load()) {
        double sr = currentSampleRate.load();
        if (sr > 0.0) {
            double timeIncrement = (double)numSamples / sr;
            double currentTime = playheadTimeSeconds.load();
            playheadTimeSeconds.store(currentTime + timeIncrement);
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
                    if (msg.trackIndex >= 0 && msg.trackIndex < tracks.size()) {
                        tracks[msg.trackIndex].volume = msg.floatValue;
                    }
                    break;
                case EngineCommandType::SetTrackArm:
                    if (msg.trackIndex >= 0 && msg.trackIndex < tracks.size()) {
                        tracks[msg.trackIndex].isArmed = msg.boolValue;
                    }
                    break;
                case EngineCommandType::SetTrackSelect:
                    if (msg.trackIndex >= 0 && msg.trackIndex < tracks.size()) {
                        for (auto& t : tracks) t.isSelected = false;
                        tracks[msg.trackIndex].isSelected = true;
                        selectedTrackIndex.store(msg.trackIndex, std::memory_order_relaxed);
                    }
                    break;
                case EngineCommandType::LoadPlugin:
                    if (msg.trackIndex >= 0 && msg.trackIndex < tracks.size()) {
                        if (msg.ptrValue != nullptr) {
                            auto* proc = static_cast<juce::AudioProcessor*>(msg.ptrValue);
                            tracks[msg.trackIndex].plugins.push_back(std::unique_ptr<juce::AudioProcessor>(proc));
                        }
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
class PluginWindow : public juce::DocumentWindow
{
public:
    PluginWindow(juce::AudioProcessor* processor)
        : juce::DocumentWindow(processor->getName(),
                               juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
                               juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        if (auto* editor = processor->createEditorIfNeeded()) {
            setContentOwned(editor, true);
            setResizable(editor->isResizable(), false);
        }
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }
    
    void closeButtonPressed() override {
        setVisible(false);
    }
};

std::vector<std::string> AudioEngine::getTrackPluginNames(int trackIndex) const
{
    std::vector<std::string> names;
    if (trackIndex >= 0 && trackIndex < tracks.size()) {
        // Technically reading this while audio thread might be writing is a race condition, 
        // but for this phase we accept the minor risk.
        for (const auto& plugin : tracks[trackIndex].plugins) {
            if (plugin) {
                names.push_back(plugin->getName().toStdString());
            }
        }
    }
    return names;
}

static std::unordered_map<juce::AudioProcessor*, std::unique_ptr<PluginWindow>> cachedPluginWindows;

void AudioEngine::openPluginEditor(int trackIndex, int pluginIndex)
{
    // Must be called on Qt/JUCE main thread!
    if (trackIndex >= 0 && trackIndex < tracks.size()) {
        auto& plugins = tracks[trackIndex].plugins;
        if (pluginIndex >= 0 && pluginIndex < plugins.size()) {
            auto* proc = plugins[pluginIndex].get();
            if (proc && proc->hasEditor()) {
                if (cachedPluginWindows.find(proc) == cachedPluginWindows.end()) {
                    cachedPluginWindows[proc] = std::make_unique<PluginWindow>(proc);
                }
                cachedPluginWindows[proc]->setVisible(true);
                cachedPluginWindows[proc]->toFront(true);
            }
        }
    }
}

void AudioEngine::loadPluginSynchronous(int trackIndex, const juce::String& identifierOrPath)
{
    if (trackIndex < 0 || trackIndex >= tracks.size()) return;
    
    std::unique_ptr<juce::AudioProcessor> newPlugin;
    double sr = currentSampleRate.load();
    if (sr <= 0.0) sr = 44100.0;
    
    if (identifierOrPath == "NAM_BUILTIN") {
        auto namProc = std::make_unique<NamAudioProcessor>();
        namProc->prepareToPlay(sr, 512);
        newPlugin = std::move(namProc);
    } else {
        std::unique_ptr<juce::PluginDescription> foundDesc;
        for (const auto& type : knownPluginList.getTypes()) {
            if (type.fileOrIdentifier == identifierOrPath || type.name == identifierOrPath) {
                foundDesc = std::make_unique<juce::PluginDescription>(type);
                break;
            }
        }
        
        juce::String errorMessage;
        std::unique_ptr<juce::AudioPluginInstance> instance;
        
        if (foundDesc) {
            instance = pluginFormatManager.createPluginInstance(*foundDesc, sr, 512, errorMessage);
        } else {
            juce::PluginDescription desc;
            desc.fileOrIdentifier = identifierOrPath;
            desc.pluginFormatName = "VST3";
            instance = pluginFormatManager.createPluginInstance(desc, sr, 512, errorMessage);
        }
        
        if (instance != nullptr) {
            instance->prepareToPlay(sr, 512);
            newPlugin = std::move(instance);
        }
    }
    
    if (newPlugin != nullptr) {
        EngineMessage msg;
        msg.type = EngineCommandType::LoadPlugin;
        msg.trackIndex = trackIndex;
        msg.ptrValue = newPlugin.release(); // Hand ownership over to the audio thread
        sendMessageFromUI(msg);
    }
}

} // namespace dsp
