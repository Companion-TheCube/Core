# ████████╗ ██████╗ ██████╗  ██████╗ 
# ╚══██╔══╝██╔═══██╗██╔══██╗██╔═══██╗
#    ██║   ██║   ██║██║  ██║██║   ██║
#    ██║   ██║   ██║██║  ██║██║   ██║
#    ██║   ╚██████╔╝██████╔╝╚██████╔╝
#    ╚═╝    ╚═════╝ ╚═════╝  ╚═════╝ 
# Build targets for Debug and Release are configured below.
# Release builds enable optimizations, strip debug symbols, and define PRODUCTION_BUILD.

# Minimum version of CMake required
cmake_minimum_required(VERSION 3.5)

# Project name and the language used
project(CubeCore VERSION 0.1.0 LANGUAGES CXX)

add_definitions(-DMAJOR_VERSION=${PROJECT_VERSION_MAJOR})
add_definitions(-DMINOR_VERSION=${PROJECT_VERSION_MINOR})
add_definitions(-DPATCH_VERSION=${PROJECT_VERSION_PATCH})

# Add definition to enable httplib SSL support
add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)

# Comment out the follwoing line to disable building the BTManager application
if(NOT DEFINED BUILD_BT_MANAGER)
    option(BUILD_BT_MANAGER "Build the BTManager application" ON)
endif()

# Comment out the following line to disable translation
set(TRANSLATE_ENABLED ON)

# Optionally sync build information and artifacts with the build server
option(ENABLE_BUILD_SERVER "Sync build info/artifacts with server" OFF)

# Optionally run helper scripts from the tooling directory
option(ENABLE_TOOLING_SCRIPTS "Run helper tooling scripts" OFF)

# This is a hack to make intellisense work well with a bunch of lambdas
add_definitions(-D_ENABLE_LAMBDAS)

# Define the number of threads in the thread pool for httplib
add_definitions(-DCPPHTTPLIB_THREAD_POOL_COUNT=32)

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

# Configure compiler and linker flags for each build type
if(MSVC)
    add_compile_options(
        "$<$<CONFIG:Debug>:/Od;/Zi>"
        "$<$<CONFIG:Release>:/O2;/DNDEBUG>"
    )
    add_link_options(
        "$<$<CONFIG:Release>:/INCREMENTAL:NO;/DEBUG:NONE>"
    )
else()
    add_compile_options(
        "$<$<CONFIG:Debug>:-O0;-g3>"
        "$<$<CONFIG:Release>:-O3;-DNDEBUG>"
    )
    add_link_options(
        "$<$<CONFIG:Release>:-s>"
    )
endif()

# Define PRODUCTION_BUILD for release configuration
add_compile_definitions($<$<CONFIG:Release>:PRODUCTION_BUILD>)

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


# Use the name "json" to satisfy subprojects that call FetchContent_Declare(json ...)
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
  json_schema_validator
  GIT_REPOSITORY https://github.com/pboettch/json-schema-validator.git
  GIT_TAG 2.3.0
)
# optional: the project has tests/examples—disable to keep things minimal
set(JSON_VALIDATOR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(JSON_VALIDATOR_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(json_schema_validator)
include_directories(${json_schema_validator_SOURCE_DIR}/src)


FetchContent_Declare(
    pqxx
    GIT_REPOSITORY https://github.com/jtv/libpqxx
    GIT_TAG master
)
FetchContent_MakeAvailable(pqxx)


FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.x
    UPDATE_DISCONNECTED FALSE
)
FetchContent_MakeAvailable(SFML)

set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_SERVER   OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(WHISPER_SDL2           OFF CACHE BOOL "" FORCE)
set(WHISPER_CURL           OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    WhisperCPP
    GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
    GIT_TAG master
    UPDATE_DISCONNECTED FALSE
)
FetchContent_MakeAvailable(WhisperCPP)

FetchContent_Declare(
    cppcodec
    GIT_REPOSITORY https://github.com/tplgy/cppcodec.git
    GIT_TAG master
    UPDATE_DISCONNECTED FALSE
)
FetchContent_MakeAvailable(cppcodec)
include_directories(${cppcodec_SOURCE_DIR}/cppcodec)

FetchContent_Declare(
    json_rpc_cxx
    GIT_REPOSITORY https://github.com/jsonrpcx/json-rpc-cxx.git
    GIT_TAG master
    UPDATE_DISCONNECTED FALSE
)

FetchContent_GetProperties(json_rpc_cxx)
if(NOT json_rpc_cxx_POPULATED)
  FetchContent_Populate(json_rpc_cxx)
  # header-only: just add include path
  add_library(json-rpc-cxx INTERFACE)
  target_include_directories(json-rpc-cxx INTERFACE
    ${json_rpc_cxx_SOURCE_DIR}/include)
  # It needs nlohmann_json for types
  target_link_libraries(json-rpc-cxx INTERFACE nlohmann_json::nlohmann_json)
endif()


FetchContent_Declare(
    msgpack_c
    GIT_REPOSITORY https://github.com/msgpack/msgpack-c.git
    GIT_TAG cpp_master
    UPDATE_DISCONNECTED FALSE
)
FetchContent_MakeAvailable(msgpack_c)
include_directories(${msgpack_c_SOURCE_DIR}/include)


FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(asio)
include_directories(${asio_SOURCE_DIR}/asio/include)


file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/libraries)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/libraries/whisper_models)
set(WHISPER_MODELS_DIR ${CMAKE_SOURCE_DIR}/libraries/whisper_models)
if(NOT EXISTS ${WHISPER_MODELS_DIR}/large.bin)
    file(DOWNLOAD
    https://huggingface.co/ggerganov/whisper.cpp/blob/main/ggml-large-v3-turbo-q8_0.bin
        ${WHISPER_MODELS_DIR}/large.bin
    )
endif()

find_package(sdbus-c++ REQUIRED)


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




# Download httplib.h if not already downloaded
set(HTTPLIB_HEADER ${CMAKE_SOURCE_DIR}/libraries/httplib.h)

if(NOT EXISTS ${HTTPLIB_HEADER})
    file(DOWNLOAD
        https://raw.githubusercontent.com/andrewmcdan/cpp-httplib/master/httplib.h
        ${HTTPLIB_HEADER}
    )
endif()

find_package(OpenSSL REQUIRED)
if (OpenSSL_FOUND)
    # Ensure OpenSSL libraries are linked for targets that use httplib/OpenSSL functions
    # Some FetchContent subprojects (vendor cpp-httplib) expect SSL symbols; adding
    # these linker flags globally ensures the linker can resolve them during subproject
    # link steps. This is a pragmatic fix for environments where subprojects don't
    # automatically link OpenSSL.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenSSL_LIBRARIES}")
endif()
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

# SSL Directory
set(SSL_SOURCE_DIR "${CMAKE_SOURCE_DIR}/ssl")
if(WIN32)
    set(SSL_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/ssl")
else()
    set(SSL_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ssl")
endif()
file(MAKE_DIRECTORY ${SSL_DEST_DIR})
if(NOT EXISTS ${SSL_DEST_DIR}/server.key)
    file(COPY ${SSL_SOURCE_DIR}/server.key DESTINATION ${SSL_DEST_DIR})
endif()
if(NOT EXISTS ${SSL_DEST_DIR}/server.crt)
    file(COPY ${SSL_SOURCE_DIR}/server.crt DESTINATION ${SSL_DEST_DIR})
endif()

# copy ./.env file to the output directory
if(WIN32)
    set(ENV_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug")
else()
    set(ENV_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()
if(EXISTS ${CMAKE_SOURCE_DIR}/.env)
    file(COPY ${CMAKE_SOURCE_DIR}/.env DESTINATION ${ENV_DEST_DIR})
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
file(GLOB_RECURSE ALL_SOURCES "src/*.cpp" "src/**/*.cpp" "src/**/**/*.cpp")
# Split main from library sources
set(MAIN_SOURCE "${CMAKE_SOURCE_DIR}/src/main.cpp")
set(LIB_SOURCES ${ALL_SOURCES})
list(REMOVE_ITEM LIB_SOURCES ${MAIN_SOURCE})

# Core library with most of the code
add_library(CubeCoreLib STATIC ${LIB_SOURCES})

# Application executable contains only main and links the core library
add_executable(CubeCore ${MAIN_SOURCE})


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
add_dependencies(CubeCoreLib count_interfaces)
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

target_compile_definitions(CubeCoreLib PRIVATE ASIO_NO_DEPRECATED ASIO_STANDALONE)
# Link libraries to the core library so consumers inherit them
target_link_libraries(CubeCoreLib PUBLIC
    sfml-graphics
    sfml-audio
    sfml-network
    rtaudio
    ${ALSA_LIBRARIES}
    ${FREETYPE_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    nlohmann_json::nlohmann_json
    nlohmann_json_schema_validator
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
    json-rpc-cxx
    msgpack-cxx
    cppcodec
    SDBusCpp::sdbus-c++
)

# Executable links the core library
target_link_libraries(CubeCore PRIVATE CubeCoreLib)

target_compile_features(CubeCoreLib PUBLIC cxx_std_17)

# Include directories
if(WIN32)
    target_include_directories(CubeCoreLib PUBLIC
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
    target_include_directories(CubeCoreLib PUBLIC
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

# Copy Whisper models directory to runtime output directory
set(WHISPER_MODELS_SRC_DIR "${CMAKE_SOURCE_DIR}/libraries/whisper_models")
if(EXISTS ${WHISPER_MODELS_SRC_DIR})
    set(WHISPER_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/whisper_models")
    add_custom_command(
        TARGET CubeCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${WHISPER_DEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${WHISPER_MODELS_SRC_DIR} ${WHISPER_DEST_DIR}
        COMMENT "Copying Whisper models to output directory"
    )
else()
    message(WARNING "Whisper models directory not found at ${WHISPER_MODELS_SRC_DIR}. Skipping copy.")
endif()

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

# Ensure Python is found when needed
if(ENABLE_BUILD_SERVER OR ENABLE_TOOLING_SCRIPTS)
    find_package(Python3 REQUIRED)
endif()

if(ENABLE_BUILD_SERVER AND ENABLE_TOOLING_SCRIPTS)
    # Variables
    set(BUILD_NUMBER_HEADER "${CMAKE_SOURCE_DIR}/libraries/build_number.h")
    set(UPLOAD_ARTIFACT_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tooling/upload_artifact.py")

    add_custom_command(
        TARGET CubeCore POST_BUILD
        COMMAND ${PYTHON_EXECUTABLE} "${UPLOAD_ARTIFACT_SCRIPT}" "${CMAKE_SOURCE_DIR}" "${BUILD_NUMBER_HEADER}"
        DEPENDS "${UPLOAD_ARTIFACT_SCRIPT}"
        COMMENT "Uploading build to server..."
    )

    set(GET_BUILD_NUMBER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tooling/get_next_build_number.py")

    # Custom command to generate the build number header
    add_custom_command(
        TARGET CubeCore PRE_BUILD
        COMMAND ${PYTHON_EXECUTABLE} "${GET_BUILD_NUMBER_SCRIPT}" "${BUILD_NUMBER_HEADER}" "${CMAKE_SOURCE_DIR}"
        DEPENDS "${GET_BUILD_NUMBER_SCRIPT}"
        COMMENT "Fetching build number from server..."
    )
endif()

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

# Add test executable (tests only)
file(GLOB_RECURSE TEST_SOURCES
    "tests/*.cpp" "tests/**/*.cpp")
add_executable(CubeCoreTests ${TEST_SOURCES})
target_link_libraries(CubeCoreTests PRIVATE
    gtest
    gtest_main
    gmock
    gmock_main
    CubeCoreLib
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
        CubeCoreLib
    )
else()
    message(WARNING "Google Benchmark not found, skipping benchmarks.")
    message(WARNING "To build benchmarks, install Google Benchmark and re-run CMake.")
endif()

# ─── TODO synchronisation ────────────────────────────────────────────────────
if(ENABLE_TOOLING_SCRIPTS)
    set(UPDATE_TODOS_SCRIPT "${CMAKE_SOURCE_DIR}/tooling/update_todos.py")
    set(TODO_MARKDOWN_FILE  "${CMAKE_SOURCE_DIR}/TODO's.md")

    add_custom_target(update_todos
        COMMAND ${Python3_EXECUTABLE} "${UPDATE_TODOS_SCRIPT}"
        BYPRODUCTS "${TODO_MARKDOWN_FILE}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Scanning source tree and refreshing TODO's.md"
    )

    # Always refresh the TODO list before we compile anything else
    add_dependencies(CubeCore update_todos)
endif()
