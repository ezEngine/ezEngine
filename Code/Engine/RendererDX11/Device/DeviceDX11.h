
#pragma once

#include <RendererDX11/Basics.h>
#include <RendererFoundation/Device/Device.h>
#include <Foundation/Types/Bitflags.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGIFactory1;
struct IDXGIAdapter1;
struct IDXGIDevice1;
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

  EZ_FORCE_INLINE ID3D11Device* GetDXDevice() const;

  EZ_FORCE_INLINE IDXGIFactory1* GetDXGIFactory() const;

  EZ_FORCE_INLINE const ezGALFormatLookupTableDX11& GetFormatLookupTable() const;

// These functions need to be implemented by a render API abstraction
protected:

  // Init & shutdown functions

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

  virtual ezGALBuffer* CreateBufferPlatform(const ezGALBufferCreationDescription& Description, const void* pInitialData) override;

  virtual void DestroyBufferPlatform(ezGALBuffer* pBuffer) override;

  virtual ezGALTexture* CreateTexturePlatform(const ezGALTextureCreationDescription& Description, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData) override;

  virtual void DestroyTexturePlatform(ezGALTexture* pTexture) override;

  virtual ezGALResourceView* CreateResourceViewPlatform(const ezGALResourceViewCreationDescription& Description) override;

  virtual void DestroyResourceViewPlatform(ezGALResourceView* pResourceView) override;

  virtual ezGALRenderTargetView* CreateRenderTargetViewPlatform(const ezGALRenderTargetViewCreationDescription& Description) override;

  virtual void DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView) override;


  // Other rendering creation functions

  virtual ezGALSwapChain* CreateSwapChainPlatform(const ezGALSwapChainCreationDescription& Description) override;

  virtual void DestroySwapChainPlatform(ezGALSwapChain* pSwapChain) override;

  virtual ezGALFence* CreateFencePlatform() override;

  virtual void DestroyFencePlatform(ezGALFence* pFence) override;

  virtual ezGALQuery* CreateQueryPlatform(const ezGALQueryCreationDescription& Description) override;

  virtual void DestroyQueryPlatform(ezGALQuery* pQuery) override;

  virtual ezGALVertexDeclaration* CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description) override;

  virtual void DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) override;
  
  
  // Get Query Data

  virtual void GetQueryDataPlatform(ezGALQuery* pQuery, ezUInt64* puiRendererdPixels) override;




  // Swap chain functions

  virtual void PresentPlatform(ezGALSwapChain* pSwapChain) override;

  // Misc functions

  virtual void BeginFramePlatform() override;

  virtual void EndFramePlatform() override;

  virtual void FlushPlatform() override;

  virtual void FinishPlatform() override;

  virtual void SetPrimarySwapChainPlatform(ezGALSwapChain* pSwapChain) override;

  virtual void FillCapabilitiesPlatform() override;

  /// \endcond

private:

  void FillFormatLookupTable();

  ID3D11Device* m_pDevice;

  IDXGIFactory1* m_pDXGIFactory;

  IDXGIAdapter1* m_pDXGIAdapter;

  IDXGIDevice1* m_pDXGIDevice;

  ezGALFormatLookupTableDX11 m_FormatLookupTable;

  D3D_FEATURE_LEVEL m_FeatureLevel;
};

#include <RendererDX11/Device/Implementation/DeviceDX11_inl.h>
