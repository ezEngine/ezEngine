
#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

class ezGALRenderTargetViewVulkan : public ezGALRenderTargetView
{
public:


  EZ_ALWAYS_INLINE ID3D11RenderTargetView* GetRenderTargetView() const;

  EZ_ALWAYS_INLINE ID3D11DepthStencilView* GetDepthStencilView() const;

  EZ_ALWAYS_INLINE ID3D11UnorderedAccessView* GetUnorderedAccessView() const;

protected:

  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALRenderTargetViewVulkan(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description);

  virtual ~ezGALRenderTargetViewVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11RenderTargetView* m_pRenderTargetView;

  ID3D11DepthStencilView* m_pDepthStencilView;

  ID3D11UnorderedAccessView* m_pUnorderedAccessView;
};

#include <RendererVulkan/Resources/Implementation/RenderTargetViewVulkan_inl.h>
