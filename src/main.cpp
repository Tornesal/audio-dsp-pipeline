#include "audio.hpp"
#include "fir.hpp"
#include "pipeline.hpp"
#include "wav.hpp"

#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

struct Options {
    std::string inputPath;
    std::string outputPath;
    float cutoffHz = 3000.0f;
    size_t tapCount = 101;
    size_t bufferSize = 512;
};

// Print the command-line format the program expects
void printUsage(const char* executable) {
    std::cerr << "Usage: " << executable << " input.wav output.wav [--cutoff hz] [--taps count] [--buffer samples]\n"
              << "Defaults: --cutoff 3000 --taps 101 --buffer 512\n";
}

// Parse a floating-point option and report the option name on failure
float parseFloatOption(const std::string& name, const std::string& value) {
    try {
        size_t parsed = 0;
        const float result = std::stof(value, &parsed);
        if (parsed != value.size()) {
            throw std::runtime_error("Invalid numeric value for " + name + ": " + value);
        }
        return result;
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid numeric value for " + name + ": " + value);
    }
}

// Parse a positive integer option without allowing signed unsigned-conversion surprises
size_t parseSizeOption(const std::string& name, const std::string& value) {
    if (value.empty() || value[0] == '-' || value[0] == '+') {
        throw std::runtime_error("Invalid integer value for " + name + ": " + value);
    }

    try {
        size_t parsed = 0;
        const unsigned long long result = std::stoull(value, &parsed);
        if (parsed != value.size() || result > std::numeric_limits<size_t>::max()) {
            throw std::runtime_error("Invalid integer value for " + name + ": " + value);
        }
        return static_cast<size_t>(result);
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid integer value for " + name + ": " + value);
    }
}

// Read positional paths and optional DSP settings from the command line
Options parseArgs(int argc, char** argv) {
    if (argc < 3) {
        throw std::runtime_error("Missing required input and output paths");
    }

    Options options;
    options.inputPath = argv[1];
    options.outputPath = argv[2];

    // Walk through optional arguments after the input and output paths
    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--cutoff") {
            if (++i >= argc) {
                throw std::runtime_error("Missing value for --cutoff");
            }
            options.cutoffHz = parseFloatOption("--cutoff", argv[i]);
        } else if (arg == "--taps") {
            if (++i >= argc) {
                throw std::runtime_error("Missing value for --taps");
            }
            options.tapCount = parseSizeOption("--taps", argv[i]);
        } else if (arg == "--buffer") {
            if (++i >= argc) {
                throw std::runtime_error("Missing value for --buffer");
            }
            options.bufferSize = parseSizeOption("--buffer", argv[i]);
        } else {
            throw std::runtime_error("Unknown option: " + arg);
        }
    }

    // Validate final option values before any file work starts
    if (options.bufferSize == 0) {
        throw std::runtime_error("--buffer must be greater than 0");
    }
    if (options.tapCount == 0) {
        throw std::runtime_error("--taps must be greater than 0");
    }
    if (options.cutoffHz <= 0.0f) {
        throw std::runtime_error("--cutoff must be greater than 0");
    }

    return options;
}

} // namespace

int main(int argc, char** argv) {
    try {
        // Load options, read the WAV, process it, and write the filtered result
        const Options options = parseArgs(argc, argv);

        const AudioData input = readWavFile(options.inputPath);
        const auto coefficients = designLowPassFir(input.sampleRate, options.cutoffHz, options.tapCount);
        const PipelineResult result = processWithFir(input, coefficients, options.bufferSize);
        writeWavFile(options.outputPath, result.audio);

        // Calculate latency numbers from the FIR length and buffer size
        const double duration = durationSeconds(input);
        const double firDelaySamples = static_cast<double>(options.tapCount - 1) / 2.0;
        const double firDelayMs = firDelaySamples / static_cast<double>(input.sampleRate) * 1000.0;
        const double bufferDurationMs = static_cast<double>(options.bufferSize) / static_cast<double>(input.sampleRate) * 1000.0;

        // Final printout to show what was processed and how long it took
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "Loaded " << options.inputPath << '\n';
        std::cout << "Sample rate: " << input.sampleRate << " Hz\n";
        std::cout << "Channels: " << channelCount(input) << '\n';
        std::cout << "Frames: " << frameCount(input) << '\n';
        std::cout << "Duration: " << duration << " s\n\n";

        std::cout << "FIR low-pass:\n";
        std::cout << "Cutoff: " << options.cutoffHz << " Hz\n";
        std::cout << "Taps: " << options.tapCount << '\n';
        std::cout << "Estimated FIR delay: " << firDelayMs << " ms\n";
        std::cout << "Buffer size: " << options.bufferSize << " samples\n";
        std::cout << "Buffer duration: " << bufferDurationMs << " ms\n\n";

        std::cout << "Timing:\n";
        std::cout << "Processed buffers: " << result.timing.bufferCount << '\n';
        std::cout << "Total DSP time: " << result.timing.totalMs << " ms\n";
        std::cout << "Average buffer time: " << result.timing.averageMs() << " ms\n";
        std::cout << "Min buffer time: " << result.timing.minMs << " ms\n";
        std::cout << "Max buffer time: " << result.timing.maxMs << " ms\n";
        std::cout << "Real-time factor: " << result.timing.realTimeFactor(duration) << "x\n\n";

        std::cout << "Wrote " << options.outputPath << '\n';
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n\n";
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }
}
