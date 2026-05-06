cmake_minimum_required(VERSION 3.16)

project(RealTimeAudioDSP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(dsp_pipeline
    src/main.cpp
    src/wav.cpp
    src/fir.cpp
    src/pipeline.cpp
    src/timer.cpp
)

target_include_directories(dsp_pipeline PRIVATE include)

if(MSVC)
    target_compile_options(dsp_pipeline PRIVATE /W4)
else()
    target_compile_options(dsp_pipeline PRIVATE -Wall -Wextra -pedantic)
endif()
