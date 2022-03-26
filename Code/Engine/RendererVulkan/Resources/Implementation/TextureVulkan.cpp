#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>


ezGALTextureVulkan::ezGALTextureVulkan(const ezGALTextureCreationDescription& Description)
  : ezGALTexture(Description)
  , m_image(nullptr)
  , m_pExisitingNativeObject(Description.m_pExisitingNativeObject)
{
  // TODO existing native object in descriptor?
}

ezGALTextureVulkan::~ezGALTextureVulkan() {}

ezResult ezGALTextureVulkan::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  if (m_pExisitingNativeObject != nullptr)
  {
    /// \todo Validation if interface of corresponding texture object exists
    m_image = static_cast<VkImage>(m_pExisitingNativeObject);
    if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
    {
      ezResult res = CreateStagingBuffer(pVulkanDevice);
      if (res == EZ_FAILURE)
      {
        m_image = nullptr;
        return res;
      }
    }
    return EZ_SUCCESS;
  }

  vk::ImageCreateInfo createInfo = {};
  createInfo.flags |= vk::ImageCreateFlagBits::eMutableFormat;
  createInfo.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eStorage;

  createInfo.initialLayout = pInitialData.IsEmpty() ? vk::ImageLayout::eUndefined : vk::ImageLayout::ePreinitialized;
  createInfo.pQueueFamilyIndices = pVulkanDevice->GetQueueFamilyIndices().GetPtr();
  createInfo.queueFamilyIndexCount = pVulkanDevice->GetQueueFamilyIndices().GetCount();
  createInfo.sharingMode = vk::SharingMode::eExclusive;
  createInfo.tiling = vk::ImageTiling::eOptimal;                                                   // TODO CPU readback might require linear tiling
  createInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst; // TODO immutable resources

  // TODO are these correctly populated or do they contain meaningless values depending on
  // the texture type indicated?
  createInfo.extent.width = m_Description.m_uiWidth;
  createInfo.extent.height = m_Description.m_uiHeight;
  createInfo.extent.depth = m_Description.m_uiDepth;
  createInfo.mipLevels = m_Description.m_uiMipLevelCount;

  createInfo.samples = static_cast<vk::SampleCountFlagBits>(m_Description.m_SampleCount.GetValue());

  if (m_Description.m_bAllowShaderResourceView)
    createInfo.usage |= vk::ImageUsageFlagBits::eSampled;
  if (m_Description.m_bAllowUAV)
    createInfo.usage |= vk::ImageUsageFlagBits::eStorage;

  if (createInfo.format == vk::Format::eUndefined)
  {
    ezLog::Error("No storage format available for given format: {0}", m_Description.m_Format);
    return EZ_FAILURE;
  }

  if (m_Description.m_bCreateRenderTarget)
    createInfo.usage |= ezGALResourceFormat::IsDepthFormat(m_Description.m_Format) ? vk::ImageUsageFlagBits::eDepthStencilAttachment : vk::ImageUsageFlagBits::eColorAttachment;

  switch (m_Description.m_Type)
  {
    case ezGALTextureType::Texture2D:
    case ezGALTextureType::TextureCube:
    {
      createInfo.arrayLayers = (m_Description.m_Type == ezGALTextureType::Texture2D ? m_Description.m_uiArraySize : (m_Description.m_uiArraySize * 6));
      createInfo.imageType = vk::ImageType::e2D; // TODO I think Cube maps require the 3D type

      //Tex2DDesc.CPUAccessFlags = 0; // We always use staging textures to update the data
      //Tex2DDesc.Usage = m_Description.m_ResourceAccess.IsImmutable() ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT; // TODO Vulkan

      // TODO Vulkan?
      //if (m_Description.m_bAllowDynamicMipGeneration)
      //  Tex2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

      if (m_Description.m_Type == ezGALTextureType::TextureCube)
        createInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;

      // TODO initial data in Vulkan?
      /*
      ezHybridArray<D3D11_SUBRESOURCE_DATA, 16> InitialData;
      if (!pInitialData.IsEmpty())
      {
        const ezUInt32 uiInitialDataCount =
            (m_Description.m_uiMipLevelCount * (m_Description.m_Type == ezGALTextureType::Texture2D ? 1 : 6));
        EZ_ASSERT_DEV(pInitialData.GetCount() == uiInitialDataCount,
                      "The array of initial data values is not equal to the amount of mip levels!");

        InitialData.SetCountUninitialized(uiInitialDataCount);

        for (ezUInt32 i = 0; i < uiInitialDataCount; i++)
        {
          InitialData[i].pSysMem = pInitialData[i].m_pData;
          InitialData[i].SysMemPitch = pInitialData[i].m_uiRowPitch;
          InitialData[i].SysMemSlicePitch = pInitialData[i].m_uiSlicePitch;
        }
      }*/

      ezVulkanAllocationCreateInfo allocInfo;
      allocInfo.m_usage = ezVulkanMemoryUsage::Auto;
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(ezMemoryAllocatorVulkan::CreateImage(createInfo, allocInfo, m_image, m_alloc));

      if (!m_image)
      {
        return EZ_FAILURE;
      }
    }
    break;

    case ezGALTextureType::Texture3D:
    {
      createInfo.imageType = vk::ImageType::e3D;
      // TODO Vulkan
      //Tex3DDesc.CPUAccessFlags = 0; // We always use staging textures to update the data
      //Tex3DDesc.Usage = m_Description.m_ResourceAccess.IsImmutable() ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;

      // TODO vulkan
      //if (m_Description.m_bAllowDynamicMipGeneration)
      //  Tex3DDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

      // TODO ????
      //if (m_Description.m_Type == ezGALTextureType::TextureCube)
      //  Tex3DDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

      // TODO initialData
      //ezHybridArray<D3D11_SUBRESOURCE_DATA, 16> InitialData;
      //if (!pInitialData.IsEmpty())
      //{
      //  const ezUInt32 uiInitialDataCount = m_Description.m_uiMipLevelCount;
      //  EZ_ASSERT_DEV(pInitialData.GetCount() == uiInitialDataCount,
      //    "The array of initial data values is not equal to the amount of mip levels!");
      //
      //  InitialData.SetCountUninitialized(uiInitialDataCount);
      //
      //  for (ezUInt32 i = 0; i < uiInitialDataCount; i++)
      //  {
      //    InitialData[i].pSysMem = pInitialData[i].m_pData;
      //    InitialData[i].SysMemPitch = pInitialData[i].m_uiRowPitch;
      //    InitialData[i].SysMemSlicePitch = pInitialData[i].m_uiSlicePitch;
      //  }
      //}
    }
    break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return EZ_FAILURE;
  }

  if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
    return CreateStagingBuffer(pVulkanDevice);

  m_device = pVulkanDevice->GetVulkanDevice(); // TODO replace by something better

  return EZ_SUCCESS;
}

ezResult ezGALTextureVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  if (m_image && !m_pExisitingNativeObject)
  {
    pVulkanDevice->DeleteLater(m_image, m_alloc);
  }
  m_image = nullptr;

  if (m_pStagingBuffer)
  {
    m_pStagingBuffer = nullptr;
    pDevice->DestroyBuffer(m_stagingBufferHandle);
  }

  return EZ_SUCCESS;
}

void ezGALTextureVulkan::SetDebugNamePlatform(const char* szName) const
{
  if (m_image)
  {
    vk::DebugUtilsObjectNameInfoEXT nameInfo;
    nameInfo.objectType = m_image.objectType;
    nameInfo.objectHandle = (uint64_t)(VkImage)m_image;
    nameInfo.pObjectName = szName;

    m_device.setDebugUtilsObjectNameEXT(nameInfo);
    if (m_alloc)
      ezMemoryAllocatorVulkan::SetAllocationUserData(m_alloc, szName);
  }
}

ezResult ezGALTextureVulkan::CreateStagingBuffer(ezGALDeviceVulkan* pDevice)
{
  //#TODO_VULKAN staging buffer needs to be a texture so we can transition into linear layout and access sub elements. A simple buffer thus won't cut it.

  /*ezGALBufferCreationDescription bufferDescription;
  bufferDescription.m_BufferType = ezGALBufferType::Generic;
  bufferDescription.m_uiStructSize = 1;
  bufferDescription.m_uiTotalSize = static_cast<ezUInt32>(m_memorySize);

  m_stagingBufferHandle = pDevice->CreateBuffer(bufferDescription);
  const ezGALBuffer* pStagingBuffer = pDevice->GetBuffer(m_stagingBufferHandle);
  EZ_ASSERT_DEV(pStagingBuffer, "Expected valid buffer handle here!");
  m_pStagingBuffer = static_cast<const ezGALBufferVulkan*>(pStagingBuffer);*/

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_TextureVulkan);
