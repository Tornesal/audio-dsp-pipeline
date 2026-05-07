#pragma once

#include "audio.hpp"
#include "timer.hpp"

#include <cstddef>
#include <vector>

// Stores both the filtered audio and the timing results from processing
struct PipelineResult {
    AudioData audio;
    TimingStats timing;
};

// Process audio through a FIR filter in fixed-size buffers
PipelineResult processWithFir(const AudioData& input, const std::vector<float>& coefficients, size_t bufferSize);
