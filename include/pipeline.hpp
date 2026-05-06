#pragma once

#include "audio.hpp"
#include "timer.hpp"

#include <cstddef>
#include <vector>

struct PipelineResult {
    AudioData audio;
    TimingStats timing;
};

PipelineResult processWithFir(const AudioData& input, const std::vector<float>& coefficients, size_t bufferSize);
