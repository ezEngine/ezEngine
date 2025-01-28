
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/Device/Device.h>

// TODO: This should not be included in a header, it exposes Windows.h to the outside
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <dxgi.h>

struct ID3D11Device;
struct ID3D11Device3;
struct ID3D11DeviceContext;
struct ID3D11Debug;
struct IDXGIFactory1;
struct IDXGIAdapter1;
struct IDXGIDevice1;
struct ID3D11Resource;
struct ID3D11Query;
struct IDXGIAdapter;

using ezGALFormatLookupEntryDX11 = ezGALFormatLookupEntry<DXGI_FORMAT, (DXGI_FORMAT)0>;
using ezGALFormatLookupTableDX11 = ezGALFormatLookupTable<ezGALFormatLookupEntryDX11>;

class ezFenceQueueDX11;
class ezQueryPoolDX11;

/// \brief The DX11 device implementation of the graphics abstraction layer.
class EZ_RENDERERDX11_DLL ezGALDeviceDX11 : public ezGALDevice
{
private:
  friend ezInternal::NewInstance<ezGALDevice> CreateDX11Device(ezAllocator* pAllocator, const ezGALDeviceCreationDescription& description);
  ezGALDeviceDX11(const ezGALDeviceCreationDescription& Description);

public:
  virtual ~ezGALDeviceDX11();

public:
  ID3D11Device* GetDXDevice() const;
  ID3D11Device3* GetDXDevice3() const;
  ID3D11DeviceContext* GetDXImmediateContext() const;
  IDXGIFactory1* GetDXGIFactory() const;
  ezGALCommandEncoder* GetCommandEncoder() const;

  ezFenceQueueDX11& GetFenceQueue() const;
  ezQueryPoolDX11& GetQueryPool() const;

  const ezGALFormatLookupTableDX11& GetFormatLookupTable() const;

  void ReportLiveGpuObjects();

  void FlushDeadObjects();

  // These functions need to be implemented by a render API abstraction
protected:
  // Init & shutdown functions

  /// \brief Internal version of device init that allows to modify device creation flags and graphics adapter.
  ///
  /// \param pUsedAdapter
  ///   Null means default adapter.
  ezResult InitPlatform(DWORD flags, IDXGIAdapter* pUsedAdapter);

  virtual ezStringView GetRendererPlatform() override;
  virtual ezResult InitPlatform() override;
  virtual ezResult ShutdownPlatform() override;

  // Command encoder functions

  virtual ezGALCommandEncoder* BeginCommandsPlatform(const char* szName) override;
  virtual void EndCommandsPlatform(ezGALCommandEncoder* pPass) override;

  virtual void FlushPlatform() override;


  // State creation functions

  virtual ezGALBlendState* CreateBlendStatePlatform(const ezGALBlendStateCreationDescription& Description) override;
  virtual void DestroyBlendStatePlatform(ezGALBlendState* pBlendState) override;

  virtual ezGALDepthStencilState* CreateDepthStencilStatePlatform(const ezGALDepthStencilStateCreationDescription& Description) override;
  virtual void DestroyDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState) override;

  virtual ezGALRasterizerState* CreateRasterizerStatePlatform(const ezGALRasterizerStateCreationDescription& Description) override;
  virtual void DestroyRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState) override;

  virtual ezGALSamplerState* CreateSamplerStatePlatform(const ezGALSamplerStateCreationDescription& Description) override;
  virtual void DestroySamplerStatePlatform(ezGALSamplerState* pSamplerState) override;


  // Resource creation functions

  virtual ezGALShader* CreateShaderPlatform(const ezGALShaderCreationDescription& Description) override;
  virtual void DestroyShaderPlatform(ezGALShader* pShader) override;

  virtual ezGALBuffer* CreateBufferPlatform(const ezGALBufferCreationDescription& Description, ezArrayPtr<const ezUInt8> pInitialData) override;
  virtual void DestroyBufferPlatform(ezGALBuffer* pBuffer) override;

  virtual ezGALTexture* CreateTexturePlatform(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual void DestroyTexturePlatform(ezGALTexture* pTexture) override;

  virtual ezGALTexture* CreateSharedTexturePlatform(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData, ezEnum<ezGALSharedTextureType> sharedType, ezGALPlatformSharedHandle handle) override;
  virtual void DestroySharedTexturePlatform(ezGALTexture* pTexture) override;

  virtual ezGALReadbackBuffer* CreateReadbackBufferPlatform(const ezGALBufferCreationDescription& Description) override;
  virtual void DestroyReadbackBufferPlatform(ezGALReadbackBuffer* pReadbackBuffer) override;

  virtual ezGALReadbackTexture* CreateReadbackTexturePlatform(const ezGALTextureCreationDescription& Description) override;
  virtual void DestroyReadbackTexturePlatform(ezGALReadbackTexture* pReadbackTexture) override;

  virtual ezGALTextureResourceView* CreateResourceViewPlatform(ezGALTexture* pResource, const ezGALTextureResourceViewCreationDescription& Description) override;
  virtual void DestroyResourceViewPlatform(ezGALTextureResourceView* pResourceView) override;

  virtual ezGALBufferResourceView* CreateResourceViewPlatform(ezGALBuffer* pResource, const ezGALBufferResourceViewCreationDescription& Description) override;
  virtual void DestroyResourceViewPlatform(ezGALBufferResourceView* pResourceView) override;

  virtual ezGALRenderTargetView* CreateRenderTargetViewPlatform(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description) override;
  virtual void DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView) override;

  ezGALTextureUnorderedAccessView* CreateUnorderedAccessViewPlatform(ezGALTexture* pResource, const ezGALTextureUnorderedAccessViewCreationDescription& Description) override;
  virtual void DestroyUnorderedAccessViewPlatform(ezGALTextureUnorderedAccessView* pUnorderedAccessView) override;

  ezGALBufferUnorderedAccessView* CreateUnorderedAccessViewPlatform(ezGALBuffer* pResource, const ezGALBufferUnorderedAccessViewCreationDescription& Description) override;
  virtual void DestroyUnorderedAccessViewPlatform(ezGALBufferUnorderedAccessView* pUnorderedAccessView) override;

  // Other rendering creation functions

  virtual ezGALVertexDeclaration* CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description) override;
  virtual void DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) override;

  // Resource update functions

  virtual void UpdateBufferForNextFramePlatform(const ezGALBuffer* pBuffer, ezConstByteArrayPtr sourceData, ezUInt32 uiDestOffset) override;
  virtual void UpdateTextureForNextFramePlatform(const ezGALTexture* pTexture, const ezGALSystemMemoryDescription& sourceData, const ezGALTextureSubresource& destinationSubResource, const ezBoundingBoxu32& destinationBox) override;

  // GPU -> CPU query functions

  virtual ezEnum<ezGALAsyncResult> GetTimestampResultPlatform(ezGALTimestampHandle hTimestamp, ezTime& out_result) override;
  virtual ezEnum<ezGALAsyncResult> GetOcclusionResultPlatform(ezGALOcclusionHandle hOcclusion, ezUInt64& out_uiResult) override;
  virtual ezEnum<ezGALAsyncResult> GetFenceResultPlatform(ezGALFenceHandle hFence, ezTime timeout) override;
  virtual ezResult LockBufferPlatform(const ezGALReadbackBuffer* pBuffer, ezArrayPtr<const ezUInt8>& out_Memory) const override;
  virtual void UnlockBufferPlatform(const ezGALReadbackBuffer* pBuffer) const override;
  virtual ezResult LockTexturePlatform(const ezGALReadbackTexture* pTexture, const ezArrayPtr<const ezGALTextureSubresource>& subResources, ezDynamicArray<ezGALSystemMemoryDescription>& out_Memory) const override;
  virtual void UnlockTexturePlatform(const ezGALReadbackTexture* pTexture, const ezArrayPtr<const ezGALTextureSubresource>& subResources) const override;
  // Swap chain functions

  void PresentPlatform(const ezGALSwapChain* pSwapChain, bool bVSync);

  // Misc functions

  virtual void BeginFramePlatform(ezArrayPtr<ezGALSwapChain*> swapchains, const ezUInt64 uiAppFrame) override;
  virtual void EndFramePlatform(ezArrayPtr<ezGALSwapChain*> swapchains) override;
  virtual ezUInt64 GetCurrentFramePlatform() const override;
  virtual ezUInt64 GetSafeFramePlatform() const override;

  virtual void FillCapabilitiesPlatform() override;

  virtual void WaitIdlePlatform() override;

  virtual const ezGALSharedTexture* GetSharedTexture(ezGALTextureHandle hTexture) const override;

  /// \endcond

private:
  friend class ezGALCommandEncoderImplDX11;

  struct TempResourceType
  {
    enum Enum
    {
      Buffer,
      Texture,

      ENUM_COUNT
    };
  };

  struct TempResource
  {
    ID3D11Resource* m_pResource = nullptr;
    void* m_pData = nullptr;
    ezUInt32 m_uiRowPitch = 0;
    ezUInt32 m_uiDepthPitch = 0;

    operator bool() const
    {
      return m_pResource != nullptr;
    }
  };

  TempResource CopyToTempBuffer(ezConstByteArrayPtr sourceData, ezUInt64 uiLastUseFrame = ezUInt64(-1));
  TempResource CopyToTempTexture(const ezGALSystemMemoryDescription& sourceData, ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiDepth, ezGALResourceFormat::Enum format, ezUInt64 uiLastUseFrame = ezUInt64(-1));
  void UnmapTempResource(TempResource& tempResource);
  void FreeTempResources(ezUInt64 uiFrame);

  void ProcessPendingCopies();

  void FillFormatLookupTable();

  static constexpr ezUInt32 FRAMES = 4;

  ID3D11Device* m_pDevice = nullptr;
  ID3D11Device3* m_pDevice3 = nullptr;
  ID3D11DeviceContext* m_pImmediateContext;
  ID3D11Debug* m_pDebug = nullptr;
  IDXGIFactory1* m_pDXGIFactory = nullptr;
  IDXGIAdapter1* m_pDXGIAdapter = nullptr;
  IDXGIDevice1* m_pDXGIDevice = nullptr;

  ezUniquePtr<ezFenceQueueDX11> m_pFenceQueue;
  ezUniquePtr<ezQueryPoolDX11> m_pQueryPool;
  ezGALFormatLookupTableDX11 m_FormatLookupTable;

  // NOLINTNEXTLINE
  ezUInt32 m_uiFeatureLevel; // D3D_FEATURE_LEVEL can't be forward declared

  ezUniquePtr<ezGALCommandEncoderImplDX11> m_pCommandEncoderImpl;
  ezUniquePtr<ezGALCommandEncoder> m_pCommandEncoder;

  struct PerFrameData
  {
    ezGALFenceHandle m_hFence = {};
    ezUInt64 m_uiFrame = ezUInt64(-1);
  };

  PerFrameData m_PerFrameData[FRAMES];

  ezUInt64 m_uiFrameCounter = 1;
  ezUInt64 m_uiSafeFrame = 0;
  ezUInt8 m_uiCurrentPerFrameData = m_uiFrameCounter % FRAMES;

  struct UsedTempResource
  {
    EZ_DECLARE_POD_TYPE();

    ID3D11Resource* m_pResource;
    ezUInt64 m_uiFrame;
    ezUInt32 m_uiHash;
  };

  ezMap<ezUInt32, ezDynamicArray<TempResource>, ezCompareHelper<ezUInt32>, ezLocalAllocatorWrapper> m_FreeTempResources[TempResourceType::ENUM_COUNT];
  ezDeque<UsedTempResource, ezLocalAllocatorWrapper> m_UsedTempResources[TempResourceType::ENUM_COUNT];

  struct PendingCopy
  {
    TempResource m_SourceResource = {};
    ID3D11Resource* m_pDestResource = nullptr;
    ezUInt32 m_uiDestSubResource = 0;
    ezVec3U32 m_vDestPoint = ezVec3U32::MakeZero();
    ezVec3U32 m_vSourceSize = ezVec3U32::MakeZero();
  };

  ezDynamicArray<PendingCopy, ezLocalAllocatorWrapper> m_PendingCopies;

  struct GPUTimingScope* m_pFrameTimingScope = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope = nullptr;
};

#include <RendererDX11/Device/Implementation/DeviceDX11_inl.h>
