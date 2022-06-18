
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

#include <vulkan/vulkan.hpp>

class ezGALBufferVulkan;
class ezGALTextureVulkan;

class ezGALResourceViewVulkan : public ezGALResourceView
{
public:
  EZ_ALWAYS_INLINE const vk::DescriptorImageInfo& GetImageInfo() const;
  const vk::DescriptorBufferInfo& GetBufferInfo() const;
  vk::ImageSubresourceRange GetRange() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALResourceViewVulkan(ezGALResourceBase* pResource, const ezGALResourceViewCreationDescription& Description);
  ~ezGALResourceViewVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  vk::ImageSubresourceRange m_range;
  mutable vk::DescriptorImageInfo m_resourceImageInfo;
  mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
};

#include <RendererVulkan/Resources/Implementation/ResourceViewVulkan_inl.h>
