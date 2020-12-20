
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/Device/Device.h>

// TODO: This should not be included in a header, it exposes Windows.h to the outside
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
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

typedef ezGALFormatLookupEntry<DXGI_FORMAT, (DXGI_FORMAT)0> ezGALFormatLookupEntryDX11;
typedef ezGALFormatLookupTable<ezGALFormatLookupEntryDX11> ezGALFormatLookupTableDX11;

class ezGALPassDX11;

/// \brief The DX11 device implementation of the graphics abstraction layer.
class EZ_RENDERERDX11_DLL ezGALDeviceDX11 : public ezGALDevice
{
private:
  friend ezGALDevice* CreateDX11Device(ezAllocatorBase* pAllocator, const ezGALDeviceCreationDescription& Description);
  ezGALDeviceDX11(const ezGALDeviceCreationDescription& Description);

public:
  virtual ~ezGALDeviceDX11();

public:
  ID3D11Device* GetDXDevice() const;
  ID3D11Device3* GetDXDevice3() const;
  ID3D11DeviceContext* GetDXImmediateContext() const;

  IDXGIFactory1* GetDXGIFactory() const;

  const ezGALFormatLookupTableDX11& GetFormatLookupTable() const;

  void ReportLiveGpuObjects();

  // These functions need to be implemented by a render API abstraction
protected:
  // Init & shutdown functions

  /// \brief Internal version of device init that allows to modify device creation flags and graphics adapter.
  ///
  /// \param pUsedAdapter
  ///   Null means default adapter.
  ezResult InitPlatform(DWORD flags, IDXGIAdapter* pUsedAdapter);

  virtual ezResult InitPlatform() override;
  virtual ezResult ShutdownPlatform() override;

  // Pass functions

  virtual ezGALPass* BeginPassPlatform(const char* szName) override;
  virtual void EndPassPlatform(ezGALPass* pPass) override;


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

  virtual ezGALResourceView* CreateResourceViewPlatform(ezGALResourceBase* pResource, const ezGALResourceViewCreationDescription& Description) override;
  virtual void DestroyResourceViewPlatform(ezGALResourceView* pResourceView) override;

  virtual ezGALRenderTargetView* CreateRenderTargetViewPlatform(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description) override;
  virtual void DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView) override;

  ezGALUnorderedAccessView* CreateUnorderedAccessViewPlatform(ezGALResourceBase* pResource, const ezGALUnorderedAccessViewCreationDescription& Description) override;
  virtual void DestroyUnorderedAccessViewPlatform(ezGALUnorderedAccessView* pResource) override;

  // Other rendering creation functions

  virtual ezGALSwapChain* CreateSwapChainPlatform(const ezGALSwapChainCreationDescription& Description) override;
  virtual void DestroySwapChainPlatform(ezGALSwapChain* pSwapChain) override;

  virtual ezGALFence* CreateFencePlatform() override;
  virtual void DestroyFencePlatform(ezGALFence* pFence) override;

  virtual ezGALQuery* CreateQueryPlatform(const ezGALQueryCreationDescription& Description) override;
  virtual void DestroyQueryPlatform(ezGALQuery* pQuery) override;

  virtual ezGALVertexDeclaration* CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description) override;
  virtual void DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) override;

  // Timestamp functions

  virtual ezGALTimestampHandle GetTimestampPlatform() override;
  virtual ezResult GetTimestampResultPlatform(ezGALTimestampHandle hTimestamp, ezTime& result) override;

  // Swap chain functions

  virtual void PresentPlatform(ezGALSwapChain* pSwapChain, bool bVSync) override;

  // Misc functions

  virtual void BeginFramePlatform() override;
  virtual void EndFramePlatform() override;

  virtual void SetPrimarySwapChainPlatform(ezGALSwapChain* pSwapChain) override;

  virtual void FillCapabilitiesPlatform() override;

  /// \endcond

public:
  ID3D11Resource* FindTempBuffer(ezUInt32 uiSize);
  ID3D11Resource* FindTempTexture(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiDepth, ezGALResourceFormat::Enum format);

  ID3D11Query* GetTimestamp(ezGALTimestampHandle hTimestamp);

private:
  friend class ezGALRenderCommandEncoderDX11;

  struct TempResourceType
  {
    enum Enum
    {
      Buffer,
      Texture,

      ENUM_COUNT
    };
  };

  void FreeTempResources(ezUInt64 uiFrame);

  void FillFormatLookupTable();

  ID3D11Device* m_pDevice;
  ID3D11Device3* m_pDevice3;
  ID3D11DeviceContext* m_pImmediateContext;

  ID3D11Debug* m_pDebug;

  IDXGIFactory1* m_pDXGIFactory;

  IDXGIAdapter1* m_pDXGIAdapter;

  IDXGIDevice1* m_pDXGIDevice;

  ezGALFormatLookupTableDX11 m_FormatLookupTable;

  ezUInt32 m_FeatureLevel; // D3D_FEATURE_LEVEL can't be forward declared

  ezUniquePtr<ezGALPassDX11> m_pDefaultPass;

  struct PerFrameData
  {
    ezGALFence* m_pFence = nullptr;
    ID3D11Query* m_pDisjointTimerQuery = nullptr;
    double m_fInvTicksPerSecond = -1.0;
    ezUInt64 m_uiFrame = -1;
  };

  PerFrameData m_PerFrameData[4];
  ezUInt8 m_uiCurrentPerFrameData = 0;
  ezUInt8 m_uiNextPerFrameData = 0;

  ezUInt64 m_uiFrameCounter = 0;

  struct UsedTempResource
  {
    EZ_DECLARE_POD_TYPE();

    ID3D11Resource* m_pResource;
    ezUInt64 m_uiFrame;
    ezUInt32 m_uiHash;
  };

  ezMap<ezUInt32, ezDynamicArray<ID3D11Resource*>, ezCompareHelper<ezUInt32>, ezLocalAllocatorWrapper> m_FreeTempResources[TempResourceType::ENUM_COUNT];
  ezDeque<UsedTempResource, ezLocalAllocatorWrapper> m_UsedTempResources[TempResourceType::ENUM_COUNT];

  ezDynamicArray<ID3D11Query*, ezLocalAllocatorWrapper> m_Timestamps;
  ezUInt32 m_uiCurrentTimestamp = 0;
  ezUInt32 m_uiNextTimestamp = 0;

  ezTime m_SyncTimeDiff;
  bool m_bSyncTimeNeeded = true;
};

#include <RendererDX11/Device/Implementation/DeviceDX11_inl.h>
