#pragma once

#include <RendererCore/Declarations.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Map.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>
#include <Core/ResourceManager/Resource.h>
#include <RendererCore/../../../Shared/Data/Shaders/Common/ConstantBufferMacros.h>
#include <RendererCore/../../../Shared/Data/Shaders/Common/GlobalConstants.h>
#include <RendererCore/Shader/ShaderStageBinary.h>


class EZ_RENDERERCORE_DLL ezRenderContext
{
private:
  ezRenderContext();
  ~ezRenderContext();
  friend class ezMemoryUtils;

  static ezRenderContext* s_DefaultInstance;
  static ezHybridArray<ezRenderContext*, 4> s_Instances;

public:
  static ezRenderContext* GetDefaultInstance();
  static ezRenderContext* CreateInstance();
  static void DestroyInstance(ezRenderContext* pRenderer);

  void SetGALContext(ezGALContext* pContext);
  ezGALContext* GetGALContext() const { return m_pGALContext; }

public:
  struct Statistics
  {
    Statistics();
    void Reset();

    ezUInt32 m_uiFailedDrawcalls;
  };


  // Member Functions
  void SetShaderPermutationVariable(const char* szVariable, const char* szValue);
  void BindTexture(const ezTempHashedString& sSlotName, const ezTextureResourceHandle& hTexture);
  void SetMaterialState(const ezMaterialResourceHandle& hMaterial);
  void BindConstantBuffer(const ezTempHashedString& sSlotName, const ezConstantBufferResourceHandle& hConstantBuffer);
  void BindMeshBuffer(const ezMeshBufferResourceHandle& hMeshBuffer);
  void DrawMeshBuffer(ezUInt32 uiPrimitiveCount = 0xFFFFFFFF, ezUInt32 uiFirstPrimitive = 0, ezUInt32 uiInstanceCount = 1);
  ezResult ApplyContextStates(bool bForce = false);
  Statistics GetAndResetStatistics();

  /// \brief Sets the currently active shader on the given render context. If pContext is null, the state is set for the primary context.
  ///
  /// This function has no effect until the next drawcall on the context.
  void BindShader(ezShaderResourceHandle hShader, ezBitflags<ezShaderBindFlags> flags = ezShaderBindFlags::Default);

  /// \brief Evaluates the currently active shader program and then returns that.
  /// Can be used to determine the shader that needs to be used to create a vertex declaration.
  ezGALShaderHandle GetActiveGALShader();

  template<typename STRUCT>
  STRUCT* BeginModifyConstantBuffer(ezConstantBufferResourceHandle hConstantBuffer)
  {
    return reinterpret_cast<STRUCT*>(InternalBeginModifyConstantBuffer(hConstantBuffer));
  }

  void EndModifyConstantBuffer();

  // Static Functions
  static void ConfigureShaderSystem(const char* szActivePlatform, bool bEnableRuntimeCompilation, const char* szShaderCacheDirectory = "ShaderCache", const char* szPermVarSubDirectory = "Shaders/PermutationVars");
  static const ezString& GetPermutationVarSubDirectory() { return s_sPermVarSubDir; }
  static const ezString& GetActiveShaderPlatform() { return s_sPlatform; }
  static const ezString& GetShaderCacheDirectory() { return s_ShaderCacheDirectory; }
  static bool IsRuntimeShaderCompilationEnabled() { return s_bEnableRuntimeCompilation; }

public:

  static void LoadShaderPermutationVarConfig(const char* szVariable);
  static const ezPermutationGenerator* GetGeneratorForShaderPermutation(ezUInt32 uiPermutationHash);
  static void PreloadShaderPermutations(ezShaderResourceHandle hShader, const ezPermutationGenerator& Generator, ezTime tShouldBeAvailableIn);
  static ezShaderPermutationResourceHandle PreloadSingleShaderPermutation(ezShaderResourceHandle hShader, const ezHybridArray<ezPermutationGenerator::PermutationVar, 16>& UsedPermVars, ezTime tShouldBeAvailableIn);

  static GlobalConstants& WriteGlobalConstants()
  {
    s_bGlobalConstantsModified = true;
    return s_GlobalConstants;
  }

  static const GlobalConstants& ReadGlobalConstants()
  {
    return s_GlobalConstants;
  }

  struct MaterialParam
  {
    ezUInt64 m_LastModification;
    ezShaderMaterialParamCB::MaterialParameter::Type m_Type;
    ezUInt8 m_uiArrayElements;
    ezUInt16 m_uiDataSize;
  };

  static void SetMaterialParameter(const ezTempHashedString& sName, const float& value);
  static void SetMaterialParameter(const ezTempHashedString& sName, const ezVec2& value);
  static void SetMaterialParameter(const ezTempHashedString& sName, const ezVec3& value);
  static void SetMaterialParameter(const ezTempHashedString& sName, const ezVec4& value);
  static void SetMaterialParameter(const ezTempHashedString& sName, const ezColor& value);
  static void SetMaterialParameter(const ezTempHashedString& sName, const ezInt32& value);
  static void SetMaterialParameter(const ezTempHashedString& sName, const ezVec2I32& value);
  static void SetMaterialParameter(const ezTempHashedString& sName, const ezVec3I32& value);
  static void SetMaterialParameter(const ezTempHashedString& sName, const ezVec4I32& value);
  //static void SetMaterialParameter(const ezTempHashedString& sName, const ezMat3& value); /// \todo ezMat3 does not work right, ezTransform maybe neither
  static void SetMaterialParameter(const ezTempHashedString& sName, const ezMat4& value);
  static void SetMaterialParameter(const ezTempHashedString& sName, const ezTransform& value);
  static void SetMaterialParameter(const ezTempHashedString& sName, const ezVariant& value);
  /// \todo Array versions of material parameters


  static const MaterialParam* GetMaterialParameterPointer(ezUInt32 uiNameHash);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Graphics, RendererContext);

  static void OnEngineShutdown();
  static void OnCoreShutdown();

private:

  Statistics m_Statistics;
  ezBitflags<ezRenderContextFlags> m_StateFlags;
  ezShaderResourceHandle m_hActiveShader;
  ezGALShaderHandle m_hActiveGALShader;
  ezMap<ezString, ezString> m_PermutationVariables;
  ezPermutationGenerator m_PermGenerator;
  ezShaderPermutationResourceHandle m_hActiveShaderPermutation;
  ezConstantBufferResource* m_pCurrentlyModifyingBuffer;
  ezBitflags<ezShaderBindFlags> m_ShaderBindFlags;
  ezMeshBufferResourceHandle m_hMeshBuffer;
  ezUInt32 m_uiMeshBufferPrimitiveCount;

  ezHashTable<ezUInt32, ezTextureResourceHandle> m_BoundTextures;
  ezHashTable<ezUInt32, ezConstantBufferResourceHandle> m_BoundConstantBuffers;

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

  static ezResult BuildVertexDeclaration(ezGALShaderHandle hShader, const ezVertexDeclarationInfo& decl, ezGALVertexDeclarationHandle& out_Declaration);
  ezUInt8* InternalBeginModifyConstantBuffer(ezConstantBufferResourceHandle hConstantBuffer);

  static MaterialParam* InternalSetMaterialParameter(const ezTempHashedString& sName, ezShaderMaterialParamCB::MaterialParameter::Type type, ezUInt32 uiMaxArrayElements);

  static ezPermutationGenerator s_AllowedPermutations;
  static bool s_bEnableRuntimeCompilation;
  static ezString s_sPlatform;
  static ezString s_sPermVarSubDir;
  static ezString s_ShaderCacheDirectory;
  static ezMap<ezUInt32, ezPermutationGenerator> s_PermutationHashCache;
  static ezMap<ShaderVertexDecl, ezGALVertexDeclarationHandle> s_GALVertexDeclarations;
  static ezUInt64 s_LastMaterialParamModification;

  static bool s_bGlobalConstantsModified;
  static GlobalConstants s_GlobalConstants;
  static ezConstantBufferResourceHandle s_hGlobalConstantBuffer;

private: // Per Renderer States
  ezGALContext* m_pGALContext;

  // Member Functions
  void UploadGlobalConstants();
  void ApplyTextureBindings(ezGALShaderStage::Enum stage, const ezShaderStageBinary* pBinary);
  void ApplyConstantBufferBindings(const ezShaderStageBinary* pBinary);
};

