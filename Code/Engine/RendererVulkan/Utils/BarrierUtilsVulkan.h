#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Descriptors/Enumerations.h>

class ezGALDeviceVulkan;

/// Describes a texture barrier for a layout/state transition using a raw Vulkan image.
struct ezTextureBarrierVulkan
{
  EZ_DECLARE_POD_TYPE();
  vk::Image m_Image;
  vk::ImageAspectFlags m_AspectMask = vk::ImageAspectFlagBits::eColor;
  ezBitflags<ezGALResourceState> m_StateBefore;
  ezBitflags<ezGALResourceState> m_StateAfter;
  ezBitflags<ezGALShaderStageFlags> m_StagesBefore;
  ezBitflags<ezGALShaderStageFlags> m_StagesAfter;
  ezGALTextureSubresource m_Subresource = {};
  bool m_bAllSubresources = true; ///< If true, the barrier applies to all subresources and m_Subresource is ignored.
  bool m_bDiscard = false;
};

/// Describes a buffer barrier for a state transition using a raw Vulkan buffer.
struct ezBufferBarrierVulkan
{
  EZ_DECLARE_POD_TYPE();
  vk::Buffer m_Buffer;
  ezBitflags<ezGALResourceState> m_StateBefore;
  ezBitflags<ezGALResourceState> m_StateAfter;
  ezBitflags<ezGALShaderStageFlags> m_StagesBefore;
  ezBitflags<ezGALShaderStageFlags> m_StagesAfter;
};

/// Translates GAL barrier descriptions into Vulkan barriers and submits them.
///
/// Uses VK_KHR_synchronization2 (vkCmdPipelineBarrier2) when the extension is available on the device, otherwise falls back to the core Vulkan 1.1 vkCmdPipelineBarrier path.
class EZ_RENDERERVULKAN_DLL ezBarrierUtilsVulkan
{
public:
  ezBarrierUtilsVulkan(const ezGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer);

  /// Submits image memory barriers for the given texture barrier descriptions.
  void TextureBarrier(ezArrayPtr<const ezGALTextureBarrier> barriers);

  /// Submits image memory barriers for the given Vulkan texture barrier descriptions.
  void TextureBarrier(ezArrayPtr<const ezTextureBarrierVulkan> barriers);

  /// Submits buffer memory barriers for the given buffer barrier descriptions.
  void BufferBarrier(ezArrayPtr<const ezGALBufferBarrier> barriers);

  /// Submits buffer memory barriers for the given Vulkan buffer barrier descriptions.
  void BufferBarrier(ezArrayPtr<const ezBufferBarrierVulkan> barriers);

  /// Submits a single image memory barrier.
  ///
  /// If stagesBefore or stagesAfter are set to Auto, the pipeline stages are inferred from the resource state.
  void TextureBarrier(
    ezGALTextureHandle hTexture,
    ezBitflags<ezGALResourceState> stateBefore,
    ezBitflags<ezGALResourceState> stateAfter,
    ezBitflags<ezGALShaderStageFlags> stagesBefore = ezGALShaderStageFlags::Auto,
    ezBitflags<ezGALShaderStageFlags> stagesAfter = ezGALShaderStageFlags::Auto);

  /// Submits a single image memory barrier for a raw Vulkan image.
  ///
  /// If stagesBefore or stagesAfter are set to Auto, the pipeline stages are inferred from the resource state.
  void TextureBarrier(
    vk::Image image,
    const vk::ImageSubresourceRange& subresourceRange,
    ezBitflags<ezGALResourceState> stateBefore,
    ezBitflags<ezGALResourceState> stateAfter,
    ezBitflags<ezGALShaderStageFlags> stagesBefore = ezGALShaderStageFlags::Auto,
    ezBitflags<ezGALShaderStageFlags> stagesAfter = ezGALShaderStageFlags::Auto);

  /// Submits a single buffer memory barrier.
  ///
  /// If stagesBefore or stagesAfter are set to Auto, the pipeline stages are inferred from the resource state.
  void BufferBarrier(
    ezGALBufferHandle hBuffer,
    ezBitflags<ezGALResourceState> stateBefore,
    ezBitflags<ezGALResourceState> stateAfter,
    ezBitflags<ezGALShaderStageFlags> stagesBefore = ezGALShaderStageFlags::Auto,
    ezBitflags<ezGALShaderStageFlags> stagesAfter = ezGALShaderStageFlags::Auto);

  /// Submits a single buffer memory barrier for a raw Vulkan buffer.
  ///
  /// If stagesBefore or stagesAfter are set to Auto, the pipeline stages are inferred from the resource state.
  void BufferBarrier(
    vk::Buffer buffer,
    ezBitflags<ezGALResourceState> stateBefore,
    ezBitflags<ezGALResourceState> stateAfter,
    ezBitflags<ezGALShaderStageFlags> stagesBefore = ezGALShaderStageFlags::Auto,
    ezBitflags<ezGALShaderStageFlags> stagesAfter = ezGALShaderStageFlags::Auto);

private:
  const ezGALDeviceVulkan& m_Device;
  vk::CommandBuffer& m_CommandBuffer;
};
