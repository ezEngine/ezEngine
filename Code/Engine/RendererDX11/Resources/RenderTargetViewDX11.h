
#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

class ezGALRenderTargetViewDX11 : public ezGALRenderTargetView
{
public:


  EZ_FORCE_INLINE ID3D11RenderTargetView* GetRenderTargetView() const;

  EZ_FORCE_INLINE ID3D11DepthStencilView* GetDepthStencilView() const;

  EZ_FORCE_INLINE ID3D11UnorderedAccessView* GetUnorderedAccessView() const;

protected:

  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALRenderTargetViewDX11(const ezGALRenderTargetViewCreationDescription& Description);

  virtual ~ezGALRenderTargetViewDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11RenderTargetView* m_pRenderTargetView;

  ID3D11DepthStencilView* m_pDepthStencilView;

  ID3D11UnorderedAccessView* m_pUnorderedAccessView;
};

#include <RendererDX11/Resources/Implementation/RenderTargetViewDX11_inl.h>
