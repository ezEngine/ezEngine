#pragma once

#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/EditorFrameworkDLL.h>

struct EZ_EDITORFRAMEWORK_DLL ezCppProject
{
  static ezString GetTargetSourceDir();

  static ezString GetGeneratorFolderName(const ezCppSettings& cfg);

  static ezString GetCMakeGeneratorName(const ezCppSettings& cfg);

  static ezString GetBuildDir(const ezCppSettings& cfg);

  static ezString GetSolutionPath(const ezCppSettings& cfg);

  static bool ExistsSolution(const ezCppSettings& cfg);

  static bool ExistsProjectCMakeListsTxt();

  static ezResult PopulateWithDefaultSources(const ezCppSettings& cfg, ezStringBuilder& inout_sOutput);

  static ezResult CleanBuildDir(const ezCppSettings& cfg);

  static ezResult RunCMake(const ezCppSettings& cfg, ezStringBuilder& inout_sOutput);

  static ezResult RunCMakeIfNecessary(const ezCppSettings& cfg, ezStringBuilder& inout_sOutput);

  static ezResult CompileSolution(const ezCppSettings& cfg, ezStringBuilder& inout_sOutput);

  static ezResult BuildCodeIfNecessary(const ezCppSettings& cfg, ezStringBuilder& inout_sOutput);
};
