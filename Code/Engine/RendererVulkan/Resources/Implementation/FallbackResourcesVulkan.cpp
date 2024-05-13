#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Resources/FallbackResourcesVulkan.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererVulkan, FallbackResourcesVulkan)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezFallbackResourcesVulkan::Initialize();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezFallbackResourcesVulkan::DeInitialize();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezGALDevice* ezFallbackResourcesVulkan::s_pDevice = nullptr;
ezEventSubscriptionID ezFallbackResourcesVulkan::s_EventID = 0;

ezHashTable<ezFallbackResourcesVulkan::Key, ezGALTextureResourceViewHandle, ezFallbackResourcesVulkan::KeyHash> ezFallbackResourcesVulkan::m_TextureResourceViews;
ezHashTable<ezEnum<ezGALShaderResourceType>, ezGALBufferResourceViewHandle, ezFallbackResourcesVulkan::KeyHash> ezFallbackResourcesVulkan::m_BufferResourceViews;
ezHashTable<ezFallbackResourcesVulkan::Key, ezGALTextureUnorderedAccessViewHandle, ezFallbackResourcesVulkan::KeyHash> ezFallbackResourcesVulkan::m_TextureUAVs;
ezHashTable<ezEnum<ezGALShaderResourceType>, ezGALBufferUnorderedAccessViewHandle, ezFallbackResourcesVulkan::KeyHash> ezFallbackResourcesVulkan::m_BufferUAVs;
ezDynamicArray<ezGALBufferHandle> ezFallbackResourcesVulkan::m_Buffers;
ezDynamicArray<ezGALTextureHandle> ezFallbackResourcesVulkan::m_Textures;

void ezFallbackResourcesVulkan::Initialize()
{
  s_EventID = ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&ezFallbackResourcesVulkan::GALDeviceEventHandler));
}

void ezFallbackResourcesVulkan::DeInitialize()
{
  ezGALDevice::s_Events.RemoveEventHandler(s_EventID);
}
void ezFallbackResourcesVulkan::GALDeviceEventHandler(const ezGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case ezGALDeviceEvent::AfterInit:
    {
      s_pDevice = e.m_pDevice;
      auto CreateTexture = [](ezGALTextureType::Enum type, ezGALMSAASampleCount::Enum samples, bool bDepth) -> ezGALTextureResourceViewHandle
      {
        ezGALTextureCreationDescription desc;
        desc.m_uiWidth = 4;
        desc.m_uiHeight = 4;
        if (type == ezGALTextureType::Texture3D)
          desc.m_uiDepth = 4;
        desc.m_uiMipLevelCount = 1;
        desc.m_Format = bDepth ? ezGALResourceFormat::D16 : ezGALResourceFormat::BGRAUByteNormalizedsRGB;
        desc.m_Type = type;
        desc.m_SampleCount = samples;
        desc.m_ResourceAccess.m_bImmutable = false;
        desc.m_bCreateRenderTarget = bDepth;
        ezGALTextureHandle hTexture = s_pDevice->CreateTexture(desc);
        EZ_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create fallback resource");
        // Debug device not set yet.
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackResourceVulkan");
        m_Textures.PushBack(hTexture);
        return s_pDevice->GetDefaultResourceView(hTexture);
      };
      {
        ezGALTextureResourceViewHandle hView = CreateTexture(ezGALTextureType::Texture2D, ezGALMSAASampleCount::None, false);
        m_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2D, false}] = hView;
        m_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2DArray, false}] = hView;
        m_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2D, false}] = hView;
        m_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2DArray, false}] = hView;
      }
      {
        ezGALTextureResourceViewHandle hView = CreateTexture(ezGALTextureType::Texture2D, ezGALMSAASampleCount::None, true);
        m_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2D, true}] = hView;
        m_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2DArray, true}] = hView;
        m_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2D, true}] = hView;
        m_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2DArray, true}] = hView;
      }

      // Swift shader can only do 4x MSAA. Add a check anyways.
      const bool bSupported = s_pDevice->GetCapabilities().m_FormatSupport[ezGALResourceFormat::BGRAUByteNormalizedsRGB].AreAllSet(ezGALResourceFormatSupport::Texture | ezGALResourceFormatSupport::MSAA4x);

      if (bSupported)
      {
        ezGALTextureResourceViewHandle hView = CreateTexture(ezGALTextureType::Texture2D, ezGALMSAASampleCount::FourSamples, false);
        m_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2DMS, false}] = hView;
        m_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2DMSArray, false}] = hView;
        m_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2DMS, false}] = hView;
        m_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2DMSArray, false}] = hView;
      }
      {
        ezGALTextureResourceViewHandle hView = CreateTexture(ezGALTextureType::TextureCube, ezGALMSAASampleCount::None, false);
        m_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::TextureCube, false}] = hView;
        m_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::TextureCubeArray, false}] = hView;
        m_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::TextureCube, false}] = hView;
        m_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::TextureCubeArray, false}] = hView;
      }
      {
        ezGALTextureResourceViewHandle hView = CreateTexture(ezGALTextureType::Texture3D, ezGALMSAASampleCount::None, false);
        m_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture3D, false}] = hView;
        m_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture3D, false}] = hView;
      }
      {
        ezGALBufferCreationDescription desc;
        desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ByteAddressBuffer | ezGALBufferUsageFlags::ShaderResource;
        desc.m_uiStructSize = 128;
        desc.m_uiTotalSize = 1280;
        desc.m_ResourceAccess.m_bImmutable = false;
        ezGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackStructuredBufferVulkan");
        m_Buffers.PushBack(hBuffer);
        ezGALBufferResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        m_BufferResourceViews[ezGALShaderResourceType::ConstantBuffer] = hView;
        m_BufferResourceViews[ezGALShaderResourceType::ConstantBuffer] = hView;
        m_BufferResourceViews[ezGALShaderResourceType::StructuredBuffer] = hView;
        m_BufferResourceViews[ezGALShaderResourceType::StructuredBuffer] = hView;
      }
      {
        ezGALBufferCreationDescription desc;
        desc.m_uiStructSize = sizeof(ezUInt32);
        desc.m_uiTotalSize = 1024;
        desc.m_BufferFlags = ezGALBufferUsageFlags::TexelBuffer | ezGALBufferUsageFlags::ShaderResource;
        desc.m_ResourceAccess.m_bImmutable = false;
        ezGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackTexelBufferVulkan");
        m_Buffers.PushBack(hBuffer);
        ezGALBufferResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        m_BufferResourceViews[ezGALShaderResourceType::TexelBuffer] = hView;
      }
      {
        ezGALTextureCreationDescription desc;
        desc.m_uiWidth = 4;
        desc.m_uiHeight = 4;
        desc.m_uiMipLevelCount = 1;
        desc.m_Format = ezGALResourceFormat::RGBAHalf;
        desc.m_Type = ezGALTextureType::Texture2D;
        desc.m_SampleCount = ezGALMSAASampleCount::None;
        desc.m_ResourceAccess.m_bImmutable = false;
        desc.m_bCreateRenderTarget = false;
        desc.m_bAllowUAV = true;
        ezGALTextureHandle hTexture = s_pDevice->CreateTexture(desc);
        EZ_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create fallback resource");
        // Debug device not set yet.
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackTextureRWVulkan");
        m_Textures.PushBack(hTexture);

        ezGALTextureUnorderedAccessViewCreationDescription descUAV;
        descUAV.m_hTexture = hTexture;
        auto hUAV = s_pDevice->CreateUnorderedAccessView(descUAV);
        m_TextureUAVs[{ezGALShaderResourceType::TextureRW, ezGALShaderTextureType::Unknown, false}] = hUAV;
      }
      {
        ezGALBufferCreationDescription desc;
        desc.m_uiStructSize = sizeof(ezUInt32);
        desc.m_uiTotalSize = 1024;
        desc.m_BufferFlags = ezGALBufferUsageFlags::TexelBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
        desc.m_ResourceAccess.m_bImmutable = false;
        ezGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackTexelBufferRWVulkan");
        m_Buffers.PushBack(hBuffer);
        ezGALBufferResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        m_BufferResourceViews[ezGALShaderResourceType::TexelBufferRW] = hView;
      }
      {
        ezGALBufferCreationDescription desc;
        desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ByteAddressBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
        desc.m_uiStructSize = 128;
        desc.m_uiTotalSize = 1280;
        desc.m_ResourceAccess.m_bImmutable = false;
        ezGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackStructuredBufferRWVulkan");
        m_Buffers.PushBack(hBuffer);
        ezGALBufferResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        m_BufferResourceViews[ezGALShaderResourceType::StructuredBufferRW] = hView;
      }
    }
    break;
    case ezGALDeviceEvent::BeforeShutdown:
    {
      m_TextureResourceViews.Clear();
      m_TextureResourceViews.Compact();
      m_BufferResourceViews.Clear();
      m_BufferResourceViews.Compact();

      m_TextureUAVs.Clear();
      m_TextureUAVs.Compact();
      m_BufferUAVs.Clear();
      m_BufferUAVs.Compact();

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
      s_pDevice = nullptr;
    }
    break;
    default:
      break;
  }
}

const ezGALTextureResourceViewVulkan* ezFallbackResourcesVulkan::GetFallbackTextureResourceView(ezGALShaderResourceType::Enum descriptorType, ezGALShaderTextureType::Enum textureType, bool bDepth)
{
  if (ezGALTextureResourceViewHandle* pView = m_TextureResourceViews.GetValue(Key{descriptorType, textureType, bDepth}))
  {
    return static_cast<const ezGALTextureResourceViewVulkan*>(s_pDevice->GetResourceView(*pView));
  }
  EZ_REPORT_FAILURE("No fallback resource set, update ezFallbackResourcesVulkan::GALDeviceEventHandler.");
  return nullptr;
}

const ezGALBufferResourceViewVulkan* ezFallbackResourcesVulkan::GetFallbackBufferResourceView(ezGALShaderResourceType::Enum descriptorType)
{
  if (ezGALBufferResourceViewHandle* pView = m_BufferResourceViews.GetValue(descriptorType))
  {
    return static_cast<const ezGALBufferResourceViewVulkan*>(s_pDevice->GetResourceView(*pView));
  }
  EZ_REPORT_FAILURE("No fallback resource set, update ezFallbackResourcesVulkan::GALDeviceEventHandler.");
  return nullptr;
}

const ezGALTextureUnorderedAccessViewVulkan* ezFallbackResourcesVulkan::GetFallbackTextureUnorderedAccessView(ezGALShaderResourceType::Enum descriptorType, ezGALShaderTextureType::Enum textureType)
{
  if (ezGALTextureUnorderedAccessViewHandle* pView = m_TextureUAVs.GetValue(Key{descriptorType, textureType, false}))
  {
    return static_cast<const ezGALTextureUnorderedAccessViewVulkan*>(s_pDevice->GetUnorderedAccessView(*pView));
  }
  EZ_REPORT_FAILURE("No fallback resource set, update ezFallbackResourcesVulkan::GALDeviceEventHandler.");
  return nullptr;
}

const ezGALBufferUnorderedAccessViewVulkan* ezFallbackResourcesVulkan::GetFallbackBufferUnorderedAccessView(ezGALShaderResourceType::Enum descriptorType)
{
  if (ezGALBufferUnorderedAccessViewHandle* pView = m_BufferUAVs.GetValue(descriptorType))
  {
    return static_cast<const ezGALBufferUnorderedAccessViewVulkan*>(s_pDevice->GetUnorderedAccessView(*pView));
  }
  EZ_REPORT_FAILURE("No fallback resource set, update ezFallbackResourcesVulkan::GALDeviceEventHandler.");
  return nullptr;
}

ezUInt32 ezFallbackResourcesVulkan::KeyHash::Hash(const Key& a)
{
  ezHashStreamWriter32 writer;
  writer << a.m_ResourceType.GetValue();
  writer << a.m_ezType.GetValue();
  writer << a.m_bDepth;
  return writer.GetHashValue();
}

bool ezFallbackResourcesVulkan::KeyHash::Equal(const Key& a, const Key& b)
{
  return a.m_ResourceType == b.m_ResourceType && a.m_ezType == b.m_ezType && a.m_bDepth == b.m_bDepth;
}

ezUInt32 ezFallbackResourcesVulkan::KeyHash::Hash(const ezEnum<ezGALShaderResourceType>& a)
{
  ezHashStreamWriter32 writer;
  writer << a.GetValue();
  return writer.GetHashValue();
}

bool ezFallbackResourcesVulkan::KeyHash::Equal(const ezEnum<ezGALShaderResourceType>& a, const ezEnum<ezGALShaderResourceType>& b)
{
  return a == b;
}


EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_FallbackResourcesVulkan);
