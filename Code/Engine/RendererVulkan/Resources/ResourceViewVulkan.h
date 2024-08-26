
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

#include <vulkan/vulkan.hpp>

class ezGALBufferVulkan;
class ezGALTextureVulkan;

class ezGALTextureResourceViewVulkan : public ezGALTextureResourceView
{
public:
  const vk::DescriptorImageInfo& GetImageInfo(bool bIsArray) const;
  vk::ImageSubresourceRange GetRange() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALTextureResourceViewVulkan(ezGALTexture* pResource, const ezGALTextureResourceViewCreationDescription& Description);
  ~ezGALTextureResourceViewVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  vk::ImageSubresourceRange m_range;
  mutable vk::DescriptorImageInfo m_resourceImageInfo;
  mutable vk::DescriptorImageInfo m_resourceImageInfoArray;
};

class ezGALBufferResourceViewVulkan : public ezGALBufferResourceView
{
public:
  const vk::DescriptorBufferInfo& GetBufferInfo() const;
  const vk::BufferView& GetBufferView() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALBufferResourceViewVulkan(ezGALBuffer* pResource, const ezGALBufferResourceViewCreationDescription& Description);
  ~ezGALBufferResourceViewVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
  vk::BufferView m_bufferView;
};

#include <RendererVulkan/Resources/Implementation/ResourceViewVulkan_inl.h>
