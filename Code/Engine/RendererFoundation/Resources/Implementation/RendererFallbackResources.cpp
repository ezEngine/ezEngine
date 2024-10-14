#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/RendererFallbackResources.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererFoundation, FallbackResources)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezGALRendererFallbackResources::Initialize();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezGALRendererFallbackResources::DeInitialize();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezGALDevice* ezGALRendererFallbackResources::s_pDevice = nullptr;
ezEventSubscriptionID ezGALRendererFallbackResources::s_EventID = 0;

ezHashTable<ezGALRendererFallbackResources::Key, ezGALTextureResourceViewHandle, ezGALRendererFallbackResources::KeyHash> ezGALRendererFallbackResources::s_TextureResourceViews;
ezHashTable<ezEnum<ezGALShaderResourceType>, ezGALBufferResourceViewHandle, ezGALRendererFallbackResources::KeyHash> ezGALRendererFallbackResources::s_BufferResourceViews;
ezHashTable<ezGALRendererFallbackResources::Key, ezGALTextureUnorderedAccessViewHandle, ezGALRendererFallbackResources::KeyHash> ezGALRendererFallbackResources::s_TextureUAVs;
ezHashTable<ezEnum<ezGALShaderResourceType>, ezGALBufferUnorderedAccessViewHandle, ezGALRendererFallbackResources::KeyHash> ezGALRendererFallbackResources::s_BufferUAVs;
ezDynamicArray<ezGALBufferHandle> ezGALRendererFallbackResources::s_Buffers;
ezDynamicArray<ezGALTextureHandle> ezGALRendererFallbackResources::s_Textures;

void ezGALRendererFallbackResources::Initialize()
{
  s_EventID = ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&ezGALRendererFallbackResources::GALDeviceEventHandler));
}

void ezGALRendererFallbackResources::DeInitialize()
{
  ezGALDevice::s_Events.RemoveEventHandler(s_EventID);
}
void ezGALRendererFallbackResources::GALDeviceEventHandler(const ezGALDeviceEvent& e)
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
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackResource");
        s_Textures.PushBack(hTexture);
        return s_pDevice->GetDefaultResourceView(hTexture);
      };
      {
        ezGALTextureResourceViewHandle hView = CreateTexture(ezGALTextureType::Texture2D, ezGALMSAASampleCount::None, false);
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2D, false}] = hView;
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2DArray, false}] = hView;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2D, false}] = hView;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2DArray, false}] = hView;
      }
      {
        ezGALTextureResourceViewHandle hView = CreateTexture(ezGALTextureType::Texture2D, ezGALMSAASampleCount::None, true);
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2D, true}] = hView;
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2DArray, true}] = hView;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2D, true}] = hView;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2DArray, true}] = hView;
      }

      // Swift shader can only do 4x MSAA. Add a check anyways.
      const bool bSupported = s_pDevice->GetCapabilities().m_FormatSupport[ezGALResourceFormat::BGRAUByteNormalizedsRGB].AreAllSet(ezGALResourceFormatSupport::Texture | ezGALResourceFormatSupport::MSAA4x);

      if (bSupported)
      {
        ezGALTextureResourceViewHandle hView = CreateTexture(ezGALTextureType::Texture2D, ezGALMSAASampleCount::FourSamples, false);
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2DMS, false}] = hView;
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2DMSArray, false}] = hView;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2DMS, false}] = hView;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2DMSArray, false}] = hView;
      }
      {
        ezGALTextureResourceViewHandle hView = CreateTexture(ezGALTextureType::TextureCube, ezGALMSAASampleCount::None, false);
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::TextureCube, false}] = hView;
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::TextureCubeArray, false}] = hView;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::TextureCube, false}] = hView;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::TextureCubeArray, false}] = hView;
      }
      {
        ezGALTextureResourceViewHandle hView = CreateTexture(ezGALTextureType::Texture3D, ezGALMSAASampleCount::None, false);
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture3D, false}] = hView;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture3D, false}] = hView;
      }
      {
        ezGALBufferCreationDescription desc;
        desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;
        desc.m_uiStructSize = 128;
        desc.m_uiTotalSize = 1280;
        desc.m_ResourceAccess.m_bImmutable = false;
        ezGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackStructuredBuffer");
        s_Buffers.PushBack(hBuffer);
        ezGALBufferResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        s_BufferResourceViews[ezGALShaderResourceType::ConstantBuffer] = hView;
        s_BufferResourceViews[ezGALShaderResourceType::ConstantBuffer] = hView;
        s_BufferResourceViews[ezGALShaderResourceType::StructuredBuffer] = hView;
        s_BufferResourceViews[ezGALShaderResourceType::StructuredBuffer] = hView;
      }
      {
        ezGALBufferCreationDescription desc;
        desc.m_uiStructSize = sizeof(ezUInt32);
        desc.m_uiTotalSize = 1024;
        desc.m_BufferFlags = ezGALBufferUsageFlags::TexelBuffer | ezGALBufferUsageFlags::ShaderResource;
        desc.m_ResourceAccess.m_bImmutable = false;
        ezGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackTexelBuffer");
        s_Buffers.PushBack(hBuffer);
        ezGALBufferResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        s_BufferResourceViews[ezGALShaderResourceType::TexelBuffer] = hView;
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
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackTextureRW");
        s_Textures.PushBack(hTexture);

        ezGALTextureUnorderedAccessViewCreationDescription descUAV;
        descUAV.m_hTexture = hTexture;
        auto hUAV = s_pDevice->CreateUnorderedAccessView(descUAV);
        s_TextureUAVs[{ezGALShaderResourceType::TextureRW, ezGALShaderTextureType::Unknown, false}] = hUAV;
      }
      {
        ezGALBufferCreationDescription desc;
        desc.m_uiStructSize = sizeof(ezUInt32);
        desc.m_uiTotalSize = 1024;
        desc.m_BufferFlags = ezGALBufferUsageFlags::TexelBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
        desc.m_ResourceAccess.m_bImmutable = false;
        ezGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackTexelBufferRW");
        s_Buffers.PushBack(hBuffer);
        ezGALBufferResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        s_BufferResourceViews[ezGALShaderResourceType::TexelBufferRW] = hView;
      }
      {
        ezGALBufferCreationDescription desc;
        desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
        desc.m_uiStructSize = 128;
        desc.m_uiTotalSize = 1280;
        desc.m_ResourceAccess.m_bImmutable = false;
        ezGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackStructuredBufferRW");
        s_Buffers.PushBack(hBuffer);
        ezGALBufferResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        s_BufferResourceViews[ezGALShaderResourceType::StructuredBufferRW] = hView;
      }
    }
    break;
    case ezGALDeviceEvent::BeforeShutdown:
    {
      s_TextureResourceViews.Clear();
      s_TextureResourceViews.Compact();
      s_BufferResourceViews.Clear();
      s_BufferResourceViews.Compact();

      s_TextureUAVs.Clear();
      s_TextureUAVs.Compact();
      s_BufferUAVs.Clear();
      s_BufferUAVs.Compact();

      for (ezGALBufferHandle hBuffer : s_Buffers)
      {
        s_pDevice->DestroyBuffer(hBuffer);
      }
      s_Buffers.Clear();
      s_Buffers.Compact();

      for (ezGALTextureHandle hTexture : s_Textures)
      {
        s_pDevice->DestroyTexture(hTexture);
      }
      s_Textures.Clear();
      s_Textures.Compact();
      s_pDevice = nullptr;
    }
    break;
    default:
      break;
  }
}

const ezGALTextureResourceView* ezGALRendererFallbackResources::GetFallbackTextureResourceView(ezGALShaderResourceType::Enum descriptorType, ezGALShaderTextureType::Enum textureType, bool bDepth)
{
  if (ezGALTextureResourceViewHandle* pView = s_TextureResourceViews.GetValue(Key{descriptorType, textureType, bDepth}))
  {
    return static_cast<const ezGALTextureResourceView*>(s_pDevice->GetResourceView(*pView));
  }
  EZ_REPORT_FAILURE("No fallback resource set, update ezGALRendererFallbackResources::GALDeviceEventHandler.");
  return nullptr;
}

const ezGALBufferResourceView* ezGALRendererFallbackResources::GetFallbackBufferResourceView(ezGALShaderResourceType::Enum descriptorType)
{
  if (ezGALBufferResourceViewHandle* pView = s_BufferResourceViews.GetValue(descriptorType))
  {
    return static_cast<const ezGALBufferResourceView*>(s_pDevice->GetResourceView(*pView));
  }
  EZ_REPORT_FAILURE("No fallback resource set, update ezGALRendererFallbackResources::GALDeviceEventHandler.");
  return nullptr;
}

const ezGALTextureUnorderedAccessView* ezGALRendererFallbackResources::GetFallbackTextureUnorderedAccessView(ezGALShaderResourceType::Enum descriptorType, ezGALShaderTextureType::Enum textureType)
{
  if (ezGALTextureUnorderedAccessViewHandle* pView = s_TextureUAVs.GetValue(Key{descriptorType, textureType, false}))
  {
    return static_cast<const ezGALTextureUnorderedAccessView*>(s_pDevice->GetUnorderedAccessView(*pView));
  }
  EZ_REPORT_FAILURE("No fallback resource set, update ezGALRendererFallbackResources::GALDeviceEventHandler.");
  return nullptr;
}

const ezGALBufferUnorderedAccessView* ezGALRendererFallbackResources::GetFallbackBufferUnorderedAccessView(ezGALShaderResourceType::Enum descriptorType)
{
  if (ezGALBufferUnorderedAccessViewHandle* pView = s_BufferUAVs.GetValue(descriptorType))
  {
    return static_cast<const ezGALBufferUnorderedAccessView*>(s_pDevice->GetUnorderedAccessView(*pView));
  }
  EZ_REPORT_FAILURE("No fallback resource set, update ezGALRendererFallbackResources::GALDeviceEventHandler.");
  return nullptr;
}

ezUInt32 ezGALRendererFallbackResources::KeyHash::Hash(const Key& a)
{
  ezHashStreamWriter32 writer;
  writer << a.m_ResourceType.GetValue();
  writer << a.m_ezType.GetValue();
  writer << a.m_bDepth;
  return writer.GetHashValue();
}

bool ezGALRendererFallbackResources::KeyHash::Equal(const Key& a, const Key& b)
{
  return a.m_ResourceType == b.m_ResourceType && a.m_ezType == b.m_ezType && a.m_bDepth == b.m_bDepth;
}

ezUInt32 ezGALRendererFallbackResources::KeyHash::Hash(const ezEnum<ezGALShaderResourceType>& a)
{
  ezHashStreamWriter32 writer;
  writer << a.GetValue();
  return writer.GetHashValue();
}

bool ezGALRendererFallbackResources::KeyHash::Equal(const ezEnum<ezGALShaderResourceType>& a, const ezEnum<ezGALShaderResourceType>& b)
{
  return a == b;
}


EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_RendererFallbackResources);
