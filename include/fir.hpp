#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// Build low-pass FIR coefficients from the sample rate and cutoff
std::vector<float> designLowPassFir(uint32_t sampleRate, float cutoffHz, size_t tapCount);

// Keeps FIR coefficients and delay-line state for one audio channel
class FirFilter {
public:
    explicit FirFilter(std::vector<float> coefficients);

    void reset();
    float processSample(float input);
    void processBuffer(const float* input, float* output, size_t count);

private:
    std::vector<float> coefficients_;
    std::vector<float> delayLine_;
    size_t writeIndex_ = 0;
};
