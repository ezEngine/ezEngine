
#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/DeviceCapabilities.h>
#include <RendererFoundation/Device/ReadbackLock.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezColor;

/// \brief The ezRenderDevice class is the primary interface for interactions with rendering APIs
/// It contains a set of (non-virtual) functions to set state, create resources etc. which rely on
/// API specific implementations provided by protected virtual functions.
/// Redundant state changes are prevented at the platform independent level in the non-virtual functions.
class EZ_RENDERERFOUNDATION_DLL ezGALDevice
{
public:
  static ezEvent<const ezGALDeviceEvent&, ezMutex> s_Events;

  // Init & shutdown functions

  ezResult Init();
  ezResult Shutdown();
  ezStringView GetRenderer();

  // Commands functions

  /// \brief Begin recording GPU commands on the returned command encoder.
  ezGALCommandEncoder* BeginCommands(const char* szName);
  /// \brief Stop recording commands on the command encoder.
  /// \param pCommandEncoder Must match the command encoder returned by BeginCommands.
  void EndCommands(ezGALCommandEncoder* pCommandEncoder);

  // State creation functions

  ezGALBlendStateHandle CreateBlendState(const ezGALBlendStateCreationDescription& description);
  void DestroyBlendState(ezGALBlendStateHandle hBlendState);

  ezGALDepthStencilStateHandle CreateDepthStencilState(const ezGALDepthStencilStateCreationDescription& description);
  void DestroyDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState);

  ezGALRasterizerStateHandle CreateRasterizerState(const ezGALRasterizerStateCreationDescription& description);
  void DestroyRasterizerState(ezGALRasterizerStateHandle hRasterizerState);

  ezGALSamplerStateHandle CreateSamplerState(const ezGALSamplerStateCreationDescription& description);
  void DestroySamplerState(ezGALSamplerStateHandle hSamplerState);

  // Resource creation functions

  ezGALShaderHandle CreateShader(const ezGALShaderCreationDescription& description);
  void DestroyShader(ezGALShaderHandle hShader);

  ezGALBufferHandle CreateBuffer(const ezGALBufferCreationDescription& description, ezArrayPtr<const ezUInt8> initialData = ezArrayPtr<const ezUInt8>());
  void DestroyBuffer(ezGALBufferHandle hBuffer);

  ezGALDynamicBufferHandle CreateDynamicBuffer(const ezGALBufferCreationDescription& description, ezStringView sDebugName);
  void DestroyDynamicBuffer(ezGALDynamicBufferHandle& inout_hBuffer);

  // Helper functions for buffers (for common, simple use cases)

  ezGALBufferHandle CreateVertexBuffer(ezUInt32 uiVertexSize, ezUInt32 uiVertexCount, ezArrayPtr<const ezUInt8> initialData = ezArrayPtr<const ezUInt8>(), bool bDataIsMutable = false);
  ezGALBufferHandle CreateIndexBuffer(ezGALIndexType::Enum indexType, ezUInt32 uiIndexCount, ezArrayPtr<const ezUInt8> initialData = ezArrayPtr<const ezUInt8>(), bool bDataIsMutable = false);
  ezGALBufferHandle CreateConstantBuffer(ezUInt32 uiBufferSize);

  ezGALTextureHandle CreateTexture(const ezGALTextureCreationDescription& description, ezArrayPtr<ezGALSystemMemoryDescription> initialData = ezArrayPtr<ezGALSystemMemoryDescription>());
  void DestroyTexture(ezGALTextureHandle hTexture);

  ezGALTextureHandle CreateProxyTexture(ezGALTextureHandle hParentTexture, ezUInt32 uiSlice);
  void DestroyProxyTexture(ezGALTextureHandle hProxyTexture);

  ezGALTextureHandle CreateSharedTexture(const ezGALTextureCreationDescription& description, ezArrayPtr<ezGALSystemMemoryDescription> initialData = {});
  ezGALTextureHandle OpenSharedTexture(const ezGALTextureCreationDescription& description, ezGALPlatformSharedHandle hSharedHandle);
  void DestroySharedTexture(ezGALTextureHandle hTexture);

  ezGALReadbackBufferHandle CreateReadbackBuffer(const ezGALBufferCreationDescription& description);
  void DestroyReadbackBuffer(ezGALReadbackBufferHandle hBuffer);

  ezGALReadbackTextureHandle CreateReadbackTexture(const ezGALTextureCreationDescription& description);
  void DestroyReadbackTexture(ezGALReadbackTextureHandle hTexture);

  // Resource update functions

  /// \brief Ensures that the given buffer is updated at the beginning of the next frame.
  void UpdateBufferForNextFrame(ezGALBufferHandle hBuffer, ezConstByteArrayPtr sourceData, ezUInt32 uiDestOffset = 0);

  /// \brief Ensures that the given texture is updated at the beginning of the next frame.
  void UpdateTextureForNextFrame(ezGALTextureHandle hTexture, const ezGALSystemMemoryDescription& sourceData, const ezGALTextureSubresource& destinationSubResource = {}, const ezBoundingBoxu32& destinationBox = ezBoundingBoxu32::MakeZero());

  // Resource views
  ezGALTextureResourceViewHandle GetDefaultResourceView(ezGALTextureHandle hTexture);
  ezGALBufferResourceViewHandle GetDefaultResourceView(ezGALBufferHandle hBuffer);

  ezGALTextureResourceViewHandle CreateResourceView(const ezGALTextureResourceViewCreationDescription& description);
  void DestroyResourceView(ezGALTextureResourceViewHandle hResourceView);

  ezGALBufferResourceViewHandle CreateResourceView(const ezGALBufferResourceViewCreationDescription& description);
  void DestroyResourceView(ezGALBufferResourceViewHandle hResourceView);

  // Render target views
  ezGALRenderTargetViewHandle GetDefaultRenderTargetView(ezGALTextureHandle hTexture);

  ezGALRenderTargetViewHandle CreateRenderTargetView(const ezGALRenderTargetViewCreationDescription& description);
  void DestroyRenderTargetView(ezGALRenderTargetViewHandle hRenderTargetView);

  // Unordered access views
  ezGALTextureUnorderedAccessViewHandle CreateUnorderedAccessView(const ezGALTextureUnorderedAccessViewCreationDescription& description);
  void DestroyUnorderedAccessView(ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView);

  ezGALBufferUnorderedAccessViewHandle CreateUnorderedAccessView(const ezGALBufferUnorderedAccessViewCreationDescription& description);
  void DestroyUnorderedAccessView(ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView);

  // Other rendering creation functions

  using SwapChainFactoryFunction = ezDelegate<ezGALSwapChain*(ezAllocator*)>;
  ezGALSwapChainHandle CreateSwapChain(const SwapChainFactoryFunction& func);
  ezResult UpdateSwapChain(ezGALSwapChainHandle hSwapChain, ezEnum<ezGALPresentMode> newPresentMode);
  void DestroySwapChain(ezGALSwapChainHandle hSwapChain);

  ezGALVertexDeclarationHandle CreateVertexDeclaration(const ezGALVertexDeclarationCreationDescription& description);
  void DestroyVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration);

  // GPU -> CPU query functions

  /// \brief Queries the result of a timestamp.
  /// Should be called every frame until ezGALAsyncResult::Ready is returned.
  /// \param hTimestamp The timestamp handle to query.
  /// \param out_result If ezGALAsyncResult::Ready is returned, this will be the timestamp at which this handle was inserted into the command encoder.
  /// \return If ezGALAsyncResult::Expired is returned, the result was in a ready state for more than 4 frames and was thus deleted.
  /// \sa ezCommandEncoder::InsertTimestamp
  ezEnum<ezGALAsyncResult> GetTimestampResult(ezGALTimestampHandle hTimestamp, ezTime& out_result);

  /// \brief Queries the result of an occlusion query.
  /// Should be called every frame until ezGALAsyncResult::Ready is returned.
  /// \param hOcclusion The occlusion query handle to query.
  /// \param out_uiResult If ezGALAsyncResult::Ready is returned, this will be the number of pixels of the occlusion query.
  /// \return If ezGALAsyncResult::Expired is returned, the result was in a ready state for more than 4 frames and was thus deleted.
  /// \sa ezCommandEncoder::BeginOcclusionQuery, ezCommandEncoder::EndOcclusionQuery
  ezEnum<ezGALAsyncResult> GetOcclusionQueryResult(ezGALOcclusionHandle hOcclusion, ezUInt64& out_uiResult);

  /// \brief Queries the result of a fence.
  /// Fences can never expire as they are just monotonically increasing numbers over time.
  /// \param hFence The fence handle to query.
  /// \param timeout If set to > 0, the function will block until the fence is ready or the timeout is reached.
  /// \return Returns either Ready or Pending.
  /// \sa ezCommandEncoder::InsertFence
  ezEnum<ezGALAsyncResult> GetFenceResult(ezGALFenceHandle hFence, ezTime timeout = ezTime::MakeZero());

  /// \brief Tries to lock a readback buffer for reading. Only fails if the handle is invalid.
  /// \param hReadbackBuffer The buffer to lock.
  /// \param out_memory If successful, contains the memory of the buffer. Only allowed to be accessed within the lifetime of the returns lock object.
  /// \return Returns the lock. ezReadbackBufferLock::IsValid needs to be called to ensure the locking was successful.
  ezReadbackBufferLock LockBuffer(ezGALReadbackBufferHandle hReadbackBuffer, ezArrayPtr<const ezUInt8>& out_memory);

  /// \brief Tries to lock a readback texture for reading. Only fails if the handle is invalid.
  /// \param hReadbackTexture The texture to lock.
  /// \param subResources The sub-resources that should to be locked.
  /// \param out_memory If successful, contains the memory locations of each sub-resource. Only allowed to be accessed within the lifetime of the returns lock object.
  /// \return Returns the lock. ezReadbackTextureLock::IsValid needs to be called to ensure the locking was successful.
  ezReadbackTextureLock LockTexture(ezGALReadbackTextureHandle hReadbackTexture, const ezArrayPtr<const ezGALTextureSubresource>& subResources, ezDynamicArray<ezGALSystemMemoryDescription>& out_memory);


  // Swap chain functions

  ezGALTextureHandle GetBackBufferTextureFromSwapChain(ezGALSwapChainHandle hSwapChain);


  // Misc functions

  /// \brief Adds a swap-chain to be used for the next frame.
  /// Must be called before or during the ezGALDeviceEvent::BeforeBeginFrame event (BeginFrame function) and repeated for every frame the swap-chain is to be used. This approach guarantees that all swap-chains of a frame acquire and present at the same time, which improves frame pacing.
  /// \param hSwapChain Swap-chain used in this frame. The device will ensure to acquire an image from the swap-chain during BeginFrame and present it when calling EndFrame.
  void EnqueueFrameSwapChain(ezGALSwapChainHandle hSwapChain);

  /// \brief Begins rendering of a frame. This needs to be called first before any rendering function can be called.
  /// \param uiAppFrame Frame index for debugging purposes, has no effect on GetCurrentFrame.
  void BeginFrame(const ezUInt64 uiAppFrame = 0);

  /// \brief Ends rendering of a frame and submits all data to the GPU. No further rendering calls are allowed until BeginFrame is called again.
  void EndFrame();

  /// \brief The current rendering frame.
  /// This is a monotonically increasing number which changes +1 every time EndFrame is called. You can use this to synchronize read/writes between CPU and GPU, see GetSafeFrame.
  /// \sa GetSafeFrame
  ezUInt64 GetCurrentFrame() const;
  /// \brief The latest frame that has been fully executed on the GPU.
  /// Whenever you execute any work that requires synchronization between CPU and GPU, remember the GetCurrentFrame result in which the operation was done. When GetSafeFrame reaches this number, you know for sure that the GPU has completed all operations of that frame.
  /// \sa GetCurrentFrame
  ezUInt64 GetSafeFrame() const;


  const ezGALDeviceCreationDescription* GetDescription() const;

  const ezGALSwapChain* GetSwapChain(ezGALSwapChainHandle hSwapChain) const;
  template <typename T>
  const T* GetSwapChain(ezGALSwapChainHandle hSwapChain) const
  {
    return static_cast<const T*>(GetSwapChainInternal(hSwapChain, ezGetStaticRTTI<T>()));
  }

  const ezGALShader* GetShader(ezGALShaderHandle hShader) const;
  const ezGALTexture* GetTexture(ezGALTextureHandle hTexture) const;
  virtual const ezGALSharedTexture* GetSharedTexture(ezGALTextureHandle hTexture) const = 0;
  const ezGALBuffer* GetBuffer(ezGALBufferHandle hBuffer) const;
  const ezGALDynamicBuffer* GetDynamicBuffer(ezGALDynamicBufferHandle hBuffer) const;
  ezGALDynamicBuffer* GetDynamicBuffer(ezGALDynamicBufferHandle hBuffer);
  const ezGALReadbackBuffer* GetReadbackBuffer(ezGALReadbackBufferHandle hBuffer) const;
  const ezGALReadbackTexture* GetReadbackTexture(ezGALReadbackTextureHandle hTexture) const;
  const ezGALDepthStencilState* GetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState) const;
  const ezGALBlendState* GetBlendState(ezGALBlendStateHandle hBlendState) const;
  const ezGALRasterizerState* GetRasterizerState(ezGALRasterizerStateHandle hRasterizerState) const;
  const ezGALVertexDeclaration* GetVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration) const;
  const ezGALSamplerState* GetSamplerState(ezGALSamplerStateHandle hSamplerState) const;
  const ezGALTextureResourceView* GetResourceView(ezGALTextureResourceViewHandle hResourceView) const;
  const ezGALBufferResourceView* GetResourceView(ezGALBufferResourceViewHandle hResourceView) const;
  const ezGALRenderTargetView* GetRenderTargetView(ezGALRenderTargetViewHandle hRenderTargetView) const;
  const ezGALTextureUnorderedAccessView* GetUnorderedAccessView(ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView) const;
  const ezGALBufferUnorderedAccessView* GetUnorderedAccessView(ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView) const;

  const ezGALDeviceCapabilities& GetCapabilities() const;

  virtual ezUInt64 GetMemoryConsumptionForTexture(const ezGALTextureCreationDescription& description) const;
  virtual ezUInt64 GetMemoryConsumptionForBuffer(const ezGALBufferCreationDescription& description) const;

  static void SetDefaultDevice(ezGALDevice* pDefaultDevice);
  static ezGALDevice* GetDefaultDevice();
  static bool HasDefaultDevice();

  // \brief Sends the queued up commands to the GPU.
  // Same as ezCommandEncoder:Flush.
  void Flush();

  /// \brief Waits for the GPU to be idle and destroys any pending resources and GPU objects.
  void WaitIdle();

  // public in case someone external needs to lock multiple operations
  mutable ezMutex m_Mutex;

  /// Internal: Returns the allocator used by the device.
  ezAllocator* GetAllocator();

private:
  static ezGALDevice* s_pDefaultDevice;

protected:
  ezGALDevice(const ezGALDeviceCreationDescription& Description);

  virtual ~ezGALDevice();

  template <typename IdTableType, typename ReturnType>
  ReturnType* Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const;

  void DestroyViews(ezGALTexture* pResource);
  void DestroyViews(ezGALBuffer* pResource);

  template <typename HandleType>
  void AddDeadObject(ezUInt32 uiType, HandleType handle);

  template <typename HandleType>
  void ReviveDeadObject(ezUInt32 uiType, HandleType handle);

  void DestroyDeadObjects();

  /// \brief Asserts that either this device supports multi-threaded resource creation, or that this function is executed on the main thread.
  void VerifyMultithreadedAccess() const;

  const ezGALSwapChain* GetSwapChainInternal(ezGALSwapChainHandle hSwapChain, const ezRTTI* pRequestedType) const;

  ezGALTextureHandle FinalizeTextureInternal(const ezGALTextureCreationDescription& desc, ezGALTexture* pTexture);
  ezGALBufferHandle FinalizeBufferInternal(const ezGALBufferCreationDescription& desc, ezGALBuffer* pBuffer);

  ezProxyAllocator m_Allocator;
  ezLocalAllocatorWrapper m_AllocatorWrapper;

  using ShaderTable = ezIdTable<ezGALShaderHandle::IdType, ezGALShader*, ezLocalAllocatorWrapper>;
  using BlendStateTable = ezIdTable<ezGALBlendStateHandle::IdType, ezGALBlendState*, ezLocalAllocatorWrapper>;
  using DepthStencilStateTable = ezIdTable<ezGALDepthStencilStateHandle::IdType, ezGALDepthStencilState*, ezLocalAllocatorWrapper>;
  using RasterizerStateTable = ezIdTable<ezGALRasterizerStateHandle::IdType, ezGALRasterizerState*, ezLocalAllocatorWrapper>;
  using BufferTable = ezIdTable<ezGALBufferHandle::IdType, ezGALBuffer*, ezLocalAllocatorWrapper>;
  using DynamicBufferTable = ezIdTable<ezGALDynamicBufferHandle::IdType, ezGALDynamicBuffer*, ezLocalAllocatorWrapper>;
  using TextureTable = ezIdTable<ezGALTextureHandle::IdType, ezGALTexture*, ezLocalAllocatorWrapper>;
  using ReadbackBufferTable = ezIdTable<ezGALReadbackBufferHandle::IdType, ezGALReadbackBuffer*, ezLocalAllocatorWrapper>;
  using ReadbackTextureTable = ezIdTable<ezGALReadbackTextureHandle::IdType, ezGALReadbackTexture*, ezLocalAllocatorWrapper>;
  using TextureResourceViewTable = ezIdTable<ezGALTextureResourceViewHandle::IdType, ezGALTextureResourceView*, ezLocalAllocatorWrapper>;
  using BufferResourceViewTable = ezIdTable<ezGALBufferResourceViewHandle::IdType, ezGALBufferResourceView*, ezLocalAllocatorWrapper>;
  using SamplerStateTable = ezIdTable<ezGALSamplerStateHandle::IdType, ezGALSamplerState*, ezLocalAllocatorWrapper>;
  using RenderTargetViewTable = ezIdTable<ezGALRenderTargetViewHandle::IdType, ezGALRenderTargetView*, ezLocalAllocatorWrapper>;
  using TextureUnorderedAccessViewTable = ezIdTable<ezGALTextureUnorderedAccessViewHandle::IdType, ezGALTextureUnorderedAccessView*, ezLocalAllocatorWrapper>;
  using BufferUnorderedAccessViewTable = ezIdTable<ezGALBufferUnorderedAccessViewHandle::IdType, ezGALBufferUnorderedAccessView*, ezLocalAllocatorWrapper>;
  using SwapChainTable = ezIdTable<ezGALSwapChainHandle::IdType, ezGALSwapChain*, ezLocalAllocatorWrapper>;
  using VertexDeclarationTable = ezIdTable<ezGALVertexDeclarationHandle::IdType, ezGALVertexDeclaration*, ezLocalAllocatorWrapper>;

  ShaderTable m_Shaders;
  BlendStateTable m_BlendStates;
  DepthStencilStateTable m_DepthStencilStates;
  RasterizerStateTable m_RasterizerStates;
  BufferTable m_Buffers;
  DynamicBufferTable m_DynamicBuffers;
  TextureTable m_Textures;
  ReadbackBufferTable m_ReadbackBuffers;
  ReadbackTextureTable m_ReadbackTextures;
  TextureResourceViewTable m_TextureResourceViews;
  BufferResourceViewTable m_BufferResourceViews;
  SamplerStateTable m_SamplerStates;
  RenderTargetViewTable m_RenderTargetViews;
  TextureUnorderedAccessViewTable m_TextureUnorderedAccessViews;
  BufferUnorderedAccessViewTable m_BufferUnorderedAccessViews;
  SwapChainTable m_SwapChains;
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

  ezGALDeviceCapabilities m_Capabilities;

  // Deactivate Doxygen document generation for the following block. (API abstraction only)
  /// \cond

  // These functions need to be implemented by a render API abstraction
protected:
  friend class ezMemoryUtils;
  friend class ezReadbackBufferLock;
  friend class ezReadbackTextureLock;

  // Init & shutdown functions

  virtual ezResult InitPlatform() = 0;
  virtual ezResult ShutdownPlatform() = 0;
  virtual ezStringView GetRendererPlatform() = 0;

  // Pipeline & Pass functions



  // Command Encoder

  virtual ezGALCommandEncoder* BeginCommandsPlatform(const char* szName) = 0;
  virtual void EndCommandsPlatform(ezGALCommandEncoder* pPass) = 0;

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

  virtual ezGALTexture* CreateSharedTexturePlatform(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData, ezEnum<ezGALSharedTextureType> sharedType, ezGALPlatformSharedHandle handle) = 0;
  virtual void DestroySharedTexturePlatform(ezGALTexture* pTexture) = 0;

  virtual ezGALReadbackBuffer* CreateReadbackBufferPlatform(const ezGALBufferCreationDescription& Description) = 0;
  virtual void DestroyReadbackBufferPlatform(ezGALReadbackBuffer* pReadbackBuffer) = 0;

  virtual ezGALReadbackTexture* CreateReadbackTexturePlatform(const ezGALTextureCreationDescription& Description) = 0;
  virtual void DestroyReadbackTexturePlatform(ezGALReadbackTexture* pReadbackTexture) = 0;

  virtual ezGALTextureResourceView* CreateResourceViewPlatform(ezGALTexture* pResource, const ezGALTextureResourceViewCreationDescription& Description) = 0;
  virtual void DestroyResourceViewPlatform(ezGALTextureResourceView* pResourceView) = 0;

  virtual ezGALBufferResourceView* CreateResourceViewPlatform(ezGALBuffer* pResource, const ezGALBufferResourceViewCreationDescription& Description) = 0;
  virtual void DestroyResourceViewPlatform(ezGALBufferResourceView* pResourceView) = 0;

  virtual ezGALRenderTargetView* CreateRenderTargetViewPlatform(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description) = 0;
  virtual void DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView) = 0;

  virtual ezGALTextureUnorderedAccessView* CreateUnorderedAccessViewPlatform(ezGALTexture* pResource, const ezGALTextureUnorderedAccessViewCreationDescription& Description) = 0;
  virtual void DestroyUnorderedAccessViewPlatform(ezGALTextureUnorderedAccessView* pUnorderedAccessView) = 0;

  virtual ezGALBufferUnorderedAccessView* CreateUnorderedAccessViewPlatform(ezGALBuffer* pResource, const ezGALBufferUnorderedAccessViewCreationDescription& Description) = 0;
  virtual void DestroyUnorderedAccessViewPlatform(ezGALBufferUnorderedAccessView* pUnorderedAccessView) = 0;

  // Other rendering creation functions

  virtual ezGALVertexDeclaration* CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description) = 0;
  virtual void DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) = 0;

  // Resource update functions

  virtual void UpdateBufferForNextFramePlatform(const ezGALBuffer* pBuffer, ezConstByteArrayPtr sourceData, ezUInt32 uiDestOffset) = 0;
  virtual void UpdateTextureForNextFramePlatform(const ezGALTexture* pTexture, const ezGALSystemMemoryDescription& sourceData, const ezGALTextureSubresource& destinationSubResource, const ezBoundingBoxu32& destinationBox) = 0;

  // GPU -> CPU query functions

  virtual ezEnum<ezGALAsyncResult> GetTimestampResultPlatform(ezGALTimestampHandle hTimestamp, ezTime& out_result) = 0;
  virtual ezEnum<ezGALAsyncResult> GetOcclusionResultPlatform(ezGALOcclusionHandle hOcclusion, ezUInt64& out_uiResult) = 0;
  virtual ezEnum<ezGALAsyncResult> GetFenceResultPlatform(ezGALFenceHandle hFence, ezTime timeout) = 0;
  virtual ezResult LockBufferPlatform(const ezGALReadbackBuffer* pBuffer, ezArrayPtr<const ezUInt8>& out_memory) const = 0;
  virtual void UnlockBufferPlatform(const ezGALReadbackBuffer* pBuffer) const = 0;
  virtual ezResult LockTexturePlatform(const ezGALReadbackTexture* pTexture, const ezArrayPtr<const ezGALTextureSubresource>& subResources, ezDynamicArray<ezGALSystemMemoryDescription>& out_memory) const = 0;
  virtual void UnlockTexturePlatform(const ezGALReadbackTexture* pTexture, const ezArrayPtr<const ezGALTextureSubresource>& subResources) const = 0;

  // Misc functions

  virtual void BeginFramePlatform(ezArrayPtr<ezGALSwapChain*> swapchains, const ezUInt64 uiAppFrame) = 0;
  virtual void EndFramePlatform(ezArrayPtr<ezGALSwapChain*> swapchains) = 0;

  virtual ezUInt64 GetCurrentFramePlatform() const = 0;
  virtual ezUInt64 GetSafeFramePlatform() const = 0;

  virtual void FillCapabilitiesPlatform() = 0;

  virtual void FlushPlatform() = 0;
  virtual void WaitIdlePlatform() = 0;


  /// \endcond

private:
  bool m_bBeginFrameCalled = false;
  ezHybridArray<ezGALSwapChain*, 8> m_FrameSwapChains;
  bool m_bBeginPipelineCalled = false;
  ezGALCommandEncoder* m_pCommandEncoder = nullptr;
};

#include <RendererFoundation/Device/Implementation/Device_inl.h>
