#pragma once

#include <RendererFoundation/Resources/Texture.h>

#include <vulkan/vulkan.hpp>

class ezGALBufferVulkan;
class ezGALDeviceVulkan;

class ezGALTextureVulkan : public ezGALTexture
{
public:
  struct SubResourceOffset
  {
    EZ_DECLARE_POD_TYPE();
    ezUInt32 m_uiOffset;
    ezUInt32 m_uiSize;
    ezUInt32 m_uiRowLength;
    ezUInt32 m_uiImageHeight;
  };

  static vk::Format ComputeImageFormat(const ezGALDeviceVulkan* pDevice, ezEnum<ezGALResourceFormat> galFormat, vk::ImageCreateInfo& ref_createInfo, vk::ImageFormatListCreateInfo& ref_imageFormats);
  static void ComputeCreateInfo(const ezGALDeviceVulkan* pDevice, const ezGALTextureCreationDescription& description, vk::ImageCreateInfo& ref_createInfo, vk::PipelineStageFlags& ref_stages, vk::AccessFlags& ref_access, vk::ImageLayout& ref_preferredLayout);
  static void ComputeAllocInfo(ezVulkanAllocationCreateInfo& ref_allocInfo);
  static ezUInt32 ComputeSubResourceOffsets(const ezGALDeviceVulkan* pDevice, const ezGALTextureCreationDescription& description, ezDynamicArray<SubResourceOffset>& subResourceSizes);
  static vk::Extent3D GetMipLevelSize(const ezGALTextureCreationDescription& description, ezUInt32 uiMipLevel);

public:
  EZ_ALWAYS_INLINE vk::Image GetImage() const;
  EZ_ALWAYS_INLINE vk::Format GetImageFormat() const { return m_imageFormat; }
  EZ_ALWAYS_INLINE vk::ImageLayout GetPreferredLayout() const;
  EZ_ALWAYS_INLINE vk::ImageLayout GetPreferredLayout(vk::ImageLayout targetLayout) const;
  EZ_ALWAYS_INLINE vk::PipelineStageFlags GetUsedByPipelineStage() const;
  EZ_ALWAYS_INLINE vk::AccessFlags GetAccessMask() const;

  EZ_ALWAYS_INLINE ezVulkanAllocation GetAllocation() const;
  EZ_ALWAYS_INLINE const ezVulkanAllocationInfo& GetAllocationInfo() const;

  vk::Extent3D GetMipLevelSize(ezUInt32 uiMipLevel) const { return GetMipLevelSize(m_Description, uiMipLevel); }
  vk::ImageSubresourceRange GetFullRange() const;
  vk::ImageAspectFlags GetAspectMask() const;

  vk::DescriptorImageInfo GetDescriptorImageInfo(ezGALTextureRange textureRange, ezEnum<ezGALShaderResourceType> resourceType, ezEnum<ezGALShaderTextureType> textureType, ezEnum<ezGALResourceFormat> overrideViewFormat) const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALTextureVulkan(const ezGALTextureCreationDescription& Description);
  ~ezGALTextureVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

protected:
  vk::Image m_image = {};
  vk::Format m_imageFormat = vk::Format::eUndefined;
  vk::ImageLayout m_preferredLayout = vk::ImageLayout::eUndefined;
  vk::PipelineStageFlags m_stages = {};
  vk::AccessFlags m_access = {};

  ezVulkanAllocation m_alloc = nullptr;
  ezVulkanAllocationInfo m_allocInfo;

  // Views
  struct View : ezHashableStruct<View>
  {
    ezGALTextureRange m_TextureRange;
    ezEnum<ezGALShaderResourceType> m_ResourceType;
    ezEnum<ezGALShaderTextureType> m_TextureType;
    ezEnum<ezGALResourceFormat> m_OverrideViewFormat;

    EZ_ALWAYS_INLINE static ezUInt32 Hash(const View& value) { return value.CalculateHash(); }
    EZ_ALWAYS_INLINE static bool Equal(const View& a, const View& b) { return a == b; }
  };
  mutable ezHashTable<View, vk::DescriptorImageInfo, View> m_TextureViews;

  ezGALDeviceVulkan* m_pDevice = nullptr;
};

#include <RendererVulkan/Resources/Implementation/TextureVulkan_inl.h>
