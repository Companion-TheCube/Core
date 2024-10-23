# Companion, TheCube Core Application
Welcome to the repository for the core application running on "Companion, TheCube." This application powers a Raspberry Pi-based desktop companion that integrates smart home features with personal productivity tools in a modular, interactive device.
## Getting Started
These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.
### Prerequisites
What things you need to install the software and how to install them:
```bash
sudo apt-get update
sudo apt-get install build-essential
sudo apt-get install cmake
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
cd build
cmake ..
make
```
4. Run the application:
```bash
./CubeCore
```
## Running the tests
**TODO** Explain how to run the automated tests for this system:
## Deployment
**TODO** Add additional notes about how to deploy this on a live system.
## Dependencies
* [SFML](https://www.sfml-dev.org/) - Simple and Fast Multimedia Library. **to be replaced with GLFW**
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
* [spdlog] ?
* [fmt] ?
* [doctest] ?
* [Catch2] ?
* [GoogleTest] ?
* [GoogleBenchmark] ?

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