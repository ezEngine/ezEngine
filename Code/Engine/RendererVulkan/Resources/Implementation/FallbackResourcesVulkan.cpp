#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Resources/FallbackResourcesVulkan.h>

#include <Foundation/Algorithm/HashStream.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererFoundation/Resources/Buffer.h>

ezGALDeviceVulkan* ezFallbackResourcesVulkan::s_pDevice = nullptr;
ezEventSubscriptionID ezFallbackResourcesVulkan::s_EventID = 0;

ezHashTable<ezFallbackResourcesVulkan::Key, ezGALResourceViewHandle, ezFallbackResourcesVulkan::KeyHash> ezFallbackResourcesVulkan::m_ResourceViews;
ezHashTable<ezFallbackResourcesVulkan::Key, ezGALUnorderedAccessViewHandle, ezFallbackResourcesVulkan::KeyHash> ezFallbackResourcesVulkan::m_UAVs;
ezDynamicArray<ezGALBufferHandle> ezFallbackResourcesVulkan::m_Buffers;
ezDynamicArray<ezGALTextureHandle> ezFallbackResourcesVulkan::m_Textures;

void ezFallbackResourcesVulkan::Initialize(ezGALDeviceVulkan* pDevice)
{
  s_pDevice = pDevice;
  s_EventID = pDevice->m_Events.AddEventHandler(ezMakeDelegate(&ezFallbackResourcesVulkan::GALDeviceEventHandler));
}

void ezFallbackResourcesVulkan::DeInitialize()
{
  s_pDevice->m_Events.RemoveEventHandler(s_EventID);
  s_pDevice = nullptr;
}
void ezFallbackResourcesVulkan::GALDeviceEventHandler(const ezGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case ezGALDeviceEvent::AfterInit:
    {
      auto CreateTexture = [](ezGALTextureType::Enum type, ezGALMSAASampleCount::Enum samples) -> ezGALResourceViewHandle {
        ezGALTextureCreationDescription desc;
        desc.m_uiWidth = 4;
        desc.m_uiHeight = 4;
        if (type == ezGALTextureType::Texture3D)
          desc.m_uiDepth = 4;
        desc.m_uiMipLevelCount = 1;
        desc.m_Format = ezGALResourceFormat::BGRAUByteNormalizedsRGB;
        desc.m_Type = type;
        desc.m_SampleCount = samples;
        desc.m_ResourceAccess.m_bImmutable = false;
        ezGALTextureHandle hTexture = s_pDevice->CreateTexture(desc);
        EZ_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create fallback resource");
        // Debug device not set yet.
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackResourceVulkan");
        m_Textures.PushBack(hTexture);
        return s_pDevice->GetDefaultResourceView(hTexture);
      };
      {
        ezGALResourceViewHandle hView = CreateTexture(ezGALTextureType::Texture2D, ezGALMSAASampleCount::None);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, ezShaderResourceType::Texture2D}] = hView;
        m_ResourceViews[{vk::DescriptorType::eSampledImage, ezShaderResourceType::Texture2DArray}] = hView;
      }
      {
        ezGALResourceViewHandle hView = CreateTexture(ezGALTextureType::Texture2D, ezGALMSAASampleCount::TwoSamples);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, ezShaderResourceType::Texture2DMS}] = hView;
        m_ResourceViews[{vk::DescriptorType::eSampledImage, ezShaderResourceType::Texture2DMSArray}] = hView;
      }
      {
        ezGALResourceViewHandle hView = CreateTexture(ezGALTextureType::TextureCube, ezGALMSAASampleCount::None);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, ezShaderResourceType::TextureCube}] = hView;
        m_ResourceViews[{vk::DescriptorType::eSampledImage, ezShaderResourceType::TextureCubeArray}] = hView;
      }
      {
        ezGALResourceViewHandle hView = CreateTexture(ezGALTextureType::Texture3D, ezGALMSAASampleCount::None);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, ezShaderResourceType::Texture3D}] = hView;
      }
      {
        ezGALBufferCreationDescription desc;
        desc.m_bUseForIndirectArguments = false;
        desc.m_bUseAsStructuredBuffer = true;
        desc.m_bAllowRawViews = true;
        desc.m_bStreamOutputTarget = false;
        desc.m_bAllowShaderResourceView = true;
        desc.m_bAllowUAV = true;
        desc.m_uiStructSize = 128;
        desc.m_uiTotalSize = 1280;
        desc.m_ResourceAccess.m_bImmutable = false;
        ezGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackResourceVulkan");
        m_Buffers.PushBack(hBuffer);
        ezGALResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        m_ResourceViews[{vk::DescriptorType::eUniformBuffer, ezShaderResourceType::ConstantBuffer}] = hView;
        m_ResourceViews[{vk::DescriptorType::eStorageBuffer, ezShaderResourceType::GenericBuffer}] = hView;

      }
    }
    break;
    case ezGALDeviceEvent::BeforeShutdown:
    {
      m_ResourceViews.Clear();
      m_ResourceViews.Compact();

      m_UAVs.Clear();
      m_UAVs.Compact();

      for (ezGALBufferHandle hBuffer : m_Buffers)
      {
        s_pDevice->DestroyBuffer(hBuffer);
      }
      m_Buffers.Clear();
      m_Buffers.Compact();

      for (ezGALTextureHandle hTexture : m_Textures)
      {
        s_pDevice->DestroyTexture(hTexture);
      }
      m_Textures.Clear();
      m_Textures.Compact();
    }
    break;
    default:
      break;
  }
}

const ezGALResourceViewVulkan* ezFallbackResourcesVulkan::GetFallbackResourceView(vk::DescriptorType descriptorType, ezShaderResourceType::Enum ezType)
{
  if (ezGALResourceViewHandle* pView = m_ResourceViews.GetValue(Key{descriptorType, ezType}))
  {
    return static_cast<const ezGALResourceViewVulkan*>(s_pDevice->GetResourceView(*pView));
  }
  EZ_REPORT_FAILURE("No fallback resource set, update ezFallbackResourcesVulkan::GALDeviceEventHandler.");
  return nullptr;
}

const ezGALUnorderedAccessViewVulkan* ezFallbackResourcesVulkan::GetFallbackUnorderedAccessView(vk::DescriptorType descriptorType, ezShaderResourceType::Enum ezType)
{
  if (ezGALUnorderedAccessViewHandle* pView = m_UAVs.GetValue(Key{descriptorType, ezType}))
  {
    return static_cast<const ezGALUnorderedAccessViewVulkan*>(s_pDevice->GetUnorderedAccessView(*pView));
  }
  EZ_REPORT_FAILURE("No fallback resource set, update ezFallbackResourcesVulkan::GALDeviceEventHandler.");
  return nullptr;
}

ezUInt32 ezFallbackResourcesVulkan::KeyHash::Hash(const Key& a)
{
  ezHashStreamWriter32 writer;
  writer << ezConversionUtilsVulkan::GetUnderlyingValue(a.m_descriptorType);
  writer << ezConversionUtilsVulkan::GetUnderlyingValue(a.m_ezType);
  return writer.GetHashValue();
}

bool ezFallbackResourcesVulkan::KeyHash::Equal(const Key& a, const Key& b)
{
  return a.m_descriptorType == b.m_descriptorType && a.m_ezType == b.m_ezType;
}
