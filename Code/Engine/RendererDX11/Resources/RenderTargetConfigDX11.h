
#pragma once

#include <RendererFoundation/Resources/RenderTargetConfig.h>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

class ezGALRenderTargetConfigDX11 : public ezGALRenderTargetConfig
{
public:

protected:

  friend class ezGALDeviceDX11;
  friend class ezGALContextDX11;
  friend class ezMemoryUtils;

  ezGALRenderTargetConfigDX11(const ezGALRenderTargetConfigCreationDescription& Description);

  virtual ~ezGALRenderTargetConfigDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11RenderTargetView* m_pRenderTargetViews[EZ_GAL_MAX_RENDERTARGET_COUNT];

  ID3D11DepthStencilView* m_pDepthStencilTargetView;
};

#include <RendererDX11/Resources/Implementation/RenderTargetConfigDX11_inl.h>
