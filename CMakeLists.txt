# Minimum version of CMake required
cmake_minimum_required(VERSION 3.10)

# Project name and the language used
project(CubeCoreProject VERSION 1.0 LANGUAGES CXX)

SET(PortAudio_DIR /home/cube/portaudio)
include_directories(/home/cube/portaudio/include)
link_directories(${PORTAUDIO_DIR}/lib)

# Find ALSA library
find_package(ALSA REQUIRED)
include_directories(${ALSA_INCLUDE_DIRS})

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add executable
add_executable(cube src/main.cpp)

# Link against PortAudio and ALSA libraries
target_link_libraries(cube portaudio ${ALSA_LIBRARIES})

# Include directories
target_include_directories(cube PRIVATE ${PROJECT_SOURCE_DIR}/src/libs)
