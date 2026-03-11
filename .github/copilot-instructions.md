# ezEngine

ezEngine is an open-source C++ game engine. It uses CMake for builds and supports Windows (Visual Studio) and Linux (GCC/Clang).

## Project Layout

- `Code/Engine/` - Core engine libraries (Foundation, Core, RendererCore, etc.)
- `Code/EnginePlugins/` - Optional engine plugins (Jolt physics, Fmod audio, etc.)
- `Code/Editor/` - Editor application and framework
- `Code/EditorPlugins/` - Editor-specific plugins
- `Code/Tools/` - Standalone tools and utilities
- `Code/UnitTests/` - Test executables (FoundationTest, CoreTest, etc.)
- `Code/ThirdParty/` - Third-party dependencies
- `Data/` - Runtime data, shaders, and assets
- `Workspace/` - CMake build directories (generated)
- `Output/` - Build outputs (binaries and libraries)

## Build Instructions

Always run CMake configure before building after adding new source files.

### Linux

```shell
./RunCMake.sh --target linux-gcc-debug --solution-name Copilot --workspace-dir copilot && cmake --build Workspace/copilot
```

Build outputs: `Workspace/copilot-output/Bin/LinuxNinjaGccDebug64/` and `Workspace/copilot-output/Lib/LinuxNinjaGccDebug64/`

Available presets: `linux-gcc-debug`, `linux-gcc-dev`, `linux-clang-debug`, `linux-clang-dev`

### Windows

```shell
powershell -NoProfile -ExecutionPolicy ByPass ./RunCMake.ps1 -Target vs2022x64 -SolutionName Copilot -WorkspaceDir copilot
cmake --build Workspace/copilot --config Debug
```

Build outputs: `Workspace/copilot-output/Bin/WinVs2022Debug64/`

## Running Tests

For basic validation, build and run **FoundationTest**. Use `-run -noGui -all` flags for headless execution.

### Linux

```shell
cmake --build Workspace/copilot --target FoundationTest
Workspace/copilot-output/Bin/LinuxNinjaGccDebug64/FoundationTest -run -noGui -all
```

### Windows

```shell
cmake --build Workspace/copilot --config Debug --target FoundationTest
Workspace/copilot-output/Bin/WinVs2022Debug64/FoundationTest.exe -run -noGui -all
```

### Test Arguments

- `-run` - Execute immediately without GUI interaction
- `-noGui` - Headless execution (no window)
- `-all` - Enable all tests
- `-list` - List available tests that can be used with `-filter`
- `-filter <regex>` - Run only tests matching the pattern
- `-help` - Show all arguments

## Code Style

Use `///` for documentation comments. Keep documentation factual and concise. Don't document obvious getters/setters. Mention edge cases and non-obvious behavior. Rely on word-wrap instead of manual line breaks.

```cpp
/// \brief Brief description of the class purpose.
/// Additional detailed description if necessary.
class Object
{
public:
  /// \brief Brief function description.
  void Function();

  int m_iMember = 0; ///< Member description if non-obvious.
};
```

Only use `\return` for non-obvious cases. Use `\param` only when individual parameters need explanation.
