# Minimum version of CMake required
cmake_minimum_required(VERSION 3.16)

# Project name and the language used
project(CubeCoreProject VERSION 1.0 LANGUAGES CXX)

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

# find_package(GLEW REQUIRED)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.x)
FetchContent_MakeAvailable(SFML)

include(FetchContent)

FetchContent_Declare(
    rtaudio
    GIT_REPOSITORY https://github.com/thestk/rtaudio.git
    GIT_TAG master # You can specify a particular tag or commit if needed
)
FetchContent_MakeAvailable(rtaudio)
FetchContent_GetProperties(rtaudio)
if(NOT rtaudio_POPULATED)
  FetchContent_Populate(rtaudio)
  add_subdirectory(${rtaudio_SOURCE_DIR} ${rtaudio_BINARY_DIR})
  include_directories(${rtaudio_SOURCE_DIR})
endif()


include_directories(${CMAKE_SOURCE_DIR}/src/logger)

# Define the source and destination directories
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/gui/shaders")
if(WIN32)
    set(SHADER_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/shaders")
else()
    set(SHADER_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders")
endif()

set(MESHES_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/gui/renderables/meshes/meshFiles")
if(WIN32)
    set(MESHES_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/meshes")
else()
    set(MESHES_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/meshes")
endif()

# Create the destination directory
file(MAKE_DIRECTORY ${SHADER_DEST_DIR})
file(MAKE_DIRECTORY ${MESHES_DEST_DIR})

# Add executable
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/**/*.cpp" "src/**/**/*.cpp")
add_executable(CubeCoreProject ${SOURCES})

target_link_libraries(CubeCoreProject PRIVATE sfml-graphics sfml-audio rtaudio ${ALSA_LIBRARIES} ${FREETYPE_LIBRARY} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})
target_compile_features(CubeCoreProject PRIVATE cxx_std_17)

# Include directories
if(WIN32)
    target_include_directories(CubeCoreProject PRIVATE ${RTAUDIO_INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/src/libs ${FREETYPE_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${SFML_INCLUDE_DIR} "C:/Users/Andrew/Documents/glm-1.0.1-light")
else()
    target_include_directories(CubeCoreProject PRIVATE ${PROJECT_SOURCE_DIR}/src/libs ${FREETYPE_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${SFML_INCLUDE_DIR})
endif()

# Add a custom command to copy the shaders directory
add_custom_command(
    TARGET CubeCoreProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${SHADER_SOURCE_DIR}
    ${SHADER_DEST_DIR}
    COMMENT "Copying shaders to output directory"
)

# Add a custom command to copy the meshes directory
add_custom_command(
    TARGET CubeCoreProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${MESHES_SOURCE_DIR}
    ${MESHES_DEST_DIR}
    COMMENT "Copying meshess to output directory"
)

# if win32 move executable up one folder
# if(WIN32)
#     add_custom_command(
#         TARGET CubeCoreProject POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy
#         ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/CubeCoreProject.exe
#         ${CMAKE_BINARY_DIR}/CubeCoreProject.exe
#         COMMENT "Copying executable to output directory"
#     )
# endif()

install(TARGETS CubeCoreProject)