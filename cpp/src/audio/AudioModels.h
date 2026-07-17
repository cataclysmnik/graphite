#pragma once

#include <string>
#include <vector>
#include <memory>
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace dsp {

struct AudioItem {
    int id;
    std::string filePath;
    double startTimeSecs;
    double offsetSecs;
    double durationSecs;
    
    // In a real application, we'd store a lock-free reference to an audio buffer pool
    // juce::AudioBuffer<float>* buffer;
};

struct Track {
    int id;
    std::string name;
    float volume = 1.0f; // Linear gain (0.0 to 1.0+)
    float pan = 0.0f;    // -1.0 (Left) to 1.0 (Right)
    bool isMuted = false;
    bool isSolo = false;
    bool isArmed = false;
    bool isSelected = false;

    std::vector<AudioItem> items;
    
    // The plugin chain for this track
    std::vector<std::unique_ptr<juce::AudioProcessor>> plugins;
};

// Messaging structures for the lock-free queue (Qt -> Audio Thread)
enum class EngineCommandType {
    Play,
    Pause,
    Stop,
    SetTrackVolume,
    SetTrackPan,
    SetTrackMute,
    SetTrackSolo,
    AddTrack,
    AddAudioItem,
    LoadPlugin,
    SetTrackArm,
    SetTrackSelect,
    MovePlugin,
    DeletePlugin
};

struct EngineMessage {
    EngineCommandType type;
    int trackIndex = -1;
    float floatValue = 0.0f;
    bool boolValue = false;
    int pluginIndex1 = -1;
    int pluginIndex2 = -1;
    char stringValue[256] = {0}; // Fixed size for lock-free safety
    void* ptrValue = nullptr;
};

} // namespace dsp
