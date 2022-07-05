#pragma once

#include <RendererFoundation/Resources/Texture.h>

#include <vulkan/vulkan.hpp>

class ezGALBufferVulkan;
class ezGALDeviceVulkan;

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
  EZ_ALWAYS_INLINE vk::Format GetStagingImageFormat() const { return m_stagingImageFormat; }
  EZ_ALWAYS_INLINE ezVulkanAllocation GetStagingAllocation() const;
  EZ_ALWAYS_INLINE const ezVulkanAllocationInfo& GetStagingAllocationInfo() const;

  EZ_ALWAYS_INLINE bool GetFormatOverrideEnabled() const;

  vk::Extent3D GetMipLevelSize(ezUInt32 uiMipLevel) const;
  vk::ImageSubresourceRange GetFullRange() const;
  vk::ImageAspectFlags GetStagingAspectMask() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALTextureVulkan(const ezGALTextureCreationDescription& Description);
  ezGALTextureVulkan(const ezGALTextureCreationDescription& Description, vk::Format OverrideFormat);

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

  ezGALDeviceVulkan* m_pDevice = nullptr;
  vk::Device m_device;
  void* m_pExisitingNativeObject = nullptr;

  vk::Image m_stagingImage;
  vk::Format m_stagingImageFormat = vk::Format::eUndefined;
  vk::ComponentMapping m_componentMapping;
  ezVulkanAllocation m_stagingAlloc = nullptr;
  ezVulkanAllocationInfo m_stagingAllocInfo;

  bool m_formatOverride = false;
};

#include <RendererVulkan/Resources/Implementation/TextureVulkan_inl.h>
