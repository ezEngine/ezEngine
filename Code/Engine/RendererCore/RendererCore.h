#pragma once

#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Textures/TextureResource.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Map.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>

class ezGALContext;
struct ezVertexDeclarationInfo;


class EZ_RENDERERCORE_DLL ezRendererCore
{
public:
  static void SetMaterialState(ezGALContext* pContext, const ezMaterialResourceHandle& hMaterial);

  static void DrawMeshBuffer(ezGALContext* pContext, const ezMeshBufferResourceHandle& hMeshBuffer,
    ezUInt32 uiPrimitiveCount = 0xFFFFFFFF, ezUInt32 uiFirstPrimitive = 0, ezUInt32 uiInstanceCount = 1);

  static void BindTexture(ezGALContext* pContext, const ezTempHashedString& sSlotName, const ezTextureResourceHandle& hTexture);

  static ezResult ApplyContextStates(ezGALContext* pContext = nullptr, bool bForce = false);

  static void SetShaderPlatform(const char* szPlatform, bool bEnableRuntimeCompilation);

  static const ezString& GetShaderPlatform() { return s_sPlatform; }

  static void SetShaderPermutationVariable(const char* szVariable, const char* szValue, ezGALContext* pContext = nullptr);

  /// \brief Sets the currently active shader on the given render context. If pContext is null, the state is set for the primary context.
  ///
  /// This function has no effect until the next drawcall on the context.
  static void SetActiveShader(ezShaderResourceHandle hShader, ezGALContext* pContext = nullptr);

  /// \brief Evaluates the currently active shader program and then returns that.
  /// Can be used to determine the shader that needs to be used to create a vertex declaration.
  static ezGALShaderHandle GetActiveGALShader(ezGALContext* pContext = nullptr);

  static void SetShaderCacheDirectory(const char* szDirectory) { s_ShaderCacheDirectory = szDirectory; }

  static const ezString& GetShaderCacheDirectory() { return s_ShaderCacheDirectory; }

  static bool IsRuntimeShaderCompilationEnabled() { return s_bEnableRuntimeCompilation; }

  static const ezPermutationGenerator* GetGeneratorForShaderPermutation(ezUInt32 uiPermutationHash);

  static void PreloadShaderPermutations(ezShaderResourceHandle hShader, const ezPermutationGenerator& Generator, ezTime tShouldBeAvailableIn);

  static ezShaderPermutationResourceHandle PreloadSingleShaderPermutation(ezShaderResourceHandle hShader, const ezHybridArray<ezPermutationGenerator::PermutationVar, 16>& UsedPermVars, ezTime tShouldBeAvailableIn);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Graphics, RendererCore);

  static void OnEngineShutdown();
  static void OnCoreShutdown();

private:
  struct ContextState
  {
    ContextState()
    {
      m_bShaderStateChanged = true;
      m_bShaderStateValid = false;
      m_bTextureBindingsChanged = true;
    }

    bool m_bShaderStateChanged;
    bool m_bShaderStateValid;
    bool m_bTextureBindingsChanged;
    ezShaderResourceHandle m_hActiveShader;
    ezGALShaderHandle m_hActiveGALShader;
    ezMap<ezString, ezString> m_PermutationVariables;
    ezPermutationGenerator m_PermGenerator;
    ezShaderPermutationResourceHandle m_hActiveShaderPermutation;

    ezHashTable<ezUInt32, ezTextureResourceHandle> m_BoundTextures;
  };

  struct ShaderVertexDecl
  {
    ezGALShaderHandle m_hShader;
    ezUInt32 m_uiVertexDeclarationHash;

    EZ_FORCE_INLINE bool operator<(const ShaderVertexDecl& rhs) const
    {
      if (m_hShader < rhs.m_hShader)
        return true;
      if (rhs.m_hShader < m_hShader)
        return false;
      return m_uiVertexDeclarationHash < rhs.m_uiVertexDeclarationHash;
    }

    EZ_FORCE_INLINE bool operator==(const ShaderVertexDecl& rhs) const
    {
      return (m_hShader == rhs.m_hShader && m_uiVertexDeclarationHash == rhs.m_uiVertexDeclarationHash);
    }
  };

  static void SetShaderContextState(ezGALContext* pContext, ContextState& state, bool bForce);
  static void ApplyTextureBindings(ezGALContext* pContext, ezGALShaderStage::Enum stage, const ezShaderStageBinary* pBinary);
  static ezGALVertexDeclarationHandle GetVertexDeclaration(ezGALShaderHandle hShader, const ezVertexDeclarationInfo& decl);

  static ezPermutationGenerator s_AllowedPermutations;
  static bool s_bEnableRuntimeCompilation;
  static ezString s_sPlatform;
  static ezMap<ezGALContext*, ContextState> s_ContextState;
  static ezString s_ShaderCacheDirectory;
  static ezMap<ezUInt32, ezPermutationGenerator> s_PermutationHashCache;
  static ezMap<ShaderVertexDecl, ezGALVertexDeclarationHandle> s_GALVertexDeclarations;
};

