#include "wav.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

struct WavFormat {
    uint16_t audioFormat = 0;
    uint16_t channels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
};

uint16_t readU16(std::istream& input) {
    unsigned char bytes[2] = {};
    input.read(reinterpret_cast<char*>(bytes), 2);
    if (!input) {
        throw std::runtime_error("Unexpected end of WAV file");
    }
    return static_cast<uint16_t>(bytes[0] | (bytes[1] << 8));
}

uint32_t readU32(std::istream& input) {
    unsigned char bytes[4] = {};
    input.read(reinterpret_cast<char*>(bytes), 4);
    if (!input) {
        throw std::runtime_error("Unexpected end of WAV file");
    }
    return static_cast<uint32_t>(bytes[0]) |
        (static_cast<uint32_t>(bytes[1]) << 8) |
        (static_cast<uint32_t>(bytes[2]) << 16) |
        (static_cast<uint32_t>(bytes[3]) << 24);
}

void writeU16(std::ostream& output, uint16_t value) {
    const unsigned char bytes[2] = {
        static_cast<unsigned char>(value & 0xff),
        static_cast<unsigned char>((value >> 8) & 0xff),
    };
    output.write(reinterpret_cast<const char*>(bytes), 2);
}

void writeU32(std::ostream& output, uint32_t value) {
    const unsigned char bytes[4] = {
        static_cast<unsigned char>(value & 0xff),
        static_cast<unsigned char>((value >> 8) & 0xff),
        static_cast<unsigned char>((value >> 16) & 0xff),
        static_cast<unsigned char>((value >> 24) & 0xff),
    };
    output.write(reinterpret_cast<const char*>(bytes), 4);
}

std::string readId(std::istream& input) {
    char id[4] = {};
    input.read(id, 4);
    if (!input) {
        throw std::runtime_error("Unexpected end of WAV file");
    }
    return std::string(id, 4);
}

void skipBytes(std::istream& input, uint32_t byteCount) {
    input.seekg(byteCount, std::ios::cur);
    if (!input) {
        throw std::runtime_error("Failed to skip WAV chunk data");
    }
}

int16_t floatToPcm16(float sample) {
    const float clamped = std::max(-1.0f, std::min(1.0f, sample));
    int value = 0;
    if (clamped <= -1.0f) {
        value = std::numeric_limits<int16_t>::min();
    } else if (clamped >= 1.0f) {
        value = std::numeric_limits<int16_t>::max();
    } else if (clamped >= 0.0f) {
        value = static_cast<int>(clamped * static_cast<float>(std::numeric_limits<int16_t>::max()) + 0.5f);
    } else {
        value = static_cast<int>(clamped * 32768.0f - 0.5f);
    }
    return static_cast<int16_t>(std::max<int>(std::numeric_limits<int16_t>::min(), std::min<int>(std::numeric_limits<int16_t>::max(), value)));
}

} // namespace

AudioData readWavFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Failed to open WAV file for reading: " + path);
    }

    if (readId(input) != "RIFF") {
        throw std::runtime_error("Not a RIFF file: " + path);
    }

    (void)readU32(input);

    if (readId(input) != "WAVE") {
        throw std::runtime_error("Not a WAVE file: " + path);
    }

    WavFormat format;
    bool foundFormat = false;
    bool foundData = false;
    uint32_t dataSize = 0;
    std::streampos dataOffset = 0;

    while (input && !(foundFormat && foundData)) {
        const std::string chunkId = readId(input);
        const uint32_t chunkSize = readU32(input);

        if (chunkId == "fmt ") {
            if (chunkSize < 16) {
                throw std::runtime_error("Invalid WAV fmt chunk");
            }

            format.audioFormat = readU16(input);
            format.channels = readU16(input);
            format.sampleRate = readU32(input);
            (void)readU32(input);
            (void)readU16(input);
            format.bitsPerSample = readU16(input);

            if (chunkSize > 16) {
                skipBytes(input, chunkSize - 16);
            }
            foundFormat = true;
        } else if (chunkId == "data") {
            dataOffset = input.tellg();
            dataSize = chunkSize;
            skipBytes(input, chunkSize);
            foundData = true;
        } else {
            skipBytes(input, chunkSize);
        }

        if (chunkSize % 2 != 0) {
            skipBytes(input, 1);
        }
    }

    if (!foundFormat) {
        throw std::runtime_error("WAV fmt chunk not found: " + path);
    }
    if (!foundData) {
        throw std::runtime_error("WAV data chunk not found: " + path);
    }
    if (format.audioFormat != 1) {
        throw std::runtime_error("Unsupported WAV format: only PCM integer WAV is supported");
    }
    if (format.channels != 1) {
        throw std::runtime_error("Unsupported WAV channel count: Stage 1 supports mono only");
    }
    if (format.sampleRate == 0) {
        throw std::runtime_error("Invalid WAV sample rate: 0");
    }
    if (format.bitsPerSample != 16) {
        throw std::runtime_error("Unsupported WAV bit depth: only 16-bit PCM is supported");
    }
    if (dataSize % 2 != 0) {
        throw std::runtime_error("Invalid PCM16 data size in WAV file");
    }

    AudioData audio;
    audio.sampleRate = format.sampleRate;
    audio.channels.resize(1);
    audio.channels[0].resize(dataSize / 2);

    input.clear();
    input.seekg(dataOffset);
    if (!input) {
        throw std::runtime_error("Failed to seek to WAV data chunk");
    }

    for (float& sample : audio.channels[0]) {
        const uint16_t raw = readU16(input);
        const auto signedSample = static_cast<int16_t>(raw);
        sample = static_cast<float>(signedSample) / 32768.0f;
    }

    return audio;
}

void writeWavFile(const std::string& path, const AudioData& audio) {
    if (audio.sampleRate == 0) {
        throw std::runtime_error("Cannot write WAV with sample rate 0");
    }
    if (audio.channels.size() != 1) {
        throw std::runtime_error("Stage 1 WAV writer supports mono audio only");
    }

    const size_t frames = frameCount(audio);
    const uint64_t dataSize64 = frames * sizeof(int16_t);
    if (dataSize64 > std::numeric_limits<uint32_t>::max() - 36ull) {
        throw std::runtime_error("Audio data is too large for a standard WAV file");
    }

    const uint32_t dataSize = static_cast<uint32_t>(dataSize64);
    const uint32_t riffSize = 36u + dataSize;

    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Failed to open WAV file for writing: " + path);
    }

    output.write("RIFF", 4);
    writeU32(output, riffSize);
    output.write("WAVE", 4);

    output.write("fmt ", 4);
    writeU32(output, 16);
    writeU16(output, 1);
    writeU16(output, 1);
    writeU32(output, audio.sampleRate);
    writeU32(output, audio.sampleRate * 2u);
    writeU16(output, 2);
    writeU16(output, 16);

    output.write("data", 4);
    writeU32(output, dataSize);

    for (float sample : audio.channels[0]) {
        writeU16(output, static_cast<uint16_t>(floatToPcm16(sample)));
    }

    if (!output) {
        throw std::runtime_error("Failed while writing WAV file: " + path);
    }
}
