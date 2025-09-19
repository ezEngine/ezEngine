# ezEngine Instructions for Claude

## Build Instructions

### Build Commands

```shell
# Generate and build Debug configuration
powershell -NoProfile -ExecutionPolicy ByPass ./RunCMake.ps1 -Target Win64vs2022 -SolutionName "ClaudeBuild" -WorkspaceDir "claude-build"
cmake --build Workspace/claude-build --config Debug

# Generate and build Dev configuration
powershell -NoProfile -ExecutionPolicy ByPass ./RunCMake.ps1 -Target Win64vs2022 -SolutionName "ClaudeBuild" -WorkspaceDir "claude-build"
cmake --build Workspace/claude-build --config Dev

# Clean build
cmake --build Workspace/claude-build --target clean
```

### Build Outputs

- **Solution Location**: `Workspace/claude-build/`
- **Binary Output**: `Workspace/claude-build-output/Bin/WinVs2022[Config]64/`
- **Library Output**: `Workspace/claude-build-output/Lib/WinVs2022[Config]64/`

### Running Tests

```shell
# Build and run specific test
cmake --build Workspace/claude-build --config Debug --target FoundationTest
ctest --test-dir Workspace/claude-build -C Debug -R FoundationTest

# Run all available tests
ctest --test-dir Workspace/claude-build -C Debug

# Run tests with verbose output
ctest --test-dir Workspace/claude-build -C Debug -V
```

### Important Notes

- **Isolated Builds**: Using `-WorkspaceDir "claude-build"` creates a completely separate build environment, avoiding any conflicts with other builds

- **Workspace Cleanup**: To clean up, simply delete the `Workspace/claude-build/` directory and regenerate

- For a basic compilation test, build the **Foundation** library.
