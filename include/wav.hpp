#pragma once

#include "audio.hpp"

#include <string>

// Read a mono PCM16 WAV file into normalized float audio
AudioData readWavFile(const std::string& path);

// Write normalized float audio back out as a mono PCM16 WAV file
void writeWavFile(const std::string& path, const AudioData& audio);
