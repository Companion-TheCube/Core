# Minimum version of CMake required
cmake_minimum_required(VERSION 3.16)

# Project name and the language used
project(CubeCoreProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Freetype REQUIRED)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.x)
FetchContent_MakeAvailable(SFML)

# Add executable
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(CubeCoreProject ${SOURCES})

target_link_libraries(CubeCoreProject PRIVATE sfml-graphics sfml-audio ${ALSA_LIBRARIES} freetype)
target_compile_features(CubeCoreProject PRIVATE cxx_std_17)

install(TARGETS CubeCoreProject)

# Include directories
target_include_directories(CubeCoreProject PRIVATE ${PROJECT_SOURCE_DIR}/src/libs)










