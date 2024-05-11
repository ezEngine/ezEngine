#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Strings/String.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>
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
  ezRenderContext();
  ~ezRenderContext();
  friend class ezMemoryUtils;

  static ezRenderContext* s_pDefaultInstance;
  static ezHybridArray<ezRenderContext*, 4> s_Instances;

public:
  static ezRenderContext* GetDefaultInstance();
  static ezRenderContext* CreateInstance();
  static void DestroyInstance(ezRenderContext* pRenderer);

public:
  struct Statistics
  {
    Statistics();
    void Reset();

    ezUInt32 m_uiFailedDrawcalls;
  };

  Statistics GetAndResetStatistics();

  ezGALRenderCommandEncoder* BeginRendering(ezGALPass* pGALPass, const ezGALRenderingSetup& renderingSetup, const ezRectFloat& viewport, const char* szName = "", bool bStereoRendering = false);
  void EndRendering();

  ezGALComputeCommandEncoder* BeginCompute(ezGALPass* pGALPass, const char* szName = "");
  void EndCompute();

  // Helper class to automatically end rendering or compute on scope exit
  template <typename T>
  class CommandEncoderScope
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(CommandEncoderScope);

  public:
    EZ_ALWAYS_INLINE ~CommandEncoderScope()
    {
      m_RenderContext.EndCommandEncoder(m_pGALCommandEncoder);

      if (m_pGALPass != nullptr)
      {
        ezGALDevice::GetDefaultDevice()->EndPass(m_pGALPass);
      }
    }

    EZ_ALWAYS_INLINE T* operator->() { return m_pGALCommandEncoder; }
    EZ_ALWAYS_INLINE operator const T*() { return m_pGALCommandEncoder; }

  private:
    friend class ezRenderContext;

    EZ_ALWAYS_INLINE CommandEncoderScope(ezRenderContext& renderContext, ezGALPass* pGALPass, T* pGALCommandEncoder)
      : m_RenderContext(renderContext)
      , m_pGALPass(pGALPass)
      , m_pGALCommandEncoder(pGALCommandEncoder)
    {
    }

    ezRenderContext& m_RenderContext;
    ezGALPass* m_pGALPass;
    T* m_pGALCommandEncoder;
  };

  using RenderingScope = CommandEncoderScope<ezGALRenderCommandEncoder>;
  EZ_ALWAYS_INLINE static RenderingScope BeginRenderingScope(ezGALPass* pGALPass, const ezRenderViewContext& viewContext, const ezGALRenderingSetup& renderingSetup, const char* szName = "", bool bStereoRendering = false)
  {
    return RenderingScope(*viewContext.m_pRenderContext, nullptr, viewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, viewContext.m_pViewData->m_ViewPortRect, szName, bStereoRendering));
  }

  EZ_ALWAYS_INLINE static RenderingScope BeginPassAndRenderingScope(const ezRenderViewContext& viewContext, const ezGALRenderingSetup& renderingSetup, const char* szName, bool bStereoRendering = false)
  {
    ezGALPass* pGALPass = ezGALDevice::GetDefaultDevice()->BeginPass(szName);

    return RenderingScope(*viewContext.m_pRenderContext, pGALPass, viewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, viewContext.m_pViewData->m_ViewPortRect, "", bStereoRendering));
  }

  using ComputeScope = CommandEncoderScope<ezGALComputeCommandEncoder>;
  EZ_ALWAYS_INLINE static ComputeScope BeginComputeScope(ezGALPass* pGALPass, const ezRenderViewContext& viewContext, const char* szName = "")
  {
    return ComputeScope(*viewContext.m_pRenderContext, nullptr, viewContext.m_pRenderContext->BeginCompute(pGALPass, szName));
  }

  EZ_ALWAYS_INLINE static ComputeScope BeginPassAndComputeScope(const ezRenderViewContext& viewContext, const char* szName)
  {
    ezGALPass* pGALPass = ezGALDevice::GetDefaultDevice()->BeginPass(szName);

    return ComputeScope(*viewContext.m_pRenderContext, pGALPass, viewContext.m_pRenderContext->BeginCompute(pGALPass));
  }

  EZ_ALWAYS_INLINE ezGALCommandEncoder* GetCommandEncoder()
  {
    EZ_ASSERT_DEBUG(m_pGALCommandEncoder != nullptr, "BeginRendering/Compute has not been called");
    return m_pGALCommandEncoder;
  }

  EZ_ALWAYS_INLINE ezGALRenderCommandEncoder* GetRenderCommandEncoder()
  {
    EZ_ASSERT_DEBUG(m_pGALCommandEncoder != nullptr && !m_bCompute, "BeginRendering has not been called");
    return static_cast<ezGALRenderCommandEncoder*>(m_pGALCommandEncoder);
  }

  EZ_ALWAYS_INLINE ezGALComputeCommandEncoder* GetComputeCommandEncoder()
  {
    EZ_ASSERT_DEBUG(m_pGALCommandEncoder != nullptr && m_bCompute, "BeginCompute has not been called");
    return static_cast<ezGALComputeCommandEncoder*>(m_pGALCommandEncoder);
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
  void BindUAV(const ezTempHashedString& sSlotName, ezGALUnorderedAccessViewHandle hUnorderedAccessViewHandle);

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

  void BindMeshBuffer(const ezDynamicMeshBufferResourceHandle& hDynamicMeshBuffer);
  void BindMeshBuffer(const ezMeshBufferResourceHandle& hMeshBuffer);
  void BindMeshBuffer(ezGALBufferHandle hVertexBuffer, ezGALBufferHandle hIndexBuffer, const ezVertexDeclarationInfo* pVertexDeclarationInfo, ezGALPrimitiveTopology::Enum topology, ezUInt32 uiPrimitiveCount, ezGALBufferHandle hVertexBuffer2 = {}, ezGALBufferHandle hVertexBuffer3 = {}, ezGALBufferHandle hVertexBuffer4 = {});
  EZ_ALWAYS_INLINE void BindNullMeshBuffer(ezGALPrimitiveTopology::Enum topology, ezUInt32 uiPrimitiveCount)
  {
    BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, topology, uiPrimitiveCount);
  }

  ezResult DrawMeshBuffer(ezUInt32 uiPrimitiveCount = 0xFFFFFFFF, ezUInt32 uiFirstPrimitive = 0, ezUInt32 uiInstanceCount = 1);

  ezResult Dispatch(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY = 1, ezUInt32 uiThreadGroupCountZ = 1);

  ezResult ApplyContextStates(bool bForce = false);
  void ResetContextState();

  ezGlobalConstants& WriteGlobalConstants();
  const ezGlobalConstants& ReadGlobalConstants() const;

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

  ezGALBufferHandle m_hVertexBuffers[4];
  ezGALBufferHandle m_hIndexBuffer;
  const ezVertexDeclarationInfo* m_pVertexDeclarationInfo;
  ezGALPrimitiveTopology::Enum m_Topology;
  ezUInt32 m_uiMeshBufferPrimitiveCount;
  ezEnum<ezTextureFilterSetting> m_DefaultTextureFilter;
  bool m_bAllowAsyncShaderLoading;
  bool m_bStereoRendering = false;

  ezHashTable<ezUInt64, ezGALTextureResourceViewHandle> m_BoundTextures2D;
  ezHashTable<ezUInt64, ezGALTextureResourceViewHandle> m_BoundTextures3D;
  ezHashTable<ezUInt64, ezGALTextureResourceViewHandle> m_BoundTexturesCube;
  ezHashTable<ezUInt64, ezGALUnorderedAccessViewHandle> m_BoundUAVs;
  ezHashTable<ezUInt64, ezGALSamplerStateHandle> m_BoundSamplers;
  ezHashTable<ezUInt64, ezGALBufferResourceViewHandle> m_BoundBuffer;
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

  static ezResult BuildVertexDeclaration(ezGALShaderHandle hShader, const ezVertexDeclarationInfo& decl, ezGALVertexDeclarationHandle& out_Declaration);

  static ezMap<ShaderVertexDecl, ezGALVertexDeclarationHandle> s_GALVertexDeclarations;

  static ezMutex s_ConstantBufferStorageMutex;
  static ezIdTable<ezConstantBufferStorageId, ezConstantBufferStorageBase*> s_ConstantBufferStorageTable;
  static ezMap<ezUInt32, ezDynamicArray<ezConstantBufferStorageBase*>> s_FreeConstantBufferStorage;

private: // Per Renderer States
  friend RenderingScope;
  friend ComputeScope;
  EZ_ALWAYS_INLINE void EndCommandEncoder(ezGALRenderCommandEncoder*) { EndRendering(); }
  EZ_ALWAYS_INLINE void EndCommandEncoder(ezGALComputeCommandEncoder*) { EndCompute(); }

  ezGALPass* m_pGALPass = nullptr;
  ezGALCommandEncoder* m_pGALCommandEncoder = nullptr;
  bool m_bCompute = false;

  // Member Functions
  void UploadConstants();

  void SetShaderPermutationVariableInternal(const ezHashedString& sName, const ezHashedString& sValue);
  void BindShaderInternal(const ezShaderResourceHandle& hShader, ezBitflags<ezShaderBindFlags> flags);
  ezShaderPermutationResource* ApplyShaderState();
  ezMaterialResource* ApplyMaterialState();
  void ApplyConstantBufferBindings(const ezGALShader* pShader);
  void ApplyTextureBindings(const ezGALShader* pShader);
  void ApplyUAVBindings(const ezGALShader* pShader);
  void ApplySamplerBindings(const ezGALShader* pShader);
  void ApplyBufferBindings(const ezGALShader* pShader);
};
