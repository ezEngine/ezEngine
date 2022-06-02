#pragma once

#include <RendererFoundation/Resources/Texture.h>

#include <vulkan/vulkan.hpp>

class ezGALBufferVulkan;

class ezGALTextureVulkan : public ezGALTexture
{
public:
  EZ_ALWAYS_INLINE vk::Image GetImage() const;
  EZ_ALWAYS_INLINE vk::Format GetImageFormat() const { return m_imageFormat; }
  EZ_ALWAYS_INLINE vk::ImageLayout GetCurrentLayout() const;
  EZ_ALWAYS_INLINE vk::ImageLayout GetPreferredLayout() const;
  EZ_ALWAYS_INLINE vk::ImageLayout GetPreferredLayout(vk::ImageLayout targetLayout) const;
  EZ_ALWAYS_INLINE vk::PipelineStageFlags GetUsedByPipelineStage() const;
  EZ_ALWAYS_INLINE vk::AccessFlags GetAccessMask() const;

  EZ_ALWAYS_INLINE ezVulkanAllocation GetAllocation() const;
  EZ_ALWAYS_INLINE const ezVulkanAllocationInfo& GetAllocationInfo() const;

  EZ_ALWAYS_INLINE const vk::Image GetStagingTexture() const;
  EZ_ALWAYS_INLINE ezVulkanAllocation GetStagingAllocation() const;
  EZ_ALWAYS_INLINE const ezVulkanAllocationInfo& GetStagingAllocationInfo() const;

  vk::Extent3D GetMipLevelSize(ezUInt32 uiMipLevel) const;
  vk::ImageSubresourceRange GetFullRange() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALTextureVulkan(const ezGALTextureCreationDescription& Description);

  ~ezGALTextureVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  ezResult CreateStagingBuffer(ezGALDeviceVulkan* pDevice, const vk::ImageCreateInfo& imageCreateInfo);

  vk::Image m_image;
  vk::Format m_imageFormat = vk::Format::eUndefined;
  vk::ImageLayout m_currentLayout = vk::ImageLayout::eUndefined;
  vk::ImageLayout m_preferredLayout = vk::ImageLayout::eUndefined;
  vk::PipelineStageFlags m_stages = {};
  vk::AccessFlags m_access = {};

  ezVulkanAllocation m_alloc = nullptr;
  ezVulkanAllocationInfo m_allocInfo;

  vk::Device m_device;
  void* m_pExisitingNativeObject = nullptr;

  vk::Image m_stagingImage;
  ezVulkanAllocation m_stagingAlloc = nullptr;
  ezVulkanAllocationInfo m_stagingAllocInfo;
};

#include <RendererVulkan/Resources/Implementation/TextureVulkan_inl.h>
