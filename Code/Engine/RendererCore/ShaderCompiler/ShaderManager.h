#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

class EZ_RENDERERCORE_DLL ezShaderManager
{
public:
  static void Configure(const char* szActivePlatform, bool bEnableRuntimeCompilation, const char* szShaderCacheDirectory = "ShaderCache", const char* szPermVarSubDirectory = "Shaders/PermutationVars");
  static const ezString& GetPermutationVarSubDirectory() { return s_sPermVarSubDir; }
  static const ezString& GetActivePlatform() { return s_sPlatform; }
  static const ezString& GetCacheDirectory() { return s_ShaderCacheDirectory; }
  static bool IsRuntimeCompilationEnabled() { return s_bEnableRuntimeCompilation; }
  
  static void ReloadPermutationVarConfig(const ezHashedString& sName);
  static bool IsPermutationValueAllowed(const ezHashedString& sName, const ezHashedString& sValue);
  static ezArrayPtr<const ezHashedString> GetPermutationEnumValues(const ezHashedString& sName);

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
