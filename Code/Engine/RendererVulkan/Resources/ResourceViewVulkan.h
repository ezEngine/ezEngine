
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

#include <vulkan/vulkan.hpp>

class ezGALResourceViewVulkan : public ezGALResourceView
{
public:
  EZ_ALWAYS_INLINE const vk::DescriptorSetLayoutBinding& GetResourceBinding() const;
  EZ_ALWAYS_INLINE const vk::WriteDescriptorSet& GetResourceBindingData() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALResourceViewVulkan(ezGALResourceBase* pResource, const ezGALResourceViewCreationDescription& Description);

  ~ezGALResourceViewVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  vk::ImageView m_imageView;
  vk::DescriptorSetLayoutBinding m_resourceBinding;
  vk::WriteDescriptorSet m_resourceBindingData;
  vk::DescriptorBufferInfo m_resourceBufferInfo;
  vk::DescriptorImageInfo m_resourceImageInfo;
};

#include <RendererVulkan/Resources/Implementation/ResourceViewVulkan_inl.h>
