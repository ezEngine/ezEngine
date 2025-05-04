#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Basics.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

ezGALDeviceVulkan* ezResourceCacheVulkan::s_pDevice;
vk::Device ezResourceCacheVulkan::s_device;
vk::PipelineCache ezResourceCacheVulkan::s_pipelineCache;

ezHashTable<ezGALRenderPassDescriptor, vk::RenderPass, ezResourceCacheVulkan::ResourceCacheHash> ezResourceCacheVulkan::s_renderPasses;
ezHashTable<ezResourceCacheVulkan::FramebufferKey, vk::Framebuffer, ezResourceCacheVulkan::ResourceCacheHash> ezResourceCacheVulkan::s_frameBuffers;
ezHashTable<ezResourceCacheVulkan::PipelineLayoutDesc, vk::PipelineLayout, ezResourceCacheVulkan::ResourceCacheHash> ezResourceCacheVulkan::s_pipelineLayouts;

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

  static constexpr ezUInt32 PIPELINE_CACHE_MAGIC = 0x45A9BCD7; // Arbitrary m_uiMagic number
  // Header structure for Vulkan pipeline cache
  struct PipelineCachePrefixHeader
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiMagic;             // An arbitrary m_uiMagic header to make sure this is actually our file
    ezUInt32 m_uiDataSize;          // Equal to *pDataSize returned by vkGetPipelineCacheData
    ezUInt64 m_uiDataHash;          // A hash of pipeline cache data, including the header
    ezUInt32 m_uiVendorID;          // Equal to VkPhysicalDeviceProperties::vendorID
    ezUInt32 m_uiDeviceID;          // Equal to VkPhysicalDeviceProperties::deviceID
    ezUInt32 m_uiDriverVersion;     // Equal to VkPhysicalDeviceProperties::driverVersion
    ezUInt32 m_uiDriverABI;         // Equal to sizeof(void*)
    ezUInt8 m_uiUuid[VK_UUID_SIZE]; // Equal to VkPhysicalDeviceProperties::pipelineCacheUUID
  };

  ezString GetPipelineCacheFilename(const vk::PhysicalDeviceProperties& deviceProperties)
  {
    ezStringBuilder sCacheFilePath;
    ezString sAppName = ezApplication::GetApplicationInstance() ? ezApplication::GetApplicationInstance()->GetApplicationName().GetView() : "ezEngine"_ezsv;
#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
    // Be extra pedantic on Android and don't reuse caches on driver version changes, see https://zeux.io/2019/07/17/serializing-pipeline-cache/
    sCacheFilePath.SetFormat("{}/PipelineCache_{}_{}_{}.bin",
      ezOSFile::GetUserDataFolder(sAppName),
      ezArgU(deviceProperties.vendorID, 8, true, 16, true),
      ezArgU(deviceProperties.deviceID, 8, true, 16, true),
      ezArgU(deviceProperties.driverVersion, 8, true, 16, true));
#else
    // On desktop, we assume that vendors can actually write proper code and caches can be reused after driver updates.
    sCacheFilePath.SetFormat("{}/PipelineCache_{}_{}.bin",
      ezOSFile::GetUserDataFolder(sAppName),
      ezArgU(deviceProperties.vendorID, 8, true, 16, true),
      ezArgU(deviceProperties.deviceID, 8, true, 16, true));
#endif
    return sCacheFilePath;
  }
} // namespace



void ezResourceCacheVulkan::Initialize(ezGALDeviceVulkan* pDevice, vk::Device device)
{
  s_pDevice = pDevice;
  s_device = device;

  if (LoadPipelineCache(s_pipelineCache).Failed())
  {
    vk::PipelineCacheCreateInfo pipelineCacheInfo;
    pipelineCacheInfo.initialDataSize = 0;
    pipelineCacheInfo.pInitialData = nullptr;
    VK_ASSERT_DEV(s_device.createPipelineCache(&pipelineCacheInfo, nullptr, &s_pipelineCache));
  }
}

void ezResourceCacheVulkan::DeInitialize()
{
  if (s_pipelineCache)
  {
    if (SavePipelineCache().Failed())
    {
      ezLog::Error("Failed to save Vulkan pipeline cache");
    }
    // Destroy the pipeline cache
    s_device.destroyPipelineCache(s_pipelineCache, nullptr);
    s_pipelineCache = nullptr;
  }

  // Destroy other resources
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
    vkAttachment.initialLayout = vkAttachment.loadOp == vk::AttachmentLoadOp::eLoad || vkAttachment.stencilLoadOp == vk::AttachmentLoadOp::eLoad ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eUndefined;
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

ezResult ezResourceCacheVulkan::SavePipelineCache()
{
  // Get physical device properties for cache validation
  vk::PhysicalDeviceProperties deviceProperties = s_pDevice->GetVulkanPhysicalDevice().getProperties();

  // Get the cache data
  size_t dataSize = 0;
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(s_device.getPipelineCacheData(s_pipelineCache, &dataSize, nullptr));
  if (dataSize == 0)
    return EZ_SUCCESS;

  ezDynamicArray<ezUInt8> pipelineCacheData;
  pipelineCacheData.SetCountUninitialized(static_cast<ezUInt32>(dataSize));
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(s_device.getPipelineCacheData(s_pipelineCache, &dataSize, pipelineCacheData.GetData()));

  // Create our custom prefix header
  PipelineCachePrefixHeader prefixHeader;
  prefixHeader.m_uiMagic = PIPELINE_CACHE_MAGIC;
  prefixHeader.m_uiDataSize = static_cast<ezUInt32>(dataSize);
  prefixHeader.m_uiVendorID = deviceProperties.vendorID;
  prefixHeader.m_uiDeviceID = deviceProperties.deviceID;
  prefixHeader.m_uiDriverVersion = deviceProperties.driverVersion;
  prefixHeader.m_uiDriverABI = sizeof(void*);
  for (ezUInt32 i = 0; i < VK_UUID_SIZE; ++i)
  {
    prefixHeader.m_uiUuid[i] = deviceProperties.pipelineCacheUUID[i];
  }
  prefixHeader.m_uiDataHash = ezHashingUtils::xxHash64(pipelineCacheData.GetData(), dataSize);

  ezString sCacheFilePath = GetPipelineCacheFilename(deviceProperties);
  ezStringBuilder sTempFilePath;
  sTempFilePath.SetFormat("{}_temp", sCacheFilePath);
  // Write to the temporary file first
  {
    ezOSFile file;
    // ezFileWriter file;
    EZ_SUCCEED_OR_RETURN(file.Open(sTempFilePath, ezFileOpenMode::Write, ezFileShareMode::Exclusive));
    EZ_SUCCEED_OR_RETURN(file.Write(&prefixHeader, sizeof(PipelineCachePrefixHeader)));
    EZ_SUCCEED_OR_RETURN(file.Write(pipelineCacheData.GetData(), dataSize));
    file.Close();
  }

  // Now rename the temporary file to the final file
  EZ_SUCCEED_OR_RETURN(ezOSFile::DeleteFile(sCacheFilePath));
  EZ_SUCCEED_OR_RETURN(ezOSFile::MoveFileOrDirectory(sTempFilePath, sCacheFilePath));
  return EZ_SUCCESS;
}

ezResult ezResourceCacheVulkan::LoadPipelineCache(vk::PipelineCache& out_pipelineCache)
{
  // Pipeline cache implementation following https://zeux.io/2019/07/17/serializing-pipeline-cache/
  vk::PhysicalDeviceProperties deviceProperties = s_pDevice->GetVulkanPhysicalDevice().getProperties();

  // Try to load the pipeline cache from file
  ezString cacheFilePath = GetPipelineCacheFilename(deviceProperties);

  ezOSFile file;
  EZ_SUCCEED_OR_RETURN(file.Open(cacheFilePath, ezFileOpenMode::Read, ezFileShareMode::Default));

  // Read our custom prefix header
  PipelineCachePrefixHeader prefixHeader;
  ezUInt64 uiReadSize = file.Read(&prefixHeader, sizeof(PipelineCachePrefixHeader));
  if (uiReadSize != sizeof(PipelineCachePrefixHeader))
    return EZ_FAILURE;

  bool bCacheUuidValid = true;
  for (ezUInt32 i = 0; i < VK_UUID_SIZE; ++i)
  {
    if (prefixHeader.m_uiUuid[i] != deviceProperties.pipelineCacheUUID[i])
    {
      bCacheUuidValid = false;
      break;
    }
  }
  const bool bIsValid =
    bCacheUuidValid && prefixHeader.m_uiMagic == PIPELINE_CACHE_MAGIC && prefixHeader.m_uiVendorID == deviceProperties.vendorID && prefixHeader.m_uiDeviceID == deviceProperties.deviceID
#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
    && prefixHeader.m_uiDriverVersion == deviceProperties.driverVersion && prefixHeader.m_uiDriverABI == sizeof(void*)
#endif
    ;

  if (!bIsValid)
    return EZ_FAILURE;

  // Read the cache data into memory
  ezDynamicArray<ezUInt8> initialData;
  initialData.SetCountUninitialized(prefixHeader.m_uiDataSize);
  uiReadSize = file.Read(initialData.GetData(), initialData.GetCount());
  if (uiReadSize != prefixHeader.m_uiDataSize)
    return EZ_FAILURE;

  file.Close();

  // Verify hash
  const ezUInt64 computedHash = ezHashingUtils::xxHash64(initialData.GetData(), initialData.GetCount());
  if (computedHash != prefixHeader.m_uiDataHash)
    return EZ_FAILURE;

  // Create the pipeline cache
  vk::PipelineCacheCreateInfo pipelineCacheInfo;
  pipelineCacheInfo.initialDataSize = initialData.GetCount();
  pipelineCacheInfo.pInitialData = initialData.GetData();
  if (s_device.createPipelineCache(&pipelineCacheInfo, nullptr, &s_pipelineCache) != vk::Result::eSuccess)
    return EZ_FAILURE;

  return EZ_SUCCESS;
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
