#pragma once

#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Communication/Event.h>

struct EZ_EDITORFRAMEWORK_DLL ezCppProject
{
  static ezString GetTargetSourceDir(ezStringView sProjectDirectory = {});

  static ezString GetGeneratorFolderName(const ezCppSettings& cfg);

  static ezString GetCMakeGeneratorName(const ezCppSettings& cfg);

  static ezString GetPluginSourceDir(const ezCppSettings& cfg, ezStringView sProjectDirectory = {});

  static ezString GetBuildDir(const ezCppSettings& cfg);

  static ezString GetSolutionPath(const ezCppSettings& cfg);

  static ezResult CheckCMakeCache(const ezCppSettings& cfg);

  static bool ExistsSolution(const ezCppSettings& cfg);

  static bool ExistsProjectCMakeListsTxt();

  static ezResult PopulateWithDefaultSources(const ezCppSettings& cfg);

  static ezResult CleanBuildDir(const ezCppSettings& cfg);

  static ezResult RunCMake(const ezCppSettings& cfg);

  static ezResult RunCMakeIfNecessary(const ezCppSettings& cfg);

  static ezResult CompileSolution(const ezCppSettings& cfg);

  static ezResult BuildCodeIfNecessary(const ezCppSettings& cfg);

  static ezResult FindMsBuild(const ezCppSettings& cfg);

  static void UpdatePluginConfig(const ezCppSettings& cfg);

  /// \brief Fired when a notable change has been made.
  static ezEvent<const ezCppSettings&> s_ChangeEvents;
};
