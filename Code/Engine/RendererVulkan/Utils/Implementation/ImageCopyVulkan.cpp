#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Shader/ShaderUtils.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/ImageCopyVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

namespace
{

} // namespace

ezImageCopyVulkan::ezImageCopyVulkan(ezGALDeviceVulkan& GALDeviceVulkan)
  : m_GALDeviceVulkan(GALDeviceVulkan)
{
}

ezImageCopyVulkan::~ezImageCopyVulkan()
{
  m_GALDeviceVulkan.DeleteLater(m_renderPass);

  m_GALDeviceVulkan.DestroyVertexDeclaration(m_hVertexDecl);

  m_GALDeviceVulkan.DeleteLater(m_proxyImage, m_proxyAlloc);
}

void ezImageCopyVulkan::Init(const ezGALTextureVulkan* pSource, const ezGALTextureVulkan* pTarget, bool useReadbackTarget, ezShaderUtils::ezBuiltinShaderType type)
{
  m_pSource = pSource;
  m_pTarget = pTarget;
  m_bUseReadbackTarget = useReadbackTarget;
  m_type = type;

  auto& targetDesc = m_pTarget->GetDescription();

  m_targetImage = m_pTarget->GetImage();
  m_targetFormat = m_pTarget->GetImageFormat();

  bool bTargetIsDepth = ezGALResourceFormat::IsDepthFormat(targetDesc.m_Format);
  m_bRequiresProxy = false;
  if (m_bUseReadbackTarget)
  {
    m_targetImage = m_pTarget->GetStagingTexture();
    m_targetFormat = m_pTarget->GetStagingImageFormat();
    bTargetIsDepth = ezConversionUtilsVulkan::IsDepthFormat(m_targetFormat);

    const vk::FormatProperties dstFormatProps = m_GALDeviceVulkan.GetVulkanPhysicalDevice().getFormatProperties(m_targetFormat);
    m_bRequiresProxy = !(dstFormatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment);
    EZ_ASSERT_DEV((dstFormatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment), "Writing to stagiing texture impossible.");
  }
  EZ_ASSERT_DEV(bTargetIsDepth == false, "Writing to depth is not implemented");

  // Create proxy render target if we can't render to target directly.
  if (m_bRequiresProxy)
  {
    vk::ImageCreateInfo proxyImageCreateInfo;

    proxyImageCreateInfo.imageType = targetDesc.m_Type == ezGALTextureType::Texture3D ? vk::ImageType::e3D : vk::ImageType::e2D;
    proxyImageCreateInfo.format = m_targetFormat;
    proxyImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    proxyImageCreateInfo.pQueueFamilyIndices = nullptr;
    proxyImageCreateInfo.queueFamilyIndexCount = 0;
    proxyImageCreateInfo.extent.width = targetDesc.m_uiWidth;
    proxyImageCreateInfo.extent.height = targetDesc.m_uiHeight;
    proxyImageCreateInfo.extent.depth = targetDesc.m_uiDepth;
    proxyImageCreateInfo.mipLevels = targetDesc.m_uiMipLevelCount;
    proxyImageCreateInfo.arrayLayers = (targetDesc.m_Type == ezGALTextureType::Texture2D ? targetDesc.m_uiArraySize : (targetDesc.m_uiArraySize * 6));
    proxyImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
    proxyImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
    proxyImageCreateInfo.usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eColorAttachment;
    proxyImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    ezVulkanAllocationCreateInfo allocInfo;
    allocInfo.m_usage = ezVulkanMemoryUsage::Auto;
    VK_ASSERT_DEV(ezMemoryAllocatorVulkan::CreateImage(proxyImageCreateInfo, allocInfo, m_proxyImage, m_proxyAlloc, &m_proxyAllocInfo));

    m_targetImage = m_proxyImage;
  }

  // Render pass
  {
    ezHybridArray<vk::AttachmentDescription, 4> attachments;
    ezHybridArray<vk::AttachmentReference, 4> colorAttachmentRefs;
    vk::AttachmentDescription& vkAttachment = attachments.ExpandAndGetRef();
    vkAttachment.format = m_targetFormat;
    vkAttachment.samples = ezConversionUtilsVulkan::GetSamples(targetDesc.m_SampleCount);
    vkAttachment.loadOp = vk::AttachmentLoadOp::eLoad; //#TODO_VULKAN we could replace this with don't care if we knew that all copy commands render to the entire sub-resource.
    vkAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    vkAttachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
    vkAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentReference& colorAttachment = colorAttachmentRefs.ExpandAndGetRef();
    colorAttachment.attachment = 0;
    colorAttachment.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = colorAttachmentRefs.GetCount();
    subpass.pColorAttachments = colorAttachmentRefs.GetData();
    subpass.pDepthStencilAttachment = nullptr;

    vk::SubpassDependency dependency;
    dependency.dstSubpass = 0;
    dependency.dstAccessMask |= vk::AccessFlagBits::eColorAttachmentWrite;

    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcAccessMask = {};
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;

    vk::RenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.attachmentCount = attachments.GetCount();
    renderPassCreateInfo.pAttachments = attachments.GetData();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;

    VK_ASSERT_DEV(m_GALDeviceVulkan.GetVulkanDevice().createRenderPass(&renderPassCreateInfo, nullptr, &m_renderPass));
  }

  // Pipeline
  {
    m_PipelineDesc.m_renderPass = m_renderPass;
    m_PipelineDesc.m_topology = ezGALPrimitiveTopology::Triangles;
    m_PipelineDesc.m_msaa = targetDesc.m_SampleCount;
    m_PipelineDesc.m_uiAttachmentCount = 1;


    ezShaderUtils::RequestBuiltinShader(type, m_shader);
    m_PipelineDesc.m_pCurrentRasterizerState = static_cast<const ezGALRasterizerStateVulkan*>(m_GALDeviceVulkan.GetRasterizerState(m_shader.m_hRasterizerState));
    m_PipelineDesc.m_pCurrentBlendState = static_cast<const ezGALBlendStateVulkan*>(m_GALDeviceVulkan.GetBlendState(m_shader.m_hBlendState));
    m_PipelineDesc.m_pCurrentDepthStencilState = static_cast<const ezGALDepthStencilStateVulkan*>(m_GALDeviceVulkan.GetDepthStencilState(m_shader.m_hDepthStencilState));
    m_PipelineDesc.m_pCurrentShader = static_cast<const ezGALShaderVulkan*>(m_GALDeviceVulkan.GetShader(m_shader.m_hActiveGALShader));

    {
      ezGALVertexDeclarationCreationDescription desc;
      desc.m_hShader = m_shader.m_hActiveGALShader;
      m_hVertexDecl = m_GALDeviceVulkan.CreateVertexDeclaration(desc);
      m_PipelineDesc.m_pCurrentVertexDecl = static_cast<const ezGALVertexDeclarationVulkan*>(m_GALDeviceVulkan.GetVertexDeclaration(m_hVertexDecl));
    }

    const ezGALShaderVulkan::DescriptorSetLayoutDesc& descriptorLayoutDesc = m_PipelineDesc.m_pCurrentShader->GetDescriptorSetLayout();
    m_LayoutDesc.m_layout = ezResourceCacheVulkan::RequestDescriptorSetLayout(descriptorLayoutDesc);
    m_PipelineDesc.m_layout = ezResourceCacheVulkan::RequestPipelineLayout(m_LayoutDesc);
    m_pipeline = ezResourceCacheVulkan::RequestGraphicsPipeline(m_PipelineDesc);
  }
}

void ezImageCopyVulkan::Copy(const ezVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const ezVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const ezVec3U32& extends)
{
  EZ_ASSERT_DEV(sourceOffset.IsZero(), "Offset not implemented yet.");
  EZ_ASSERT_DEV(targetOffset.IsZero(), "Offset not implemented yet.");

  vk::CommandBuffer commandBuffer = m_GALDeviceVulkan.GetCurrentCommandBuffer();
  ezPipelineBarrierVulkan& pipelineBarrier = m_GALDeviceVulkan.GetCurrentPipelineBarrier();

  // Barriers
  {
    const bool bSourceIsDepth = ezGALResourceFormat::IsDepthFormat(m_pSource->GetDescription().m_Format);
    pipelineBarrier.EnsureImageLayout(m_pSource, ezConversionUtilsVulkan::GetSubresourceRange(sourceLayers), bSourceIsDepth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eShaderRead);
    if (!m_bUseReadbackTarget)
    {
      pipelineBarrier.EnsureImageLayout(m_pTarget, ezConversionUtilsVulkan::GetSubresourceRange(targetLayers), vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite);
    }
    else
    {
      // staging / proxy to color attachment
      vk::ImageMemoryBarrier imageBarrier;
      imageBarrier.srcAccessMask = {};
      imageBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
      imageBarrier.oldLayout = vk::ImageLayout::eUndefined;
      imageBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
      imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imageBarrier.image = m_targetImage;
      imageBarrier.subresourceRange = ezConversionUtilsVulkan::GetSubresourceRange(targetLayers);
      imageBarrier.subresourceRange.aspectMask = m_pTarget->GetStagingAspectMask();

      commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &imageBarrier);
    }
    pipelineBarrier.Flush();
  }

  RenderInternal(sourceOffset, sourceLayers, targetOffset, targetLayers, extends);

  // Barriers
  {
    if (!m_bUseReadbackTarget)
    {
      // If the target is not a read-back target, we just assume a shader will read it next.
      pipelineBarrier.EnsureImageLayout(m_pTarget, ezConversionUtilsVulkan::GetSubresourceRange(targetLayers), vk::ImageLayout::eShaderReadOnlyOptimal, m_pTarget->GetUsedByPipelineStage(), m_pTarget->GetAccessMask());
    }
    else
    {
      // If we use the read-back target, we just assume that the target layout is memory read.
      if (m_bRequiresProxy)
      {
        {
          // proxy to transfer read
          vk::ImageMemoryBarrier imageBarrier;
          imageBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
          imageBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
          imageBarrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
          imageBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
          imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          imageBarrier.image = m_targetImage;
          imageBarrier.subresourceRange = ezConversionUtilsVulkan::GetSubresourceRange(targetLayers);
          imageBarrier.subresourceRange.aspectMask = m_pTarget->GetStagingAspectMask();

          commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &imageBarrier);
        }
        {
          // staging texture to transfer write
          vk::ImageMemoryBarrier imageBarrier;
          imageBarrier.srcAccessMask = {};
          imageBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
          imageBarrier.oldLayout = vk::ImageLayout::eUndefined;
          imageBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
          imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          imageBarrier.image = m_pTarget->GetStagingTexture();
          imageBarrier.subresourceRange = ezConversionUtilsVulkan::GetSubresourceRange(targetLayers);
          imageBarrier.subresourceRange.aspectMask = m_pTarget->GetStagingAspectMask();

          commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &imageBarrier);
        }

        // Copy proxy to target
        {
          ezHybridArray<vk::ImageCopy, 16> regions;
          {
            vk::Extent3D mipLevelSize = m_pTarget->GetMipLevelSize(targetLayers.mipLevel);

            vk::ImageCopy mipCopy;
            mipCopy.srcSubresource = targetLayers;
            mipCopy.dstSubresource = targetLayers;
            mipCopy.extent = mipLevelSize;

            regions.PushBack(mipCopy);
          }
          commandBuffer.copyImage(m_targetImage, vk::ImageLayout::eTransferSrcOptimal, m_pTarget->GetStagingTexture(), vk::ImageLayout::eTransferDstOptimal, vk::ArrayProxy(regions.GetCount(), regions.GetData()));
        }

        {
          // staging texture to memory read
          vk::ImageMemoryBarrier imageBarrier;
          imageBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
          imageBarrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
          imageBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
          imageBarrier.newLayout = vk::ImageLayout::eGeneral;
          imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          imageBarrier.image = m_pTarget->GetStagingTexture();
          imageBarrier.subresourceRange = m_pTarget->GetFullRange();
          imageBarrier.subresourceRange.aspectMask = m_pTarget->GetStagingAspectMask();

          commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &imageBarrier);
        }
      }
      else
      {
        // target to memory read
        vk::ImageMemoryBarrier imageBarrier;
        imageBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        imageBarrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
        imageBarrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
        imageBarrier.newLayout = vk::ImageLayout::eGeneral;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.image = m_targetImage;
        imageBarrier.subresourceRange = m_pTarget->GetFullRange();
        imageBarrier.subresourceRange.aspectMask = m_pTarget->GetStagingAspectMask();

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &imageBarrier);
      }
    }
  }
}

void ezImageCopyVulkan::RenderInternal(const ezVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const ezVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const ezVec3U32& extends)
{
  vk::CommandBuffer commandBuffer = m_GALDeviceVulkan.GetCurrentCommandBuffer();
  ezPipelineBarrierVulkan& pipelineBarrier = m_GALDeviceVulkan.GetCurrentPipelineBarrier();

  auto& sourceDesc = m_pSource->GetDescription();
  auto& targetDesc = m_pTarget->GetDescription();

  const bool bSourceIsDepth = ezGALResourceFormat::IsDepthFormat(sourceDesc.m_Format);

  vk::Image sourceImage = m_pSource->GetImage();
  vk::Format sourceFormat = m_pSource->GetImageFormat();

  vk::ImageView sourceView;
  vk::ImageView targetView;
  vk::Framebuffer frameBuffer;

  // Image Views
  {
    vk::ImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.format = sourceFormat;
    viewCreateInfo.image = sourceImage;
    viewCreateInfo.subresourceRange = ezConversionUtilsVulkan::GetSubresourceRange(sourceLayers);
    viewCreateInfo.subresourceRange.aspectMask &= ~vk::ImageAspectFlagBits::eStencil;
    viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(sourceDesc.m_Type, true);
    VK_ASSERT_DEV(m_GALDeviceVulkan.GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &sourceView));
    m_GALDeviceVulkan.SetDebugName("ImageCopy-SRV", sourceView);
  }
  {
    vk::ImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.format = m_targetFormat;
    viewCreateInfo.image = m_targetImage;
    viewCreateInfo.subresourceRange = ezConversionUtilsVulkan::GetSubresourceRange(targetLayers);
    viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageViewType(targetDesc.m_Type, true);
    VK_ASSERT_DEV(m_GALDeviceVulkan.GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &targetView));
    m_GALDeviceVulkan.SetDebugName("ImageCopy-RTV", targetView);
  }

  // Framebuffer
  {
    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = m_renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &targetView;
    framebufferInfo.width = extends.x;
    framebufferInfo.height = extends.y;
    framebufferInfo.layers = targetLayers.layerCount;
    VK_ASSERT_DEV(m_GALDeviceVulkan.GetVulkanDevice().createFramebuffer(&framebufferInfo, nullptr, &frameBuffer));
  }

  // Descriptor Set
  vk::DescriptorSet descriptorSet = ezDescriptorSetPoolVulkan::CreateDescriptorSet(m_LayoutDesc.m_layout);
  {
    ezHybridArray<vk::WriteDescriptorSet, 16> descriptorWrites;

    vk::DescriptorImageInfo sourceInfo;
    sourceInfo.imageLayout = bSourceIsDepth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
    sourceInfo.imageView = sourceView;

    ezArrayPtr<const ezGALShaderVulkan::BindingMapping> bindingMapping = m_PipelineDesc.m_pCurrentShader->GetBindingMapping();
    const ezUInt32 uiCount = bindingMapping.GetCount();
    for (ezUInt32 i = 0; i < uiCount; i++)
    {
      const ezGALShaderVulkan::BindingMapping& mapping = bindingMapping[i];
      vk::WriteDescriptorSet& write = descriptorWrites.ExpandAndGetRef();
      write.dstArrayElement = 0;
      write.descriptorType = mapping.m_descriptorType;
      write.dstBinding = mapping.m_uiTarget;
      write.dstSet = descriptorSet;
      write.descriptorCount = 1;
      switch (mapping.m_type)
      {
        case ezGALShaderVulkan::BindingMapping::ConstantBuffer:
        {
          //#TODO_VULKAN constant buffer for offset in the shader to allow region copy
          //const ezGALBufferVulkan* pBuffer = m_pBoundConstantBuffers[mapping.m_uiSource];
          //write.pBufferInfo = &pBuffer->GetBufferInfo();
        }
        break;
        case ezGALShaderVulkan::BindingMapping::ResourceView:
        {
          write.pImageInfo = &sourceInfo;
        }
      }
    }
    ezDescriptorSetPoolVulkan::UpdateDescriptorSet(descriptorSet, descriptorWrites);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PipelineDesc.m_layout, 0, 1, &descriptorSet, 0, nullptr);
  }

  // Render
  {
    {
      vk::RenderPassBeginInfo begin;
      begin.renderPass = m_PipelineDesc.m_renderPass;
      begin.framebuffer = frameBuffer;
      begin.renderArea.offset.setX(0).setY(0);
      begin.renderArea.extent.setWidth(extends.x).setHeight(extends.y);
      ezHybridArray<vk::ClearValue, EZ_GAL_MAX_RENDERTARGET_COUNT + 1> m_clearValues;
      vk::ClearValue& colorClear = m_clearValues.ExpandAndGetRef();
      ezColor col = ezColor::Pink;
      colorClear.color.setFloat32({col.r, col.g, col.b, col.a});
      begin.clearValueCount = m_clearValues.GetCount();
      begin.pClearValues = m_clearValues.GetData();

      commandBuffer.beginRenderPass(begin, vk::SubpassContents::eInline);
    }

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
    vk::Viewport viewport((float)targetOffset.x, (float)extends.y + (float)targetOffset.y, (float)extends.x, -(float)extends.y, 0, 1.0f);
    commandBuffer.setViewport(0, 1, &viewport);
    vk::Rect2D noScissor({int(viewport.x), int(viewport.y + viewport.height)}, {ezUInt32(viewport.width), ezUInt32(-viewport.height)});
    commandBuffer.setScissor(0, 1, &noScissor);
    commandBuffer.draw(3, targetLayers.layerCount, 0, 0);

    commandBuffer.endRenderPass();
  }

  m_GALDeviceVulkan.DeleteLater(sourceView);
  m_GALDeviceVulkan.DeleteLater(targetView);
  m_GALDeviceVulkan.DeleteLater(frameBuffer);
}
