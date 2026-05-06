#include "pipeline.hpp"

#include "fir.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>

PipelineResult processWithFir(const AudioData& input, const std::vector<float>& coefficients, size_t bufferSize) {
    if (input.sampleRate == 0) {
        throw std::runtime_error("Cannot process audio with sample rate 0");
    }
    if (input.channels.empty()) {
        throw std::runtime_error("Cannot process audio with no channels");
    }
    if (bufferSize == 0) {
        throw std::runtime_error("Buffer size must be greater than 0");
    }

    const size_t frames = frameCount(input);

    PipelineResult result;
    result.audio.sampleRate = input.sampleRate;
    result.audio.channels.resize(input.channels.size());

    for (size_t channel = 0; channel < input.channels.size(); ++channel) {
        result.audio.channels[channel].resize(frames);
    }

    std::vector<FirFilter> filters;
    filters.reserve(input.channels.size());
    for (size_t channel = 0; channel < input.channels.size(); ++channel) {
        filters.emplace_back(coefficients);
    }

    BufferTimer timer;
    for (size_t offset = 0; offset < frames; offset += bufferSize) {
        const size_t count = std::min(bufferSize, frames - offset);
        const auto start = std::chrono::steady_clock::now();

        for (size_t channel = 0; channel < input.channels.size(); ++channel) {
            filters[channel].processBuffer(
                input.channels[channel].data() + offset,
                result.audio.channels[channel].data() + offset,
                count);
        }

        const auto end = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration<double, std::milli>(end - start).count();
        timer.addMeasurement(elapsed);
    }

    result.timing = timer.stats();
    return result;
}
