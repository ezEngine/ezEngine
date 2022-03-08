
#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

class ezGALRenderTargetViewVulkan : public ezGALRenderTargetView
{
public:

  EZ_ALWAYS_INLINE vk::ImageView GetImageView() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALRenderTargetViewVulkan(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description);

  virtual ~ezGALRenderTargetViewVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  vk::ImageView m_imageView;
};

#include <RendererVulkan/Resources/Implementation/RenderTargetViewVulkan_inl.h>
