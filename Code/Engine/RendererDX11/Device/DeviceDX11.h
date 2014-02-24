
#pragma once

#include <RendererDX11/Basics.h>
#include <RendererFoundation/Device/Device.h>
#include <Foundation/Basics/Types/Bitflags.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGIFactory1;
struct IDXGIAdapter1;
struct IDXGIDevice1;
enum DXGI_FORMAT;

typedef ezGALFormatLookupEntry<DXGI_FORMAT, (DXGI_FORMAT)0> ezGALFormatLookupEntryDX11;
typedef ezGALFormatLookupTable<ezGALFormatLookupEntryDX11> ezGALFormatLookupTableDX11;


/// \brief The DX11 device implementation of the graphics abstraction layer.
class EZ_RENDERERDX11_DLL ezGALDeviceDX11 : public ezGALDevice
{

  // TODO: This shouldn't be accessible, there should be a factory instantiating the correct renderer class via RTTI for example
public:

  ezGALDeviceDX11(const ezGALDeviceCreationDescription& Description);

  virtual ~ezGALDeviceDX11();


public:

  EZ_FORCE_INLINE ID3D11Device* GetDXDevice() const;

  EZ_FORCE_INLINE const ezGALFormatLookupTableDX11& GetFormatLookupTable() const;

// These functions need to be implemented by a render API abstraction
protected:

  // Init & shutdown functions

  virtual ezResult InitPlatform() EZ_OVERRIDE;

  virtual ezResult ShutdownPlatform() EZ_OVERRIDE;


  // State creation functions

  virtual ezGALBlendState* CreateBlendStatePlatform(const ezGALBlendStateCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyBlendStatePlatform(ezGALBlendState* pBlendState) EZ_OVERRIDE;

  virtual ezGALDepthStencilState* CreateDepthStencilStatePlatform(const ezGALDepthStencilStateCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState) EZ_OVERRIDE;

  virtual ezGALRasterizerState* CreateRasterizerStatePlatform(const ezGALRasterizerStateCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState) EZ_OVERRIDE;

  virtual ezGALSamplerState* CreateSamplerStatePlatform(const ezGALSamplerStateCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroySamplerStatePlatform(ezGALSamplerState* pSamplerState) EZ_OVERRIDE;


  // Resource creation functions

  virtual ezGALShader* CreateShaderPlatform(const ezGALShaderCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyShaderPlatform(ezGALShader* pShader) EZ_OVERRIDE;

  virtual ezGALBuffer* CreateBufferPlatform(const ezGALBufferCreationDescription& Description, const void* pInitialData) EZ_OVERRIDE;

  virtual void DestroyBufferPlatform(ezGALBuffer* pBuffer) EZ_OVERRIDE;

  virtual ezGALTexture* CreateTexturePlatform(const ezGALTextureCreationDescription& Description, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData) EZ_OVERRIDE;

  virtual void DestroyTexturePlatform(ezGALTexture* pTexture) EZ_OVERRIDE;

  virtual ezGALResourceView* CreateResourceViewPlatform(const ezGALResourceViewCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyResourceViewPlatform(ezGALResourceView* pResourceView) EZ_OVERRIDE;

  virtual ezGALRenderTargetView* CreateRenderTargetViewPlatform(const ezGALRenderTargetViewCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView) EZ_OVERRIDE;


  // Other rendering creation functions

  virtual ezGALSwapChain* CreateSwapChainPlatform(const ezGALSwapChainCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroySwapChainPlatform(ezGALSwapChain* pSwapChain) EZ_OVERRIDE;

  virtual ezGALFence* CreateFencePlatform() EZ_OVERRIDE;

  virtual void DestroyFencePlatform(ezGALFence* pFence) EZ_OVERRIDE;

  virtual ezGALQuery* CreateQueryPlatform(const ezGALQueryCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyQueryPlatform(ezGALQuery* pQuery) EZ_OVERRIDE;

  virtual ezGALRenderTargetConfig* CreateRenderTargetConfigPlatform(const ezGALRenderTargetConfigCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyRenderTargetConfigPlatform(ezGALRenderTargetConfig* pRenderTargetConfig) EZ_OVERRIDE;

  virtual ezGALVertexDeclaration* CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description) EZ_OVERRIDE;

  virtual void DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) EZ_OVERRIDE;
  
  
  // Get Query Data

  virtual void GetQueryDataPlatform(ezGALQuery* pQuery, ezUInt64* puiRendererdPixels) EZ_OVERRIDE;




  // Swap chain functions

  virtual void PresentPlatform(ezGALSwapChain* pSwapChain) EZ_OVERRIDE;

  // Misc functions

  virtual void BeginFramePlatform() EZ_OVERRIDE;

  virtual void EndFramePlatform() EZ_OVERRIDE;

  virtual void FlushPlatform() EZ_OVERRIDE;

  virtual void FinishPlatform() EZ_OVERRIDE;

  virtual void SetPrimarySwapChainPlatform(ezGALSwapChain* pSwapChain) EZ_OVERRIDE;

  /// \endcond

private:

  void FillFormatLookupTable();

  ID3D11Device* m_pDevice;

  IDXGIFactory1* m_pDXGIFactory;

  IDXGIAdapter1* m_pDXGIAdapter;

  IDXGIDevice1* m_pDXGIDevice;

  ezGALFormatLookupTableDX11 m_FormatLookupTable;
};

#include <RendererDX11/Device/Implementation/DeviceDX11_inl.h>
