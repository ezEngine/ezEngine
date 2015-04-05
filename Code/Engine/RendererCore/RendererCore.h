#pragma once

#include <RendererCore/Basics.h>
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

class ezGALContext;
class ezShaderStageBinary;
struct ezVertexDeclarationInfo;

typedef ezResourceHandle<class ezTextureResource> ezTextureResourceHandle;
typedef ezResourceHandle<class ezConstantBufferResource> ezConstantBufferResourceHandle;
typedef ezResourceHandle<class ezMeshBufferResource> ezMeshBufferResourceHandle;
typedef ezResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;
typedef ezResourceHandle<class ezShaderResource> ezShaderResourceHandle;
typedef ezResourceHandle<class ezShaderPermutationResource> ezShaderPermutationResourceHandle;

struct ezShaderBindFlags
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    None = 0,                           ///< No flags causes the default shader binding behavior (all render states are applied)
    ForceRebind         = EZ_BIT(0),    ///< Executes shader binding (and state setting), even if the shader hasn't changed. Use this, when the same shader was previously used with custom bound states
    NoRasterizerState   = EZ_BIT(1),    ///< The rasterizer state that is associated with the shader will not be bound. Use this when you intend to bind a custom rasterizer m_ContextState.
    NoDepthStencilState = EZ_BIT(2),    ///< The depth-stencil state that is associated with the shader will not be bound. Use this when you intend to bind a custom depth-stencil m_ContextState.
    NoBlendState        = EZ_BIT(3),    ///< The blend state that is associated with the shader will not be bound. Use this when you intend to bind a custom blend m_ContextState.
    NoStateBinding      = NoRasterizerState | NoDepthStencilState | NoBlendState,

    Default = None
  };

  struct Bits
  {
    StorageType NoRasterizerState   : 1;
    StorageType NoDepthStencilState : 1;
    StorageType NoBlendState        : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezShaderBindFlags);

class EZ_RENDERERCORE_DLL ezRendererCore
{
private:
  ezRendererCore();
  ~ezRendererCore();
  friend class ezMemoryUtils;

  static ezRendererCore* s_DefaultInstance;
  static ezHybridArray<ezRendererCore*, 4> s_Instances;

public:
  static ezRendererCore* GetDefaultInstance();
  static ezRendererCore* CreateInstance();
  static void DestroyInstance(ezRendererCore* pRenderer);

  void SetGALContext(ezGALContext* pContext);
  ezGALContext* GetGALContext() const { return m_pGALContext; }

  // Member Functions
  void SetShaderPermutationVariable(const char* szVariable, const char* szValue);
  void BindTexture(const ezTempHashedString& sSlotName, const ezTextureResourceHandle& hTexture);
  void SetMaterialState(const ezMaterialResourceHandle& hMaterial);
  void BindConstantBuffer(const ezTempHashedString& sSlotName, const ezConstantBufferResourceHandle& hConstantBuffer);
  void DrawMeshBuffer(const ezMeshBufferResourceHandle& hMeshBuffer, ezUInt32 uiPrimitiveCount = 0xFFFFFFFF, ezUInt32 uiFirstPrimitive = 0, ezUInt32 uiInstanceCount = 1);
  ezResult ApplyContextStates(bool bForce = false);
  ezUInt32 RetrieveFailedDrawcalls();

  /// \brief Sets the currently active shader on the given render context. If pContext is null, the state is set for the primary context.
  ///
  /// This function has no effect until the next drawcall on the context.
  void SetActiveShader(ezShaderResourceHandle hShader, ezBitflags<ezShaderBindFlags> flags = ezShaderBindFlags::Default);

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
      m_bConstantBufferBindingsChanged = true;
      m_uiFailedDrawcalls = 0;
      m_pCurrentlyModifyingBuffer = nullptr;
    }

    ezUInt32 m_uiFailedDrawcalls;
    bool m_bShaderStateChanged;
    bool m_bShaderStateValid;
    bool m_bTextureBindingsChanged;
    bool m_bConstantBufferBindingsChanged;
    ezShaderResourceHandle m_hActiveShader;
    ezGALShaderHandle m_hActiveGALShader;
    ezMap<ezString, ezString> m_PermutationVariables;
    ezPermutationGenerator m_PermGenerator;
    ezShaderPermutationResourceHandle m_hActiveShaderPermutation;
    ezConstantBufferResource* m_pCurrentlyModifyingBuffer;
    ezBitflags<ezShaderBindFlags> m_ShaderBindFlags;

    ezHashTable<ezUInt32, ezTextureResourceHandle> m_BoundTextures;
    ezHashTable<ezUInt32, ezConstantBufferResourceHandle> m_BoundConstantBuffers;
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

  static ezGALVertexDeclarationHandle GetVertexDeclaration(ezGALShaderHandle hShader, const ezVertexDeclarationInfo& decl);
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
  ContextState m_ContextState;

  // Member Functions
  void UploadGlobalConstants();
  void SetShaderContextState(bool bForce);
  void ApplyTextureBindings(ezGALShaderStage::Enum stage, const ezShaderStageBinary* pBinary);
  void ApplyConstantBufferBindings(const ezShaderStageBinary* pBinary);
};

