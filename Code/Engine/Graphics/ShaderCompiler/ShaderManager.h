#pragma once

#include <Graphics/Basics.h>
#include <Graphics/ShaderCompiler/PermutationGenerator.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Map.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>
#include <Graphics/Shader/ShaderResource.h>
#include <Graphics/Shader/ShaderPermutationResource.h>

class EZ_GRAPHICS_DLL ezShaderManager
{
public:

  static void SetPlatform(const char* szPlatform, ezGALDevice* pDevice, bool bEnableRuntimeCompilation);

  static const ezString& GetPlatform() { return s_sPlatform; }

  static ezGALDevice* GetDevice() { return s_pDevice; }

  static void SetPermutationVariable(const char* szVariable, const char* szValue, ezGALContext* pContext = nullptr);

  /// \brief Sets the currently active shader on the given render context. If pContext is null, the state is set for the primary context.
  ///
  /// This function has no effect until the next drawcall on the context.
  static void SetActiveShader(ezShaderResourceHandle hShader, ezGALContext* pContext = nullptr);

  /// \brief Evaluates the currently active shader program and then returns that.
  /// Can be used to determine the shader that needs to be used to create a vertex declaration.
  static ezGALShaderHandle GetActiveGALShader(ezGALContext* pContext = nullptr);

  static void SetShaderCacheDirectory(const char* szDirectory) { s_ShaderCacheDirectory = szDirectory; }

  static const ezString& GetShaderCacheDirectory() { return s_ShaderCacheDirectory; }

  static bool IsRuntimeCompilationEnabled() { return s_bEnableRuntimeCompilation; }

  static const ezPermutationGenerator* GetGeneratorForPermutation(ezUInt32 uiPermutationHash);

  static void PreloadPermutations(ezShaderResourceHandle hShader, const ezPermutationGenerator& Generator, ezTime tShouldBeAvailableIn);

  static ezShaderPermutationResourceHandle PreloadSinglePermutation(ezShaderResourceHandle hShader, const ezHybridArray<ezPermutationGenerator::PermutationVar, 16>& UsedPermVars, ezTime tShouldBeAvailableIn);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Graphics, ShaderManager);

  static void OnEngineShutdown();
  static void OnCoreShutdown();

  struct ContextState
  {
    ContextState()
    {
      m_bStateChanged = true;
      m_bStateValid = false;
    }

    bool m_bStateChanged;
    bool m_bStateValid;
    ezShaderResourceHandle m_hActiveShader;
    ezGALShaderHandle m_hActiveGALShader;
    ezMap<ezString, ezString> m_PermutationVariables;
    ezPermutationGenerator m_PermGenerator;
  };

  static void SetContextState(ezGALContext* pContext, ContextState& state);
  static void ContextEventHandler(ezGALContext::ezGALContextEvent& ed);

  static ezPermutationGenerator s_AllowedPermutations;
  static bool s_bEnableRuntimeCompilation;
  static ezString s_sPlatform;
  static ezGALDevice* s_pDevice;
  static ezMap<ezGALContext*, ContextState> s_ContextState;
  static ezString s_ShaderCacheDirectory;
  static ezMap<ezUInt32, ezPermutationGenerator> s_PermutationHashCache;
};


