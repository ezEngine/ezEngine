
#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

class ezGALRenderTargetViewVulkan : public ezGALRenderTargetView
{
public:
  vk::ImageView GetImageView() const;
  bool IsFullRange() const;
  vk::ImageSubresourceRange GetRange() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALRenderTargetViewVulkan(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description);
  virtual ~ezGALRenderTargetViewVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  vk::ImageView m_imageView;
  bool m_bfullRange = false;
  vk::ImageSubresourceRange m_range;
};

#include <RendererVulkan/Resources/Implementation/RenderTargetViewVulkan_inl.h>
