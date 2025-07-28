# ████████╗ ██████╗ ██████╗  ██████╗ 
# ╚══██╔══╝██╔═══██╗██╔══██╗██╔═══██╗
#    ██║   ██║   ██║██║  ██║██║   ██║
#    ██║   ██║   ██║██║  ██║██║   ██║
#    ██║   ╚██████╔╝██████╔╝╚██████╔╝
#    ╚═╝    ╚═════╝ ╚═════╝  ╚═════╝ 
# TODO: add build targets for debug and release. release target needs to set a build define for PRODUCTION_BUILD.
# Release build should have optimizations enabled and debug symbols stripped.
# Release build should also set the PRODUCTION_BUILD define.

# Minimum version of CMake required
cmake_minimum_required(VERSION 3.25)

# Project name and the language used
project(CubeCore VERSION 0.1.0 LANGUAGES CXX)

add_definitions(-DMAJOR_VERSION=${PROJECT_VERSION_MAJOR})
add_definitions(-DMINOR_VERSION=${PROJECT_VERSION_MINOR})
add_definitions(-DPATCH_VERSION=${PROJECT_VERSION_PATCH})

add_definitions(-DDOCTEST_CONFIG_DISABLE)

# Comment out the follwoing line to disable building the BTManager application
set(BUILD_BT_MANAGER ON)

# Comment out the following line to disable translation
set(TRANSLATE_ENABLED ON)

# This is a hack to make intellisense work well with a bunch of lambdas
add_definitions(-D_ENABLE_LAMBDAS)

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE "ON" FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_STANDARD_REQUIRED True)
# set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTORCC ON)
# set(CMAKE_AUTOUIC ON)

# Set the build type if it isn't already
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(WIN32)
    set(FREETYPE_INCLUDE_DIRS "C:/Users/Andrew/Documents/freetype/include")
    set(FREETYPE_LIBRARY "C:/Users/Andrew/Documents/freetype/release static/vs2015-2022/win64/freetype.lib")
    set(GLEW_INCLUDE_DIRS "C:/Users/Andrew/Documents/glew-2.1.0/include")
    set(GLEW_LIBRARIES "C:/Users/Andrew/Documents/glew-2.1.0/lib/Release/x64/glew32.lib")
else()
    find_package(GLEW REQUIRED)
endif()

if(UNIX)
    find_package(Gettext REQUIRED)
endif()

find_package(Freetype REQUIRED)

include(FetchContent)
FetchContent_Declare(
    stduuid
    GIT_REPOSITORY https://github.com/mariusbancila/stduuid.git
    GIT_TAG master
    UPDATE_DISCONNECTED FALSE
)
FetchContent_MakeAvailable(stduuid)
include_directories(${stduuid_SOURCE_DIR}/include)

include(FetchContent)
FetchContent_Declare(
    infoware
    GIT_REPOSITORY https://github.com/ThePhD/infoware.git
    GIT_TAG main
    UPDATE_DISCONNECTED FALSE
)
FetchContent_MakeAvailable(infoware)
include_directories(${infoware_SOURCE_DIR}/include)

include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

include(FetchContent)
FetchContent_Declare(
    pqxx
    GIT_REPOSITORY https://github.com/jtv/libpqxx
    GIT_TAG master
)
FetchContent_MakeAvailable(pqxx)

include(FetchContent)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.x
    UPDATE_DISCONNECTED FALSE
)
FetchContent_MakeAvailable(SFML)
include(FetchContent)
FetchContent_Declare(
    WhisperCPP
    GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
    GIT_TAG master
    UPDATE_DISCONNECTED FALSE
)
FetchContent_MakeAvailable(WhisperCPP)
include(FetchContent)
FetchContent_Declare(
    cppcodec
    GIT_REPOSITORY https://github.com/tplgy/cppcodec.git
    GIT_TAG master
    UPDATE_DISCONNECTED FALSE
)
FetchContent_MakeAvailable(cppcodec)
include_directories(${cppcodec_SOURCE_DIR}/cppcodec)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/libraries)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/libraries/whisper_models)
set(WHISPER_MODELS_DIR ${CMAKE_SOURCE_DIR}/libraries/whisper_models)
if(NOT EXISTS ${WHISPER_MODELS_DIR}/large.bin)
    file(DOWNLOAD
    https://huggingface.co/ggerganov/whisper.cpp/blob/main/ggml-large-v3-turbo-q8_0.bin
        ${WHISPER_MODELS_DIR}/large.bin
    )
endif()




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
    set(RTAUDIO_API_PULSE ON CACHE BOOL "" FORCE)
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



set(DOCTEST_HEADER ${CMAKE_SOURCE_DIR}/libraries/doctest.h)

if(NOT EXISTS ${DOCTEST_HEADER})
    file(DOWNLOAD
    https://raw.githubusercontent.com/doctest/doctest/master/doctest/doctest.h
        ${DOCTEST_HEADER}
    )
endif()

# Download httplib.h if not already downloaded
set(HTTPLIB_HEADER ${CMAKE_SOURCE_DIR}/libraries/httplib.h)

if(NOT EXISTS ${HTTPLIB_HEADER})
    file(DOWNLOAD
        https://raw.githubusercontent.com/andrewmcdan/cpp-httplib/master/httplib.h
        ${HTTPLIB_HEADER}
    )
endif()

find_package(OpenSSL REQUIRED)
####### spdlog library (for file logging) #######
include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
    UPDATE_DISCONNECTED FALSE
)
FetchContent_MakeAvailable(spdlog)
# note: provides targets spdlog::spdlog and spdlog::spdlog_header_only

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
# add_definitions(-DSODIUM_STATIC)
if(WIN32)
    include_directories(${CMAKE_SOURCE_DIR}/libraries/libsodium/include)
    find_library(SODIUM_LIBRARIES libsodium HINTS "${CMAKE_SOURCE_DIR}/libraries/libsodium/x64/Release/v143/*" "${CMAKE_SOURCE_DIR}/libraries/libsodium/x64/Debug/v143/*")

    if(NOT SODIUM_LIBRARIES)
        message(FATAL_ERROR "libsodium library not found")
    endif()
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SODIUM REQUIRED libsodium)
    include_directories(${SODIUM_INCLUDE_DIRS})
endif()

# Add SQLiteCpp
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/libraries/SQLiteCpp)

# Add the BTManager application
if(BUILD_BT_MANAGER)
    add_subdirectory(${CMAKE_SOURCE_DIR}/BTManager)
endif()

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

if(WIN32)
    set(DATA_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/data")
else()
    set(DATA_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data")
endif()

# apps directory
if(WIN32)
    set(APPS_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/apps")
else()
    set(APPS_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/apps")
endif()

# Create the destination directories if they don't exist
file(MAKE_DIRECTORY ${SHADER_DEST_DIR})
file(MAKE_DIRECTORY ${MESHES_DEST_DIR})
file(MAKE_DIRECTORY ${FONTS_DEST_DIR})
file(MAKE_DIRECTORY ${HTTP_DEST_DIR})
file(MAKE_DIRECTORY ${MTL_DEST_DIR})
file(MAKE_DIRECTORY ${DATA_DEST_DIR})
file(MAKE_DIRECTORY ${APPS_DEST_DIR})

# Define the regex pattern that matches AutoRegisterAPI<XXXX> where XXXX is the class name
set(MACRO_REGEX "public AutoRegisterAPI<([a-zA-Z_][a-zA-Z0-9_:]*)>")


# Add the executable
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/**/*.cpp" "src/**/**/*.cpp")
add_executable(CubeCore ${SOURCES})


set(InterfaceCount_OUTPUTFILE "${CMAKE_SOURCE_DIR}/src/api/InterfaceCount.h")

file(WRITE "${CMAKE_BINARY_DIR}/count_interfaces.cmake" "
cmake_minimum_required(VERSION 3.25)
file(GLOB_RECURSE SOURCES_AND_HEADERS \"${CMAKE_SOURCE_DIR}/src/*.h\" \"${CMAKE_SOURCE_DIR}/src/**/*.h\" \"${CMAKE_SOURCE_DIR}/src/**/**/*.h\" \"${CMAKE_SOURCE_DIR}/src/*.cpp\" \"${CMAKE_SOURCE_DIR}/src/**/*.cpp\" \"${CMAKE_SOURCE_DIR}/src/**/**/*.cpp\")
function(count_interfaces regex_pattern count_var)
    # Initialize count
    set(count 0)

    # Loop over all source files
    foreach(file \${ARGN})
        # Read the file content
        file(READ \"\${file}\" file_content)

        # Count occurrences of the pattern in the file
        string(REGEX MATCHALL \"\${regex_pattern}\" matches \"\${file_content}\")
        list(LENGTH matches num_matches)

        # Add to total count
        math(EXPR count \"\${count} + \${num_matches}\")
    endforeach()

    # Set the count variable
    set(\${count_var} \${count} PARENT_SCOPE)
endfunction()

count_interfaces(\"public AutoRegisterAPI<([a-zA-Z_][a-zA-Z0-9_:]*)>\" num_interfaces \"\${SOURCES_AND_HEADERS}\")
message(STATUS \"Number of interfaces: \${num_interfaces}\")
# add_compile_definitions(NUM_INTERFACES=\${num_interfaces})
file(WRITE \"${InterfaceCount_OUTPUTFILE}\" \"#pragma once\n#define NUM_INTERFACES \${num_interfaces}\")
")

add_custom_target(
    count_interfaces
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_BINARY_DIR}/count_interfaces.cmake"
)

# add dependency of count_interfaces target to CubeCore target
add_dependencies(CubeCore count_interfaces)


# this is needed for Qt6
# if(WIN32)
#     target_compile_options(CubeCore PRIVATE "/Zc:__cplusplus")
# endif()
# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#     target_compile_definitions(CubeCore PRIVATE DEBUG)
# endif()
# if(CMAKE_BUILD_TYPE STREQUAL "Release")
#     target_compile_definitions(CubeCore PRIVATE RELEASE)
# endif()
if(WIN32)
    set(WIRINGPI_LIB "")
else()
    # set(WIRINGPI_LIB "wiringPi") # Commented out for building on Ubuntu. Uncomment for building on Raspberry Pi.
    set(WIRINGPI_LIB "")
endif()
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
    # Qt6::Core
    # Qt6::SerialPort${WIRINGPI_LIB}
    whisper
    OpenSSL::SSL
    OpenSSL::Crypto
    infoware
    pqxx
    pq
    spdlog::spdlog_header_only
)

# set_target_properties(CubeCore PROPERTIES
#     WIN32_EXECUTABLE TRUE
# )

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
        ${whispercpp_SOURCE_DIR}
   )
else()
    target_include_directories(CubeCore PRIVATE
        ${PROJECT_SOURCE_DIR}/src/libs
        ${FREETYPE_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
        ${SFML_INCLUDE_DIR}
        ${whispercpp_SOURCE_DIR}
        ${PQXX_INCLUDE_DIRS}
    )
endif()

# ##############################
# Localization
# ##############################
if(UNIX AND TRANSLATE_ENABLED)
    add_compile_definitions(TRANSLATE_ENABLED)
    set(LOCALE_DIR "${CMAKE_SOURCE_DIR}/localization")
    set(LOCALE_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/localization")
    set(PO_DIR "${CMAKE_SOURCE_DIR}/localization/po")
    set(MO_DIR "${CMAKE_SOURCE_DIR}/localization/mo")

    # Locales to install
    set(LANGUAGES en_US es fr de it ja ko)

    # Create the destination directory if it doesn't exist
    file(MAKE_DIRECTORY ${LOCALE_DEST_DIR})

    if(NOT EXISTS ${PO_DIR})
        file(MAKE_DIRECTORY ${PO_DIR})
    endif()
    if(NOT EXISTS ${MO_DIR})
        file(MAKE_DIRECTORY ${MO_DIR})
    endif()
    foreach(LANG ${LANGUAGES})
        if(NOT EXISTS ${MO_DIR}/${LANG})
            file(MAKE_DIRECTORY ${MO_DIR}/${LANG})
            file(MAKE_DIRECTORY ${MO_DIR}/${LANG}/LC_MESSAGES)
            file(MAKE_DIRECTORY ${PO_DIR}/${LANG})
        endif()
    endforeach()
    
    add_custom_target(GenrateTranslationPotFiles # not working
        foreach(SRC ${SOURCES})
            add_custom_command(
                OUTPUT ${PO_DIR}/${SRC}.pot
                COMMAND xgettext --keyword=_ --language=C++ --sort-output --from-code=UTF-8 --package-name=CubeCore --package-version=1.0.0 -o ${PO_DIR}/${SRC}.pot ${SRC}
                # DEPENDS ${SRC}
                COMMENT "Extracting strings from ${SRC}"
            )
        endforeach()
    )

    add_custom_target(GenerateTranslationMoFiles
        foreach(LANG ${LANGUAGES})
            add_custom_command(
                OUTPUT ${MO_DIR}/${LANG}/LC_MESSAGES/CubeCore.mo
                COMMAND ${CMAKE_COMMAND} -E make_directory ${MO_DIR}/${LANG}/LC_MESSAGES
                COMMAND msgfmt -o ${MO_DIR}/${LANG}/LC_MESSAGES/CubeCore.mo ${PO_DIR}/${LANG}.po
                DEPENDS ${PO_DIR}/${LANG}.po
                COMMENT "Compiling ${LANG} translation"
            )
        endforeach()
    )

    add_custom_target(UpdateTranslationsPoFiles
        foreach(LANG ${LANGUAGES})
            add_custom_command(
                OUTPUT ${PO_DIR}/${LANG}.po
                COMMAND msgmerge --update ${PO_DIR}/${LANG}.po ${PO_DIR}/${LANG}.pot
                DEPENDS ${PO_DIR}/${LANG}.pot
                COMMENT "Updating ${LANG} translation"
            )
        endforeach()
    )

    # Create a custom target to run all translation-related tasks
    add_custom_target(update_translations
        foreach(LANG ${LANGUAGES})
        DEPENDS ${MO_DIR}/${LANG}/LC_MESSAGES/CubeCore.mo
        endforeach()
        COMMENT "Updating and compiling translation files"
    )

    # ###############################
    # Copy the locale files to the output directory
    # ###############################
    foreach(LANG ${LANGUAGES})
        add_custom_command(
            TARGET CubeCore POST_BUILD
            TARGET update_translations
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${MO_DIR}/${LANG}
            ${LOCALE_DEST_DIR}/${LANG}
            COMMENT "Copying ${LANG} locale files to output directory"
        )
    endforeach()

    
endif()

# ###############################
# Copy data files to the output directory
# ###############################

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

# Add a custom command to copy the data directory
add_custom_command(
    TARGET CubeCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/data
    ${DATA_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Copying data to output directory"
)

# copy base apps to the apps directory
add_custom_command(
    TARGET CubeCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_HOME_DIRECTORY}/BTManager/
    ${APPS_DEST_DIR}/BTManager/
    COMMAND ${CMAKE_COMMAND} -E echo "Copying BTManager to apps directory"
)
add_custom_command(
    TARGET CubeCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_HOME_DIRECTORY}/openwakeword/
    ${APPS_DEST_DIR}/openwakeword/
    COMMAND ${CMAKE_COMMAND} -E echo "Copying openwakeword to apps directory"
)

# Variables
set(BUILD_NUMBER_HEADER "${CMAKE_SOURCE_DIR}/libraries/build_number.h")
set(GET_BUILD_NUMBER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/get_next_build_number.py")

# Ensure Python is found
find_package(Python3 REQUIRED)

# Custom command to generate the build number header
add_custom_command(
    TARGET CubeCore PRE_BUILD
    # OUTPUT "${BUILD_NUMBER_HEADER}"
    COMMAND ${PYTHON_EXECUTABLE} "${GET_BUILD_NUMBER_SCRIPT}" "${BUILD_NUMBER_HEADER}" "${CMAKE_SOURCE_DIR}"
    DEPENDS "${GET_BUILD_NUMBER_SCRIPT}"
    COMMENT "Fetching build number from server..."
)

# Custom target that depends on the custom command
# add_custom_target(
#     FetchBuildNumber ALL
#     DEPENDS "${BUILD_NUMBER_HEADER}"
# )


# # Ensure the build number is fetched before building the executable
# add_dependencies(CubeCore FetchBuildNumber)

# Include the directory where the header is located
# include_directories("${CMAKE_BINARY_DIR}")

# Define path to the build number file
# set(BUILD_NUMBER_FILE "${CMAKE_SOURCE_DIR}/build_number.txt")

# Custom command to increment the build number and write it to the file
# add_custom_command(
#     TARGET CubeCore POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/incr_build_num.cmake"
#     COMMENT "Incrementing build number..."
# )

# Custom target that will always run
# add_custom_target(incr_build_num ALL
#     DEPENDS "${BUILD_NUMBER_FILE}"
#     COMMENT "Build number increment target"
# )

# Ensure that the build number target runs before your actual target
# add_dependencies(CubeCore incr_build_num)

# Optionally, you can define this build number as a preprocessor definition
# file(READ "${BUILD_NUMBER_FILE}" BUILD_NUMBER)
# add_definitions(-DBUILD_NUMBER=${BUILD_NUMBER})

# # Print the build number in the output
# message(STATUS "Build number: ${BUILD_NUMBER}")

install(TARGETS CubeCore)

# ###############################
# Tests
# ###############################

# Add tests
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG main
)

# Fetch googletest
FetchContent_MakeAvailable(googletest)

# Add test executable
file(GLOB_RECURSE TEST_SOURCES
    "tests/*.cpp" "tests/**/*.cpp"
    "src/*.cpp" "src/**/*.cpp" "src/**/**/*.cpp")
# Exclude the application entry point from the test build
list(FILTER TEST_SOURCES EXCLUDE REGEX "/main\\.cpp$")
add_executable(CubeCoreTests ${TEST_SOURCES})
target_link_libraries(CubeCoreTests PRIVATE
    gtest
    gtest_main
    gmock
    gmock_main
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
    whisper
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Set compiler flags to output assembly
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -save-temps")

enable_testing()
add_test(NAME CubeCoreTests COMMAND CubeCoreTests)

# ###############################
# Benchmarks
# ###############################

# Add benchmarks if Google Benchmark is found
find_package(benchmark QUIET)

if(benchmark_FOUND)
    # Add benchmarks executable
    file(GLOB_RECURSE BENCHMARK_SOURCES "benchmarks/*.cpp" "benchmarks/**/*.cpp")
    add_executable(CubeCoreBenchmarks ${BENCHMARK_SOURCES})
    target_link_libraries(CubeCoreBenchmarks PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
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
        whisper
        OpenSSL::SSL
        OpenSSL::Crypto
    )
else()
    message(WARNING "Google Benchmark not found, skipping benchmarks.")
    message(WARNING "To build benchmarks, install Google Benchmark and re-run CMake.")
endif()

# ─── TODO synchronisation ────────────────────────────────────────────────────
set(UPDATE_TODOS_SCRIPT "${CMAKE_SOURCE_DIR}/update_todos.py")
set(TODO_MARKDOWN_FILE  "${CMAKE_SOURCE_DIR}/TODO's.md")

add_custom_target(update_todos
    COMMAND ${Python3_EXECUTABLE} "${UPDATE_TODOS_SCRIPT}"
    BYPRODUCTS "${TODO_MARKDOWN_FILE}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Scanning source tree and refreshing TODO's.md"
)

# Always refresh the TODO list before we compile anything else
add_dependencies(CubeCore update_todos)
