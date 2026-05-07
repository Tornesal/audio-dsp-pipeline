#pragma once

#include <cstddef>

// Stores timing numbers collected while processing audio buffers
struct TimingStats {
    size_t bufferCount = 0;
    double totalMs = 0.0;
    double minMs = 0.0;
    double maxMs = 0.0;

    double averageMs() const;
    double realTimeFactor(double audioDurationSeconds) const;
};

// Adds one timing measurement at a time and updates summary stats
class BufferTimer {
public:
    void addMeasurement(double elapsedMs);
    const TimingStats& stats() const;

private:
    TimingStats stats_;
};
