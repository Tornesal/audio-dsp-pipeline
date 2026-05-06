#pragma once

#include <cstddef>

struct TimingStats {
    size_t bufferCount = 0;
    double totalMs = 0.0;
    double minMs = 0.0;
    double maxMs = 0.0;

    double averageMs() const;
    double realTimeFactor(double audioDurationSeconds) const;
};

class BufferTimer {
public:
    void addMeasurement(double elapsedMs);
    const TimingStats& stats() const;

private:
    TimingStats stats_;
};
