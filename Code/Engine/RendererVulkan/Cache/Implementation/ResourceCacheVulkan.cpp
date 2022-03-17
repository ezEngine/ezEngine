#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Basics.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

ezGALDeviceVulkan* ezResourceCacheVulkan::s_pDevice;
vk::Device ezResourceCacheVulkan::s_device;
ezHashTable<ezResourceCacheVulkan::RenderPassDesc, vk::RenderPass, ezResourceCacheVulkan::ezRenderPassHash> ezResourceCacheVulkan::s_renderPasses;
ezHashTable<ezResourceCacheVulkan::FramebufferDesc, vk::Framebuffer, ezResourceCacheVulkan::ezFrameBufferHash> ezResourceCacheVulkan::s_frameBuffers;

void ezResourceCacheVulkan::Initialize(ezGALDeviceVulkan* pDevice, vk::Device device)
{
  s_pDevice = pDevice;
  s_device = device;
}

void ezResourceCacheVulkan::DeInitialize()
{
  for (auto it : s_renderPasses)
  {
    s_device.destroyRenderPass(it.Value(), nullptr);
  }
  s_renderPasses.Clear();
  s_renderPasses.Compact();

  for (auto it : s_frameBuffers)
  {
    s_device.destroyFramebuffer(it.Value(), nullptr);
  }
  s_frameBuffers.Clear();
  s_frameBuffers.Compact();

  s_device = nullptr;
}

void ezResourceCacheVulkan::GetRenderPassDesc(const ezGALRenderingSetup& renderingSetup, RenderPassDesc& out_desc)
{
  const bool bHasDepth = !renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
  const ezUInt32 uiColorCount = renderingSetup.m_RenderTargetSetup.HasRenderTargets() ? renderingSetup.m_RenderTargetSetup.GetMaxRenderTargetIndex() + 1 : 0;
  out_desc.attachments.Clear();

  if (bHasDepth)
  {
    const ezGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const ezGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget()));

    auto hTexture = pRenderTargetView->GetDescription().m_hTexture;
    const ezGALTexture* pTex = s_pDevice->GetTexture(hTexture);

    const ezGALTextureCreationDescription& texDesc = pTex->GetDescription();
    ezEnum<ezGALResourceFormat> format = texDesc.m_Format;
    const auto& formatInfo = s_pDevice->GetFormatLookupTable().GetFormatInfo(format);

    AttachmentDesc& depthAttachment = out_desc.attachments.ExpandAndGetRef();
    depthAttachment.format = formatInfo.m_eRenderTarget;
    depthAttachment.samples = ezConversionUtilsVulkan::GetSamples(texDesc.m_SampleCount);

    depthAttachment.loadOp = renderingSetup.m_bClearDepth ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;

    if (format == ezGALResourceFormat::Enum::D24S8)
    {
      depthAttachment.stencilLoadOp = renderingSetup.m_bClearStencil ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
      depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eStore;
    }
    else
    {
      depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
      depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    }
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
  }

  for (size_t i = 0; i < uiColorCount; i++)
  {
    auto colorRT = renderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<ezUInt8>(i));
    const ezGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const ezGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(colorRT));

    auto hTexture = pRenderTargetView->GetDescription().m_hTexture;
    const ezGALTexture* pTex = s_pDevice->GetTexture(hTexture);

    const ezGALTextureCreationDescription& texDesc = pTex->GetDescription();
    ezEnum<ezGALResourceFormat> format = texDesc.m_Format;
    const auto& formatInfo = s_pDevice->GetFormatLookupTable().GetFormatInfo(format);

    AttachmentDesc& colorAttachment = out_desc.attachments.ExpandAndGetRef();
    colorAttachment.format = formatInfo.m_eRenderTarget;
    colorAttachment.samples = ezConversionUtilsVulkan::GetSamples(texDesc.m_SampleCount);

    if (renderingSetup.m_uiRenderTargetClearMask & (1u << i))
    {
      colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    }
    else
    {
      colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
    }
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
  }
}

EZ_DEFINE_AS_POD_TYPE(vk::AttachmentDescription);
EZ_DEFINE_AS_POD_TYPE(vk::AttachmentReference);

vk::RenderPass ezResourceCacheVulkan::RequestRenderPass(const RenderPassDesc& desc)
{
  if (const vk::RenderPass* pPass = s_renderPasses.GetValue(desc))
  {
    return *pPass;
  }

  ezHybridArray<vk::AttachmentDescription, 4> attachments;
  ezHybridArray<vk::AttachmentReference, 1> depthAttachmentRefs;
  ezHybridArray<vk::AttachmentReference, 4> colorAttachmentRefs;

  const ezUInt32 uiCount = desc.attachments.GetCount();
  for (ezUInt32 i = 0; i < uiCount; i++)
  {
    const AttachmentDesc& attachmentDesc = desc.attachments[i];
    vk::AttachmentDescription& vkAttachment = attachments.ExpandAndGetRef();
    vkAttachment.format = attachmentDesc.format;
    vkAttachment.samples = attachmentDesc.samples;
    vkAttachment.loadOp = attachmentDesc.loadOp;
    vkAttachment.storeOp = attachmentDesc.storeOp;
    vkAttachment.stencilLoadOp = attachmentDesc.stencilLoadOp;
    vkAttachment.stencilStoreOp = attachmentDesc.stencilStoreOp;
    vkAttachment.initialLayout = attachmentDesc.initialLayout;

    const bool bIsDepth = ezConversionUtilsVulkan::IsDepthFormat(attachmentDesc.format);
    if (bIsDepth)
    {
      vkAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
      vk::AttachmentReference& depthAttachment = depthAttachmentRefs.ExpandAndGetRef();
      depthAttachment.attachment = i;
      depthAttachment.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    }
    else
    {
      vkAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
      vk::AttachmentReference& colorAttachment = colorAttachmentRefs.ExpandAndGetRef();
      colorAttachment.attachment = i;
      colorAttachment.layout = vk::ImageLayout::eColorAttachmentOptimal;
    }
  }

  EZ_ASSERT_DEV(depthAttachmentRefs.GetCount() <= 1, "There can be no more than 1 depth attachment.");
  const bool bHasColor = !colorAttachmentRefs.IsEmpty();
  const bool bHasDepth = !depthAttachmentRefs.IsEmpty();
  vk::SubpassDescription subpass;
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount = colorAttachmentRefs.GetCount();
  subpass.pColorAttachments = bHasColor ? colorAttachmentRefs.GetData() : nullptr;
  subpass.pDepthStencilAttachment = bHasDepth ? depthAttachmentRefs.GetData() : nullptr;

  vk::SubpassDependency dependency;
  dependency.dstSubpass = 0;
  if (bHasColor)
    dependency.dstAccessMask |= vk::AccessFlagBits::eColorAttachmentWrite;

  if (bHasDepth)
    dependency.dstAccessMask |= vk::AccessFlagBits::eDepthStencilAttachmentWrite;

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

  vk::RenderPass renderPass;
  VK_LOG_ERROR(s_device.createRenderPass(&renderPassCreateInfo, nullptr, &renderPass));

  s_renderPasses.Insert(desc, renderPass);
  return renderPass;
}

void ezResourceCacheVulkan::GetFrameBufferDesc(vk::RenderPass renderPass, const ezGALRenderingSetup& renderingSetup, FramebufferDesc& out_desc)
{
  const bool bHasDepth = !renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
  const ezUInt32 uiColorCount = renderingSetup.m_RenderTargetSetup.HasRenderTargets() ? renderingSetup.m_RenderTargetSetup.GetMaxRenderTargetIndex() + 1 : 0;

  out_desc.renderPass = renderPass;
  if (bHasDepth)
  {
    const ezGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const ezGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget()));
    out_desc.attachments.PushBack(pRenderTargetView->GetImageView());

    auto hTexture = pRenderTargetView->GetDescription().m_hTexture;
    const ezGALTexture* pTex = s_pDevice->GetTexture(hTexture);
    const ezGALTextureCreationDescription& texDesc = pTex->GetDescription();

    out_desc.width = texDesc.m_uiWidth;
    out_desc.height = texDesc.m_uiHeight;
    out_desc.layers = 1;
  }
  for (size_t i = 0; i < uiColorCount; i++)
  {
    auto colorRT = renderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<ezUInt8>(i));
    const ezGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const ezGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(colorRT));
    out_desc.attachments.PushBack(pRenderTargetView->GetImageView());

    auto hTexture = pRenderTargetView->GetDescription().m_hTexture;
    const ezGALTexture* pTex = s_pDevice->GetTexture(hTexture);
    const ezGALTextureCreationDescription& texDesc = pTex->GetDescription();

    out_desc.width = texDesc.m_uiWidth;
    out_desc.height = texDesc.m_uiHeight;
    out_desc.layers = 1;
  }
}

vk::Framebuffer ezResourceCacheVulkan::RequestFrameBuffer(const FramebufferDesc& desc)
{
  if (const vk::Framebuffer* pFrameBuffer = s_frameBuffers.GetValue(desc))
  {
    return *pFrameBuffer;
  }

  vk::FramebufferCreateInfo framebufferInfo;
  framebufferInfo.renderPass = desc.renderPass;
  framebufferInfo.attachmentCount = desc.attachments.GetCount();
  framebufferInfo.pAttachments = desc.attachments.GetData();
  framebufferInfo.width = desc.width;
  framebufferInfo.height = desc.height;
  framebufferInfo.layers = desc.layers;

  vk::Framebuffer frameBuffer;
  VK_LOG_ERROR(s_device.createFramebuffer(&framebufferInfo, nullptr, &frameBuffer));

  s_frameBuffers.Insert(desc, frameBuffer);
  return frameBuffer;
}

template <typename T, typename R = typename std::underlying_type<T>::type>
R GetUnderlyingValue(T value)
{
  return static_cast<std::underlying_type<T>::type>(value);
}

template <typename T>
auto GetUnderlyingFlagsValue(T value)
{
  return static_cast<T::MaskType>(value);
}

ezUInt32 ezResourceCacheVulkan::ezRenderPassHash::Hash(const RenderPassDesc& renderingSetup)
{
  ezHashStreamWriter32 writer;
  for (const auto& attachment : renderingSetup.attachments)
  {
    writer << GetUnderlyingValue(attachment.format);
    writer << GetUnderlyingValue(attachment.samples);
    writer << GetUnderlyingFlagsValue(attachment.usage);
    writer << GetUnderlyingValue(attachment.initialLayout);
    writer << GetUnderlyingValue(attachment.loadOp);
    writer << GetUnderlyingValue(attachment.storeOp);
    writer << GetUnderlyingValue(attachment.stencilLoadOp);
    writer << GetUnderlyingValue(attachment.stencilStoreOp);
  }
  return writer.GetHashValue();
}

bool ezResourceCacheVulkan::ezRenderPassHash::Equal(const RenderPassDesc& a, const RenderPassDesc& b)
{
  const ezUInt32 uiCount = a.attachments.GetCount();
  if (uiCount != a.attachments.GetCount())
    return false;

  for (ezUInt32 i = 0; i < uiCount; i++)
  {
    const AttachmentDesc& aE = a.attachments[i];
    const AttachmentDesc& bE = b.attachments[i];

    if (aE.format != bE.format || aE.samples != bE.samples || aE.usage != bE.usage || aE.initialLayout != bE.initialLayout || aE.loadOp != bE.loadOp || aE.storeOp != bE.storeOp || aE.stencilLoadOp != bE.stencilLoadOp || aE.stencilStoreOp != bE.stencilStoreOp)
      return false;
  }

  return true;
}

ezUInt32 ezResourceCacheVulkan::ezFrameBufferHash::Hash(const FramebufferDesc& framebufferDesc)
{
  ezHashStreamWriter32 writer;
  writer << (void*)framebufferDesc.renderPass;
  writer << framebufferDesc.width;
  writer << framebufferDesc.height;
  writer << framebufferDesc.layers;
  ezUInt32 uiCount = framebufferDesc.attachments.GetCount();
  writer << uiCount;
  for (ezUInt32 i = 0; i < uiCount; i++)
  {
    writer << (void*)framebufferDesc.attachments[i];
  }
  return writer.GetHashValue();
}

bool ezResourceCacheVulkan::ezFrameBufferHash::Equal(const FramebufferDesc& a, const FramebufferDesc& b)
{
  const bool bEqual = a.renderPass == b.renderPass && a.height == b.height && a.width == b.width && a.layers == b.layers && a.attachments.GetCount() == b.attachments.GetCount();
  if (bEqual)
  {
    const ezUInt32 uiCount = a.attachments.GetCount();
    for (ezUInt32 i = 0; i < uiCount; i++)
    {
      if (a.attachments[i] != b.attachments[i])
        return false;
    }
  }
  return bEqual;
}
