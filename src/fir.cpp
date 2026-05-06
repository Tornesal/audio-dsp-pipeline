#include "fir.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace {

constexpr double pi = 3.14159265358979323846;

} // namespace

std::vector<float> designLowPassFir(uint32_t sampleRate, float cutoffHz, size_t tapCount) {
    if (sampleRate == 0) {
        throw std::runtime_error("Sample rate must be greater than 0");
    }
    if (tapCount == 0) {
        throw std::runtime_error("FIR tap count must be greater than 0");
    }

    const double nyquist = static_cast<double>(sampleRate) / 2.0;
    if (cutoffHz <= 0.0f || static_cast<double>(cutoffHz) >= nyquist) {
        throw std::runtime_error("Cutoff frequency must be greater than 0 and below Nyquist");
    }

    const double normalizedCutoff = static_cast<double>(cutoffHz) / static_cast<double>(sampleRate);
    const double center = static_cast<double>(tapCount - 1) / 2.0;

    std::vector<float> coefficients(tapCount);
    for (size_t i = 0; i < tapCount; ++i) {
        const double n = static_cast<double>(i) - center;
        double sinc = 0.0;
        if (std::abs(n) < 1.0e-12) {
            sinc = 2.0 * normalizedCutoff;
        } else {
            sinc = std::sin(2.0 * pi * normalizedCutoff * n) / (pi * n);
        }

        const double window = tapCount == 1
            ? 1.0
            : 0.54 - 0.46 * std::cos((2.0 * pi * static_cast<double>(i)) / static_cast<double>(tapCount - 1));
        coefficients[i] = static_cast<float>(sinc * window);
    }

    const float sum = std::accumulate(coefficients.begin(), coefficients.end(), 0.0f);
    if (std::abs(sum) < 1.0e-12f) {
        throw std::runtime_error("Generated FIR coefficients cannot be normalized");
    }

    for (float& coefficient : coefficients) {
        coefficient /= sum;
    }

    return coefficients;
}

FirFilter::FirFilter(std::vector<float> coefficients)
    : coefficients_(std::move(coefficients)), delayLine_(coefficients_.size(), 0.0f) {
    if (coefficients_.empty()) {
        throw std::runtime_error("FIR filter requires at least one coefficient");
    }
}

void FirFilter::reset() {
    std::fill(delayLine_.begin(), delayLine_.end(), 0.0f);
    writeIndex_ = 0;
}

float FirFilter::processSample(float input) {
    delayLine_[writeIndex_] = input;

    float output = 0.0f;
    size_t delayIndex = writeIndex_;
    for (size_t i = 0; i < coefficients_.size(); ++i) {
        output += coefficients_[i] * delayLine_[delayIndex];
        if (delayIndex == 0) {
            delayIndex = delayLine_.size() - 1;
        } else {
            --delayIndex;
        }
    }

    ++writeIndex_;
    if (writeIndex_ == delayLine_.size()) {
        writeIndex_ = 0;
    }

    return output;
}

void FirFilter::processBuffer(const float* input, float* output, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        output[i] = processSample(input[i]);
    }
}
