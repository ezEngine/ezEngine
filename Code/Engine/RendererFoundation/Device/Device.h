
#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/DeviceCapabilities.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezColor;

/// \brief The ezRenderDevice class is the primary interface for interactions with rendering APIs
/// It contains a set of (non-virtual) functions to set state, create resources etc. which rely on
/// API specific implementations provided by protected virtual functions.
/// Redundant state changes are prevented at the platform independent level in the non-virtual functions.
class EZ_RENDERERFOUNDATION_DLL ezGALDevice
{
public:
  ezEvent<const ezGALDeviceEvent&> m_Events;

  // Init & shutdown functions

  ezResult Init();
  ezResult Shutdown();

  // Pass functions

  ezGALPass* BeginPass(const char* szName);
  void EndPass(ezGALPass* pPass);

  // State creation functions

  ezGALBlendStateHandle CreateBlendState(const ezGALBlendStateCreationDescription& Description);
  void DestroyBlendState(ezGALBlendStateHandle hBlendState);

  ezGALDepthStencilStateHandle CreateDepthStencilState(const ezGALDepthStencilStateCreationDescription& Description);
  void DestroyDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState);

  ezGALRasterizerStateHandle CreateRasterizerState(const ezGALRasterizerStateCreationDescription& Description);
  void DestroyRasterizerState(ezGALRasterizerStateHandle hRasterizerState);

  ezGALSamplerStateHandle CreateSamplerState(const ezGALSamplerStateCreationDescription& Description);
  void DestroySamplerState(ezGALSamplerStateHandle hSamplerState);

  // Resource creation functions

  ezGALShaderHandle CreateShader(const ezGALShaderCreationDescription& Description);
  void DestroyShader(ezGALShaderHandle hShader);

  ezGALBufferHandle CreateBuffer(const ezGALBufferCreationDescription& Description, ezArrayPtr<const ezUInt8> pInitialData = ezArrayPtr<const ezUInt8>());
  void DestroyBuffer(ezGALBufferHandle hBuffer);

  // Helper functions for buffers (for common, simple use cases)

  ezGALBufferHandle CreateVertexBuffer(ezUInt32 uiVertexSize, ezUInt32 uiVertexCount, ezArrayPtr<const ezUInt8> pInitialData = ezArrayPtr<const ezUInt8>());
  ezGALBufferHandle CreateIndexBuffer(ezGALIndexType::Enum IndexType, ezUInt32 uiIndexCount, ezArrayPtr<const ezUInt8> pInitialData = ezArrayPtr<const ezUInt8>());
  ezGALBufferHandle CreateConstantBuffer(ezUInt32 uiBufferSize);

  ezGALTextureHandle CreateTexture(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData = ezArrayPtr<ezGALSystemMemoryDescription>());
  ezResult ReplaceExisitingNativeObject(ezGALTextureHandle hTexture, void* pExisitingNativeObject);
  void DestroyTexture(ezGALTextureHandle hTexture);

  ezGALTextureHandle CreateProxyTexture(ezGALTextureHandle hParentTexture, ezUInt32 uiSlice);
  void DestroyProxyTexture(ezGALTextureHandle hProxyTexture);

  // Resource views
  ezGALResourceViewHandle GetDefaultResourceView(ezGALTextureHandle hTexture);
  ezGALResourceViewHandle GetDefaultResourceView(ezGALBufferHandle hBuffer);

  ezGALResourceViewHandle CreateResourceView(const ezGALResourceViewCreationDescription& Description);
  void DestroyResourceView(ezGALResourceViewHandle hResourceView);

  // Render target views
  ezGALRenderTargetViewHandle GetDefaultRenderTargetView(ezGALTextureHandle hTexture);

  ezGALRenderTargetViewHandle CreateRenderTargetView(const ezGALRenderTargetViewCreationDescription& Description);
  void DestroyRenderTargetView(ezGALRenderTargetViewHandle hRenderTargetView);

  // Unordered access views
  ezGALUnorderedAccessViewHandle CreateUnorderedAccessView(const ezGALUnorderedAccessViewCreationDescription& Description);
  void DestroyUnorderedAccessView(ezGALUnorderedAccessViewHandle hUnorderedAccessView);


  // Other rendering creation functions

  ezGALSwapChainHandle CreateSwapChain(const ezGALSwapChainCreationDescription& Description);
  void DestroySwapChain(ezGALSwapChainHandle hSwapChain);

  ezGALFenceHandle CreateFence();
  void DestroyFence(ezGALFenceHandle& hFence);

  ezGALQueryHandle CreateQuery(const ezGALQueryCreationDescription& Description);
  void DestroyQuery(ezGALQueryHandle hQuery);

  ezGALVertexDeclarationHandle CreateVertexDeclaration(const ezGALVertexDeclarationCreationDescription& Description);
  void DestroyVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration);

  // Timestamp functions

  ezResult GetTimestampResult(ezGALTimestampHandle hTimestamp, ezTime& result);

  /// \todo Map functions to save on memcpys

  // Swap chain functions

  void Present(ezGALSwapChainHandle hSwapChain, bool bVSync);
  ezGALTextureHandle GetBackBufferTextureFromSwapChain(ezGALSwapChainHandle hSwapChain);


  // Misc functions

  void BeginFrame();
  void EndFrame();

  void SetPrimarySwapChain(ezGALSwapChainHandle hSwapChain);
  ezGALSwapChainHandle GetPrimarySwapChain() const;

  ezGALTimestampHandle GetTimestamp();

  const ezGALDeviceCreationDescription* GetDescription() const;


  const ezGALSwapChain* GetSwapChain(ezGALSwapChainHandle hSwapChain) const;
  const ezGALShader* GetShader(ezGALShaderHandle hShader) const;
  const ezGALTexture* GetTexture(ezGALTextureHandle hTexture) const;
  const ezGALBuffer* GetBuffer(ezGALBufferHandle hBuffer) const;
  const ezGALDepthStencilState* GetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState) const;
  const ezGALBlendState* GetBlendState(ezGALBlendStateHandle hBlendState) const;
  const ezGALRasterizerState* GetRasterizerState(ezGALRasterizerStateHandle hRasterizerState) const;
  const ezGALVertexDeclaration* GetVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration) const;
  const ezGALSamplerState* GetSamplerState(ezGALSamplerStateHandle hSamplerState) const;
  const ezGALResourceView* GetResourceView(ezGALResourceViewHandle hResourceView) const;
  const ezGALRenderTargetView* GetRenderTargetView(ezGALRenderTargetViewHandle hRenderTargetView) const;
  const ezGALUnorderedAccessView* GetUnorderedAccessView(ezGALUnorderedAccessViewHandle hUnorderedAccessView) const;
  const ezGALFence* GetFence(ezGALFenceHandle hFence) const;
  const ezGALQuery* GetQuery(ezGALQueryHandle hQuery) const;

  const ezGALDeviceCapabilities& GetCapabilities() const;

  virtual ezUInt64 GetMemoryConsumptionForTexture(const ezGALTextureCreationDescription& Description) const;
  virtual ezUInt64 GetMemoryConsumptionForBuffer(const ezGALBufferCreationDescription& Description) const;

  static void SetDefaultDevice(ezGALDevice* pDefaultDevice);
  static ezGALDevice* GetDefaultDevice();
  static bool HasDefaultDevice();

  // public in case someone external needs to lock multiple operations
  mutable ezMutex m_Mutex;

private:
  static ezGALDevice* s_pDefaultDevice;

protected:
  ezGALDevice(const ezGALDeviceCreationDescription& Description);

  virtual ~ezGALDevice();

  template <typename IdTableType, typename ReturnType>
  ReturnType* Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const;

  void DestroyViews(ezGALResourceBase* pResource);

  template <typename HandleType>
  void AddDeadObject(ezUInt32 uiType, HandleType handle);

  template <typename HandleType>
  void ReviveDeadObject(ezUInt32 uiType, HandleType handle);

  void DestroyDeadObjects();

  /// \brief Asserts that either this device supports multi-threaded resource creation, or that this function is executed on the main thread.
  void VerifyMultithreadedAccess() const;

  ezProxyAllocator m_Allocator;
  ezLocalAllocatorWrapper m_AllocatorWrapper;

  using ShaderTable = ezIdTable<ezGALShaderHandle::IdType, ezGALShader*, ezLocalAllocatorWrapper>;
  using BlendStateTable = ezIdTable<ezGALBlendStateHandle::IdType, ezGALBlendState*, ezLocalAllocatorWrapper>;
  using DepthStencilStateTable = ezIdTable<ezGALDepthStencilStateHandle::IdType, ezGALDepthStencilState*, ezLocalAllocatorWrapper>;
  using RasterizerStateTable = ezIdTable<ezGALRasterizerStateHandle::IdType, ezGALRasterizerState*, ezLocalAllocatorWrapper>;
  using BufferTable = ezIdTable<ezGALBufferHandle::IdType, ezGALBuffer*, ezLocalAllocatorWrapper>;
  using TextureTable = ezIdTable<ezGALTextureHandle::IdType, ezGALTexture*, ezLocalAllocatorWrapper>;
  using ResourceViewTable = ezIdTable<ezGALResourceViewHandle::IdType, ezGALResourceView*, ezLocalAllocatorWrapper>;
  using SamplerStateTable = ezIdTable<ezGALSamplerStateHandle::IdType, ezGALSamplerState*, ezLocalAllocatorWrapper>;
  using RenderTargetViewTable = ezIdTable<ezGALRenderTargetViewHandle::IdType, ezGALRenderTargetView*, ezLocalAllocatorWrapper>;
  using UnorderedAccessViewTable = ezIdTable<ezGALUnorderedAccessViewHandle::IdType, ezGALUnorderedAccessView*, ezLocalAllocatorWrapper>;
  using SwapChainTable = ezIdTable<ezGALSwapChainHandle::IdType, ezGALSwapChain*, ezLocalAllocatorWrapper>;
  using FenceTable = ezIdTable<ezGALFenceHandle::IdType, ezGALFence*, ezLocalAllocatorWrapper>;
  using QueryTable = ezIdTable<ezGALQueryHandle::IdType, ezGALQuery*, ezLocalAllocatorWrapper>;
  using VertexDeclarationTable = ezIdTable<ezGALVertexDeclarationHandle::IdType, ezGALVertexDeclaration*, ezLocalAllocatorWrapper>;

  ShaderTable m_Shaders;
  BlendStateTable m_BlendStates;
  DepthStencilStateTable m_DepthStencilStates;
  RasterizerStateTable m_RasterizerStates;
  BufferTable m_Buffers;
  TextureTable m_Textures;
  ResourceViewTable m_ResourceViews;
  SamplerStateTable m_SamplerStates;
  RenderTargetViewTable m_RenderTargetViews;
  UnorderedAccessViewTable m_UnorderedAccessViews;
  SwapChainTable m_SwapChains;
  FenceTable m_Fences;
  QueryTable m_Queries;
  VertexDeclarationTable m_VertexDeclarations;


  // Hash tables used to prevent state object duplication
  ezHashTable<ezUInt32, ezGALBlendStateHandle, ezHashHelper<ezUInt32>, ezLocalAllocatorWrapper> m_BlendStateTable;
  ezHashTable<ezUInt32, ezGALDepthStencilStateHandle, ezHashHelper<ezUInt32>, ezLocalAllocatorWrapper> m_DepthStencilStateTable;
  ezHashTable<ezUInt32, ezGALRasterizerStateHandle, ezHashHelper<ezUInt32>, ezLocalAllocatorWrapper> m_RasterizerStateTable;
  ezHashTable<ezUInt32, ezGALSamplerStateHandle, ezHashHelper<ezUInt32>, ezLocalAllocatorWrapper> m_SamplerStateTable;
  ezHashTable<ezUInt32, ezGALVertexDeclarationHandle, ezHashHelper<ezUInt32>, ezLocalAllocatorWrapper> m_VertexDeclarationTable;

  struct DeadObject
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiType;
    ezUInt32 m_uiHandle;
  };

  ezDynamicArray<DeadObject, ezLocalAllocatorWrapper> m_DeadObjects;

  ezGALDeviceCreationDescription m_Description;

  ezGALSwapChainHandle m_hPrimarySwapChain;

  ezGALDeviceCapabilities m_Capabilities;

  // Deactivate Doxygen document generation for the following block. (API abstraction only)
  /// \cond

  // These functions need to be implemented by a render API abstraction
protected:
  friend class ezMemoryUtils;

  // Init & shutdown functions

  virtual ezResult InitPlatform() = 0;
  virtual ezResult ShutdownPlatform() = 0;

  // Pass functions

  virtual ezGALPass* BeginPassPlatform(const char* szName) = 0;
  virtual void EndPassPlatform(ezGALPass* pPass) = 0;

  // State creation functions

  virtual ezGALBlendState* CreateBlendStatePlatform(const ezGALBlendStateCreationDescription& Description) = 0;
  virtual void DestroyBlendStatePlatform(ezGALBlendState* pBlendState) = 0;

  virtual ezGALDepthStencilState* CreateDepthStencilStatePlatform(const ezGALDepthStencilStateCreationDescription& Description) = 0;
  virtual void DestroyDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState) = 0;

  virtual ezGALRasterizerState* CreateRasterizerStatePlatform(const ezGALRasterizerStateCreationDescription& Description) = 0;
  virtual void DestroyRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState) = 0;

  virtual ezGALSamplerState* CreateSamplerStatePlatform(const ezGALSamplerStateCreationDescription& Description) = 0;
  virtual void DestroySamplerStatePlatform(ezGALSamplerState* pSamplerState) = 0;

  // Resource creation functions

  virtual ezGALShader* CreateShaderPlatform(const ezGALShaderCreationDescription& Description) = 0;
  virtual void DestroyShaderPlatform(ezGALShader* pShader) = 0;

  virtual ezGALBuffer* CreateBufferPlatform(const ezGALBufferCreationDescription& Description, ezArrayPtr<const ezUInt8> pInitialData) = 0;
  virtual void DestroyBufferPlatform(ezGALBuffer* pBuffer) = 0;

  virtual ezGALTexture* CreateTexturePlatform(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) = 0;
  virtual void DestroyTexturePlatform(ezGALTexture* pTexture) = 0;

  virtual ezGALResourceView* CreateResourceViewPlatform(ezGALResourceBase* pResource, const ezGALResourceViewCreationDescription& Description) = 0;
  virtual void DestroyResourceViewPlatform(ezGALResourceView* pResourceView) = 0;

  virtual ezGALRenderTargetView* CreateRenderTargetViewPlatform(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description) = 0;
  virtual void DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView) = 0;

  virtual ezGALUnorderedAccessView* CreateUnorderedAccessViewPlatform(ezGALResourceBase* pResource, const ezGALUnorderedAccessViewCreationDescription& Description) = 0;
  virtual void DestroyUnorderedAccessViewPlatform(ezGALUnorderedAccessView* pRenderTargetView) = 0;

  // Other rendering creation functions

  virtual ezGALSwapChain* CreateSwapChainPlatform(const ezGALSwapChainCreationDescription& Description) = 0;
  virtual void DestroySwapChainPlatform(ezGALSwapChain* pSwapChain) = 0;

  virtual ezGALFence* CreateFencePlatform() = 0;
  virtual void DestroyFencePlatform(ezGALFence* pFence) = 0;

  virtual ezGALQuery* CreateQueryPlatform(const ezGALQueryCreationDescription& Description) = 0;
  virtual void DestroyQueryPlatform(ezGALQuery* pQuery) = 0;

  virtual ezGALVertexDeclaration* CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description) = 0;
  virtual void DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) = 0;

  // Timestamp functions

  virtual ezGALTimestampHandle GetTimestampPlatform() = 0;
  virtual ezResult GetTimestampResultPlatform(ezGALTimestampHandle hTimestamp, ezTime& result) = 0;

  // Swap chain functions

  virtual void PresentPlatform(ezGALSwapChain* pSwapChain, bool bVSync) = 0;

  // Misc functions

  virtual void BeginFramePlatform() = 0;
  virtual void EndFramePlatform() = 0;

  virtual void SetPrimarySwapChainPlatform(ezGALSwapChain* pSwapChain) = 0;

  virtual void FillCapabilitiesPlatform() = 0;

  /// \endcond

private:
  bool m_bBeginFrameCalled = false;
  bool m_bBeginPassCalled = false;
};

#include <RendererFoundation/Device/Implementation/Device_inl.h>
