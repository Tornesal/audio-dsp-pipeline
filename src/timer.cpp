#include "timer.hpp"

#include <algorithm>

double TimingStats::averageMs() const {
    // Avoid dividing by zero if no buffers were processed
    if (bufferCount == 0) {
        return 0.0;
    }
    return totalMs / static_cast<double>(bufferCount);
}

double TimingStats::realTimeFactor(double audioDurationSeconds) const {
    // Compare total processing time against the actual audio duration
    if (audioDurationSeconds <= 0.0) {
        return 0.0;
    }
    return (totalMs / 1000.0) / audioDurationSeconds;
}

void BufferTimer::addMeasurement(double elapsedMs) {
    // Use the first measurement to initialize min and max timing
    if (stats_.bufferCount == 0) {
        stats_.minMs = elapsedMs;
        stats_.maxMs = elapsedMs;
    } else {
        stats_.minMs = std::min(stats_.minMs, elapsedMs);
        stats_.maxMs = std::max(stats_.maxMs, elapsedMs);
    }

    stats_.totalMs += elapsedMs;
    ++stats_.bufferCount;
}

const TimingStats& BufferTimer::stats() const {
    // Return the accumulated timing summary
    return stats_;
}
