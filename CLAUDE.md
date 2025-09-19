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

Don't use `/brief` or `/return`.
Only use `/param` when it is really necessary to document arguments individually.

For inspiration how detailed to document code, see the Foundation library, where a large part of the code is well documented.
