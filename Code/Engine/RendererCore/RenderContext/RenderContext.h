#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Strings/String.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/GlobalConstants.h>

struct ezRenderWorldRenderEvent;

//////////////////////////////////////////////////////////////////////////
// ezRenderContext
//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezRenderContext
{
private:
  ezRenderContext(ezGALCommandEncoder* pCommandEncoder);
  ~ezRenderContext();
  friend class ezMemoryUtils;

  static ezRenderContext* s_pDefaultInstance;
  static ezGALCommandEncoder* s_pCommandEncoder;
  static ezHybridArray<ezRenderContext*, 4> s_Instances;

public:
  static ezRenderContext* GetDefaultInstance();
  static ezRenderContext* CreateInstance(ezGALCommandEncoder* pCommandEncoder);
  static void DestroyInstance(ezRenderContext* pRenderer);

public:
  struct Statistics
  {
    Statistics();
    void Reset();

    ezUInt32 m_uiFailedDrawcalls;
  };

  Statistics GetAndResetStatistics();

  void BeginRendering(const ezGALRenderingSetup& renderingSetup, const ezRectFloat& viewport, const char* szName = "", bool bStereoRendering = false);
  void EndRendering();

  void BeginCompute(const char* szName = "");
  void EndCompute();

  // Helper class to automatically end rendering or compute on scope exit
  template <int ScopeType>
  class CommandEncoderScope
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(CommandEncoderScope);

  public:
    EZ_ALWAYS_INLINE ~CommandEncoderScope()
    {
      if constexpr (ScopeType == 0)
        m_RenderContext.EndRendering();
      else
        m_RenderContext.EndCompute();

      if (m_pCommandsScope != nullptr)
      {
        ezGALDevice::GetDefaultDevice()->EndCommands(m_pCommandsScope);
      }
    }

    EZ_ALWAYS_INLINE ezGALCommandEncoder* operator->() { return m_pGALCommandEncoder; }
    EZ_ALWAYS_INLINE operator const ezGALCommandEncoder*() { return m_pGALCommandEncoder; }

  private:
    friend class ezRenderContext;

    EZ_ALWAYS_INLINE CommandEncoderScope(ezRenderContext& renderContext, ezGALCommandEncoder* pCommandsScope)
      : m_RenderContext(renderContext)
      , m_pCommandsScope(pCommandsScope)
    {
      m_pGALCommandEncoder = renderContext.GetCommandEncoder();
    }

    ezRenderContext& m_RenderContext;
    ezGALCommandEncoder* m_pCommandsScope;
    ezGALCommandEncoder* m_pGALCommandEncoder;
  };

  using RenderingScope = CommandEncoderScope<0>;
  EZ_ALWAYS_INLINE static RenderingScope BeginRenderingScope(const ezRenderViewContext& viewContext, const ezGALRenderingSetup& renderingSetup, const char* szName = "", bool bStereoRendering = false)
  {
    viewContext.m_pRenderContext->BeginRendering(renderingSetup, viewContext.m_pViewData->m_ViewPortRect, szName, bStereoRendering);
    return RenderingScope(*viewContext.m_pRenderContext, nullptr);
  }

  EZ_ALWAYS_INLINE static RenderingScope BeginCommandsAndRenderingScope(const ezRenderViewContext& viewContext, const ezGALRenderingSetup& renderingSetup, const char* szName, bool bStereoRendering = false)
  {
    ezGALCommandEncoder* pCommandEncoder = ezGALDevice::GetDefaultDevice()->BeginCommands(szName);
    viewContext.m_pRenderContext->BeginRendering(renderingSetup, viewContext.m_pViewData->m_ViewPortRect, "", bStereoRendering);
    return RenderingScope(*viewContext.m_pRenderContext, pCommandEncoder);
  }

  using ComputeScope = CommandEncoderScope<1>;
  EZ_ALWAYS_INLINE static ComputeScope BeginComputeScope(const ezRenderViewContext& viewContext, const char* szName = "")
  {
    viewContext.m_pRenderContext->BeginCompute(szName);
    return ComputeScope(*viewContext.m_pRenderContext, nullptr);
  }

  EZ_ALWAYS_INLINE static ComputeScope BeginCommandsAndComputeScope(const ezRenderViewContext& viewContext, const char* szName)
  {
    ezGALCommandEncoder* pCommandEncoder = ezGALDevice::GetDefaultDevice()->BeginCommands(szName);

    viewContext.m_pRenderContext->BeginCompute();
    return ComputeScope(*viewContext.m_pRenderContext, pCommandEncoder);
  }

  EZ_ALWAYS_INLINE ezGALCommandEncoder* GetCommandEncoder()
  {
    EZ_ASSERT_DEBUG(m_pGALCommandEncoder != nullptr, "Outside of BeginCommands / EndCommands scope of the device");
    return m_pGALCommandEncoder;
  }

  // Member Functions
  void SetShaderPermutationVariable(const char* szName, const ezTempHashedString& sValue);
  void SetShaderPermutationVariable(const ezHashedString& sName, const ezHashedString& sValue);

  void BindMaterial(const ezMaterialResourceHandle& hMaterial);

  void BindTexture2D(const ezTempHashedString& sSlotName, const ezTexture2DResourceHandle& hTexture, ezResourceAcquireMode acquireMode = ezResourceAcquireMode::AllowLoadingFallback);
  void BindTexture3D(const ezTempHashedString& sSlotName, const ezTexture3DResourceHandle& hTexture, ezResourceAcquireMode acquireMode = ezResourceAcquireMode::AllowLoadingFallback);
  void BindTextureCube(const ezTempHashedString& sSlotName, const ezTextureCubeResourceHandle& hTexture, ezResourceAcquireMode acquireMode = ezResourceAcquireMode::AllowLoadingFallback);

  void BindTexture2D(const ezTempHashedString& sSlotName, ezGALTextureResourceViewHandle hResourceView);
  void BindTexture3D(const ezTempHashedString& sSlotName, ezGALTextureResourceViewHandle hResourceView);
  void BindTextureCube(const ezTempHashedString& sSlotName, ezGALTextureResourceViewHandle hResourceView);

  /// Binds a read+write texture or buffer
  void BindUAV(const ezTempHashedString& sSlotName, ezGALTextureUnorderedAccessViewHandle hUnorderedAccessViewHandle);
  void BindUAV(const ezTempHashedString& sSlotName, ezGALBufferUnorderedAccessViewHandle hUnorderedAccessViewHandle);

  void BindSamplerState(const ezTempHashedString& sSlotName, ezGALSamplerStateHandle hSamplerSate);

  void BindBuffer(const ezTempHashedString& sSlotName, ezGALBufferResourceViewHandle hResourceView);

  void BindConstantBuffer(const ezTempHashedString& sSlotName, ezGALBufferHandle hConstantBuffer);
  void BindConstantBuffer(const ezTempHashedString& sSlotName, ezConstantBufferStorageHandle hConstantBufferStorage);

  /// \brief Sets push constants to the given data block.
  /// Note that for platforms that don't support push constants, this is emulated via a constant buffer. Thus, a slot name must be provided as well which matches the name of the BEGIN_PUSH_CONSTANTS block in the shader.
  /// \param sSlotName Name of the BEGIN_PUSH_CONSTANTS block in the shader.
  /// \param data Data of the push constants. If more than 128 bytes, ezGALDeviceCapabilities::m_uiMaxPushConstantsSize should be checked to ensure the data block is not too big for the platform.
  void SetPushConstants(const ezTempHashedString& sSlotName, ezArrayPtr<const ezUInt8> data);

  /// Templated version of SetPushConstants.
  /// \tparam T Type of the push constants struct.
  /// \param sSlotName Name of the BEGIN_PUSH_CONSTANTS block in the shader.
  /// \param constants Instance of type T that contains the push constants.
  template <typename T>
  EZ_ALWAYS_INLINE void SetPushConstants(const ezTempHashedString& sSlotName, const T& constants)
  {
    SetPushConstants(sSlotName, ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(&constants), sizeof(T)));
  }

  /// \brief Sets the currently active shader on the given render context.
  ///
  /// This function has no effect until the next draw or dispatch call on the context.
  void BindShader(const ezShaderResourceHandle& hShader, ezBitflags<ezShaderBindFlags> flags = ezShaderBindFlags::Default);
  void SetBlendState(ezGALBlendStateHandle hBlendState);
  void SetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState);
  void SetRasterizerState(ezGALRasterizerStateHandle hRasterizerState);

  void BindMeshBuffer(const ezDynamicMeshBufferResourceHandle& hDynamicMeshBuffer);
  void BindMeshBuffer(const ezMeshBufferResourceHandle& hMeshBuffer);
  void BindMeshBuffer(ezGALBufferHandle hVertexBuffer, ezGALBufferHandle hIndexBuffer, const ezVertexDeclarationInfo* pVertexDeclarationInfo, ezGALPrimitiveTopology::Enum topology, ezUInt32 uiPrimitiveCount, ezGALBufferHandle hVertexBuffer2 = {}, ezGALBufferHandle hVertexBuffer3 = {}, ezGALBufferHandle hVertexBuffer4 = {});
  EZ_ALWAYS_INLINE void BindNullMeshBuffer(ezGALPrimitiveTopology::Enum topology, ezUInt32 uiPrimitiveCount)
  {
    BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, topology, uiPrimitiveCount);
  }
  void BindVertexBuffer(ezGALBufferHandle hVertexBuffer, ezUInt32 uiSlot, ezEnum<ezGALVertexBindingRate> rate = ezGALVertexBindingRate::Vertex, ezUInt32 uiOffset = 0);
  void SetCustomVertexStreams(ezArrayPtr<ezVertexStreamInfo> customStreams);

  ezResult DrawMeshBuffer(ezUInt32 uiPrimitiveCount = 0xFFFFFFFF, ezUInt32 uiFirstPrimitive = 0, ezUInt32 uiInstanceCount = 1);

  ezResult Dispatch(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY = 1, ezUInt32 uiThreadGroupCountZ = 1);

  ezResult ApplyContextStates(bool bForce = false);
  void ResetContextState();

  ezGlobalConstants& WriteGlobalConstants();
  const ezGlobalConstants& ReadGlobalConstants() const;
  void SetGlobalAndWorldTimeConstants(ezTime worldTime);

  /// \brief Sets the texture filter mode that is used by default for texture resources.
  ///
  /// The built in default is Anisotropic 4x.
  /// If the default setting is changed, already loaded textures might not adjust.
  /// Nearest filtering is not allowed as a default filter.
  void SetDefaultTextureFilter(ezTextureFilterSetting::Enum filter);

  /// \brief Returns the texture filter mode that is used by default for textures.
  ezTextureFilterSetting::Enum GetDefaultTextureFilter() const { return m_DefaultTextureFilter; }

  /// \brief Returns the 'fixed' texture filter setting that the combination of default texture filter and given \a configuration defines.
  ///
  /// If \a configuration is set to a fixed filter, that setting is returned.
  /// If it is one of LowestQuality to HighestQuality, the adjusted default filter is returned.
  /// When the default filter is used (with adjustments), the allowed range is Bilinear to Aniso16x, the Nearest filter is never used.
  ezTextureFilterSetting::Enum GetSpecificTextureFilter(ezTextureFilterSetting::Enum configuration) const;

  /// \brief Set async shader loading. During runtime all shaders should be preloaded so this is off by default.
  void SetAllowAsyncShaderLoading(bool bAllow);

  /// \brief Returns async shader loading. During runtime all shaders should be preloaded so this is off by default.
  bool GetAllowAsyncShaderLoading();


  // Static Functions
public:
  // Constant buffer storage handling
  template <typename T>
  EZ_ALWAYS_INLINE static ezConstantBufferStorageHandle CreateConstantBufferStorage()
  {
    return CreateConstantBufferStorage(sizeof(T));
  }

  template <typename T>
  EZ_FORCE_INLINE static ezConstantBufferStorageHandle CreateConstantBufferStorage(ezConstantBufferStorage<T>*& out_pStorage)
  {
    ezConstantBufferStorageBase* pStorage;
    ezConstantBufferStorageHandle hStorage = CreateConstantBufferStorage(sizeof(T), pStorage);
    out_pStorage = static_cast<ezConstantBufferStorage<T>*>(pStorage);
    return hStorage;
  }

  EZ_FORCE_INLINE static ezConstantBufferStorageHandle CreateConstantBufferStorage(ezUInt32 uiSizeInBytes)
  {
    ezConstantBufferStorageBase* pStorage;
    return CreateConstantBufferStorage(uiSizeInBytes, pStorage);
  }

  static ezConstantBufferStorageHandle CreateConstantBufferStorage(ezUInt32 uiSizeInBytes, ezConstantBufferStorageBase*& out_pStorage);
  static void DeleteConstantBufferStorage(ezConstantBufferStorageHandle hStorage);

  template <typename T>
  EZ_FORCE_INLINE static bool TryGetConstantBufferStorage(ezConstantBufferStorageHandle hStorage, ezConstantBufferStorage<T>*& out_pStorage)
  {
    ezConstantBufferStorageBase* pStorage = nullptr;
    bool bResult = TryGetConstantBufferStorage(hStorage, pStorage);
    out_pStorage = static_cast<ezConstantBufferStorage<T>*>(pStorage);
    return bResult;
  }

  static bool TryGetConstantBufferStorage(ezConstantBufferStorageHandle hStorage, ezConstantBufferStorageBase*& out_pStorage);

  template <typename T>
  EZ_FORCE_INLINE static T* GetConstantBufferData(ezConstantBufferStorageHandle hStorage)
  {
    ezConstantBufferStorage<T>* pStorage = nullptr;
    if (TryGetConstantBufferStorage(hStorage, pStorage))
    {
      return &(pStorage->GetDataForWriting());
    }

    return nullptr;
  }

  // Default sampler state
  static ezGALSamplerStateCreationDescription GetDefaultSamplerState(ezBitflags<ezDefaultSamplerFlags> flags);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, RendererContext);

  static void LoadBuiltinShader(ezShaderUtils::ezBuiltinShaderType type, ezShaderUtils::ezBuiltinShader& out_shader);
  static void RegisterImmutableSamplers();
  static void OnEngineStartup();
  static void OnEngineShutdown();
  static void GALStaticDeviceEventHandler(const ezGALDeviceEvent& e);

private:
  Statistics m_Statistics;
  ezBitflags<ezRenderContextFlags> m_StateFlags;
  ezShaderResourceHandle m_hActiveShader;
  ezGALShaderHandle m_hActiveGALShader;

  ezHashTable<ezHashedString, ezHashedString> m_PermutationVariables;
  ezMaterialResourceHandle m_hNewMaterial;
  ezMaterialResourceHandle m_hMaterial;

  ezShaderPermutationResourceHandle m_hActiveShaderPermutation;

  ezBitflags<ezShaderBindFlags> m_ShaderBindFlags;

  ezGALBufferHandle m_hVertexBuffers[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];
  ezUInt32 m_VertexBufferStrides[EZ_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  ezEnum<ezGALVertexBindingRate> m_VertexBufferBindingRates[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];
  ezUInt32 m_VertexBufferOffsets[EZ_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  ezVertexDeclarationInfo m_CustomVertexStreams;
  ezGALBufferHandle m_hIndexBuffer;
  const ezVertexDeclarationInfo* m_pVertexDeclarationInfo;

  ezUInt32 m_uiMeshBufferPrimitiveCount;
  ezEnum<ezTextureFilterSetting> m_DefaultTextureFilter;
  bool m_bAllowAsyncShaderLoading;
  bool m_bStereoRendering = false;

  ezGALGraphicsPipelineCreationDescription m_GraphicsPipeline;
  ezGALComputePipelineCreationDescription m_ComputePipeline;

  ezHashTable<ezUInt64, ezGALTextureResourceViewHandle> m_BoundTextures2D;
  ezHashTable<ezUInt64, ezGALTextureResourceViewHandle> m_BoundTextures3D;
  ezHashTable<ezUInt64, ezGALTextureResourceViewHandle> m_BoundTexturesCube;
  ezHashTable<ezUInt64, ezGALTextureUnorderedAccessViewHandle> m_BoundTextureUAVs;

  ezHashTable<ezUInt64, ezGALSamplerStateHandle> m_BoundSamplers;

  ezHashTable<ezUInt64, ezGALBufferResourceViewHandle> m_BoundBuffer;
  ezHashTable<ezUInt64, ezGALBufferUnorderedAccessViewHandle> m_BoundBufferUAVs;
  ezGALSamplerStateHandle m_hFallbackSampler;

  struct BoundConstantBuffer
  {
    EZ_DECLARE_POD_TYPE();

    BoundConstantBuffer() = default;
    BoundConstantBuffer(ezGALBufferHandle hConstantBuffer)
      : m_hConstantBuffer(hConstantBuffer)
    {
    }
    BoundConstantBuffer(ezConstantBufferStorageHandle hConstantBufferStorage)
      : m_hConstantBufferStorage(hConstantBufferStorage)
    {
    }

    ezGALBufferHandle m_hConstantBuffer;
    ezConstantBufferStorageHandle m_hConstantBufferStorage;
  };

  ezHashTable<ezUInt64, BoundConstantBuffer> m_BoundConstantBuffers;

  ezConstantBufferStorageHandle m_hGlobalConstantBufferStorage;
  ezConstantBufferStorageHandle m_hPushConstantsStorage;

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

  static ezResult BuildVertexDeclaration(ezGALShaderHandle hShader, ezArrayPtr<ezUInt32> vertexBufferStrides, ezArrayPtr<ezEnum<ezGALVertexBindingRate>> vertexBufferBindingRates, const ezVertexDeclarationInfo& decl, const ezVertexDeclarationInfo& customVertexDecl, ezGALVertexDeclarationHandle& out_Declaration);

  static ezMap<ShaderVertexDecl, ezGALVertexDeclarationHandle> s_GALVertexDeclarations;

  static ezMutex s_ConstantBufferStorageMutex;
  static ezIdTable<ezConstantBufferStorageId, ezConstantBufferStorageBase*> s_ConstantBufferStorageTable;
  static ezMap<ezUInt32, ezDynamicArray<ezConstantBufferStorageBase*>> s_FreeConstantBufferStorage;

private: // Per Renderer States
  ezGALCommandEncoder* m_pGALCommandEncoder = nullptr;
  ezEventSubscriptionID m_GALdeviceEventsId = 0;
  bool m_bRendering = false;
  bool m_bCompute = false;

  // Member Functions
  void UploadConstants();

  void SetShaderPermutationVariableInternal(const ezHashedString& sName, const ezHashedString& sValue);
  void BindShaderInternal(const ezShaderResourceHandle& hShader, ezBitflags<ezShaderBindFlags> flags);
  ezShaderPermutationResource* ApplyShaderState();
  void ApplyMaterialState();
  void ApplyConstantBufferBindings(const ezGALShader* pShader);
  void ApplyTextureBindings(const ezGALShader* pShader);
  void ApplyUAVBindings(const ezGALShader* pShader);
  void ApplySamplerBindings(const ezGALShader* pShader);
  void ApplyBufferBindings(const ezGALShader* pShader);
};
