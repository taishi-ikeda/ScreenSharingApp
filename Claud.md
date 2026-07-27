# CLAUDE.md - c++ Project Guidlines

## Project Overview
- **Language**: C++17
- **Framework**: Qt 5.15
- **Build System**: CMake
- **Target OS**: Windows, macOS, Linux

## Directory Structure
- `src/`: Source files
- `include/`: Header files
- `resources/`: UI assets and other resources
- `tests/`: Unit tests

## Coding Guidelines
- **Formatting**: Adhere to Google C++ Style Guide (managed via `.clang-format`).
- **Naming Conventions**:
  - Classes / Structs: `PascalCase` (e.g., `MatrixSolver`)
  - Functions / Methods: `camelCase` or `snake_case` (Be consistent within the same module)
  - Member Variables: `m_` prefix or trailing underscore (e.g., `m_data` or `data_`)
  - Constants / Enums: `kPascalCase` or `ALL_CAPS`
- **Modern C++ Guidelines**:
  - **Memory Management**: Avoid raw `new`/`delete`. Always use standard smart pointers (`std::unique_ptr`, `std::shared_ptr`) or value semantics.
  - **Const Correctness**: Mark functions `const` and variables `const`/`constexpr` whenever possible.
  - Pass heavy objects by `const&` or move-semantics (`std::move`).

## Rules & Safety Checks
1. Always make sure the project compiles without warnings (`-Wall -Wextra -Werror`).
2. Run `ctest` or relevant unit test executables before finalizing any major logic changes.
3. Keep headers minimal: use forward declarations in header files where possible to speed up compilation.