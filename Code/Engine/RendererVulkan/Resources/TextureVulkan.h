#pragma once

#include <RendererFoundation/Resources/Texture.h>

#include <vulkan/vulkan.hpp>

class ezGALBufferVulkan;
class ezGALDeviceVulkan;

class ezGALTextureVulkan : public ezGALTexture
{
public:
  enum class StagingMode : ezUInt8
  {
    None,
    Buffer,          ///< We can use vkCopyImageToBuffer to a CPU buffer.
    Texture,         ///< Formats differ and we need to render to a linear CPU texture to do the conversion.
    TextureAndBuffer ///< Formats differ and linear texture can't be rendered to. Render to optimal layout GPU texture and then use vkCopyImageToBuffer to CPU buffer.
  };
  struct SubResourceOffset
  {
    EZ_DECLARE_POD_TYPE();
    ezUInt32 m_uiOffset;
    ezUInt32 m_uiSize;
    ezUInt32 m_uiRowLength;
    ezUInt32 m_uiImageHeight;
  };

  EZ_ALWAYS_INLINE vk::Image GetImage() const;
  EZ_ALWAYS_INLINE vk::Format GetImageFormat() const { return m_imageFormat; }
  EZ_ALWAYS_INLINE vk::ImageLayout GetPreferredLayout() const;
  EZ_ALWAYS_INLINE vk::ImageLayout GetPreferredLayout(vk::ImageLayout targetLayout) const;
  EZ_ALWAYS_INLINE vk::PipelineStageFlags GetUsedByPipelineStage() const;
  EZ_ALWAYS_INLINE vk::AccessFlags GetAccessMask() const;

  EZ_ALWAYS_INLINE ezVulkanAllocation GetAllocation() const;
  EZ_ALWAYS_INLINE const ezVulkanAllocationInfo& GetAllocationInfo() const;

  EZ_ALWAYS_INLINE bool GetFormatOverrideEnabled() const;
  EZ_ALWAYS_INLINE bool IsLinearLayout() const;

  vk::Extent3D GetMipLevelSize(ezUInt32 uiMipLevel) const;
  vk::ImageSubresourceRange GetFullRange() const;
  vk::ImageAspectFlags GetAspectMask() const;

  // Read-back staging resources
  EZ_ALWAYS_INLINE StagingMode GetStagingMode() const;
  EZ_ALWAYS_INLINE ezGALTextureHandle GetStagingTexture() const;
  EZ_ALWAYS_INLINE ezGALBufferHandle GetStagingBuffer() const;
  ezUInt32 ComputeSubResourceOffsets(ezDynamicArray<SubResourceOffset>& out_subResourceOffsets) const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALTextureVulkan(const ezGALTextureCreationDescription& Description);
  ezGALTextureVulkan(const ezGALTextureCreationDescription& Description, vk::Format OverrideFormat, bool bLinearCPU);

  ~ezGALTextureVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  StagingMode ComputeStagingMode(const vk::ImageCreateInfo& createInfo) const;
  ezResult CreateStagingBuffer(const vk::ImageCreateInfo& createInfo);

  vk::Image m_image;
  vk::Format m_imageFormat = vk::Format::eUndefined;
  vk::ImageLayout m_preferredLayout = vk::ImageLayout::eUndefined;
  vk::PipelineStageFlags m_stages = {};
  vk::AccessFlags m_access = {};

  ezVulkanAllocation m_alloc = nullptr;
  ezVulkanAllocationInfo m_allocInfo;

  ezGALDeviceVulkan* m_pDevice = nullptr;
  void* m_pExisitingNativeObject = nullptr;

  bool m_formatOverride = false;
  bool m_bLinearCPU = false;

  StagingMode m_stagingMode = StagingMode::None;
  ezGALTextureHandle m_hStagingTexture;
  ezGALBufferHandle m_hStagingBuffer;
};

#include <RendererVulkan/Resources/Implementation/TextureVulkan_inl.h>
