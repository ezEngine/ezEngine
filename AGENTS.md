# ezEngine Instructions for AI Agents

This file applies to every coding agent. `CLAUDE.md` only imports it, so put shared instructions here.

ezEngine is an open-source C++ game engine with an editor. It uses CMake and supports Windows (Visual Studio) and Linux (GCC/Clang).

## Project Layout

- `Code/Engine/` - core engine libraries (Foundation, Core, RendererCore, GameEngine, ...)
- `Code/EnginePlugins/` - optional engine plugins (Jolt physics, Fmod audio, ...)
- `Code/Editor/` - editor application and framework, `ezEditorProcessor`
- `Code/EditorPlugins/` - editor plugins (asset types, scene editing, MCP server, ...)
- `Code/Tools/` - standalone tools
- `Code/UnitTests/` - test executables (FoundationTest, CoreTest, GameEngineTest, RendererTest, EditorTest, ...)
- `Code/ThirdParty/` - third-party dependencies
- `Data/` - runtime data: `Base` (shared assets and shaders), `Samples` (sample projects), `UnitTests` (test data), `Tools/Precompiled` (cmake, clang-format, ...)
- `Workspace/` - generated build directories and their `-output` folders
- `Utilities/` - scripts (CI, asset maintenance, `Scripts/ezddl.py`)

## Skills

Task specific instructions live in skill files. Agents that don't load skills automatically should read the matching file before starting such a task:

- [.claude/skills/ez-mcp/SKILL.md](.claude/skills/ez-mcp/SKILL.md) - launch ezEditor or a game with an MCP server and drive it (assets, documents, scene editing, screenshots, input).
- [.claude/skills/ez-ddl/SKILL.md](.claude/skills/ez-ddl/SKILL.md) - read and edit scenes, prefabs and assets as text files with `Utilities/Scripts/ezddl.py`.
- [.claude/skills/android-build-test/SKILL.md](.claude/skills/android-build-test/SKILL.md) - build, deploy and test on Android devices.
- [.claude/skills/debug-pretty-printers/SKILL.md](.claude/skills/debug-pretty-printers/SKILL.md) - develop the GDB/LLDB pretty printers and natvis visualizers for ezEngine types.

## Build Instructions

### Windows

```shell
# Generate and build Debug configuration (use by default)
powershell -NoProfile -ExecutionPolicy ByPass ./RunCMake.ps1 -Target vs2026x64 -SolutionName "AgentBuild" -WorkspaceDir "agent-build"
Data/Tools/Precompiled/cmake/bin/cmake --build Workspace/agent-build --config Debug -- -m -p:CL_MPCount=16

# Generate and build Dev configuration
powershell -NoProfile -ExecutionPolicy ByPass ./RunCMake.ps1 -Target vs2026x64 -SolutionName "AgentBuild" -WorkspaceDir "agent-build"
Data/Tools/Precompiled/cmake/bin/cmake --build Workspace/agent-build --config Dev -- -m -p:CL_MPCount=16

# Clean build
Data/Tools/Precompiled/cmake/bin/cmake --build Workspace/agent-build --target clean
```

- **Solution Location**: `Workspace/agent-build/`
- **Binary Output**: `Workspace/agent-build-output/Bin/WinVs2026[Config]64/`
- **Library Output**: `Workspace/agent-build-output/Lib/WinVs2026[Config]64/`

### Linux

```shell
./RunCMake.sh --target linux-gcc-debug --solution-name AgentBuild --workspace-dir agent-build
cmake --build Workspace/agent-build
Workspace/agent-build-output/Bin/LinuxNinjaGccDebug64/FoundationTest -noGui
```

Linux uses the system `cmake` (the precompiled tools in `Data/Tools/Precompiled` are Windows binaries). Presets: `linux-gcc-debug`, `linux-gcc-dev`, `linux-clang-debug`, `linux-clang-dev`. `./RunCMake.sh --setup` installs the required packages. Outputs go to `Workspace/agent-build-output/Bin/LinuxNinja<Compiler><Config>64/`.

### Running Tests

The paths below are for Windows; on Linux use `cmake`/`ctest` from the system and the Linux output folder.

```shell
# Build and run specific test
Data/Tools/Precompiled/cmake/bin/cmake --build Workspace/agent-build --config Debug --target FoundationTest -- -m -p:CL_MPCount=16
Data/Tools/Precompiled/cmake/bin/ctest --test-dir Workspace/agent-build -C Debug -R FoundationTest

# Run all available tests
Data/Tools/Precompiled/cmake/bin/ctest --test-dir Workspace/agent-build -C Debug

# Run tests with verbose output
Data/Tools/Precompiled/cmake/bin/ctest --test-dir Workspace/agent-build -C Debug -V

# Run a test executable directly. -noGui is what makes it usable from a script: the tests start on
# their own, output goes to the console, and the process exits when they are done. Without it a
# window opens and waits for interaction.
Workspace/agent-build-output/Bin/WinVs2026Debug64/FoundationTest.exe -noGui

# Run only some tests. -filter does a case insensitive 'contains' check against both test and
# sub-test names; add '*'/'?' for a full-match wildcard pattern instead. Check the output actually
# says what you expect - a filter that matches nothing is reported as a failure, not as a pass.
Workspace/agent-build-output/Bin/WinVs2026Debug64/FoundationTest.exe -noGui -filter "JSON"
Workspace/agent-build-output/Bin/WinVs2026Debug64/FoundationTest.exe -noGui -filter "IO*"
```

`-list` prints the available test names, `-help` every option. `-filter` is not a regex: alternation like `-filter "IO|Serialization"` does not work, run the groups one after another instead. There are no `-run` or `-all` options anymore; `-noGui` starts the tests and runs all of them unless `-filter` is given.

### Important Notes

- **Always pass `-- -m -p:CL_MPCount=16`**: without it the build is effectively single threaded. `-m` lets MSBuild build projects in parallel, but that alone does nothing for a single-target build - `CL_MPCount` is what makes `cl.exe` compile several source files of the *same* project at once. Note that `cmake --build -j` does **not** work here: the Visual Studio generator ignores it. Lower the number on a machine with fewer cores.

- **cmake location**: On Windows the repository ships a precompiled cmake at `Data/Tools/Precompiled/cmake/bin/`. Always use this path for `cmake` and `ctest` commands rather than any system-installed version.

- **Isolated Builds**: Using `-WorkspaceDir "agent-build"` creates a completely separate build environment, avoiding any conflicts with the user's own builds

- **Workspace Cleanup**: To clean up, simply delete the `Workspace/agent-build/` directory and regenerate

- **When adding, removing or renaming a file**: CMake has to be run again to update the solution.

- For a basic compilation test, build the **Foundation** library.

- At startup, check which tool permissions you have. Prefer to use tools that can be executed without permission requests.

- EZ_STATICLINK_FILE and EZ_STATICLINK_REFERENCE can be ignored during code generation, they are only relevant for static builds (which we don't do during development) and can automatically be updated with a script. This is not necessary for you to take care of.

## Code Style

- Follow [CodingGuidelines.md](CodingGuidelines.md), in particular the mandatory type prefixes for member variables and function parameters (`m_uiCount`, `fScale`, `szName`, ...). Match the style of the surrounding code.
- Format the lines you changed with the clang-format version shipped in the repository (formatting whole files would also reformat unrelated code). CI rejects unformatted changes. On Windows (the backslashes in the `-binary` path are required there):

```shell
git diff -U0 --no-color HEAD -- "*.cpp" "*.h" "*.inl" | python Data/Tools/Precompiled/clang-format/clang-format-diff.py -p1 -i -style file -binary "Data\Tools\Precompiled\clang-format\clang-format.exe"
```

On Linux pass `-binary clang-format-18` (a different clang-format version formats some constructs differently).

## Engine Data (Scenes, Prefabs, Assets)

Documents (`.ezScene`, `.ezPrefab`, `.ez*Asset`) are OpenDDL text. Don't write ad-hoc code to decode GUIDs or hex floats; use `python Utilities/Scripts/ezddl.py` (`guid`, `float`, `asset`, `uses`, `show`, `tree`, `check`, `set`, `new-object`, `type`, `example`, `touch`; Python 3.8+, standard library only). The `ez-ddl` skill describes the format and its pitfalls, most importantly:

- After editing a document on disk, run `ezddl.py touch` on it, otherwise the editor considers the asset up to date and does not transform it.
- A document that is open in a running editor is not reloaded from disk and the next save overwrites file edits. Use MCP for open documents (`ez-mcp` skill).
- Run `ezddl.py check` on every document you changed.

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
