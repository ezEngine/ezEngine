#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

class EZ_RENDERERCORE_DLL ezShaderManager
{
public:
  static void Configure(const char* szActivePlatform, bool bEnableRuntimeCompilation, const char* szShaderCacheDirectory = ":shadercache/ShaderCache", const char* szPermVarSubDirectory = "Shaders/PermutationVars");
  static const ezString& GetPermutationVarSubDirectory() { return s_sPermVarSubDir; }
  static const ezString& GetActivePlatform() { return s_sPlatform; }
  static const ezString& GetCacheDirectory() { return s_ShaderCacheDirectory; }
  static bool IsRuntimeCompilationEnabled() { return s_bEnableRuntimeCompilation; }
  
  static void ReloadPermutationVarConfig(const char* szName, const ezTempHashedString& sHashedName);
  static bool IsPermutationValueAllowed(const char* szName, const ezTempHashedString& sHashedName, const ezTempHashedString& sValue, ezHashedString& out_sName, ezHashedString& out_sValue);
  static bool IsPermutationValueAllowed(const ezHashedString& sName, const ezHashedString& sValue);

  /// \brief If the given permutation variable is an enum variable, this returns the possible values.
  /// Returns an empty array for other types of permutation variables.
  static ezArrayPtr<const ezHashedString> GetPermutationEnumValues(const ezHashedString& sName);

  /// \brief Same as GetPermutationEnumValues() but also returns values for other types of variables.
  /// E.g. returns TRUE and FALSE for boolean variables.
  static void GetPermutationValues(const ezHashedString& sName, ezHybridArray<ezHashedString, 4>& out_Values);

  static void PreloadPermutations(ezShaderResourceHandle hShader, const ezHashTable<ezHashedString, ezHashedString>& permVars, ezTime tShouldBeAvailableIn);
  static ezShaderPermutationResourceHandle PreloadSinglePermutation(ezShaderResourceHandle hShader, const ezHashTable<ezHashedString, ezHashedString>& permVars, ezTime tShouldBeAvailableIn);

private:

  static ezUInt32 FilterPermutationVars(const ezArrayPtr<const ezHashedString>& usedVars, const ezHashTable<ezHashedString, ezHashedString>& permVars);
  static ezShaderPermutationResourceHandle PreloadSinglePermutationInternal(const char* szResourceId, ezUInt32 uiPermutationHash, ezTime tShouldBeAvailableIn);

  static bool s_bEnableRuntimeCompilation;
  static ezString s_sPlatform;
  static ezString s_sPermVarSubDir;
  static ezString s_ShaderCacheDirectory;
};
