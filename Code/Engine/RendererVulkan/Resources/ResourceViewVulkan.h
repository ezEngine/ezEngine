
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

#include <vulkan/vulkan.hpp>

class ezGALBufferVulkan;
class ezGALTextureVulkan;

class ezGALResourceViewVulkan : public ezGALResourceView
{
public:
  const vk::DescriptorImageInfo& GetImageInfo(bool bIsArray) const;
  const vk::DescriptorBufferInfo& GetBufferInfo() const;
  vk::ImageSubresourceRange GetRange() const;
  const vk::BufferView& GetBufferView() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALResourceViewVulkan(ezGALResourceBase* pResource, const ezGALResourceViewCreationDescription& Description);
  ~ezGALResourceViewVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  vk::ImageSubresourceRange m_range;
  mutable vk::DescriptorImageInfo m_resourceImageInfo;
  mutable vk::DescriptorImageInfo m_resourceImageInfoArray;
  mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
  vk::BufferView m_bufferView;
};

#include <RendererVulkan/Resources/Implementation/ResourceViewVulkan_inl.h>
