#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Basics.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

ezGALDeviceVulkan* ezResourceCacheVulkan::s_pDevice;
vk::Device ezResourceCacheVulkan::s_device;

ezHashTable<ezGALRenderPassDescriptor, vk::RenderPass, ezResourceCacheVulkan::ResourceCacheHash> ezResourceCacheVulkan::s_renderPasses;
ezHashTable<ezResourceCacheVulkan::FramebufferKey, vk::Framebuffer, ezResourceCacheVulkan::ResourceCacheHash> ezResourceCacheVulkan::s_frameBuffers;
ezHashTable<ezResourceCacheVulkan::PipelineLayoutDesc, vk::PipelineLayout, ezResourceCacheVulkan::ResourceCacheHash> ezResourceCacheVulkan::s_pipelineLayouts;
ezResourceCacheVulkan::GraphicsPipelineMap ezResourceCacheVulkan::s_graphicsPipelines;
ezResourceCacheVulkan::ComputePipelineMap ezResourceCacheVulkan::s_computePipelines;
ezMap<const ezRefCounted*, ezHybridArray<ezResourceCacheVulkan::GraphicsPipelineMap::Iterator, 1>> ezResourceCacheVulkan::s_graphicsPipelineUsedBy;
ezMap<const ezRefCounted*, ezHybridArray<ezResourceCacheVulkan::ComputePipelineMap::Iterator, 1>> ezResourceCacheVulkan::s_computePipelineUsedBy;

ezHashTable<ezGALShaderVulkan::DescriptorSetLayoutDesc, vk::DescriptorSetLayout, ezResourceCacheVulkan::ResourceCacheHash> ezResourceCacheVulkan::s_descriptorSetLayouts;

// #define EZ_LOG_VULKAN_RESOURCES

static_assert(sizeof(ezUInt32) == sizeof(ezGALRenderTargetViewHandle));
namespace
{
  EZ_ALWAYS_INLINE ezStreamWriter& operator<<(ezStreamWriter& Stream, const ezGALRenderTargetViewHandle& Value)
  {
    Stream << reinterpret_cast<const ezUInt32&>(Value);
    return Stream;
  }
} // namespace

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

  // graphic
  {
    for (auto it : s_graphicsPipelines)
    {
      s_device.destroyPipeline(it.Value(), nullptr);
    }
    s_graphicsPipelines.Clear();
    GraphicsPipelineMap tmp;
    s_graphicsPipelines.Swap(tmp);
    s_graphicsPipelineUsedBy.Clear();
    ezMap<const ezRefCounted*, ezHybridArray<GraphicsPipelineMap::Iterator, 1>> tmp2;
    s_graphicsPipelineUsedBy.Swap(tmp2);
  }

  // compute
  {
    for (auto it : s_computePipelines)
    {
      s_device.destroyPipeline(it.Value(), nullptr);
    }
    s_computePipelines.Clear();
    ComputePipelineMap tmp;
    s_computePipelines.Swap(tmp);
    s_computePipelineUsedBy.Clear();
    ezMap<const ezRefCounted*, ezHybridArray<ComputePipelineMap::Iterator, 1>> tmp2;
    s_computePipelineUsedBy.Swap(tmp2);
  }

  for (auto it : s_pipelineLayouts)
  {
    s_device.destroyPipelineLayout(it.Value(), nullptr);
  }
  s_pipelineLayouts.Clear();
  s_pipelineLayouts.Compact();

  for (auto it : s_descriptorSetLayouts)
  {
    s_device.destroyDescriptorSetLayout(it.Value(), nullptr);
  }
  s_descriptorSetLayouts.Clear();
  s_descriptorSetLayouts.Compact();

  s_device = nullptr;
}

EZ_DEFINE_AS_POD_TYPE(vk::AttachmentDescription);
EZ_DEFINE_AS_POD_TYPE(vk::AttachmentReference);

vk::RenderPass ezResourceCacheVulkan::RequestRenderPass(const ezGALRenderPassDescriptor& renderPass)
{
  if (const vk::RenderPass* pPass = s_renderPasses.GetValue(renderPass))
  {
    return *pPass;
  }

#ifdef EZ_LOG_VULKAN_RESOURCES
  ezLog::Info("Creating RenderPass #{}", s_renderPasses.GetCount());
#endif // EZ_LOG_VULKAN_RESOURCES

  ezHybridArray<vk::AttachmentDescription, 4> attachments;
  ezHybridArray<vk::AttachmentReference, 1> depthAttachmentRefs;
  ezHybridArray<vk::AttachmentReference, 4> colorAttachmentRefs;

  if (renderPass.m_DepthFormat != ezGALResourceFormat::Invalid)
  {
    vk::AttachmentDescription& vkAttachment = attachments.ExpandAndGetRef();
    const auto& formatInfo = s_pDevice->GetFormatLookupTable().GetFormatInfo(renderPass.m_DepthFormat);

    vkAttachment.format = formatInfo.m_format;
    vkAttachment.samples = ezConversionUtilsVulkan::GetSamples(renderPass.m_Msaa);
    vkAttachment.loadOp = ezConversionUtilsVulkan::GetAttachmentLoadOp(renderPass.m_DepthLoadOp);
    vkAttachment.storeOp = ezConversionUtilsVulkan::GetAttachmentStoreOp(renderPass.m_DepthStoreOp);
    vkAttachment.stencilLoadOp = ezConversionUtilsVulkan::GetAttachmentLoadOp(renderPass.m_StencilLoadOp);
    vkAttachment.stencilStoreOp = ezConversionUtilsVulkan::GetAttachmentStoreOp(renderPass.m_StencilStoreOp);
    vkAttachment.initialLayout = vkAttachment.loadOp == vk::AttachmentLoadOp::eLoad ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eUndefined;
    vkAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference& depthAttachment = depthAttachmentRefs.ExpandAndGetRef();
    depthAttachment.attachment = attachments.GetCount() - 1;
    depthAttachment.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
  }

  const ezUInt32 uiCount = renderPass.m_uiRTCount;
  for (ezUInt32 i = 0; i < uiCount; i++)
  {
    vk::AttachmentDescription& vkAttachment = attachments.ExpandAndGetRef();
    const auto& formatInfo = s_pDevice->GetFormatLookupTable().GetFormatInfo(renderPass.m_ColorFormat[i]);

    vkAttachment.format = formatInfo.m_format;
    vkAttachment.samples = ezConversionUtilsVulkan::GetSamples(renderPass.m_Msaa);
    vkAttachment.loadOp = ezConversionUtilsVulkan::GetAttachmentLoadOp(renderPass.m_ColorLoadOp[i]);
    vkAttachment.storeOp = ezConversionUtilsVulkan::GetAttachmentStoreOp(renderPass.m_ColorStoreOp[i]);
    vkAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    vkAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    vkAttachment.initialLayout = vkAttachment.loadOp == vk::AttachmentLoadOp::eLoad ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::eUndefined;
    vkAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentReference& colorAttachment = colorAttachmentRefs.ExpandAndGetRef();
    colorAttachment.attachment = attachments.GetCount() - 1;
    colorAttachment.layout = vk::ImageLayout::eColorAttachmentOptimal;
  }

  const bool bHasColor = !colorAttachmentRefs.IsEmpty();
  const bool bHasDepth = !depthAttachmentRefs.IsEmpty();
  vk::SubpassDescription subpass;
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount = colorAttachmentRefs.GetCount();
  subpass.pColorAttachments = bHasColor ? colorAttachmentRefs.GetData() : nullptr;
  subpass.pDepthStencilAttachment = bHasDepth ? depthAttachmentRefs.GetData() : nullptr;

  vk::SubpassDependency dependency;
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion; // VK_DEPENDENCY_BY_REGION_BIT;

  dependency.srcAccessMask = {};
  if (bHasColor)
    dependency.dstAccessMask |= vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead;

  if (bHasDepth)
    dependency.dstAccessMask |= vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;

  dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
  dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;

  vk::RenderPassCreateInfo renderPassCreateInfo;
  renderPassCreateInfo.attachmentCount = attachments.GetCount();
  renderPassCreateInfo.pAttachments = attachments.GetData();
  renderPassCreateInfo.subpassCount = 1;
  renderPassCreateInfo.pSubpasses = &subpass;
  renderPassCreateInfo.dependencyCount = 1;
  renderPassCreateInfo.pDependencies = &dependency;

  vk::RenderPass vkRenderPass;
  VK_LOG_ERROR(s_device.createRenderPass(&renderPassCreateInfo, nullptr, &vkRenderPass));

  s_renderPasses.Insert(renderPass, vkRenderPass);
  return vkRenderPass;
}

vk::Framebuffer ezResourceCacheVulkan::RequestFrameBuffer(vk::RenderPass vkRenderPass, const ezGALFrameBufferDescriptor& frameBuffer)
{
  FramebufferKey key;
  key.m_renderPass = vkRenderPass;
  key.m_frameBuffer = frameBuffer;

  if (const vk::Framebuffer* pFrameBuffer = s_frameBuffers.GetValue(key))
  {
    return *pFrameBuffer;
  }

#ifdef EZ_LOG_VULKAN_RESOURCES
  ezLog::Info("Creating FrameBuffer #{}", s_frameBuffers.GetCount());
#endif // EZ_LOG_VULKAN_RESOURCES

  ezHybridArray<vk::ImageView, EZ_GAL_MAX_RENDERTARGET_COUNT + 1> attachments;
  if (!frameBuffer.m_hDepthTarget.IsInvalidated())
  {
    const ezGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const ezGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(frameBuffer.m_hDepthTarget));
    attachments.PushBack(pRenderTargetView->GetImageView());
  }
  for (ezUInt32 i = 0; i < EZ_GAL_MAX_RENDERTARGET_COUNT; ++i)
  {
    if (frameBuffer.m_hColorTarget[i].IsInvalidated())
      break;

    const ezGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const ezGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(frameBuffer.m_hColorTarget[i]));
    attachments.PushBack(pRenderTargetView->GetImageView());
  }

  vk::FramebufferCreateInfo framebufferInfo;
  framebufferInfo.renderPass = vkRenderPass;
  framebufferInfo.attachmentCount = attachments.GetCount();
  framebufferInfo.pAttachments = attachments.GetData();
  framebufferInfo.width = frameBuffer.m_Size.width;
  framebufferInfo.height = frameBuffer.m_Size.height;
  framebufferInfo.layers = frameBuffer.m_uiSliceCount;

  vk::Framebuffer vkFrameBuffer;
  VK_LOG_ERROR(s_device.createFramebuffer(&framebufferInfo, nullptr, &vkFrameBuffer));

  s_frameBuffers.Insert(key, vkFrameBuffer);
  return vkFrameBuffer;
}

vk::PipelineLayout ezResourceCacheVulkan::RequestPipelineLayout(const PipelineLayoutDesc& desc)
{
  if (const vk::PipelineLayout* pPipelineLayout = s_pipelineLayouts.GetValue(desc))
  {
    return *pPipelineLayout;
  }

#ifdef EZ_LOG_VULKAN_RESOURCES
  ezLog::Info("Creating Pipeline Layout #{}", s_pipelineLayouts.GetCount());
#endif // EZ_LOG_VULKAN_RESOURCES

  vk::PipelineLayoutCreateInfo layoutInfo;
  layoutInfo.setLayoutCount = desc.m_layout.GetCount();
  layoutInfo.pSetLayouts = desc.m_layout.GetData();
  if (desc.m_pushConstants.size != 0)
  {
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &desc.m_pushConstants;
  }

  vk::PipelineLayout layout;
  VK_ASSERT_DEBUG(s_device.createPipelineLayout(&layoutInfo, nullptr, &layout));

  s_pipelineLayouts.Insert(desc, layout);
  return layout;
}

vk::Pipeline ezResourceCacheVulkan::RequestGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
  if (const vk::Pipeline* pPipeline = s_graphicsPipelines.GetValue(desc))
  {
    return *pPipeline;
  }

#ifdef EZ_LOG_VULKAN_RESOURCES
  ezLog::Info("Creating Graphics Pipeline #{}", s_graphicsPipelines.GetCount());
#endif // EZ_LOG_VULKAN_RESOURCES

  vk::PipelineVertexInputStateCreateInfo dummy;
  const vk::PipelineVertexInputStateCreateInfo* pVertexCreateInfo = nullptr;
  ezHybridArray<vk::VertexInputBindingDescription, EZ_GAL_MAX_VERTEX_BUFFER_COUNT> bindings;
  pVertexCreateInfo = desc.m_pCurrentVertexDecl ? &desc.m_pCurrentVertexDecl->GetCreateInfo() : &dummy;

  vk::PipelineInputAssemblyStateCreateInfo input_assembly;
  input_assembly.topology = ezConversionUtilsVulkan::GetPrimitiveTopology(desc.m_topology);
  const bool bTessellation = desc.m_pCurrentShader->GetShader(ezGALShaderStage::HullShader) != nullptr;
  if (bTessellation)
  {
    // Tessellation shaders always need to use patch list as the topology.
    input_assembly.topology = vk::PrimitiveTopology::ePatchList;
  }

  // Specify rasterization state.
  const vk::PipelineRasterizationStateCreateInfo* raster = desc.m_pCurrentRasterizerState->GetRasterizerState();

  // Our attachment will write to all color channels
  vk::PipelineColorBlendStateCreateInfo blend = *desc.m_pCurrentBlendState->GetBlendState();
  blend.attachmentCount = desc.m_renderPassDesc.m_uiRTCount;

  // We will have one viewport and scissor box.
  vk::PipelineViewportStateCreateInfo viewport;
  viewport.viewportCount = 1;
  viewport.scissorCount = 1;

  // Depth Testing
  const vk::PipelineDepthStencilStateCreateInfo* depth_stencil = desc.m_pCurrentDepthStencilState->GetDepthStencilState();

  // Multisampling.
  vk::PipelineMultisampleStateCreateInfo multisample;
  multisample.rasterizationSamples = ezConversionUtilsVulkan::GetSamples(desc.m_renderPassDesc.m_Msaa);
  if (multisample.rasterizationSamples != vk::SampleCountFlagBits::e1 && desc.m_pCurrentBlendState->GetDescription().m_bAlphaToCoverage)
  {
    multisample.alphaToCoverageEnable = true;
  }

  // Specify that these states will be dynamic, i.e. not part of pipeline state object.
  ezHybridArray<vk::DynamicState, 2> dynamics;
  dynamics.PushBack(vk::DynamicState::eViewport);
  dynamics.PushBack(vk::DynamicState::eScissor);

  vk::PipelineDynamicStateCreateInfo dynamic;
  dynamic.pDynamicStates = dynamics.GetData();
  dynamic.dynamicStateCount = dynamics.GetCount();

  // Load our SPIR-V shaders.
  ezHybridArray<vk::PipelineShaderStageCreateInfo, 6> shader_stages;
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; i++)
  {
    if (vk::ShaderModule shader = desc.m_pCurrentShader->GetShader((ezGALShaderStage::Enum)i))
    {
      vk::PipelineShaderStageCreateInfo& stage = shader_stages.ExpandAndGetRef();
      stage.stage = ezConversionUtilsVulkan::GetShaderStage((ezGALShaderStage::Enum)i);
      stage.module = shader;
      stage.pName = "main";
    }
  }

  vk::PipelineTessellationStateCreateInfo tessellationInfo;
  if (bTessellation)
  {
    tessellationInfo.patchControlPoints = desc.m_pCurrentShader->GetDescription().m_ByteCodes[ezGALShaderStage::HullShader]->m_uiTessellationPatchControlPoints;
  }

  vk::GraphicsPipelineCreateInfo pipe;
  pipe.renderPass = desc.m_renderPass;
  pipe.layout = desc.m_layout;
  pipe.stageCount = shader_stages.GetCount();
  pipe.pStages = shader_stages.GetData();
  pipe.pVertexInputState = pVertexCreateInfo;
  pipe.pInputAssemblyState = &input_assembly;
  pipe.pRasterizationState = raster;
  pipe.pColorBlendState = &blend;
  pipe.pMultisampleState = &multisample;
  pipe.pViewportState = &viewport;
  pipe.pDepthStencilState = depth_stencil;
  pipe.pDynamicState = &dynamic;
  if (bTessellation)
    pipe.pTessellationState = &tessellationInfo;

  vk::Pipeline pipeline;
  vk::PipelineCache cache;
  VK_ASSERT_DEBUG(s_device.createGraphicsPipelines(cache, 1, &pipe, nullptr, &pipeline));

  auto it = s_graphicsPipelines.Insert(desc, pipeline);
  {
    s_graphicsPipelineUsedBy[desc.m_pCurrentRasterizerState].PushBack(it);
    s_graphicsPipelineUsedBy[desc.m_pCurrentBlendState].PushBack(it);
    s_graphicsPipelineUsedBy[desc.m_pCurrentDepthStencilState].PushBack(it);
    s_graphicsPipelineUsedBy[desc.m_pCurrentShader].PushBack(it);
    s_graphicsPipelineUsedBy[desc.m_pCurrentVertexDecl].PushBack(it);
  }

  return pipeline;
}

vk::Pipeline ezResourceCacheVulkan::RequestComputePipeline(const ComputePipelineDesc& desc)
{
  if (const vk::Pipeline* pPipeline = s_computePipelines.GetValue(desc))
  {
    return *pPipeline;
  }

#ifdef EZ_LOG_VULKAN_RESOURCES
  ezLog::Info("Creating Compute Pipeline #{}", s_computePipelines.GetCount());
#endif // EZ_LOG_VULKAN_RESOURCES

  vk::ComputePipelineCreateInfo pipe;
  pipe.layout = desc.m_layout;
  {
    vk::ShaderModule shader = desc.m_pCurrentShader->GetShader(ezGALShaderStage::ComputeShader);
    EZ_ASSERT_DEV(shader != nullptr, "No compute shader stage present in the bound shader");
    pipe.stage.stage = ezConversionUtilsVulkan::GetShaderStage(ezGALShaderStage::ComputeShader);
    pipe.stage.module = shader;
    pipe.stage.pName = "main";
  }

  vk::Pipeline pipeline;
  vk::PipelineCache cache;
  VK_ASSERT_DEBUG(s_device.createComputePipelines(cache, 1, &pipe, nullptr, &pipeline));

  auto it = s_computePipelines.Insert(desc, pipeline);
  {
    s_computePipelineUsedBy[desc.m_pCurrentShader].PushBack(it);
  }

  return pipeline;
}

vk::DescriptorSetLayout ezResourceCacheVulkan::RequestDescriptorSetLayout(const ezGALShaderVulkan::DescriptorSetLayoutDesc& desc)
{
  if (const vk::DescriptorSetLayout* pLayout = s_descriptorSetLayouts.GetValue(desc))
  {
    return *pLayout;
  }

#ifdef EZ_LOG_VULKAN_RESOURCES
  ezLog::Info("Creating Descriptor Set Layout #{}", s_descriptorSetLayouts.GetCount());
#endif // EZ_LOG_VULKAN_RESOURCES

  vk::DescriptorSetLayoutCreateInfo descriptorSetLayout;
  descriptorSetLayout.bindingCount = desc.m_bindings.GetCount();
  descriptorSetLayout.pBindings = desc.m_bindings.GetData();

  vk::DescriptorSetLayout layout;
  VK_ASSERT_DEBUG(s_device.createDescriptorSetLayout(&descriptorSetLayout, nullptr, &layout));

  s_descriptorSetLayouts.Insert(desc, layout);
  return layout;
}

void ezResourceCacheVulkan::ResourceDeleted(const ezRefCounted* pResource)
{
  auto it = s_graphicsPipelineUsedBy.Find(pResource);
  if (it.IsValid())
  {
    const auto& itArray = it.Value();
    for (GraphicsPipelineMap::Iterator it2 : itArray)
    {
      s_pDevice->DeleteLater(it2.Value());

      const GraphicsPipelineDesc& desc = it2.Key();
      ezArrayPtr<const ezRefCounted*> resources((const ezRefCounted**)&desc.m_pCurrentRasterizerState, 5);
      for (const ezRefCounted* pResource2 : resources)
      {
        if (pResource2 != pResource)
        {
          s_graphicsPipelineUsedBy[pResource2].RemoveAndSwap(it2);
        }
      }

      s_graphicsPipelines.Remove(it2);
    }

    s_graphicsPipelineUsedBy.Remove(it);
  }
}

void ezResourceCacheVulkan::ShaderDeleted(const ezGALShaderVulkan* pShader)
{
  if (pShader->GetDescription().HasByteCodeForStage(ezGALShaderStage::ComputeShader))
  {
    auto it = s_computePipelineUsedBy.Find(pShader);
    if (it.IsValid())
    {
      const auto& itArray = it.Value();
      for (ComputePipelineMap::Iterator it2 : itArray)
      {
        s_pDevice->DeleteLater(it2.Value());
        s_computePipelines.Remove(it2);
      }

      s_computePipelineUsedBy.Remove(it);
    }
  }
  else
  {
    ResourceDeleted(pShader);
  }
}

ezUInt32 ezResourceCacheVulkan::ResourceCacheHash::Hash(const ezGALRenderPassDescriptor& desc)
{
  return desc.CalculateHash();
}

bool ezResourceCacheVulkan::ResourceCacheHash::Equal(const ezGALRenderPassDescriptor& a, const ezGALRenderPassDescriptor& b)
{
  bool equal = a == b;
  return equal;
}

bool ezResourceCacheVulkan::ResourceCacheHash::Equal(const ezGALShaderVulkan::DescriptorSetLayoutDesc& a, const ezGALShaderVulkan::DescriptorSetLayoutDesc& b)
{
  const ezUInt32 uiCount = a.m_bindings.GetCount();
  if (uiCount != b.m_bindings.GetCount())
    return false;

  for (ezUInt32 i = 0; i < uiCount; i++)
  {
    const vk::DescriptorSetLayoutBinding& aB = a.m_bindings[i];
    const vk::DescriptorSetLayoutBinding& bB = b.m_bindings[i];
    if (aB.binding != bB.binding || aB.descriptorType != bB.descriptorType || aB.descriptorCount != bB.descriptorCount || aB.stageFlags != bB.stageFlags || aB.pImmutableSamplers != bB.pImmutableSamplers)
      return false;
  }
  return true;
}

bool ezResourceCacheVulkan::ResourceCacheHash::Less(const GraphicsPipelineDesc& a, const GraphicsPipelineDesc& b)
{
#define LESS_CHECK(member)  \
  if (a.member != b.member) \
    return a.member < b.member;

  LESS_CHECK(m_renderPass);
  LESS_CHECK(m_layout);
  LESS_CHECK(m_topology);
  LESS_CHECK(m_renderPassDesc);
  LESS_CHECK(m_pCurrentRasterizerState);
  LESS_CHECK(m_pCurrentBlendState);
  LESS_CHECK(m_pCurrentDepthStencilState);
  LESS_CHECK(m_pCurrentShader);
  LESS_CHECK(m_pCurrentVertexDecl);
  return false;

#undef LESS_CHECK
}

ezUInt32 ezResourceCacheVulkan::ResourceCacheHash::Hash(const FramebufferKey& key)
{
  ezHashStreamWriter32 writer;
  writer << key.m_renderPass;
  writer << key.m_frameBuffer.CalculateHash();
  return writer.GetHashValue();
}

bool ezResourceCacheVulkan::ResourceCacheHash::Equal(const FramebufferKey& a, const FramebufferKey& b)
{
  return a.m_renderPass == b.m_renderPass && a.m_frameBuffer == b.m_frameBuffer;
}

ezUInt32 ezResourceCacheVulkan::ResourceCacheHash::Hash(const PipelineLayoutDesc& desc)
{
  ezHashStreamWriter32 writer;
  const ezUInt32 uiCount = desc.m_layout.GetCount();
  writer << uiCount;
  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    writer << desc.m_layout[i];
  }
  writer << desc.m_pushConstants.offset;
  writer << desc.m_pushConstants.size;
  writer << ezConversionUtilsVulkan::GetUnderlyingFlagsValue(desc.m_pushConstants.stageFlags);
  return writer.GetHashValue();
}

bool ezResourceCacheVulkan::ResourceCacheHash::Equal(const PipelineLayoutDesc& a, const PipelineLayoutDesc& b)
{
  if (a.m_layout.GetCount() != b.m_layout.GetCount())
    return false;

  const ezUInt32 uiCount = a.m_layout.GetCount();
  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    if (a.m_layout[i] != b.m_layout[i])
      return false;
  }
  return a.m_pushConstants == b.m_pushConstants;
}

ezUInt32 ezResourceCacheVulkan::ResourceCacheHash::Hash(const GraphicsPipelineDesc& desc)
{
  ezHashStreamWriter32 writer;
  writer << desc.m_renderPass;
  writer << desc.m_layout;
  writer << desc.m_topology;
  writer << desc.m_renderPassDesc.CalculateHash();
  writer << desc.m_pCurrentRasterizerState;
  writer << desc.m_pCurrentBlendState;
  writer << desc.m_pCurrentDepthStencilState;
  writer << desc.m_pCurrentShader;
  writer << desc.m_pCurrentVertexDecl;
  return writer.GetHashValue();
}

bool ArraysEqual(const ezUInt32 (&a)[EZ_GAL_MAX_VERTEX_BUFFER_COUNT], const ezUInt32 (&b)[EZ_GAL_MAX_VERTEX_BUFFER_COUNT])
{
  for (ezUInt32 i = 0; i < EZ_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    if (a[i] != b[i])
      return false;
  }
  return true;
}

bool ezResourceCacheVulkan::ResourceCacheHash::Equal(const GraphicsPipelineDesc& a, const GraphicsPipelineDesc& b)
{
  return a.m_renderPass == b.m_renderPass && a.m_layout == b.m_layout && a.m_topology == b.m_topology && a.m_renderPassDesc == b.m_renderPassDesc && a.m_pCurrentRasterizerState == b.m_pCurrentRasterizerState && a.m_pCurrentBlendState == b.m_pCurrentBlendState && a.m_pCurrentDepthStencilState == b.m_pCurrentDepthStencilState && a.m_pCurrentShader == b.m_pCurrentShader && a.m_pCurrentVertexDecl == b.m_pCurrentVertexDecl;
}

bool ezResourceCacheVulkan::ResourceCacheHash::Less(const ComputePipelineDesc& a, const ComputePipelineDesc& b)
{
  if (a.m_layout != b.m_layout)
    return a.m_layout < b.m_layout;
  return a.m_pCurrentShader < b.m_pCurrentShader;
}

bool ezResourceCacheVulkan::ResourceCacheHash::Equal(const ComputePipelineDesc& a, const ComputePipelineDesc& b)
{
  return a.m_layout == b.m_layout && a.m_pCurrentShader == b.m_pCurrentShader;
}
