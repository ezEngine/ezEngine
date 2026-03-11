#pragma once

#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/VariantType.h>

/// Specifies which IDE to use for opening C++ project solution files.
/// Only saved in editor preferences, does not have to work cross-platform.
struct EZ_EDITORFRAMEWORK_DLL ezIDE
{
  using StorageType = ezUInt8;

  enum Enum
  {
    DefaultProgram,   ///< Uses the system default way of opening an sln file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    VisualStudio,     ///< Opens with Visual Studio
#endif
    VisualStudioCode, ///< Opens with VS Code
    Rider,            ///< Opens with JetBrains Rider
    Default = DefaultProgram
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezIDE);

/// Specifies which compiler to use for building C++ plugin code.
/// Only saved in editor preferences, does not have to work cross-platform.
struct EZ_EDITORFRAMEWORK_DLL ezCompiler
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Clang,
#if EZ_ENABLED(EZ_PLATFORM_LINUX)
    Gcc,
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    Vs2022,
    Vs2026,
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    Default = Vs2022
#else
    Default = Gcc
#endif
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezCompiler);

/// Stores compiler configuration including custom compiler paths.
struct EZ_EDITORFRAMEWORK_DLL ezCompilerPreferences
{
  ezEnum<ezCompiler> m_Compiler;
  bool m_bCustomCompiler = false; ///< If true, use custom compiler paths instead of predefined compiler
  ezString m_sCppCompiler;        ///< Path to C++ compiler executable
  ezString m_sCCompiler;          ///< Path to C compiler executable
  ezString m_sRcCompiler;         ///< Path to resource compiler executable
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezCompilerPreferences);

/// Configuration for launching an external code editor.
struct EZ_EDITORFRAMEWORK_DLL ezCodeEditorPreferences
{
  bool m_bIsVisualStudio = false; ///< If true, uses Visual Studio specific command line format
  ezString m_sEditorPath;         ///< Full path to the code editor executable
  ezString m_sEditorArgs;         ///< Command line arguments passed to the editor
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezCodeEditorPreferences);

/// Manages C++ plugin project generation, compilation, and IDE integration.
///
/// Provides functionality to generate CMake projects for C++ game plugins,
/// configure compilers, build solutions, and open code in external editors.
struct EZ_EDITORFRAMEWORK_DLL ezCppProject : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCppProject, ezPreferences);

  /// Describes a compiler configuration detected or configured on this machine.
  struct MachineSpecificCompilerPaths
  {
    ezString m_sNiceName;    ///< Display name for this compiler configuration
    ezEnum<ezCompiler> m_Compiler;
    ezString m_sCCompiler;   ///< Path to C compiler
    ezString m_sCppCompiler; ///< Path to C++ compiler
    bool m_bIsCustom;        ///< If true, this is a user-configured compiler, not auto-detected
  };

  /// Result type for operations that may modify files.
  enum class ModifyResult
  {
    FAILURE,      ///< Operation failed
    NOT_MODIFIED, ///< Files already up-to-date, no changes made
    MODIFIED      ///< Files were successfully modified
  };


  ezCppProject();
  ~ezCppProject();

  /// Returns the directory where C++ plugin template files are copied to.
  /// If sProjectDirectory is empty, uses the current project directory.
  static ezString GetTargetSourceDir(ezStringView sProjectDirectory = {});

  /// Returns the CMake generator-specific folder name (e.g., "vs2022x64").
  static ezString GetGeneratorFolderName(const ezCppSettings& cfg);

  /// Returns the CMake generator name string for the -G parameter.
  static ezString GetCMakeGeneratorName(const ezCppSettings& cfg);

  /// Returns the source directory for the C++ plugin project.
  /// If sProjectDirectory is empty, uses the current project directory.
  static ezString GetPluginSourceDir(const ezCppSettings& cfg, ezStringView sProjectDirectory = {});

  /// Returns the CMake build directory path.
  static ezString GetBuildDir(const ezCppSettings& cfg);

  /// Returns the full path to the generated solution file.
  static ezString GetSolutionPath(const ezCppSettings& cfg);

  /// Opens the solution file in the configured IDE.
  static ezStatus OpenSolution(const ezCppSettings& cfg);

  /// Attempts to launch the configured code editor with the specified file and line number
  static ezStatus OpenInCodeEditor(const ezStringView& sFileName, ezInt32 iLineNumber);

  static ezStringView CompilerToString(ezCompiler::Enum compiler);

  /// Returns the compiler used to build the editor SDK itself.
  /// Plugins should typically use a compatible compiler to avoid ABI issues.
  static ezCompiler::Enum GetSdkCompiler();

  /// Returns the major version number of the compiler used to build the SDK.
  static ezString GetSdkCompilerMajorVersion();

  /// Tests if the currently configured compiler can be invoked successfully.
  static ezStatus TestCompiler();

  /// Returns the path to the CMake executable bundled with the engine.
  static const char* GetCMakePath();

  /// Verifies that the CMake cache matches the current configuration.
  /// Returns failure if the cache is outdated or incompatible.
  static ezResult CheckCMakeCache(const ezCppSettings& cfg);

  /// Checks if CMakeUserPresets.json needs updating for the given configuration.
  /// If bWriteResult is true, writes the updated file when changes are needed.
  static ModifyResult CheckCMakeUserPresets(const ezCppSettings& cfg, bool bWriteResult);

  static bool ExistsSolution(const ezCppSettings& cfg);

  static bool ExistsProjectCMakeListsTxt();

  /// Returns true if sDst doesn't exist yet, or for some file types (e.g. CMakeLists.txt), when they got an update.
  static bool ShouldOverwriteExisting(ezStringView sSrc, ezStringView sDst);

  /// Copies default C++ plugin template files to the project's source directory.
  static ezResult PopulateWithDefaultSources(const ezCppSettings& cfg, ezUInt32* pNumFilesCopied = nullptr);

  static ezResult CleanBuildDir(const ezCppSettings& cfg);

  /// Runs CMake to generate the build system files.
  static ezResult RunCMake(const ezCppSettings& cfg);

  /// Checks if CMake needs to run and executes it if required.
  /// Skips CMake if the build files are already up-to-date.
  static ezResult RunCMakeIfNecessary(const ezCppSettings& cfg);

  /// Compiles the C++ plugin solution using the configured compiler.
  static ezResult CompileSolution(const ezCppSettings& cfg);

  /// Checks if compilation is needed and builds the code if required.
  /// Skips the build if the plugin binary is already up-to-date.
  static ezResult BuildCodeIfNecessary(const ezCppSettings& cfg);

  /// Creates a minimal CMakeUserPresets.json structure for the given configuration.
  static ezVariantDictionary CreateEmptyCMakeUserPresetsJson(const ezCppSettings& cfg);

  /// Updates an existing CMakeUserPresets.json with current configuration settings.
  /// The inout_json parameter is both read and modified.
  static ModifyResult ModifyCMakeUserPresetsJson(const ezCppSettings& cfg, ezVariantDictionary& inout_json);

  /// Writes the plugin configuration header files based on current settings.
  static void UpdatePluginConfig(const ezCppSettings& cfg);

  /// Writes CMakeEngineExtensions.txt with the selected engine plugins for linking.
  static ezResult UpdateEnginePluginDependencies();

  /// Ensures the C++ plugin project is generated and compiled.
  /// Runs all necessary steps to make the plugin ready for use.
  static ezResult EnsureCppPluginReady();

  /// Checks whether the C++ plugin needs to be rebuilt.
  /// Returns true if source files have changed since the last build.
  static bool IsBuildRequired();

  /// Fired when a notable change has been made.
  static ezEvent<const ezCppSettings&> s_ChangeEvents;

  static void LoadPreferences();

  static ezArrayPtr<const MachineSpecificCompilerPaths> GetMachineSpecificCompilers() { return s_MachineSpecificCompilers.GetArrayPtr(); }

  /// Changes the current preferences to use an SDK-compatible compiler.
  /// This is necessary to avoid ABI compatibility issues between the plugin and engine.
  static ezResult ForceSdkCompatibleCompiler();

private:
  ezEnum<ezIDE> m_Ide;
  ezCompilerPreferences m_CompilerPreferences;
  ezCodeEditorPreferences m_CodeEditorPreferences;

  static ezDynamicArray<MachineSpecificCompilerPaths> s_MachineSpecificCompilers;
};
