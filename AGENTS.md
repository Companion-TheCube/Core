# Repository Guidelines

## Project Structure & Module Organization
- Source: `src/` (modules like `api/`, `apps/`, `audio/`, `database/`, `decisionEngine/`, `gui/`, etc.). Entry point: `src/main.cpp`.
- Tests: `tests/` mirrors `src/` (e.g., `tests/utils.test.cpp`). Prefer `*.test.cpp`.
- Vendors & headers: `libraries/` (downloaded headers like `httplib.h`, models under `libraries/whisper_models/`).
- Assets: `fonts/`, `data/`, `http/`. App bundles: `BTManager/`, `openwakeword/`.
- Build output: `build/bin/` with resources copied to `build/bin/{shaders,meshes,fonts,http,data,apps}`.

## Build, Test, and Development Commands
- Configure (Debug): `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`
- Build: `cmake --build build -j`
- Run (Linux/macOS): `./build/bin/CubeCore`
- Configure (Release): `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- Optional toggles: `-DBUILD_BT_MANAGER=OFF`, `-DTRANSLATE_ENABLED=OFF`
Notes: First build fetches several dependencies and a Whisper model into `libraries/`; ensure network access.

## Coding Style & Naming Conventions
- Language: Modern C++ (C++23 enabled at project level; target requires at least C++17).
- Indentation: 4 spaces; UTF‑8; Unix line endings on non‑Windows.
- Files: headers `*.h`, sources `*.cpp`; tests `*.test.cpp`.
- Naming: Classes/Types PascalCase, functions/methods lowerCamelCase, constants/macros UPPER_SNAKE_CASE.
- Headers use `#pragma once`. Prefer minimal includes and local `namespace` scope.

## Testing Guidelines
- Framework: Google Test (fetched via CMake). Place tests under `tests/` mirroring `src/` and name `*.test.cpp`.
- Keep tests deterministic and fast; isolate filesystem and network.
- Running: `ctest --test-dir build -C Debug -j` (or `-C Release` to match your build type).

## Commit & Pull Request Guidelines
- Style: Conventional Commits (e.g., `feat: add dashboard`, `fix(gui): wrap message text`, `docs: update API endpoints`).
- Commits: small, focused, imperative mood; reference issues when applicable.
- PRs: clear description, link issues, include screenshots/gifs for UI, and note build/run steps or flags used. Keep PRs scoped; avoid unrelated refactors.

## Security & Configuration Tips
- Do not commit secrets. Use `.env` only for local values; prefer environment variables for CI.
- Large binaries/models are fetched at build; avoid committing them. Verify licenses for any new third‑party additions.
