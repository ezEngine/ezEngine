
#pragma once

#include <RendererFoundation/Resources/UnorderedAccesView.h>

#include <vulkan/vulkan.hpp>

class ezGALBufferVulkan;

class ezGALTextureUnorderedAccessViewVulkan : public ezGALTextureUnorderedAccessView
{
public:
  EZ_ALWAYS_INLINE const vk::DescriptorImageInfo& GetImageInfo() const;
  vk::ImageSubresourceRange GetRange() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALTextureUnorderedAccessViewVulkan(ezGALTexture* pResource, const ezGALTextureUnorderedAccessViewCreationDescription& Description);
  ~ezGALTextureUnorderedAccessViewVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

private:
  mutable vk::DescriptorImageInfo m_resourceImageInfo;
  //mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
  //vk::BufferView m_bufferView;
  vk::ImageSubresourceRange m_range;
};

class ezGALBufferUnorderedAccessViewVulkan : public ezGALBufferUnorderedAccessView
{
public:
  const vk::DescriptorBufferInfo& GetBufferInfo() const;
  const vk::BufferView& GetBufferView() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALBufferUnorderedAccessViewVulkan(ezGALBuffer* pResource, const ezGALBufferUnorderedAccessViewCreationDescription& Description);
  ~ezGALBufferUnorderedAccessViewVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

private:
  //mutable vk::DescriptorImageInfo m_resourceImageInfo;
  mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
  vk::BufferView m_bufferView;
  //vk::ImageSubresourceRange m_range;
};

#include <RendererVulkan/Resources/Implementation/UnorderedAccessViewVulkan_inl.h>
