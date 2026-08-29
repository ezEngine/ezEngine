# ezEngine Instructions for Claude

## Build Instructions

### Build Commands

```shell
# Generate and build Debug configuration (use by default)
powershell -NoProfile -ExecutionPolicy ByPass ./RunCMake.ps1 -Target vs2026x64 -SolutionName "ClaudeBuild" -WorkspaceDir "claude-build"
Data/Tools/Precompiled/cmake/bin/cmake --build Workspace/claude-build --config Debug -- -m -p:CL_MPCount=16

# Generate and build Dev configuration
powershell -NoProfile -ExecutionPolicy ByPass ./RunCMake.ps1 -Target vs2026x64 -SolutionName "ClaudeBuild" -WorkspaceDir "claude-build"
Data/Tools/Precompiled/cmake/bin/cmake --build Workspace/claude-build --config Dev -- -m -p:CL_MPCount=16

# Clean build
Data/Tools/Precompiled/cmake/bin/cmake --build Workspace/claude-build --target clean
```

### Build Outputs

- **Solution Location**: `Workspace/claude-build/`
- **Binary Output**: `Workspace/claude-build-output/Bin/WinVs2026[Config]64/`
- **Library Output**: `Workspace/claude-build-output/Lib/WinVs2026[Config]64/`

### Running Tests

```shell
# Build and run specific test
Data/Tools/Precompiled/cmake/bin/cmake --build Workspace/claude-build --config Debug --target FoundationTest -- -m -p:CL_MPCount=16
Data/Tools/Precompiled/cmake/bin/ctest --test-dir Workspace/claude-build -C Debug -R FoundationTest

# Run all available tests
Data/Tools/Precompiled/cmake/bin/ctest --test-dir Workspace/claude-build -C Debug

# Run tests with verbose output
Data/Tools/Precompiled/cmake/bin/ctest --test-dir Workspace/claude-build -C Debug -V

# Run a test executable directly. -noGui is what makes it usable from a script: the tests start on
# their own, output goes to the console, and the process exits when they are done. Without it a
# window opens and waits for interaction.
Workspace/claude-build-output/Bin/WinVs2026Debug64/FoundationTest.exe -noGui

# Run only some tests. -filter does a case insensitive 'contains' check against both test and
# sub-test names; add '*'/'?' for a full-match wildcard pattern instead. Check the output actually
# says what you expect - a filter that matches nothing is reported as a failure, not as a pass.
Workspace/claude-build-output/Bin/WinVs2026Debug64/FoundationTest.exe -noGui -filter "JSON"
Workspace/claude-build-output/Bin/WinVs2026Debug64/FoundationTest.exe -noGui -filter "IO*"
```

`-list` prints the available test names, `-help` every option.

### Important Notes

- **Always pass `-- -m -p:CL_MPCount=16`**: without it the build is effectively single threaded. `-m` lets MSBuild build projects in parallel, but that alone does nothing for a single-target build - `CL_MPCount` is what makes `cl.exe` compile several source files of the *same* project at once. Note that `cmake --build -j` does **not** work here: the Visual Studio generator ignores it. Lower the number on a machine with fewer cores.

- **cmake location**: The repository ships a precompiled cmake at `Data/Tools/Precompiled/cmake/bin/`. Always use this path for `cmake` and `ctest` commands rather than any system-installed version.

- **Isolated Builds**: Using `-WorkspaceDir "claude-build"` creates a completely separate build environment, avoiding any conflicts with other builds

- **Workspace Cleanup**: To clean up, simply delete the `Workspace/claude-build/` directory and regenerate

- **When adding, removing or renaming a file**: CMake has to be run again to update the solution.

- For a basic compilation test, build the **Foundation** library.

- At startup, check which tool permissions you have. Prefer to use tools that can be executed without permission requests.

- EZ_STATICLINK_FILE and EZ_STATICLINK_REFERENCE can be ignored during code generation, they are only relevant for static builds (which we don't do during development) and can automatically be updated with a script. This is not necessary for you to take care of.

## Code Documentation

When documenting code:

* Use a neutral tone for all descriptions.
* Don't use adjectives to describe how great, efficient or otherwise special some piece of code is. Keep the description factual.
* Add information that isn't obvious.
* When documenting a class don't list all its features, if that would just repeat information that is already clearly visible from its interface.
* Give context for where and how something would be used, especially when a name is ambiguous.
* Mention edge cases, for instance what effect certain argument values might have (such as asserts, errors or very different code paths).
* Clearly warn of non-obvious behavior.
* Make it clear when some code can be used for some purpose, but maybe shouldn't be used often, because of performance or other concerns.
* Don't add descriptions that would be redundant. Do not document simple getter, setter or comparison functions. Don't repeat things in comments that are already obvious from class or function names or function arguments. It is fine not to document a class or function, at all, if there is nothing of value to add.

Use this documentation comment style:

```cpp
/// Brief description
///
/// Optional detailed description
/// across multiple lines.
class Object
{
public:
  /// Brief function documentation
  ///
  /// Optional long description.
  void Function();

  /// Public member documentation
  int m_iMember = 0;
};

enum Enum
{
  Value1, ///< comment
  Value2, ///< comment
};
```

Don't use `\brief` or `\return`.
Only use `\param` when it is really necessary to document arguments individually.

For inspiration how detailed to document code, see the Foundation library, where a large part of the code is well documented.
