
#pragma once

#include <RendererDX11/Basics.h>
#include <RendererFoundation/Device/Device.h>
#include <Foundation/Types/Bitflags.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Debug;
struct IDXGIFactory1;
struct IDXGIAdapter1;
struct IDXGIDevice1;
struct ID3D11Resource;
struct ID3D11Query;
struct IDXGIAdapter;
enum DXGI_FORMAT;
enum D3D_FEATURE_LEVEL;

typedef ezGALFormatLookupEntry<DXGI_FORMAT, (DXGI_FORMAT)0> ezGALFormatLookupEntryDX11;
typedef ezGALFormatLookupTable<ezGALFormatLookupEntryDX11> ezGALFormatLookupTableDX11;


/// \brief The DX11 device implementation of the graphics abstraction layer.
class EZ_RENDERERDX11_DLL ezGALDeviceDX11 : public ezGALDevice
{

  /// \todo This shouldn't be accessible, there should be a factory instantiating the correct renderer class via RTTI for example
public:

  ezGALDeviceDX11(const ezGALDeviceCreationDescription& Description);

  virtual ~ezGALDeviceDX11();


public:

  EZ_ALWAYS_INLINE ID3D11Device* GetDXDevice() const;

  EZ_ALWAYS_INLINE IDXGIFactory1* GetDXGIFactory() const;

  EZ_ALWAYS_INLINE const ezGALFormatLookupTableDX11& GetFormatLookupTable() const;

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



  // Swap chain functions

  virtual void PresentPlatform(ezGALSwapChain* pSwapChain, bool bVSync) override;

  // Misc functions

  virtual void BeginFramePlatform() override;

  virtual void EndFramePlatform() override;

  virtual void SetPrimarySwapChainPlatform(ezGALSwapChain* pSwapChain) override;

  virtual void FillCapabilitiesPlatform() override;

  virtual ezUInt64 GetTimestampTicksPerSecondPlatform() override;

  /// \endcond

private:

  friend class ezGALContextDX11;

  struct TempResourceType
  {
    enum Enum
    {
      Buffer,
      Texture,

      ENUM_COUNT
    };
  };

  ID3D11Resource* FindTempBuffer(ezUInt32 uiSize);
  ID3D11Resource* FindTempTexture(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiDepth, ezGALResourceFormat::Enum format);
  void FreeTempResources(ezUInt64 uiFrame);

  void FillFormatLookupTable();

  ID3D11Device* m_pDevice;

  ID3D11Debug* m_pDebug;

  IDXGIFactory1* m_pDXGIFactory;

  IDXGIAdapter1* m_pDXGIAdapter;

  IDXGIDevice1* m_pDXGIDevice;

  ezGALFormatLookupTableDX11 m_FormatLookupTable;

  D3D_FEATURE_LEVEL m_FeatureLevel;

  struct EndFrameFence
  {
    EndFrameFence() : m_pFence(nullptr), m_uiFrame(0xFFFFFFFFFFu)
    {}

    ezGALFence* m_pFence;
    ezUInt64 m_uiFrame;
  };

  EndFrameFence m_EndFrameFences[4];
  ezUInt8 m_uiCurrentEndFrameFence;
  ezUInt8 m_uiNextEndFrameFence;

  ezUInt64 m_uiFrameCounter;

  struct UsedTempResource
  {
    EZ_DECLARE_POD_TYPE();

    ID3D11Resource* m_pResource;
    ezUInt64 m_uiFrame;
    ezUInt32 m_uiHash;
  };

  ezMap<ezUInt32, ezDynamicArray<ID3D11Resource*>, ezCompareHelper<ezUInt32>, ezLocalAllocatorWrapper> m_FreeTempResources[TempResourceType::ENUM_COUNT];
  ezDeque<UsedTempResource, ezLocalAllocatorWrapper> m_UsedTempResources[TempResourceType::ENUM_COUNT];

  // Utils for getting timer frequency.
  static const ezUInt32 m_uiNumDisjointTimerQueries = 3;
  ezUInt32 m_uiRunDisjointTimerQuery;
  ID3D11Query* m_pDisjointTimerQueries[m_uiNumDisjointTimerQueries];
  ezUInt64 m_uiLastTimerTicksPerSecond;
  ezUInt32 m_uiNextDisjointTimerQueryToRun;
  bool m_bStartedDisjointQuery;
};

#include <RendererDX11/Device/Implementation/DeviceDX11_inl.h>
