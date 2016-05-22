#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Map.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>
#include <Core/ResourceManager/Resource.h>
#include <RendererCore/../../../Data/Base/Shaders/Common/ConstantBufferMacros.h>
#include <RendererCore/../../../Data/Base/Shaders/Common/GlobalConstants.h>
#include <RendererCore/Shader/ShaderStageBinary.h>

struct ezShaderBindFlags
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    None = 0,                           ///< No flags causes the default shader binding behavior (all render states are applied)
    ForceRebind = EZ_BIT(0),    ///< Executes shader binding (and state setting), even if the shader hasn't changed. Use this, when the same shader was previously used with custom bound states
    NoRasterizerState = EZ_BIT(1),    ///< The rasterizer state that is associated with the shader will not be bound. Use this when you intend to bind a custom rasterizer 
    NoDepthStencilState = EZ_BIT(2),    ///< The depth-stencil state that is associated with the shader will not be bound. Use this when you intend to bind a custom depth-stencil 
    NoBlendState = EZ_BIT(3),    ///< The blend state that is associated with the shader will not be bound. Use this when you intend to bind a custom blend 
    NoStateBinding = NoRasterizerState | NoDepthStencilState | NoBlendState,

    Default = None
  };

  struct Bits
  {
    StorageType ForceRebind : 1;
    StorageType NoRasterizerState : 1;
    StorageType NoDepthStencilState : 1;
    StorageType NoBlendState : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezShaderBindFlags);


struct ezRenderContextFlags
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    None = 0,
    ShaderStateChanged = EZ_BIT(0),
    TextureBindingChanged = EZ_BIT(1),
    BufferBindingChanged = EZ_BIT(2),
    ConstantBufferBindingChanged = EZ_BIT(3),
    MeshBufferBindingChanged = EZ_BIT(4),

    ShaderStateValid = EZ_BIT(5),

    AllStatesInvalid = ShaderStateChanged | TextureBindingChanged | ConstantBufferBindingChanged | MeshBufferBindingChanged,
    Default = None
  };

  struct Bits
  {
    StorageType ShaderStateChanged : 1;
    StorageType TextureBindingChanged : 1;
    StorageType BufferBindingChanged : 1;
    StorageType ConstantBufferBindingChanged : 1;
    StorageType MeshBufferBindingChanged : 1;
    StorageType ShaderStateValid : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezRenderContextFlags);

struct ezDefaultSamplerFlags
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    PointFiltering = 0,
    LinearFiltering = EZ_BIT(0),

    Wrap = 0,
    Clamp = EZ_BIT(1)
  };

  struct Bits
  {
    StorageType LinearFiltering : 1;
    StorageType Clamp : 1;
  };
};


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

  Statistics GetAndResetStatistics();


  // Member Functions
  void SetShaderPermutationVariable(const char* szName, const ezTempHashedString& sValue);
  void SetShaderPermutationVariable(const ezHashedString& sName, const ezHashedString& sValue);

  void BindTexture(ezGALShaderStage::Enum stage, const ezTempHashedString& sSlotName, const ezTextureResourceHandle& hTexture);
  void BindTexture(ezGALShaderStage::Enum stage, const ezTempHashedString& sSlotName, ezGALResourceViewHandle hResourceView, ezGALSamplerStateHandle hSamplerState);

  void BindBuffer(ezGALShaderStage::Enum stage, const ezTempHashedString& sSlotName, ezGALResourceViewHandle hResourceView);

  void SetMaterialState(const ezMaterialResourceHandle& hMaterial);
  void BindConstantBuffer(const ezTempHashedString& sSlotName, const ezConstantBufferResourceHandle& hConstantBuffer);

  void BindMeshBuffer(const ezMeshBufferResourceHandle& hMeshBuffer);
  void BindMeshBuffer(ezGALBufferHandle hVertexBuffer, ezGALBufferHandle hIndexBuffer, const ezVertexDeclarationInfo* pVertexDeclarationInfo, 
    ezGALPrimitiveTopology::Enum topology, ezUInt32 uiPrimitiveCount);
  ezResult DrawMeshBuffer(ezUInt32 uiPrimitiveCount = 0xFFFFFFFF, ezUInt32 uiFirstPrimitive = 0, ezUInt32 uiInstanceCount = 1);

  ezResult ApplyContextStates(bool bForce = false);
  

  /// \brief Sets the currently active shader on the given render context.
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
public:

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


  static ezGALSamplerStateHandle GetDefaultSamplerState(ezBitflags<ezDefaultSamplerFlags> flags);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Graphics, RendererContext);

  static void OnEngineShutdown();
  static void OnCoreShutdown();

private:

  Statistics m_Statistics;
  ezBitflags<ezRenderContextFlags> m_StateFlags;
  ezShaderResourceHandle m_hActiveShader;
  ezGALShaderHandle m_hActiveGALShader;
  ezHashTable<ezHashedString, ezHashedString> m_PermutationVariables;
  
  ezShaderPermutationResourceHandle m_hActiveShaderPermutation;

  ezConstantBufferResource* m_pCurrentlyModifyingBuffer;
  ezBitflags<ezShaderBindFlags> m_ShaderBindFlags;

  ezGALBufferHandle m_hVertexBuffer;
  ezGALBufferHandle m_hIndexBuffer;
  const ezVertexDeclarationInfo* m_pVertexDeclarationInfo;
  ezGALPrimitiveTopology::Enum m_Topology;
  ezUInt32 m_uiMeshBufferPrimitiveCount;

  ezUInt64 m_uiLastMaterialCBSync;

  struct TextureViewSampler
  {
    TextureViewSampler() {}
    TextureViewSampler(ezGALResourceViewHandle hResourceView, ezGALSamplerStateHandle hSamplerState)
    : m_hResourceView(hResourceView), m_hSamplerState(hSamplerState) {}

    ezGALResourceViewHandle m_hResourceView;
    ezGALSamplerStateHandle m_hSamplerState;
  };
  
  ezHashTable<ezUInt32, TextureViewSampler> m_BoundTextures[ezGALShaderStage::ENUM_COUNT];
  ezHashTable<ezUInt32, ezGALResourceViewHandle> m_BoundBuffer[ezGALShaderStage::ENUM_COUNT];
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

  static ezMap<ShaderVertexDecl, ezGALVertexDeclarationHandle> s_GALVertexDeclarations;
  static ezUInt64 s_LastMaterialParamModification;

  static bool s_bGlobalConstantsModified;
  static GlobalConstants s_GlobalConstants;
  static ezConstantBufferResourceHandle s_hGlobalConstantBuffer;

  static ezGALSamplerStateHandle s_hDefaultSamplerStates[4];

private: // Per Renderer States
  ezGALContext* m_pGALContext;

  // Member Functions
  void UploadGlobalConstants();
  void ApplyTextureBindings(ezGALShaderStage::Enum stage, const ezShaderStageBinary* pBinary);
  void ApplyBufferBindings(ezGALShaderStage::Enum stage, const ezShaderStageBinary* pBinary);
  void ApplyConstantBufferBindings(const ezShaderStageBinary* pBinary);
};

