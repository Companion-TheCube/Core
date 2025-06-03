# Codex Agent Guidelines

This repository builds a C++ application with CMake and includes Google Test based unit tests.

## Building
1. Create a `build` directory.
2. Run `cmake ..` inside `build` to configure.
3. Build with `make`.

## Testing
After building, run tests from the `build` directory:
```bash
ctest --output-on-failure
```
Run these tests before committing code.

## Pull Requests
- Summarize changes clearly in the PR body and mention any test results.
- Keep commits focused and use descriptive messages.

