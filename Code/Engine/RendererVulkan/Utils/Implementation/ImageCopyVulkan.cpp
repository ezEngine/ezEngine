#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Shader/ShaderUtils.h>
#include <RendererFoundation/State/PipelineCache.h>
#include <RendererVulkan/Pools/TransientDescriptorSetPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/GraphicsPipelineVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/ImageCopyVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

template <>
struct ezHashHelper<ezGALShaderHandle>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezGALShaderHandle value)
  {
    return ezHashHelper<ezGALShaderHandle::IdType::StorageType>::Hash(value.GetInternalID().m_Data);
  }

  EZ_ALWAYS_INLINE static bool Equal(ezGALShaderHandle a, ezGALShaderHandle b)
  {
    return ezHashHelper<ezGALShaderHandle::IdType::StorageType>::Equal(a.GetInternalID().m_Data, b.GetInternalID().m_Data);
  }
};

template <>
struct ezHashHelper<vk::Image>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(vk::Image value)
  {
    return ezHashHelper<void*>::Hash((VkImage)value);
  }

  EZ_ALWAYS_INLINE static bool Equal(vk::Image a, vk::Image b)
  {
    return a == b;
  }
};

template <>
struct ezHashHelper<ezImageCopyVulkan::FramebufferCacheKey>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezImageCopyVulkan::FramebufferCacheKey& value)
  {
    ezHashStreamWriter32 writer;
    writer << (VkRenderPass)value.m_renderPass;
    writer << (VkImageView)value.m_targetView;
    writer << value.m_extends.x;
    writer << value.m_extends.y;
    writer << value.m_layerCount;
    return writer.GetHashValue();
  }

  EZ_ALWAYS_INLINE static bool Equal(const ezImageCopyVulkan::FramebufferCacheKey& a, const ezImageCopyVulkan::FramebufferCacheKey& b)
  {
    return a.m_renderPass == b.m_renderPass && a.m_targetView == b.m_targetView && a.m_extends == b.m_extends && a.m_layerCount == b.m_layerCount;
  }
};

template <>
struct ezHashHelper<ezImageCopyVulkan::ImageViewCacheKey>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezImageCopyVulkan::ImageViewCacheKey& value)
  {
    ezHashStreamWriter32 writer;
    writer << (VkImage)value.m_image;
    writer << static_cast<uint32_t>(value.m_subresourceLayers.aspectMask);
    writer << value.m_subresourceLayers.baseArrayLayer;
    writer << value.m_subresourceLayers.layerCount;
    writer << value.m_subresourceLayers.mipLevel;
    return writer.GetHashValue();
  }

  EZ_ALWAYS_INLINE static bool Equal(const ezImageCopyVulkan::ImageViewCacheKey& a, const ezImageCopyVulkan::ImageViewCacheKey& b)
  {
    return a.m_image == b.m_image && a.m_subresourceLayers == b.m_subresourceLayers;
  }
};

template <>
struct ezHashHelper<ezShaderUtils::ezBuiltinShaderType>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezShaderUtils::ezBuiltinShaderType& value)
  {
    ezHashStreamWriter32 writer;
    writer << ezConversionUtilsVulkan::GetUnderlyingValue(value);
    return writer.GetHashValue();
  }

  EZ_ALWAYS_INLINE static bool Equal(const ezShaderUtils::ezBuiltinShaderType& a, const ezShaderUtils::ezBuiltinShaderType& b)
  {
    return a == b;
  }
};

ezUniquePtr<ezImageCopyVulkan::Cache> ezImageCopyVulkan::s_cache;

ezImageCopyVulkan::Cache::Cache(ezAllocator* pAllocator)
  : m_vertexDeclarations(pAllocator)
  , m_sourceImageViews(pAllocator)
  , m_imageToSourceImageViewCacheKey(pAllocator)
  , m_targetImageViews(pAllocator)
  , m_imageToTargetImageViewCacheKey(pAllocator)
  , m_framebuffers(pAllocator)
  , m_shaders(pAllocator)
{
}

ezImageCopyVulkan::Cache::~Cache() = default;

ezImageCopyVulkan::ezImageCopyVulkan(ezGALDeviceVulkan& GALDeviceVulkan)
  : m_GALDeviceVulkan(GALDeviceVulkan)
{
}

ezImageCopyVulkan::~ezImageCopyVulkan() = default;

void ezImageCopyVulkan::Initialize(ezGALDeviceVulkan& GALDeviceVulkan)
{
  s_cache = EZ_NEW(GALDeviceVulkan.GetAllocator(), ezImageCopyVulkan::Cache, GALDeviceVulkan.GetAllocator());

  s_cache->m_onBeforeImageDeletedSubscription = GALDeviceVulkan.OnBeforeImageDestroyed.AddEventHandler(ezMakeDelegate(OnBeforeImageDestroyed));
}

void ezImageCopyVulkan::DeInitialize(ezGALDeviceVulkan& GALDeviceVulkan)
{
  GALDeviceVulkan.OnBeforeImageDestroyed.RemoveEventHandler(s_cache->m_onBeforeImageDeletedSubscription);
  for (auto& kv : (s_cache->m_vertexDeclarations))
  {
    GALDeviceVulkan.DestroyVertexDeclaration(kv.Value());
  }
  for (auto& kv : (s_cache->m_sourceImageViews))
  {
    GALDeviceVulkan.GetVulkanDevice().destroyImageView(kv.Value());
  }
  for (auto& kv : (s_cache->m_targetImageViews))
  {
    GALDeviceVulkan.GetVulkanDevice().destroyImageView(kv.Value());
  }
  for (auto& kv : (s_cache->m_framebuffers))
  {
    GALDeviceVulkan.GetVulkanDevice().destroyFramebuffer(kv.Value());
  }
  s_cache->m_shaders.Clear();
  s_cache = nullptr;
}

void ezImageCopyVulkan::OnBeforeImageDestroyed(ezGALDeviceVulkan::OnBeforeImageDestroyedData data)
{
  if (auto it = s_cache->m_imageToSourceImageViewCacheKey.Find(data.image); it.IsValid())
  {
    data.GALDeviceVulkan.GetVulkanDevice().destroyImageView(it.Value().m_imageView);

    ImageViewCacheKey cacheKey{data.image, it.Value().m_subresourceLayers};

    bool removed = s_cache->m_sourceImageViews.Remove(cacheKey);
    EZ_IGNORE_UNUSED(removed);
    EZ_ASSERT_DEV(removed, "m_imageToSourceImageViewCacheKey and m_sourceImageViews should always be in sync");
    s_cache->m_imageToSourceImageViewCacheKey.Remove(it);
  }
  if (auto it = s_cache->m_imageToTargetImageViewCacheKey.Find(data.image); it.IsValid())
  {
    data.GALDeviceVulkan.GetVulkanDevice().destroyImageView(it.Value().m_imageView);

    ImageViewCacheKey cacheKey{data.image, it.Value().m_subresourceLayers};

    bool removed = s_cache->m_targetImageViews.Remove(cacheKey);
    EZ_IGNORE_UNUSED(removed);
    EZ_ASSERT_DEV(removed, "m_imageToTargetImageViewCacheKey and m_targetImageViews should always be in sync");
    s_cache->m_imageToTargetImageViewCacheKey.Remove(it);
  }
}

void ezImageCopyVulkan::Init(const ezGALTextureVulkan* pSource, const ezGALTextureVulkan* pTarget, ezShaderUtils::ezBuiltinShaderType type)
{
  m_pSource = pSource;
  m_pTarget = pTarget;
  m_type = type;

  auto& targetDesc = m_pTarget->GetDescription();

  vk::Format targetFormat = m_pTarget->GetImageFormat();

  bool bTargetIsDepth = ezConversionUtilsVulkan::IsDepthFormat(targetFormat);
  EZ_ASSERT_DEV(bTargetIsDepth == false, "Writing to depth is not implemented");

  // Render pass
  {
    m_PipelineDesc.m_RenderPass = ezGALRenderPassDescriptor();
    m_PipelineDesc.m_RenderPass.m_Msaa = targetDesc.m_SampleCount;
    m_PipelineDesc.m_RenderPass.m_uiRTCount = 1;
    m_PipelineDesc.m_RenderPass.m_ColorFormat[0] = m_pTarget->GetDescription().m_Format;
    m_renderPass = ezResourceCacheVulkan::RequestRenderPass(m_PipelineDesc.m_RenderPass);
  }

  // Pipeline
  {

    if (auto it = s_cache->m_shaders.Find(type); it.IsValid())
    {
      m_shader = it.Value();
      m_PipelineDesc.m_hRasterizerState = m_shader.m_hRasterizerState;
      m_PipelineDesc.m_hBlendState = m_shader.m_hBlendState;
      m_PipelineDesc.m_hDepthStencilState = m_shader.m_hDepthStencilState;
      m_PipelineDesc.m_hShader = m_shader.m_hActiveGALShader;

      if (m_PipelineDesc.m_hRasterizerState.IsInvalidated() || m_PipelineDesc.m_hBlendState.IsInvalidated() || m_PipelineDesc.m_hDepthStencilState.IsInvalidated() || m_PipelineDesc.m_hShader.IsInvalidated())
      {
        s_cache->m_shaders.Clear();
        for (auto& kv : (s_cache->m_vertexDeclarations))
        {
          m_GALDeviceVulkan.DestroyVertexDeclaration(kv.Value());
        }
        s_cache->m_vertexDeclarations.Clear();
      }
    }

    if (m_PipelineDesc.m_hRasterizerState.IsInvalidated() || m_PipelineDesc.m_hBlendState.IsInvalidated() || m_PipelineDesc.m_hDepthStencilState.IsInvalidated() || m_PipelineDesc.m_hShader.IsInvalidated())
    {
      ezShaderUtils::RequestBuiltinShader(type, m_shader);
      s_cache->m_shaders.Insert(type, m_shader);

      m_PipelineDesc.m_hRasterizerState = m_shader.m_hRasterizerState;
      m_PipelineDesc.m_hBlendState = m_shader.m_hBlendState;
      m_PipelineDesc.m_hDepthStencilState = m_shader.m_hDepthStencilState;
      m_PipelineDesc.m_hShader = m_shader.m_hActiveGALShader;

      EZ_ASSERT_DEV(!m_PipelineDesc.m_hRasterizerState.IsInvalidated() && !m_PipelineDesc.m_hBlendState.IsInvalidated() && !m_PipelineDesc.m_hDepthStencilState.IsInvalidated() && !m_PipelineDesc.m_hShader.IsInvalidated(), "");
    }

    m_PipelineDesc.m_Topology = ezGALPrimitiveTopology::Triangles;

    // Vertex declaration
    {
      if (auto it = s_cache->m_vertexDeclarations.Find(m_shader.m_hActiveGALShader); it.IsValid())
      {
        m_hVertexDecl = it.Value();
      }
      else
      {
        ezGALVertexDeclarationCreationDescription desc;
        desc.m_hShader = m_shader.m_hActiveGALShader;
        m_hVertexDecl = m_GALDeviceVulkan.CreateVertexDeclaration(desc);
        s_cache->m_vertexDeclarations.Insert(m_shader.m_hActiveGALShader, m_hVertexDecl);
      }
      m_PipelineDesc.m_hVertexDeclaration = m_hVertexDecl;
    }

    m_hGalPipeline = ezGALPipelineCache::GetPipeline(m_PipelineDesc);
    const ezGALGraphicsPipelineVulkan* pGraphicsPipeline = static_cast<const ezGALGraphicsPipelineVulkan*>(m_GALDeviceVulkan.GetGraphicsPipeline(m_hGalPipeline));
    m_pipeline = pGraphicsPipeline->GetPipeline();
    m_pShader = static_cast<const ezGALShaderVulkan*>(m_GALDeviceVulkan.GetShader(pGraphicsPipeline->GetDescription().m_hShader));
  }
}

void ezImageCopyVulkan::Copy(const ezVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const ezVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const ezVec3U32& extends)
{
  EZ_ASSERT_DEV(sourceOffset.IsZero(), "Offset not implemented yet.");
  EZ_ASSERT_DEV(targetOffset.IsZero(), "Offset not implemented yet.");
  if (m_type == ezShaderUtils::ezBuiltinShaderType::CopyImage || m_type == ezShaderUtils::ezBuiltinShaderType::CopyImage)
  {
    EZ_ASSERT_DEV(sourceLayers.layerCount == 1 && targetLayers.layerCount == 1, "If ezBuiltinShaderType is not one of the array variants, layerCount must be 1.");
  }

  vk::CommandBuffer commandBuffer = m_GALDeviceVulkan.GetCurrentCommandBuffer();
  ezPipelineBarrierVulkan& pipelineBarrier = m_GALDeviceVulkan.GetCurrentPipelineBarrier();

  // Barriers
  {
    const bool bSourceIsDepth = ezConversionUtilsVulkan::IsDepthFormat(m_pSource->GetImageFormat());
    pipelineBarrier.EnsureImageLayout(m_pSource, ezConversionUtilsVulkan::GetSubresourceRange(sourceLayers), ezConversionUtilsVulkan::GetDefaultLayout(m_pSource->GetImageFormat()), vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eShaderRead);
    pipelineBarrier.EnsureImageLayout(m_pTarget, ezConversionUtilsVulkan::GetSubresourceRange(targetLayers), vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead);
    pipelineBarrier.Flush();
  }

  RenderInternal(sourceOffset, sourceLayers, targetOffset, targetLayers, extends);
}

void ezImageCopyVulkan::RenderInternal(const ezVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const ezVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const ezVec3U32& extends)
{
  vk::CommandBuffer commandBuffer = m_GALDeviceVulkan.GetCurrentCommandBuffer();
  ezPipelineBarrierVulkan& pipelineBarrier = m_GALDeviceVulkan.GetCurrentPipelineBarrier();

  auto& sourceDesc = m_pSource->GetDescription();
  auto& targetDesc = m_pTarget->GetDescription();

  vk::Image sourceImage = m_pSource->GetImage();
  vk::Format sourceFormat = m_pSource->GetImageFormat();

  vk::Image targetImage = m_pTarget->GetImage();
  vk::Format targetFormat = m_pTarget->GetImageFormat();

  vk::ImageView sourceView;
  vk::ImageView targetView;
  vk::Framebuffer frameBuffer;

  // Image Views
  {
    ImageViewCacheKey cacheKey = {};
    cacheKey.m_image = sourceImage;
    cacheKey.m_subresourceLayers = sourceLayers;

    if (auto it = s_cache->m_sourceImageViews.Find(cacheKey); it.IsValid())
    {
      sourceView = it.Value();
    }
    else
    {

      vk::ImageViewCreateInfo viewCreateInfo;
      viewCreateInfo.format = sourceFormat;
      viewCreateInfo.image = sourceImage;
      viewCreateInfo.subresourceRange = ezConversionUtilsVulkan::GetSubresourceRange(sourceLayers);
      viewCreateInfo.subresourceRange.aspectMask &= ~vk::ImageAspectFlagBits::eStencil;
      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageArrayViewType(sourceDesc.m_Type);
      if (viewCreateInfo.viewType == vk::ImageViewType::eCube || viewCreateInfo.viewType == vk::ImageViewType::eCubeArray)
      {
        viewCreateInfo.viewType = vk::ImageViewType::e2DArray;
      }
      VK_ASSERT_DEV(m_GALDeviceVulkan.GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &sourceView));
      m_GALDeviceVulkan.SetDebugName("ImageCopy-SRV", sourceView);
      s_cache->m_sourceImageViews.Insert(cacheKey, sourceView);
      s_cache->m_imageToSourceImageViewCacheKey.Insert(cacheKey.m_image, ImageViewCacheValue{cacheKey.m_subresourceLayers, sourceView});
    }
  }
  {
    ImageViewCacheKey cacheKey = {};
    cacheKey.m_image = targetImage;
    cacheKey.m_subresourceLayers = targetLayers;

    if (auto it = s_cache->m_targetImageViews.Find(cacheKey); it.IsValid())
    {
      targetView = it.Value();
    }
    else
    {

      vk::ImageViewCreateInfo viewCreateInfo;
      viewCreateInfo.format = targetFormat;
      viewCreateInfo.image = targetImage;
      viewCreateInfo.subresourceRange = ezConversionUtilsVulkan::GetSubresourceRange(targetLayers);
      viewCreateInfo.viewType = ezConversionUtilsVulkan::GetImageArrayViewType(targetDesc.m_Type);
      if (viewCreateInfo.viewType == vk::ImageViewType::eCube || viewCreateInfo.viewType == vk::ImageViewType::eCubeArray)
      {
        viewCreateInfo.viewType = vk::ImageViewType::e2DArray;
      }
      VK_ASSERT_DEV(m_GALDeviceVulkan.GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &targetView));
      m_GALDeviceVulkan.SetDebugName("ImageCopy-RTV", targetView);
      s_cache->m_targetImageViews.Insert(cacheKey, targetView);
      s_cache->m_imageToTargetImageViewCacheKey.Insert(cacheKey.m_image, ImageViewCacheValue{cacheKey.m_subresourceLayers, targetView});
    }
  }

  // Framebuffer
  {
    FramebufferCacheKey cacheEntry = {};
    cacheEntry.m_renderPass = m_renderPass;
    cacheEntry.m_targetView = targetView;
    cacheEntry.m_extends = extends;
    cacheEntry.m_layerCount = targetLayers.layerCount;

    if (auto it = s_cache->m_framebuffers.Find(cacheEntry); it.IsValid())
    {
      frameBuffer = it.Value();
    }
    else
    {
      vk::FramebufferCreateInfo framebufferInfo;
      framebufferInfo.renderPass = m_renderPass;
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments = &targetView;
      framebufferInfo.width = extends.x;
      framebufferInfo.height = extends.y;
      framebufferInfo.layers = targetLayers.layerCount;
      VK_ASSERT_DEV(m_GALDeviceVulkan.GetVulkanDevice().createFramebuffer(&framebufferInfo, nullptr, &frameBuffer));

      s_cache->m_framebuffers.Insert(cacheEntry, frameBuffer);
    }
  }

  // Descriptor Set
  vk::DescriptorSet descriptorSet = ezTransientDescriptorSetPoolVulkan::CreateTransientDescriptorSet(m_pShader->GetDescriptorSetLayout(0));
  {
    ezHybridArray<vk::WriteDescriptorSet, 16> descriptorWrites;

    vk::DescriptorImageInfo sourceInfo;
    sourceInfo.imageLayout = ezConversionUtilsVulkan::GetDefaultLayout(m_pSource->GetImageFormat());
    sourceInfo.imageView = sourceView;

    ezArrayPtr<const ezShaderResourceBinding> bindingMapping = m_pShader->GetBindings();
    const ezUInt32 uiCount = bindingMapping.GetCount();
    for (ezUInt32 i = 0; i < uiCount; i++)
    {
      const ezShaderResourceBinding& mapping = bindingMapping[i];
      vk::WriteDescriptorSet& write = descriptorWrites.ExpandAndGetRef();
      write.dstArrayElement = 0;
      write.descriptorType = ezConversionUtilsVulkan::GetDescriptorType(mapping.m_ResourceType);
      write.dstBinding = mapping.m_iSlot;
      write.dstSet = descriptorSet;
      write.descriptorCount = 1;
      switch (mapping.m_ResourceType)
      {
        case ezGALShaderResourceType::ConstantBuffer:
        {
          // #TODO_VULKAN constant buffer for offset in the shader to allow region copy
          // const ezGALBufferVulkan* pBuffer = m_pBoundConstantBuffers[mapping.m_uiSource];
          // write.pBufferInfo = &pBuffer->GetBufferInfo();
          EZ_REPORT_FAILURE("ConstantBuffer resource type not supported in copy shader.");
        }
        break;
        case ezGALShaderResourceType::Texture:
        {
          write.pImageInfo = &sourceInfo;
        }
        break;
        default:
          EZ_REPORT_FAILURE("Resource type '{}' not supported in copy shader.", mapping.m_ResourceType);
          break;
      }
    }
    ezTransientDescriptorSetPoolVulkan::UpdateDescriptorSet(descriptorSet, descriptorWrites);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pShader->GetVkPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
  }

  // Render
  {
    {
      vk::RenderPassBeginInfo begin;
      begin.renderPass = ezResourceCacheVulkan::RequestRenderPass(m_PipelineDesc.m_RenderPass);
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
}
