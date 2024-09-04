# Minimum version of CMake required
cmake_minimum_required(VERSION 3.16)

# Project name and the language used
project(CubeCore VERSION 1.0 LANGUAGES CXX)

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE "ON" FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_STANDARD_REQUIRED True)

# determine if we are building on windows
if(WIN32)
    set(FREETYPE_INCLUDE_DIRS "C:/Users/Andrew/Documents/freetype/include")
    set(FREETYPE_LIBRARY "C:/Users/Andrew/Documents/freetype/release static/vs2015-2022/win32/freetype.lib")
    set(GLEW_INCLUDE_DIRS "C:/Users/Andrew/Documents/glew-2.1.0/include")
    set(GLEW_LIBRARIES "C:/Users/Andrew/Documents/glew-2.1.0/lib/Release/Win32/glew32.lib")
else()
    find_package(GLEW REQUIRED)
endif()

find_package(Freetype REQUIRED)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.x)
FetchContent_MakeAvailable(SFML)
include(FetchContent)

if(WIN32)
    # Settings for Windows
    set(RTAUDIO_API_DS ON CACHE BOOL "" FORCE)
    set(RTAUDIO_API_ASIO OFF CACHE BOOL "" FORCE)
    set(RTAUDIO_API_WASAPI OFF CACHE BOOL "" FORCE)
    set(RTAUDIO_API_WDMKS OFF CACHE BOOL "" FORCE)
elseif(UNIX)
    # Settings for Linux
    set(RTAUDIO_API_ALSA ON CACHE BOOL "" FORCE)
    set(RTAUDIO_API_JACK OFF CACHE BOOL "" FORCE)
    set(RTAUDIO_API_OSS OFF CACHE BOOL "" FORCE)
    set(RTAUDIO_API_PULSE OFF CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
    rtaudio
    GIT_REPOSITORY https://github.com/thestk/rtaudio.git
    GIT_TAG master # You can specify a particular tag or commit if needed
)
FetchContent_MakeAvailable(rtaudio)
FetchContent_GetProperties(rtaudio)

if(NOT rtaudio_POPULATED)
    FetchContent_Populate(rtaudio)

    # Add the RtAudio subdirectory with the new configuration
    add_subdirectory(${rtaudio_SOURCE_DIR} ${rtaudio_BINARY_DIR})
    include_directories(${rtaudio_SOURCE_DIR})
endif()

# Create libraries directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/libraries)

# Download httplib.h if not already downloaded
set(HTTPLIB_HEADER ${CMAKE_SOURCE_DIR}/libraries/httplib.h)

if(NOT EXISTS ${HTTPLIB_HEADER})
    file(DOWNLOAD
        https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
        ${HTTPLIB_HEADER}
    )
endif()

set(ARGPARSE_HEADER ${CMAKE_SOURCE_DIR}/libraries/argparse.hpp)

if(NOT EXISTS ${ARGPARSE_HEADER})
    file(DOWNLOAD
        https://raw.githubusercontent.com/p-ranav/argparse/master/include/argparse/argparse.hpp
        ${ARGPARSE_HEADER}
    )
endif()

set(TINY_OBJ_LOADER_HEADER ${CMAKE_SOURCE_DIR}/libraries/tiny_obj_loader.h)

if(NOT EXISTS ${TINY_OBJ_LOADER_HEADER})
    file(DOWNLOAD
        https://raw.githubusercontent.com/tinyobjloader/tinyobjloader/50461d0e0a77c178bb478e9319d7de82f469a848/tiny_obj_loader.h
        ${TINY_OBJ_LOADER_HEADER}
    )
endif()

# Add libsodium
add_definitions(-DSODIUM_STATIC)

if(WIN32)
    include_directories(${CMAKE_SOURCE_DIR}/libraries/libsodium/include)
    find_library(SODIUM_LIBRARIES libsodium HINTS "${CMAKE_SOURCE_DIR}/libraries/libsodium/Win32/Release/v143/*" "${CMAKE_SOURCE_DIR}/libraries/libsodium/Win32/Debug/v143/*")
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SODIUM REQUIRED libsodium)
    include_directories(${SODIUM_INCLUDE_DIRS})
endif()

# Add SQLiteCpp
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/libraries/SQLiteCpp)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/logger)
include_directories(${CMAKE_SOURCE_DIR}/libraries)
include_directories(${CMAKE_SOURCE_DIR}/src/settings)

if(WIN32)
    include_directories("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.39.33519/include")
endif()

# Define the source and destination directories for shaders
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/gui/shaders")

if(WIN32)
    set(SHADER_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/shaders")
else()
    set(SHADER_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders")
endif()

# Define the source and destination directories for meshes
set(MESHES_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/gui/renderables/meshes")

if(WIN32)
    set(MESHES_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/meshes")
else()
    set(MESHES_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/meshes")
endif()

# Define the source and destination directories for fonts
set(FONTS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/fonts")

if(WIN32)
    set(FONTS_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/fonts")
else()
    set(FONTS_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fonts")
endif()

set(HTTP_SOURCE_DIR "${CMAKE_SOURCE_DIR}/http")

if(WIN32)
    set(HTTP_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/http")
else()
    set(HTTP_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/http")
endif()

if(WIN32)
    set(MTL_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug")
else()
    set(MTL_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

# Create the destination directories if they don't exist
file(MAKE_DIRECTORY ${SHADER_DEST_DIR})
file(MAKE_DIRECTORY ${MESHES_DEST_DIR})
file(MAKE_DIRECTORY ${FONTS_DEST_DIR})
file(MAKE_DIRECTORY ${HTTP_DEST_DIR})

# Add executable
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/**/*.cpp" "src/**/**/*.cpp")
add_executable(CubeCore ${SOURCES})

# Link libraries
target_link_libraries(CubeCore PRIVATE
    sfml-graphics
    sfml-audio
    sfml-network
    rtaudio
    ${ALSA_LIBRARIES}
    ${FREETYPE_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    nlohmann_json::nlohmann_json
    SQLiteCpp
    sqlite3
    ${SODIUM_LIBRARIES}
)
target_compile_features(CubeCore PRIVATE cxx_std_17)

# Include directories
if(WIN32)
    target_include_directories(CubeCore PRIVATE
        ${RTAUDIO_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/src/libs
        ${FREETYPE_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
        ${SFML_INCLUDE_DIR}
        "C:/Users/Andrew/Documents/glm-1.0.1-light"
    )
else()
    target_include_directories(CubeCore PRIVATE
        ${PROJECT_SOURCE_DIR}/src/libs
        ${FREETYPE_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
        ${SFML_INCLUDE_DIR}
    )
endif()

# Add a custom command to copy the shaders directory
add_custom_command(
    TARGET CubeCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${SHADER_SOURCE_DIR}
    ${SHADER_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying shaders to output directory"
)

# create the meshes directory if it doesn't exist
file(MAKE_DIRECTORY ${MESHES_DEST_DIR})

# Add a custom command to copy the meshes directory
add_custom_command(
    TARGET CubeCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${MESHES_SOURCE_DIR}
    ${MESHES_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying meshess to output directory"
)

# Add a custom command to copy black_and_white.mtl from the meshFiles directory to the output directory
add_custom_command(
    TARGET CubeCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${MESHES_SOURCE_DIR}/black_and_white.mtl
    ${MTL_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying black_and_white.mtl to output directory"
)

# Add a custom command to copy the fonts directory
add_custom_command(
    TARGET CubeCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${FONTS_SOURCE_DIR}
    ${FONTS_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying fonts to output directory"
)

# Add a custom command to copy the http directory
add_custom_command(
    TARGET CubeCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${HTTP_SOURCE_DIR}
    ${HTTP_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying http to output directory"
)

install(TARGETS CubeCore)

# Add tests
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG main
)

# Fetch googletest
FetchContent_MakeAvailable(googletest)

# Add test executable
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp" "tests/**/*.cpp" "tests/**/**/*.cpp")
add_executable(CubeCoreTests ${TEST_SOURCES})
target_link_libraries(CubeCoreTests PRIVATE
    gtest
    gtest_main
    sfml-graphics
    sfml-audio
    sfml-network
    rtaudio
    ${ALSA_LIBRARIES}
    ${FREETYPE_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    nlohmann_json::nlohmann_json
    SQLiteCpp
    sqlite3
    ${SODIUM_LIBRARIES}
)

enable_testing()
add_test(NAME CubeCoreTests COMMAND CubeCoreTests)