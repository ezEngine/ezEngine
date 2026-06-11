#include <RendererVulkan/RendererVulkanPCH.h>

// #define EZ_LOG_VULKAN_BARRIERS

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/DispatchContext.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/BarrierUtilsVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

namespace
{
  // ============================================================================================
  // Helpers shared between Synchronization2 and Vulkan 1.1 code paths.
  // ============================================================================================

  vk::ImageSubresourceRange CreateSubresourceRange(const ezGALTextureVulkan& texture, const ezGALTextureBarrier& barrier)
  {
    if (barrier.m_bAllSubresources)
      return texture.GetFullRange();

    vk::ImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = texture.GetAspectMask();
    subresourceRange.baseMipLevel = barrier.m_Subresource.m_uiMipLevel;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = barrier.m_Subresource.m_uiArraySlice;
    subresourceRange.layerCount = 1;
    return subresourceRange;
  }

  vk::ImageSubresourceRange CreateSubresourceRange(const ezTextureBarrierVulkan& barrier)
  {
    vk::ImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = barrier.m_AspectMask;

    if (barrier.m_bAllSubresources)
    {
      subresourceRange.baseMipLevel = 0;
      subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
      subresourceRange.baseArrayLayer = 0;
      subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    }
    else
    {
      subresourceRange.baseMipLevel = barrier.m_Subresource.m_uiMipLevel;
      subresourceRange.levelCount = 1;
      subresourceRange.baseArrayLayer = barrier.m_Subresource.m_uiArraySlice;
      subresourceRange.layerCount = 1;
    }

    return subresourceRange;
  }

  // ============================================================================================
  // Synchronization2 code path (VK_KHR_synchronization2).
  // ============================================================================================

  namespace Sync2
  {
    vk::PipelineStageFlags2 GetPipelineStages(ezBitflags<ezGALShaderStageFlags> stages)
    {
      vk::PipelineStageFlags2 res;
      if (stages.IsSet(ezGALShaderStageFlags::VertexShader))
        res |= vk::PipelineStageFlagBits2::eVertexShader;
      if (stages.IsSet(ezGALShaderStageFlags::HullShader))
        res |= vk::PipelineStageFlagBits2::eTessellationControlShader;
      if (stages.IsSet(ezGALShaderStageFlags::DomainShader))
        res |= vk::PipelineStageFlagBits2::eTessellationEvaluationShader;
      if (stages.IsSet(ezGALShaderStageFlags::GeometryShader))
        res |= vk::PipelineStageFlagBits2::eGeometryShader;
      if (stages.IsSet(ezGALShaderStageFlags::PixelShader))
        res |= vk::PipelineStageFlagBits2::eFragmentShader;
      if (stages.IsSet(ezGALShaderStageFlags::ComputeShader))
        res |= vk::PipelineStageFlagBits2::eComputeShader;
      return res;
    }

    void ResolveBarrierSync(
      ezBitflags<ezGALResourceState> stateBefore,
      ezBitflags<ezGALResourceState> stateAfter,
      ezBitflags<ezGALShaderStageFlags> stagesBefore,
      ezBitflags<ezGALShaderStageFlags> stagesAfter,
      vk::PipelineStageFlags2& out_srcStages,
      vk::PipelineStageFlags2& out_dstStages,
      vk::AccessFlags2& out_srcAccess,
      vk::AccessFlags2& out_dstAccess)
    {
      ezConversionUtilsVulkan::ConvertResourceState(stateBefore, out_srcStages, out_srcAccess);
      ezConversionUtilsVulkan::ConvertResourceState(stateAfter, out_dstStages, out_dstAccess);

      if (!stagesBefore.IsSet(ezGALShaderStageFlags::Auto))
      {
        const vk::PipelineStageFlags2 explicitStages = GetPipelineStages(stagesBefore);
        if (explicitStages != vk::PipelineStageFlags2{} && (explicitStages & out_srcStages) == explicitStages)
          out_srcStages = explicitStages;
      }

      if (!stagesAfter.IsSet(ezGALShaderStageFlags::Auto))
      {
        const vk::PipelineStageFlags2 explicitStages = GetPipelineStages(stagesAfter);
        if (explicitStages != vk::PipelineStageFlags2{} && (explicitStages & out_dstStages) == explicitStages)
          out_dstStages = explicitStages;
      }
    }

    vk::ImageMemoryBarrier2 MakeImageBarrier(
      vk::Image image,
      const vk::ImageSubresourceRange& subresourceRange,
      ezBitflags<ezGALResourceState> stateBefore,
      ezBitflags<ezGALResourceState> stateAfter,
      ezBitflags<ezGALShaderStageFlags> stagesBefore,
      ezBitflags<ezGALShaderStageFlags> stagesAfter,
      bool bDiscard = false)
    {
      vk::ImageMemoryBarrier2 vkBarrier;

      ResolveBarrierSync(stateBefore, stateAfter, stagesBefore, stagesAfter,
        vkBarrier.srcStageMask, vkBarrier.dstStageMask, vkBarrier.srcAccessMask, vkBarrier.dstAccessMask);

      vkBarrier.oldLayout = bDiscard ? vk::ImageLayout::eUndefined : ezConversionUtilsVulkan::GetTextureLayout(stateBefore);
      vkBarrier.newLayout = ezConversionUtilsVulkan::GetTextureLayout(stateAfter);
      vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.image = image;
      vkBarrier.subresourceRange = subresourceRange;

      return vkBarrier;
    }

    vk::BufferMemoryBarrier2 MakeBufferBarrier(
      vk::Buffer buffer,
      ezBitflags<ezGALResourceState> stateBefore,
      ezBitflags<ezGALResourceState> stateAfter,
      ezBitflags<ezGALShaderStageFlags> stagesBefore,
      ezBitflags<ezGALShaderStageFlags> stagesAfter)
    {
      vk::BufferMemoryBarrier2 vkBarrier;

      ResolveBarrierSync(stateBefore, stateAfter, stagesBefore, stagesAfter,
        vkBarrier.srcStageMask, vkBarrier.dstStageMask, vkBarrier.srcAccessMask, vkBarrier.dstAccessMask);

      vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.buffer = buffer;
      vkBarrier.offset = 0;
      vkBarrier.size = VK_WHOLE_SIZE;

      return vkBarrier;
    }

    void SubmitImageBarriers(const ezGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, ezArrayPtr<const vk::ImageMemoryBarrier2> barriers)
    {
      if (barriers.IsEmpty())
        return;

#ifdef EZ_LOG_VULKAN_BARRIERS
      for (const vk::ImageMemoryBarrier2& b : barriers)
      {
        void* bla = static_cast<VkImage>(b.image);
        void* blub = reinterpret_cast<void*>(0x3d000000003d);
        if (bla == blub)
        {
          ezLog::Info("CommandBuffer: {}, VkBarrier: Image {} | {} -> {}", ezArgP(static_cast<VkCommandBuffer>(commandBuffer)), ezArgP(static_cast<VkImage>(b.image)), vk::to_string(b.oldLayout).c_str(), vk::to_string(b.newLayout).c_str());
        }
      }
#endif

      vk::DependencyInfo depInfo;
      depInfo.imageMemoryBarrierCount = barriers.GetCount();
      depInfo.pImageMemoryBarriers = barriers.GetPtr();
      ref_commandBuffer.pipelineBarrier2KHR(depInfo, device.GetDispatchContext());
    }

    void SubmitBufferBarriers(const ezGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, ezArrayPtr<const vk::BufferMemoryBarrier2> barriers)
    {
      if (barriers.IsEmpty())
        return;

      vk::DependencyInfo depInfo;
      depInfo.bufferMemoryBarrierCount = barriers.GetCount();
      depInfo.pBufferMemoryBarriers = barriers.GetPtr();
      ref_commandBuffer.pipelineBarrier2KHR(depInfo, device.GetDispatchContext());
    }

    void TextureBarrier(const ezGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, ezArrayPtr<const ezGALTextureBarrier> barriers)
    {
      ezHybridArray<vk::ImageMemoryBarrier2, 16> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());

      for (const ezGALTextureBarrier& barrier : barriers)
      {
        const ezGALTextureVulkan* pTexture = static_cast<const ezGALTextureVulkan*>(device.GetTexture(barrier.m_hTexture)->GetParentResource());
        if (pTexture == nullptr)
          continue;

        vkBarriers.PushBack(MakeImageBarrier(
          pTexture->GetImage(),
          CreateSubresourceRange(*pTexture, barrier),
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter,
          barrier.m_bDiscard));
      }

      SubmitImageBarriers(device, ref_commandBuffer, vkBarriers.GetArrayPtr());
    }

    void TextureBarrier(const ezGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, ezArrayPtr<const ezTextureBarrierVulkan> barriers)
    {
      ezHybridArray<vk::ImageMemoryBarrier2, 16> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());

      for (const ezTextureBarrierVulkan& barrier : barriers)
      {
        if (!barrier.m_Image)
          continue;

        vkBarriers.PushBack(MakeImageBarrier(
          barrier.m_Image,
          CreateSubresourceRange(barrier),
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter,
          barrier.m_bDiscard));
      }

      SubmitImageBarriers(device, ref_commandBuffer, vkBarriers.GetArrayPtr());
    }

    void BufferBarrier(const ezGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, ezArrayPtr<const ezGALBufferBarrier> barriers)
    {
      ezHybridArray<vk::BufferMemoryBarrier2, 16> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());

      for (const ezGALBufferBarrier& barrier : barriers)
      {
        const ezGALBufferVulkan* pBuffer = static_cast<const ezGALBufferVulkan*>(device.GetBuffer(barrier.m_hBuffer));
        if (pBuffer == nullptr)
          continue;

        vkBarriers.PushBack(MakeBufferBarrier(
          pBuffer->GetVkBuffer(),
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter));
      }

      SubmitBufferBarriers(device, ref_commandBuffer, vkBarriers.GetArrayPtr());
    }

    void BufferBarrier(const ezGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, ezArrayPtr<const ezBufferBarrierVulkan> barriers)
    {
      ezHybridArray<vk::BufferMemoryBarrier2, 16> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());

      for (const ezBufferBarrierVulkan& barrier : barriers)
      {
        if (!barrier.m_Buffer)
          continue;

        vkBarriers.PushBack(MakeBufferBarrier(
          barrier.m_Buffer,
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter));
      }

      SubmitBufferBarriers(device, ref_commandBuffer, vkBarriers.GetArrayPtr());
    }

    void TextureBarrier(
      const ezGALDeviceVulkan& device,
      vk::CommandBuffer& ref_commandBuffer,
      vk::Image image,
      const vk::ImageSubresourceRange& subresourceRange,
      ezBitflags<ezGALResourceState> stateBefore,
      ezBitflags<ezGALResourceState> stateAfter,
      ezBitflags<ezGALShaderStageFlags> stagesBefore,
      ezBitflags<ezGALShaderStageFlags> stagesAfter)
    {
      const vk::ImageMemoryBarrier2 vkBarrier = MakeImageBarrier(image, subresourceRange, stateBefore, stateAfter, stagesBefore, stagesAfter);
      SubmitImageBarriers(device, ref_commandBuffer, ezMakeArrayPtr(&vkBarrier, 1));
    }

    void BufferBarrier(
      const ezGALDeviceVulkan& device,
      vk::CommandBuffer& ref_commandBuffer,
      vk::Buffer buffer,
      ezBitflags<ezGALResourceState> stateBefore,
      ezBitflags<ezGALResourceState> stateAfter,
      ezBitflags<ezGALShaderStageFlags> stagesBefore,
      ezBitflags<ezGALShaderStageFlags> stagesAfter)
    {
      const vk::BufferMemoryBarrier2 vkBarrier = MakeBufferBarrier(buffer, stateBefore, stateAfter, stagesBefore, stagesAfter);
      SubmitBufferBarriers(device, ref_commandBuffer, ezMakeArrayPtr(&vkBarrier, 1));
    }
  } // namespace Sync2

  // ============================================================================================
  // Vulkan 1.1 fallback code path (vkCmdPipelineBarrier).
  //
  // Unlike Synchronization2, the legacy API takes a single src/dst stage mask per pipelineBarrier
  // call rather than per-barrier. The helpers therefore accumulate the combined stage mask while
  // building the barrier list and pass it to the submit function.
  // ============================================================================================

  namespace Sync1
  {
    vk::PipelineStageFlags GetPipelineStages(ezBitflags<ezGALShaderStageFlags> stages)
    {
      vk::PipelineStageFlags res;
      if (stages.IsSet(ezGALShaderStageFlags::VertexShader))
        res |= vk::PipelineStageFlagBits::eVertexShader;
      if (stages.IsSet(ezGALShaderStageFlags::HullShader))
        res |= vk::PipelineStageFlagBits::eTessellationControlShader;
      if (stages.IsSet(ezGALShaderStageFlags::DomainShader))
        res |= vk::PipelineStageFlagBits::eTessellationEvaluationShader;
      if (stages.IsSet(ezGALShaderStageFlags::GeometryShader))
        res |= vk::PipelineStageFlagBits::eGeometryShader;
      if (stages.IsSet(ezGALShaderStageFlags::PixelShader))
        res |= vk::PipelineStageFlagBits::eFragmentShader;
      if (stages.IsSet(ezGALShaderStageFlags::ComputeShader))
        res |= vk::PipelineStageFlagBits::eComputeShader;
      return res;
    }

    void ResolveBarrierSync(
      ezBitflags<ezGALResourceState> stateBefore,
      ezBitflags<ezGALResourceState> stateAfter,
      ezBitflags<ezGALShaderStageFlags> stagesBefore,
      ezBitflags<ezGALShaderStageFlags> stagesAfter,
      vk::PipelineStageFlags& out_srcStages,
      vk::PipelineStageFlags& out_dstStages,
      vk::AccessFlags& out_srcAccess,
      vk::AccessFlags& out_dstAccess)
    {
      ezConversionUtilsVulkan::ConvertResourceState(stateBefore, out_srcStages, out_srcAccess);
      ezConversionUtilsVulkan::ConvertResourceState(stateAfter, out_dstStages, out_dstAccess);

      if (!stagesBefore.IsSet(ezGALShaderStageFlags::Auto))
      {
        const vk::PipelineStageFlags explicitStages = GetPipelineStages(stagesBefore);
        if (explicitStages != vk::PipelineStageFlags{} && (explicitStages & out_srcStages) == explicitStages)
          out_srcStages = explicitStages;
      }

      if (!stagesAfter.IsSet(ezGALShaderStageFlags::Auto))
      {
        const vk::PipelineStageFlags explicitStages = GetPipelineStages(stagesAfter);
        if (explicitStages != vk::PipelineStageFlags{} && (explicitStages & out_dstStages) == explicitStages)
          out_dstStages = explicitStages;
      }
    }

    vk::ImageMemoryBarrier MakeImageBarrier(
      vk::Image image,
      const vk::ImageSubresourceRange& subresourceRange,
      ezBitflags<ezGALResourceState> stateBefore,
      ezBitflags<ezGALResourceState> stateAfter,
      ezBitflags<ezGALShaderStageFlags> stagesBefore,
      ezBitflags<ezGALShaderStageFlags> stagesAfter,
      vk::PipelineStageFlags& inout_srcStages,
      vk::PipelineStageFlags& inout_dstStages,
      bool bDiscard = false)
    {
      vk::ImageMemoryBarrier vkBarrier;
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;

      ResolveBarrierSync(stateBefore, stateAfter, stagesBefore, stagesAfter,
        srcStages, dstStages, vkBarrier.srcAccessMask, vkBarrier.dstAccessMask);

      inout_srcStages |= srcStages;
      inout_dstStages |= dstStages;

      vkBarrier.oldLayout = bDiscard ? vk::ImageLayout::eUndefined : ezConversionUtilsVulkan::GetTextureLayout(stateBefore);
      vkBarrier.newLayout = ezConversionUtilsVulkan::GetTextureLayout(stateAfter);
      vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.image = image;
      vkBarrier.subresourceRange = subresourceRange;

      return vkBarrier;
    }

    vk::BufferMemoryBarrier MakeBufferBarrier(
      vk::Buffer buffer,
      ezBitflags<ezGALResourceState> stateBefore,
      ezBitflags<ezGALResourceState> stateAfter,
      ezBitflags<ezGALShaderStageFlags> stagesBefore,
      ezBitflags<ezGALShaderStageFlags> stagesAfter,
      vk::PipelineStageFlags& inout_srcStages,
      vk::PipelineStageFlags& inout_dstStages)
    {
      vk::BufferMemoryBarrier vkBarrier;
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;

      ResolveBarrierSync(stateBefore, stateAfter, stagesBefore, stagesAfter,
        srcStages, dstStages, vkBarrier.srcAccessMask, vkBarrier.dstAccessMask);

      inout_srcStages |= srcStages;
      inout_dstStages |= dstStages;

      vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.buffer = buffer;
      vkBarrier.offset = 0;
      vkBarrier.size = VK_WHOLE_SIZE;

      return vkBarrier;
    }

    void SubmitImageBarriers(
      vk::CommandBuffer& ref_commandBuffer,
      vk::PipelineStageFlags srcStages,
      vk::PipelineStageFlags dstStages,
      ezArrayPtr<const vk::ImageMemoryBarrier> barriers)
    {
      if (barriers.IsEmpty())
        return;

#ifdef EZ_LOG_VULKAN_BARRIERS
      for (const vk::ImageMemoryBarrier& b : barriers)
      {
        void* bla = static_cast<VkImage>(b.image);
        void* blub = reinterpret_cast<void*>(0x3d000000003d);
        if (bla == blub)
        {
          ezLog::Info("CommandBuffer: {}, VkBarrier: Image {} | {} -> {}", ezArgP(static_cast<VkCommandBuffer>(commandBuffer)), ezArgP(static_cast<VkImage>(b.image)), vk::to_string(b.oldLayout).c_str(), vk::to_string(b.newLayout).c_str());
        }
      }
#endif

      ref_commandBuffer.pipelineBarrier(srcStages, dstStages, vk::DependencyFlags(),
        0, nullptr,
        0, nullptr,
        barriers.GetCount(), barriers.GetPtr());
    }

    void SubmitBufferBarriers(
      vk::CommandBuffer& ref_commandBuffer,
      vk::PipelineStageFlags srcStages,
      vk::PipelineStageFlags dstStages,
      ezArrayPtr<const vk::BufferMemoryBarrier> barriers)
    {
      if (barriers.IsEmpty())
        return;

      ref_commandBuffer.pipelineBarrier(srcStages, dstStages, vk::DependencyFlags(),
        0, nullptr,
        barriers.GetCount(), barriers.GetPtr(),
        0, nullptr);
    }

    void TextureBarrier(const ezGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, ezArrayPtr<const ezGALTextureBarrier> barriers)
    {
      ezHybridArray<vk::ImageMemoryBarrier, 16> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;

      for (const ezGALTextureBarrier& barrier : barriers)
      {
        const ezGALTextureVulkan* pTexture = static_cast<const ezGALTextureVulkan*>(device.GetTexture(barrier.m_hTexture)->GetParentResource());
        if (pTexture == nullptr)
          continue;

        vkBarriers.PushBack(MakeImageBarrier(
          pTexture->GetImage(),
          CreateSubresourceRange(*pTexture, barrier),
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter,
          srcStages,
          dstStages,
          barrier.m_bDiscard));
      }

      SubmitImageBarriers(ref_commandBuffer, srcStages, dstStages, vkBarriers.GetArrayPtr());
    }

    void TextureBarrier(const ezGALDeviceVulkan& /*device*/, vk::CommandBuffer& ref_commandBuffer, ezArrayPtr<const ezTextureBarrierVulkan> barriers)
    {
      ezHybridArray<vk::ImageMemoryBarrier, 16> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;

      for (const ezTextureBarrierVulkan& barrier : barriers)
      {
        if (!barrier.m_Image)
          continue;

        vkBarriers.PushBack(MakeImageBarrier(
          barrier.m_Image,
          CreateSubresourceRange(barrier),
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter,
          srcStages,
          dstStages,
          barrier.m_bDiscard));
      }

      SubmitImageBarriers(ref_commandBuffer, srcStages, dstStages, vkBarriers.GetArrayPtr());
    }

    void BufferBarrier(const ezGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer, ezArrayPtr<const ezGALBufferBarrier> barriers)
    {
      ezHybridArray<vk::BufferMemoryBarrier, 16> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;

      for (const ezGALBufferBarrier& barrier : barriers)
      {
        const ezGALBufferVulkan* pBuffer = static_cast<const ezGALBufferVulkan*>(device.GetBuffer(barrier.m_hBuffer));
        if (pBuffer == nullptr)
          continue;

        vkBarriers.PushBack(MakeBufferBarrier(
          pBuffer->GetVkBuffer(),
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter,
          srcStages,
          dstStages));
      }

      SubmitBufferBarriers(ref_commandBuffer, srcStages, dstStages, vkBarriers.GetArrayPtr());
    }

    void BufferBarrier(const ezGALDeviceVulkan& /*device*/, vk::CommandBuffer& ref_commandBuffer, ezArrayPtr<const ezBufferBarrierVulkan> barriers)
    {
      ezHybridArray<vk::BufferMemoryBarrier, 16> vkBarriers;
      vkBarriers.Reserve(barriers.GetCount());
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;

      for (const ezBufferBarrierVulkan& barrier : barriers)
      {
        if (!barrier.m_Buffer)
          continue;

        vkBarriers.PushBack(MakeBufferBarrier(
          barrier.m_Buffer,
          barrier.m_StateBefore,
          barrier.m_StateAfter,
          barrier.m_StagesBefore,
          barrier.m_StagesAfter,
          srcStages,
          dstStages));
      }

      SubmitBufferBarriers(ref_commandBuffer, srcStages, dstStages, vkBarriers.GetArrayPtr());
    }

    void TextureBarrier(
      const ezGALDeviceVulkan& /*device*/,
      vk::CommandBuffer& ref_commandBuffer,
      vk::Image image,
      const vk::ImageSubresourceRange& subresourceRange,
      ezBitflags<ezGALResourceState> stateBefore,
      ezBitflags<ezGALResourceState> stateAfter,
      ezBitflags<ezGALShaderStageFlags> stagesBefore,
      ezBitflags<ezGALShaderStageFlags> stagesAfter)
    {
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;
      const vk::ImageMemoryBarrier vkBarrier = MakeImageBarrier(image, subresourceRange, stateBefore, stateAfter, stagesBefore, stagesAfter, srcStages, dstStages);
      SubmitImageBarriers(ref_commandBuffer, srcStages, dstStages, ezMakeArrayPtr(&vkBarrier, 1));
    }

    void BufferBarrier(
      const ezGALDeviceVulkan& /*device*/,
      vk::CommandBuffer& ref_commandBuffer,
      vk::Buffer buffer,
      ezBitflags<ezGALResourceState> stateBefore,
      ezBitflags<ezGALResourceState> stateAfter,
      ezBitflags<ezGALShaderStageFlags> stagesBefore,
      ezBitflags<ezGALShaderStageFlags> stagesAfter)
    {
      vk::PipelineStageFlags srcStages;
      vk::PipelineStageFlags dstStages;
      const vk::BufferMemoryBarrier vkBarrier = MakeBufferBarrier(buffer, stateBefore, stateAfter, stagesBefore, stagesAfter, srcStages, dstStages);
      SubmitBufferBarriers(ref_commandBuffer, srcStages, dstStages, ezMakeArrayPtr(&vkBarrier, 1));
    }
  } // namespace Sync1
} // namespace

ezBarrierUtilsVulkan::ezBarrierUtilsVulkan(const ezGALDeviceVulkan& device, vk::CommandBuffer& ref_commandBuffer)
  : m_Device(device)
  , m_CommandBuffer(ref_commandBuffer)
{
}

void ezBarrierUtilsVulkan::TextureBarrier(ezArrayPtr<const ezGALTextureBarrier> barriers)
{
  if (barriers.IsEmpty())
    return;

  if (m_Device.GetExtensions().m_bSynchronization2)
    Sync2::TextureBarrier(m_Device, m_CommandBuffer, barriers);
  else
    Sync1::TextureBarrier(m_Device, m_CommandBuffer, barriers);
}

void ezBarrierUtilsVulkan::TextureBarrier(ezArrayPtr<const ezTextureBarrierVulkan> barriers)
{
  if (barriers.IsEmpty())
    return;

  if (m_Device.GetExtensions().m_bSynchronization2)
    Sync2::TextureBarrier(m_Device, m_CommandBuffer, barriers);
  else
    Sync1::TextureBarrier(m_Device, m_CommandBuffer, barriers);
}

void ezBarrierUtilsVulkan::BufferBarrier(ezArrayPtr<const ezGALBufferBarrier> barriers)
{
  if (barriers.IsEmpty())
    return;

  if (m_Device.GetExtensions().m_bSynchronization2)
    Sync2::BufferBarrier(m_Device, m_CommandBuffer, barriers);
  else
    Sync1::BufferBarrier(m_Device, m_CommandBuffer, barriers);
}

void ezBarrierUtilsVulkan::BufferBarrier(ezArrayPtr<const ezBufferBarrierVulkan> barriers)
{
  if (barriers.IsEmpty())
    return;

  if (m_Device.GetExtensions().m_bSynchronization2)
    Sync2::BufferBarrier(m_Device, m_CommandBuffer, barriers);
  else
    Sync1::BufferBarrier(m_Device, m_CommandBuffer, barriers);
}

void ezBarrierUtilsVulkan::TextureBarrier(
  ezGALTextureHandle hTexture,
  ezBitflags<ezGALResourceState> stateBefore,
  ezBitflags<ezGALResourceState> stateAfter,
  ezBitflags<ezGALShaderStageFlags> stagesBefore,
  ezBitflags<ezGALShaderStageFlags> stagesAfter)
{
  const ezGALTextureVulkan* pTexture = static_cast<const ezGALTextureVulkan*>(m_Device.GetTexture(hTexture)->GetParentResource());
  EZ_ASSERT_DEV(pTexture != nullptr, "Invalid texture handle.");

  TextureBarrier(pTexture->GetImage(), pTexture->GetFullRange(), stateBefore, stateAfter, stagesBefore, stagesAfter);
}

void ezBarrierUtilsVulkan::TextureBarrier(
  vk::Image image,
  const vk::ImageSubresourceRange& subresourceRange,
  ezBitflags<ezGALResourceState> stateBefore,
  ezBitflags<ezGALResourceState> stateAfter,
  ezBitflags<ezGALShaderStageFlags> stagesBefore,
  ezBitflags<ezGALShaderStageFlags> stagesAfter)
{
  if (m_Device.GetExtensions().m_bSynchronization2)
    Sync2::TextureBarrier(m_Device, m_CommandBuffer, image, subresourceRange, stateBefore, stateAfter, stagesBefore, stagesAfter);
  else
    Sync1::TextureBarrier(m_Device, m_CommandBuffer, image, subresourceRange, stateBefore, stateAfter, stagesBefore, stagesAfter);
}

void ezBarrierUtilsVulkan::BufferBarrier(
  ezGALBufferHandle hBuffer,
  ezBitflags<ezGALResourceState> stateBefore,
  ezBitflags<ezGALResourceState> stateAfter,
  ezBitflags<ezGALShaderStageFlags> stagesBefore,
  ezBitflags<ezGALShaderStageFlags> stagesAfter)
{
  const ezGALBufferVulkan* pBuffer = static_cast<const ezGALBufferVulkan*>(m_Device.GetBuffer(hBuffer));
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle.");

  BufferBarrier(pBuffer->GetVkBuffer(), stateBefore, stateAfter, stagesBefore, stagesAfter);
}

void ezBarrierUtilsVulkan::BufferBarrier(
  vk::Buffer buffer,
  ezBitflags<ezGALResourceState> stateBefore,
  ezBitflags<ezGALResourceState> stateAfter,
  ezBitflags<ezGALShaderStageFlags> stagesBefore,
  ezBitflags<ezGALShaderStageFlags> stagesAfter)
{
  if (m_Device.GetExtensions().m_bSynchronization2)
    Sync2::BufferBarrier(m_Device, m_CommandBuffer, buffer, stateBefore, stateAfter, stagesBefore, stagesAfter);
  else
    Sync1::BufferBarrier(m_Device, m_CommandBuffer, buffer, stateBefore, stateAfter, stagesBefore, stagesAfter);
}
