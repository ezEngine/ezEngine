#pragma once

#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/VariantType.h>

// Only saved in editor preferences, does not have to work cross-platform
struct EZ_EDITORFRAMEWORK_DLL ezIDE
{
  using StorageType = ezUInt8;

  enum Enum
  {
    DefaultProgram, // Uses the system default way of opening an sln file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    VisualStudio,
#endif
    VisualStudioCode,
    Rider,
    Default = DefaultProgram
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezIDE);

// Only saved in editor preferences, does not have to work cross-platform
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
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    Default = Vs2022
#else
    Default = Gcc
#endif
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezCompiler);

struct EZ_EDITORFRAMEWORK_DLL ezCompilerPreferences
{
  ezEnum<ezCompiler> m_Compiler;
  bool m_bCustomCompiler;
  ezString m_sCppCompiler;
  ezString m_sCCompiler;
  ezString m_sRcCompiler;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezCompilerPreferences);

struct EZ_EDITORFRAMEWORK_DLL ezCodeEditorPreferences
{
  bool m_bIsVisualStudio;
  ezString m_sEditorPath;
  ezString m_sEditorArgs;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezCodeEditorPreferences);

struct EZ_EDITORFRAMEWORK_DLL ezCppProject : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCppProject, ezPreferences);

  struct MachineSpecificCompilerPaths
  {
    ezString m_sNiceName;
    ezEnum<ezCompiler> m_Compiler;
    ezString m_sCCompiler;
    ezString m_sCppCompiler;
    bool m_bIsCustom;
  };

  enum class ModifyResult
  {
    FAILURE,
    NOT_MODIFIED,
    MODIFIED
  };


  ezCppProject();
  ~ezCppProject();

  static ezString GetTargetSourceDir(ezStringView sProjectDirectory = {});

  static ezString GetGeneratorFolderName(const ezCppSettings& cfg);

  static ezString GetCMakeGeneratorName(const ezCppSettings& cfg);

  static ezString GetPluginSourceDir(const ezCppSettings& cfg, ezStringView sProjectDirectory = {});

  static ezString GetBuildDir(const ezCppSettings& cfg);

  static ezString GetSolutionPath(const ezCppSettings& cfg);

  static ezStatus OpenSolution(const ezCppSettings& cfg);

  /// \brief Attempts to launch the configured code editor with the specified file and line number
  static ezStatus OpenInCodeEditor(const ezStringView& sFileName, ezInt32 iLineNumber);

  static ezStringView CompilerToString(ezCompiler::Enum compiler);

  static ezCompiler::Enum GetSdkCompiler();

  static ezString GetSdkCompilerMajorVersion();

  static ezStatus TestCompiler();

  static const char* GetCMakePath();

  static ezResult CheckCMakeCache(const ezCppSettings& cfg);

  static ModifyResult CheckCMakeUserPresets(const ezCppSettings& cfg, bool bWriteResult);

  static bool ExistsSolution(const ezCppSettings& cfg);

  static bool ExistsProjectCMakeListsTxt();

  static ezResult PopulateWithDefaultSources(const ezCppSettings& cfg);

  static ezResult CleanBuildDir(const ezCppSettings& cfg);

  static ezResult RunCMake(const ezCppSettings& cfg);

  static ezResult RunCMakeIfNecessary(const ezCppSettings& cfg);

  static ezResult CompileSolution(const ezCppSettings& cfg);

  static ezResult BuildCodeIfNecessary(const ezCppSettings& cfg);

  static ezVariantDictionary CreateEmptyCMakeUserPresetsJson(const ezCppSettings& cfg);

  static ModifyResult ModifyCMakeUserPresetsJson(const ezCppSettings& cfg, ezVariantDictionary& inout_json);

  static void UpdatePluginConfig(const ezCppSettings& cfg);

  static ezResult EnsureCppPluginReady();

  static bool IsBuildRequired();

  /// \brief Fired when a notable change has been made.
  static ezEvent<const ezCppSettings&> s_ChangeEvents;

  static void LoadPreferences();

  static ezArrayPtr<const MachineSpecificCompilerPaths> GetMachineSpecificCompilers() { return s_MachineSpecificCompilers.GetArrayPtr(); }

  // Change the current preferences to point to a SDK compatible compiler
  static ezResult ForceSdkCompatibleCompiler();

private:
  ezEnum<ezIDE> m_Ide;
  ezCompilerPreferences m_CompilerPreferences;
  ezCodeEditorPreferences m_CodeEditorPreferences;

  static ezDynamicArray<MachineSpecificCompilerPaths> s_MachineSpecificCompilers;
};
