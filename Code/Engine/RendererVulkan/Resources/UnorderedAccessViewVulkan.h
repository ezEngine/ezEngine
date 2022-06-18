
#pragma once

#include <RendererFoundation/Resources/UnorderedAccesView.h>

#include <vulkan/vulkan.hpp>

class ezGALBufferVulkan;

class ezGALUnorderedAccessViewVulkan : public ezGALUnorderedAccessView
{
public:
  EZ_ALWAYS_INLINE const vk::DescriptorImageInfo& GetImageInfo() const;
  const vk::DescriptorBufferInfo& GetBufferInfo() const;
  vk::ImageSubresourceRange GetRange() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALUnorderedAccessViewVulkan(ezGALResourceBase* pResource, const ezGALUnorderedAccessViewCreationDescription& Description);
  ~ezGALUnorderedAccessViewVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

private:
  mutable vk::DescriptorImageInfo m_resourceImageInfo;
  mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
  vk::ImageSubresourceRange m_range;

};

#include <RendererVulkan/Resources/Implementation/UnorderedAccessViewVulkan_inl.h>
