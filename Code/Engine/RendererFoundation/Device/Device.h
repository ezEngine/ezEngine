
#pragma once

#include <RendererFoundation/Basics.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/DeviceCapabilities.h>

class ezColor;

/// \brief The ezRenderDevice class is the primary interface for interactions with rendering APIs
/// It contains a set of (non-virtual) functions to set state, create resources etc. which rely on
/// API specific implementations provided by protected virtual functions.
/// Redundant state changes are prevented at the platform independent level in the non-virtual functions.
class EZ_RENDERERFOUNDATION_DLL ezGALDevice
{
public:

  // Init & shutdown functions

  ezResult Init();

  ezResult Shutdown();


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

  ezGALBufferHandle CreateBuffer(const ezGALBufferCreationDescription& Description, const void* pInitialData);

  void DestroyBuffer(ezGALBufferHandle hBuffer);


  // Helper functions for buffers (for common, simple use cases)
  ezGALBufferHandle CreateVertexBuffer(ezUInt32 uiVertexSize, ezUInt32 uiVertexCount, const void* pInitialData = nullptr);

  ezGALBufferHandle CreateIndexBuffer(ezGALIndexType::Enum IndexType, ezUInt32 uiIndexCount, const void* pInitialData = nullptr);

  ezGALBufferHandle CreateConstantBuffer(ezUInt32 uiBufferSize);


  ezGALTextureHandle CreateTexture(const ezGALTextureCreationDescription& Description, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData = nullptr);

  void DestroyTexture(ezGALTextureHandle hTexture);

  ezGALResourceViewHandle CreateResourceView(const ezGALResourceViewCreationDescription& Description);

  void DestroyResourceView(ezGALResourceViewHandle hResourceView);

  ezGALRenderTargetViewHandle CreateRenderTargetView(const ezGALRenderTargetViewCreationDescription& Description);

  void DestroyRenderTargetView(ezGALRenderTargetViewHandle hRenderTargetView);


  // Other rendering creation functions

  ezGALSwapChainHandle CreateSwapChain(const ezGALSwapChainCreationDescription& Description);

  void DestroySwapChain(ezGALSwapChainHandle hSwapChain);

  ezGALFenceHandle CreateFence();

  void DestroyFence(ezGALFenceHandle& hFence);

  ezGALQueryHandle CreateQuery(const ezGALQueryCreationDescription& Description);

  void DestroyQuery(ezGALQueryHandle hQuery);

  ezGALVertexDeclarationHandle CreateVertexDeclaration(const ezGALVertexDeclarationCreationDescription& Description);

  void DestroyVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration);



  // Get Query Data

  void GetQueryData(ezGALQueryHandle hQuery, ezUInt64* puiRendererdPixels);



  /// \todo Map functions to save on memcpys

  // Swap chain functions

  void Present(ezGALSwapChainHandle hSwapChain);

  ezGALTextureHandle GetBackBufferTextureFromSwapChain(ezGALSwapChainHandle hSwapChain);


  // Misc functions

  void BeginFrame();

  void EndFrame();

  void Flush();

  void Finish();

  void SetPrimarySwapChain(ezGALSwapChainHandle hSwapChain);

  ezGALSwapChainHandle GetPrimarySwapChain() const;

  ezGALContext* GetPrimaryContext() const;

  template<typename T>
  T* GetPrimaryContext() const;

  const ezGALDeviceCreationDescription* GetDescription() const;


  const ezGALSwapChain* GetSwapChain(ezGALSwapChainHandle hSwapChain) const;
  const ezGALShader* GetShader(ezGALShaderHandle hShader) const;
  const ezGALTexture* GetTexture(ezGALTextureHandle hTexture) const;
  const ezGALBuffer* GetBuffer(ezGALBufferHandle hBuffer) const;
  const ezGALDepthStencilState* GetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState) const;
  const ezGALBlendState* GetBlendState(ezGALBlendStateHandle hBlendState) const;
  const ezGALRasterizerState* GetRasterizerState(ezGALRasterizerStateHandle hRasterizerState) const;
  const ezGALVertexDeclaration* GetVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration) const;
  const ezGALQuery* GetQuery(ezGALQueryHandle hQuery) const;
  const ezGALSamplerState* GetSamplerState(ezGALSamplerStateHandle hSamplerState) const;
  const ezGALResourceView* GetResourceView(ezGALResourceViewHandle hResourceView) const;
  const ezGALRenderTargetView* GetRenderTargetView(ezGALRenderTargetViewHandle hRenderTargetView) const;

  const ezGALDeviceCapabilities& GetCapabilities() const;

  virtual ezUInt64 GetMemoryConsumptionForTexture(const ezGALTextureCreationDescription& Description) const;
  virtual ezUInt64 GetMemoryConsumptionForBuffer(const ezGALBufferCreationDescription& Description) const;

  static void SetDefaultDevice(ezGALDevice* pDefaultDevice);
  static ezGALDevice* GetDefaultDevice();

private:
  static ezGALDevice* s_pDefaultDevice;

protected:

  friend class ezGALContext;

  template<typename IdTableType, typename ReturnType> ReturnType* Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const;

  ezGALDevice(const ezGALDeviceCreationDescription& Description);

  virtual ~ezGALDevice();

  ezProxyAllocator m_Allocator;
  ezLocalAllocatorWrapper m_AllocatorWrapper;

  typedef ezIdTable<ezGALShaderHandle::IdType, ezGALShader*, ezLocalAllocatorWrapper> ShaderTable;

  typedef ezIdTable<ezGALBlendStateHandle::IdType, ezGALBlendState*, ezLocalAllocatorWrapper> BlendStateTable;

  typedef ezIdTable<ezGALDepthStencilStateHandle::IdType, ezGALDepthStencilState*, ezLocalAllocatorWrapper> DepthStencilStateTable;

  typedef ezIdTable<ezGALRasterizerStateHandle::IdType, ezGALRasterizerState*, ezLocalAllocatorWrapper> RasterizerStateTable;

  typedef ezIdTable<ezGALBufferHandle::IdType, ezGALBuffer*, ezLocalAllocatorWrapper> BufferTable;

  typedef ezIdTable<ezGALTextureHandle::IdType, ezGALTexture*, ezLocalAllocatorWrapper> TextureTable;

  typedef ezIdTable<ezGALResourceViewHandle::IdType, ezGALResourceView*, ezLocalAllocatorWrapper> ResourceViewTable;

  typedef ezIdTable<ezGALSamplerStateHandle::IdType, ezGALSamplerState*, ezLocalAllocatorWrapper> SamplerStateTable;

  typedef ezIdTable<ezGALRenderTargetViewHandle::IdType, ezGALRenderTargetView*, ezLocalAllocatorWrapper> RenderTargetViewTable;

  typedef ezIdTable<ezGALSwapChainHandle::IdType, ezGALSwapChain*, ezLocalAllocatorWrapper> SwapChainTable;

  typedef ezIdTable<ezGALFenceHandle::IdType, ezGALFence*, ezLocalAllocatorWrapper> FenceTable;

  typedef ezIdTable<ezGALQueryHandle::IdType, ezGALQuery*, ezLocalAllocatorWrapper> QueryTable;

  typedef ezIdTable<ezGALVertexDeclarationHandle::IdType, ezGALVertexDeclaration*, ezLocalAllocatorWrapper> VertexDeclarationTable;

  ShaderTable m_Shaders;

  BlendStateTable m_BlendStates;

  DepthStencilStateTable m_DepthStencilStates;

  RasterizerStateTable m_RasterizerStates;

  BufferTable m_Buffers;

  TextureTable m_Textures;

  ResourceViewTable m_ResourceViews;

  SamplerStateTable m_SamplerStates;

  RenderTargetViewTable m_RenderTargetViews;

  SwapChainTable m_SwapChains;

  FenceTable m_Fences;

  QueryTable m_Queries;

  VertexDeclarationTable m_VertexDeclarations;


  // Hash maps used to prevent state object duplication
  ezMap<ezUInt32, ezGALBlendStateHandle, ezCompareHelper<ezUInt32>, ezLocalAllocatorWrapper> m_BlendStateMap;
  ezMap<ezUInt32, ezGALDepthStencilStateHandle, ezCompareHelper<ezUInt32>, ezLocalAllocatorWrapper> m_DepthStencilStateMap;
  ezMap<ezUInt32, ezGALRasterizerStateHandle, ezCompareHelper<ezUInt32>, ezLocalAllocatorWrapper> m_RasterizerStateMap;
  ezMap<ezUInt32, ezGALResourceViewHandle, ezCompareHelper<ezUInt32>, ezLocalAllocatorWrapper> m_ResourceViewMap;
  ezMap<ezUInt32, ezGALSamplerStateHandle, ezCompareHelper<ezUInt32>, ezLocalAllocatorWrapper> m_SamplerStateMap;
  ezMap<ezUInt32, ezGALRenderTargetViewHandle, ezCompareHelper<ezUInt32>, ezLocalAllocatorWrapper> m_RenderTargetViewMap;
  ezMap<ezUInt32, ezGALVertexDeclarationHandle, ezCompareHelper<ezUInt32>, ezLocalAllocatorWrapper> m_VertexDeclarationMap;


  ezGALDeviceCreationDescription m_Description;

  ezGALSwapChainHandle m_hPrimarySwapChain;

  ezGALContext* m_pPrimaryContext;

  ezGALDeviceCapabilities m_Capabilities;

  // Deactivate Doxygen document generation for the following block. (API abstraction only)
  /// \cond

// These functions need to be implemented by a render API abstraction
protected:

  friend class ezMemoryUtils;

  // Init & shutdown functions

  virtual ezResult InitPlatform() = 0;

  virtual ezResult ShutdownPlatform() = 0;


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

  virtual ezGALBuffer* CreateBufferPlatform(const ezGALBufferCreationDescription& Description, const void* pInitialData) = 0;

  virtual void DestroyBufferPlatform(ezGALBuffer* pBuffer) = 0;

  virtual ezGALTexture* CreateTexturePlatform(const ezGALTextureCreationDescription& Description, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData) = 0;

  virtual void DestroyTexturePlatform(ezGALTexture* pTexture) = 0;

  virtual ezGALResourceView* CreateResourceViewPlatform(const ezGALResourceViewCreationDescription& Description) = 0;

  virtual void DestroyResourceViewPlatform(ezGALResourceView* pResourceView) = 0;

  virtual ezGALRenderTargetView* CreateRenderTargetViewPlatform(const ezGALRenderTargetViewCreationDescription& Description) = 0;

  virtual void DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView) = 0;


  // Other rendering creation functions

  virtual ezGALSwapChain* CreateSwapChainPlatform(const ezGALSwapChainCreationDescription& Description) = 0;

  virtual void DestroySwapChainPlatform(ezGALSwapChain* pSwapChain) = 0;

  virtual ezGALFence* CreateFencePlatform() = 0;

  virtual void DestroyFencePlatform(ezGALFence* pFence) = 0;

  virtual ezGALQuery* CreateQueryPlatform(const ezGALQueryCreationDescription& Description) = 0;

  virtual void DestroyQueryPlatform(ezGALQuery* pQuery) = 0;

  virtual ezGALVertexDeclaration* CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description) = 0;

  virtual void DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) = 0;
  
  


  // Get Query Data

  virtual void GetQueryDataPlatform(ezGALQuery* pQuery, ezUInt64* puiRendererdPixels) = 0;





  // Swap chain functions

  virtual void PresentPlatform(ezGALSwapChain* pSwapChain) = 0;

  // Misc functions

  virtual void BeginFramePlatform() = 0;

  virtual void EndFramePlatform() = 0;

  virtual void FlushPlatform() = 0;

  virtual void FinishPlatform() = 0;

  virtual void SetPrimarySwapChainPlatform(ezGALSwapChain* pSwapChain) = 0;

  virtual void FillCapabilitiesPlatform() = 0;

  /// \endcond

private:
  bool m_bFrameBeginCalled;
};

#include <RendererFoundation/Device/Implementation/Device_inl.h>

