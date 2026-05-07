#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

// Holds audio samples in separate channel buffers
struct AudioData {
    uint32_t sampleRate = 0;
    std::vector<std::vector<float>> channels;
};

// Return how many channels are stored in the audio buffer
inline size_t channelCount(const AudioData& audio) {
    return audio.channels.size();
}

// Count frames and make sure every channel has the same length
inline size_t frameCount(const AudioData& audio) {
    if (audio.channels.empty()) {
        return 0;
    }

    const size_t frames = audio.channels.front().size();
    for (const auto& channel : audio.channels) {
        if (channel.size() != frames) {
            throw std::runtime_error("Audio channels have mismatched frame counts");
        }
    }

    return frames;
}

// Convert frame count and sample rate into seconds
inline double durationSeconds(const AudioData& audio) {
    if (audio.sampleRate == 0) {
        return 0.0;
    }

    return static_cast<double>(frameCount(audio)) / static_cast<double>(audio.sampleRate);
}
