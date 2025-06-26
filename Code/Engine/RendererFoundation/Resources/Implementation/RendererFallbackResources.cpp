#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/RendererFallbackResources.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

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

ezHashTable<ezGALRendererFallbackResources::Key, ezGALTextureHandle, ezGALRendererFallbackResources::KeyHash> ezGALRendererFallbackResources::s_TextureResourceViews;
ezHashTable<ezEnum<ezGALShaderResourceType>, ezGALBufferHandle, ezGALRendererFallbackResources::KeyHash> ezGALRendererFallbackResources::s_BufferResourceViews;
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
      auto CreateTexture = [](ezGALTextureType::Enum type, ezGALMSAASampleCount::Enum samples, bool bDepth) -> ezGALTextureHandle
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
        desc.m_bAllowRenderTargetView = bDepth;
        ezGALTextureHandle hTexture = s_pDevice->CreateTexture(desc);
        EZ_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create fallback resource");
        // Debug device not set yet.
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackResource");
        s_Textures.PushBack(hTexture);

        return hTexture;
      };
      {
        ezGALTextureHandle tex = CreateTexture(ezGALTextureType::Texture2D, ezGALMSAASampleCount::None, false);
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2D, false}] = tex;
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2DArray, false}] = tex;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2D, false}] = tex;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2DArray, false}] = tex;
      }
      {
        ezGALTextureHandle tex = CreateTexture(ezGALTextureType::Texture2D, ezGALMSAASampleCount::None, true);
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2D, true}] = tex;
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2DArray, true}] = tex;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2D, true}] = tex;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2DArray, true}] = tex;
      }

      // Swift shader can only do 4x MSAA. Add a check anyways.
      const bool bSupported = s_pDevice->GetCapabilities().m_FormatSupport[ezGALResourceFormat::BGRAUByteNormalizedsRGB].AreAllSet(ezGALResourceFormatSupport::Texture | ezGALResourceFormatSupport::MSAA4x);

      if (bSupported)
      {
        ezGALTextureHandle tex = CreateTexture(ezGALTextureType::Texture2D, ezGALMSAASampleCount::FourSamples, false);
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2DMS, false}] = tex;
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2DMSArray, false}] = tex;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2DMS, false}] = tex;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture2DMSArray, false}] = tex;
      }
      {
        ezGALTextureHandle tex = CreateTexture(ezGALTextureType::TextureCube, ezGALMSAASampleCount::None, false);
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::TextureCube, false}] = tex;
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::TextureCubeArray, false}] = tex;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::TextureCube, false}] = tex;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::TextureCubeArray, false}] = tex;
      }
      {
        ezGALTextureHandle tex = CreateTexture(ezGALTextureType::Texture3D, ezGALMSAASampleCount::None, false);
        s_TextureResourceViews[{ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture3D, false}] = tex;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureAndSampler, ezGALShaderTextureType::Texture3D, false}] = tex;
      }
      {
        ezGALBufferCreationDescription desc;
        desc.m_BufferFlags = ezGALBufferUsageFlags::ConstantBuffer | ezGALBufferUsageFlags::ShaderResource;
        desc.m_uiStructSize = 0;
        desc.m_uiTotalSize = 128;
        desc.m_ResourceAccess.m_bImmutable = false;
        ezGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackConstantBuffer");
        s_Buffers.PushBack(hBuffer);
        s_BufferResourceViews[ezGALShaderResourceType::ConstantBuffer] = hBuffer;
        s_BufferResourceViews[ezGALShaderResourceType::ConstantBuffer] = hBuffer;
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
        s_BufferResourceViews[ezGALShaderResourceType::StructuredBuffer] = hBuffer;
        s_BufferResourceViews[ezGALShaderResourceType::StructuredBuffer] = hBuffer;
      }
      {
        ezGALBufferCreationDescription desc;
        desc.m_uiStructSize = 0;
        desc.m_uiTotalSize = 1024;
        desc.m_Format = ezGALResourceFormat::RUInt;
        desc.m_BufferFlags = ezGALBufferUsageFlags::TexelBuffer | ezGALBufferUsageFlags::ShaderResource;
        desc.m_ResourceAccess.m_bImmutable = false;
        ezGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackTexelBuffer");
        s_Buffers.PushBack(hBuffer);
        s_BufferResourceViews[ezGALShaderResourceType::TexelBuffer] = hBuffer;
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
        desc.m_bAllowRenderTargetView = false;
        desc.m_bAllowUAV = true;
        ezGALTextureHandle hTexture = s_pDevice->CreateTexture(desc);
        EZ_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create fallback resource");
        // Debug device not set yet.
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackTextureRW");
        s_Textures.PushBack(hTexture);

        s_TextureResourceViews[{ezGALShaderResourceType::TextureRW, ezGALShaderTextureType::Texture2D, false}] = hTexture;
        s_TextureResourceViews[{ezGALShaderResourceType::TextureRW, ezGALShaderTextureType::Texture2DArray, false}] = hTexture;
      }
      {
        ezGALBufferCreationDescription desc;
        desc.m_uiStructSize = 0;
        desc.m_uiTotalSize = 1024;
        desc.m_Format = ezGALResourceFormat::RUInt;
        desc.m_BufferFlags = ezGALBufferUsageFlags::TexelBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
        desc.m_ResourceAccess.m_bImmutable = false;
        ezGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackTexelBufferRW");
        s_Buffers.PushBack(hBuffer);
        s_BufferResourceViews[ezGALShaderResourceType::TexelBufferRW] = hBuffer;
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
        s_BufferResourceViews[ezGALShaderResourceType::StructuredBufferRW] = hBuffer;
      }
    }
    break;
    case ezGALDeviceEvent::BeforeShutdown:
    {
      s_TextureResourceViews.Clear();
      s_TextureResourceViews.Compact();
      s_BufferResourceViews.Clear();
      s_BufferResourceViews.Compact();

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

const ezGALBufferHandle ezGALRendererFallbackResources::GetFallbackBuffer(ezEnum<ezGALShaderResourceType> resourceType)
{
  if (ezGALBufferHandle* pView = s_BufferResourceViews.GetValue(resourceType))
  {
    return *pView;
  }
  EZ_REPORT_FAILURE("No fallback resource set, update ezGALRendererFallbackResources::GALDeviceEventHandler.");
  return {};
}

const ezGALTextureHandle ezGALRendererFallbackResources::GetFallbackTexture(ezEnum<ezGALShaderResourceType> resourceType, ezEnum<ezGALShaderTextureType> textureType, bool bDepth)
{
  if (ezGALTextureHandle* pView = s_TextureResourceViews.GetValue(Key{resourceType, textureType, bDepth}))
  {
    return *pView;
  }
  EZ_REPORT_FAILURE("No fallback resource set, update ezGALRendererFallbackResources::GALDeviceEventHandler.");
  return {};
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
