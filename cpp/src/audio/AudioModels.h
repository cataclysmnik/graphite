#pragma once

#include <string>
#include <vector>
#include <juce_core/juce_core.h>

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

    std::vector<AudioItem> items;
};

// Messaging structures for the lock-free queue (Qt -> Audio Thread)
enum class EngineCommandType {
    Play,
    Pause,
    Stop,
    SetTrackVolume,
    AddTrack,
    AddAudioItem,
    LoadNamModel
};

struct EngineMessage {
    EngineCommandType type;
    int trackId = -1;
    float value = 0.0f;
    std::string* stringPayload = nullptr; // Pointer for passing strings across thread safely
};

} // namespace dsp
