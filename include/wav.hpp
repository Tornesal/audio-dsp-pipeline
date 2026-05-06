#pragma once

#include "audio.hpp"

#include <string>

AudioData readWavFile(const std::string& path);
void writeWavFile(const std::string& path, const AudioData& audio);
