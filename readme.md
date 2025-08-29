# Companion, TheCube Core Application
Welcome to the repository for the core application running on "Companion, TheCube." This application powers a Raspberry Pi-based desktop companion that integrates smart home features with personal productivity tools in a modular, interactive device.

[![tests](https://github.com/Companion-TheCube/Core/actions/workflows/c-cpp.yml/badge.svg?branch=master)](https://github.com/Companion-TheCube/Core/actions/workflows/c-cpp.yml)

## Getting Started
These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.
### Dependencies
This project targets Debian/Ubuntu. Use the script `./install-deps.sh` to install dependencies.

Build (development) dependencies:
```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake pkg-config git python3 \
  libglew-dev libfreetype6-dev libgl1-mesa-dev libglu1-mesa-dev \
  libasound2-dev libpulse-dev \
  libssl-dev libsodium-dev libsqlite3-dev libglm-dev libpq-dev gettext libboost-all-dev \
  # Windowing/X11 headers used by SFML
  libx11-dev libxrandr-dev libxcursor-dev libxinerama-dev libxi-dev libudev-dev \
  # SFML audio stack
  libopenal-dev libsndfile1-dev libflac-dev libvorbis-dev libogg-dev
  # Images and compression (used by SFML/graphics)
  libjpeg-dev libpng-dev zlib1g-dev
```

Runtime (deployment) dependencies:
```bash
sudo apt-get update
sudo apt-get install -y \
  ca-certificates \
  # Core runtime libs
  libglew2.2 libfreetype6 libgl1 libglu1-mesa libpulse0 \
  libssl3 libsodium23 libsqlite3-0 libpq5 \
  # Windowing/X11 runtime used by SFML
  libx11-6 libxrandr2 libxcursor1 libxinerama1 libxi6 libudev1 \
  # SFML audio runtime codecs
  libopenal1 libsndfile1 libvorbis0a libvorbisfile3 libogg0 \
  # Images and compression runtime
  libjpeg-turbo8 libpng16-16 zlib1g
```

Scripted install:
```bash
# Development (build + test)
./install-deps.sh dev
# Deployment (runtime only)
./install-deps.sh deploy
```
### Installing
A step-by-step series of examples that tell you how to get a development environment running:
1. Clone the repository:
```bash
git clone https://github.com/andrewmcdan/Companion-TheCube---Core.git
```
2. Navigate to the project directory:
```bash
cd Companion-TheCube---Core
```
3. Build the project:
```bash
mkdir build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_BUILD_SERVER=ON
# or for Debug build
# cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_BUILD_SERVER=ON
cmake --build build -j --target CubeCore
```

To skip syncing build information or artifacts with the remote server, configure with:

```
-DENABLE_BUILD_SERVER=OFF
```

Alternatively set the environment variable `CUBECORE_OFFLINE=1` to keep the option enabled but avoid network calls during the build.
4. Run the application:
```bash
./CubeCore
```

## Environment Configuration (.env)
The application reads configuration from a `.env` file in the repo root. Values are loaded once at startup and are available anywhere via `Config::get("KEY")`. Environment variables (exported in your shell or CI) override values in `.env`.

- HTTP: `HTTP_ADDRESS` (e.g., `0.0.0.0`), `HTTP_PORT` (e.g., `55280`).
- IPC: `IPC_SOCKET_PATH` (UNIX domain socket path, e.g., `cube.sock`).
- Tests: `HTTP_PORT_TEST` (e.g., `55281`), `IPC_SOCKET_PATH_TEST` (e.g., `test_ipc.sock`).

Quick start:
```bash
cp .env.example .env
# then edit .env as needed
```

Internals:
- `.env` is parsed by `Config::loadFromDotEnv()` (called in `main.cpp`).
- Access config anywhere with `#include "src/utils.h"` and `Config::get("KEY", "default")`.

## Running the tests
Tests use Google Test and are built as `CubeCoreTests`.

Quick start (Debug):
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_BUILD_SERVER=OFF
cmake --build build -j --target CubeCoreTests
ctest --test-dir build -C Debug -j
```

Notes:
- Ensure you have a `.env` (copy `.env.example`), especially for integration tests.
- Integration tests read `HTTP_PORT_TEST` and `IPC_SOCKET_PATH_TEST`; avoid conflicts with other processes.
- Run a single test via CTest:
  ```bash
  ctest --test-dir build -C Debug -R ApiIntegration -VV
  ```
- Or run the binary directly with Google Test filters:
  ```bash
  ./build/bin/CubeCoreTests --gtest_filter=ApiIntegration.*
  ```
## Deployment
**TODO** Add additional notes about how to deploy this on a live system.
## Dependencies
* [SFML](https://www.sfml-dev.org/) - Simple and Fast Multimedia Library. **to be replaced with SDL2**
* [OpenGL](https://www.opengl.org/) - The Industry's Foundation for High Performance Graphics.
* [GLFW](https://www.glfw.org/) - A multi-platform library for OpenGL, OpenGL ES, Vulkan, window and input.
* [GLEW](http://glew.sourceforge.net/) - The OpenGL Extension Wrangler Library.
* [argparse](https://github.com/p-ranav/argparse) - Command line argument parser for C++.
* [cpp-httplib](https://github.com/yhirose/cpp-httplib) - C++ HTTP server/client library.
* [FreeType](https://www.freetype.org/) - A freely available software library to render fonts.
* [nlohmann/json](https://github.com/nlohmann/json) - JSON for Modern C++.
* [SQLite](https://www.sqlite.org/index.html) - A small, fast, self-contained, high-reliability, full-featured, SQL database engine. 
* [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp) - C++ wrapper around SQLite.
* [Cpp-Base64](https://github.com/ReneNyffenegger/cpp-base64) - Base64 encoding and decoding with c++. **Currently using a version based on code from [here](https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp)**
* [GetText](https://www.gnu.org/software/gettext/) - GNU `gettext` for internationalization.
* [spdlog](https://github.com/gabime/spdlog) - Fast C++ logging library (used for disk logging).
* [fmt]() - A modern formatting library for C++.
* [GoogleTest]() - Google Test framework for C++ unit testing.
* [GoogleBenchmark]() - Google Benchmark library for performance testing.
* [json-rpc-cxx](https://github.com/jsonrpcx/json-rpc-cxx) - A JSON-RPC client and server library for C++.
* [cppcodec](https://github.com/tplgy/cppcodec) - C++ codec library for Base64, Hex, and other encodings.
* [msgpack-c](https://github.com/msgpack/msgpack-c/tree/cpp_master) - C++ implementation of MessagePack, an efficient binary serialization format.

All required system packages for these dependencies can be installed with:
```bash
./install.sh
```

The following fonts are used in the application and have been obtained from [Google Fonts](https://fonts.google.com/):
* [Roboto](https://fonts.google.com/specimen/Roboto)
* [Radio Canada Big](https://fonts.google.com/specimen/Radio+Canada+Big)
* [Assistant](https://fonts.google.com/specimen/Assistant)
## Built With
* [C++](https://en.cppreference.com/w/) - The programming language used.
* [CMake](https://cmake.org/) - Build system.
## Versioning
We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://yourrepositorylink.com/tags).
## Authors
* **Andrew McDaniel** - *Initial work* - [AndrewMcDan](https://github.com/AndrewMcDan)
## License
This project is licensed under an end user license agreement - see the [LICENSE.md]() file for details. Various parts of the project are licensed under different licenses. See the [DEPENDENCIES.md]() file for details.
## Contributing
Please read [CONTRIBUTING.md]() for details on our code of conduct, and the process for submitting pull requests to us.
## Roadmap
**TODO** - See the [open issues]() for a list of proposed features (and known issues).

## Code of Conduct
Please read [CODE_OF_CONDUCT.md]() for details on our code of conduct.

## Acknowledgments
* Hat tip to ChatGPT for all the help
* Inspiration: I saw a video on [YouTube](https://youtu.be/KgEp91__0cY?t=170) that showed an Eilik Robot and thought it was dumb. Decided to make my own version that would be more useful.

https://patorjk.com/software/taag/#p=display&h=0&v=0&f=ANSI%20Shadow&t=The%0ADecision%0AEngine
