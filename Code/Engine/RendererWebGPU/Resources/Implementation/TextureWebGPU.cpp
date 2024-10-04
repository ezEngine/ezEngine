#include <RendererWebGPU/RendererWebGPUPCH.h>

#include <RendererCore/Textures/TextureUtils.h>
#include <RendererWebGPU/Device/DeviceWebGPU.h>
#include <RendererWebGPU/Resources/TextureWebGPU.h>
#include <Texture/Image/ImageFormat.h>

wgpu::TextureFormat ToWGPU(ezGALResourceFormat::Enum format)
{
  switch (format)
  {
    case ezGALResourceFormat::RGBAFloat:
      return wgpu::TextureFormat::RGBA32Float;
    case ezGALResourceFormat::RGBAUInt:
      return wgpu::TextureFormat::RGBA32Uint;
    case ezGALResourceFormat::RGBAInt:
      return wgpu::TextureFormat::RGBA32Sint;
    case ezGALResourceFormat::BGRAUByteNormalized:
      return wgpu::TextureFormat::BGRA8Unorm;
    case ezGALResourceFormat::BGRAUByteNormalizedsRGB:
      return wgpu::TextureFormat::BGRA8UnormSrgb;
    case ezGALResourceFormat::RGBAHalf:
      return wgpu::TextureFormat::RGBA16Float;
    case ezGALResourceFormat::RGBAUShort:
      return wgpu::TextureFormat::RGBA16Uint;

#if EZ_DISABLED(EZ_PLATFORM_WEB)
    case ezGALResourceFormat::RGBAUShortNormalized:
      return wgpu::TextureFormat::RGBA16Unorm;
    case ezGALResourceFormat::RGBAShort:
      return wgpu::TextureFormat::RGBA16Sint;
    case ezGALResourceFormat::RGBAShortNormalized:
      return wgpu::TextureFormat::RGBA16Snorm;
#endif

    case ezGALResourceFormat::RGFloat:
      return wgpu::TextureFormat::RG32Float;
    case ezGALResourceFormat::RGUInt:
      return wgpu::TextureFormat::RG32Uint;
    case ezGALResourceFormat::RGInt:
      return wgpu::TextureFormat::RG32Sint;
    case ezGALResourceFormat::RGB10A2UInt:
      return wgpu::TextureFormat::RGB10A2Uint;
    case ezGALResourceFormat::RGB10A2UIntNormalized:
      return wgpu::TextureFormat::RGB10A2Unorm;
    case ezGALResourceFormat::RG11B10Float:
      return wgpu::TextureFormat::RG11B10Ufloat;
    case ezGALResourceFormat::RGBAUByteNormalized:
      return wgpu::TextureFormat::RGBA8Unorm;
    case ezGALResourceFormat::RGBAUByteNormalizedsRGB:
      return wgpu::TextureFormat::RGBA8UnormSrgb;
    case ezGALResourceFormat::RGBAUByte:
      return wgpu::TextureFormat::RGBA8Uint;
    case ezGALResourceFormat::RGBAByteNormalized:
      return wgpu::TextureFormat::RGBA8Snorm;
    case ezGALResourceFormat::RGBAByte:
      return wgpu::TextureFormat::RGBA8Sint;
    case ezGALResourceFormat::RGHalf:
      return wgpu::TextureFormat::RG16Float;
    case ezGALResourceFormat::RGUShort:
      return wgpu::TextureFormat::RG16Uint;

#if EZ_DISABLED(EZ_PLATFORM_WEB)
    case ezGALResourceFormat::RGUShortNormalized:
      return wgpu::TextureFormat::RG16Unorm;
    case ezGALResourceFormat::RGShort:
      return wgpu::TextureFormat::RG16Sint;
    case ezGALResourceFormat::RGShortNormalized:
      return wgpu::TextureFormat::RG16Snorm;
#endif

    case ezGALResourceFormat::RGUByte:
      return wgpu::TextureFormat::RG8Uint;
    case ezGALResourceFormat::RGUByteNormalized:
      return wgpu::TextureFormat::RG8Unorm;
    case ezGALResourceFormat::RGByte:
      return wgpu::TextureFormat::RG8Sint;
    case ezGALResourceFormat::RGByteNormalized:
      return wgpu::TextureFormat::RG8Snorm;
    case ezGALResourceFormat::DFloat:
      return wgpu::TextureFormat::Depth32Float;
    case ezGALResourceFormat::RFloat:
      return wgpu::TextureFormat::R32Float;
    case ezGALResourceFormat::RUInt:
      return wgpu::TextureFormat::R32Uint;
    case ezGALResourceFormat::RInt:
      return wgpu::TextureFormat::R32Sint;
    case ezGALResourceFormat::RHalf:
      return wgpu::TextureFormat::R16Float;
    case ezGALResourceFormat::RUShort:
      return wgpu::TextureFormat::R16Uint;

#if EZ_DISABLED(EZ_PLATFORM_WEB)
    case ezGALResourceFormat::RUShortNormalized:
      return wgpu::TextureFormat::R16Unorm;
    case ezGALResourceFormat::RShort:
      return wgpu::TextureFormat::R16Sint;
    case ezGALResourceFormat::RShortNormalized:
      return wgpu::TextureFormat::R16Snorm;
#endif

    case ezGALResourceFormat::RUByte:
      return wgpu::TextureFormat::R8Uint;
    case ezGALResourceFormat::RUByteNormalized:
      return wgpu::TextureFormat::R8Unorm;
    case ezGALResourceFormat::RByte:
      return wgpu::TextureFormat::R8Sint;
    case ezGALResourceFormat::RByteNormalized:
      return wgpu::TextureFormat::R8Snorm;
    case ezGALResourceFormat::D16:
      return wgpu::TextureFormat::Depth16Unorm;
    case ezGALResourceFormat::D24S8:
      return wgpu::TextureFormat::Depth24PlusStencil8;
    case ezGALResourceFormat::BC1:
      return wgpu::TextureFormat::BC1RGBAUnorm;
    case ezGALResourceFormat::BC1sRGB:
      return wgpu::TextureFormat::BC1RGBAUnormSrgb;
    case ezGALResourceFormat::BC2:
      return wgpu::TextureFormat::BC2RGBAUnorm;
    case ezGALResourceFormat::BC2sRGB:
      return wgpu::TextureFormat::BC2RGBAUnormSrgb;
    case ezGALResourceFormat::BC3:
      return wgpu::TextureFormat::BC3RGBAUnorm;
    case ezGALResourceFormat::BC3sRGB:
      return wgpu::TextureFormat::BC3RGBAUnormSrgb;
    case ezGALResourceFormat::BC4UNormalized:
      return wgpu::TextureFormat::BC4RUnorm;
    case ezGALResourceFormat::BC4Normalized:
      return wgpu::TextureFormat::BC4RSnorm;
    case ezGALResourceFormat::BC5UNormalized:
      return wgpu::TextureFormat::BC5RGUnorm;
    case ezGALResourceFormat::BC5Normalized:
      return wgpu::TextureFormat::BC5RGSnorm;
    case ezGALResourceFormat::BC6UFloat:
      return wgpu::TextureFormat::BC6HRGBUfloat;
    case ezGALResourceFormat::BC6Float:
      return wgpu::TextureFormat::BC6HRGBFloat;
    case ezGALResourceFormat::BC7UNormalized:
      return wgpu::TextureFormat::BC7RGBAUnorm;
    case ezGALResourceFormat::BC7UNormalizedsRGB:
      return wgpu::TextureFormat::BC7RGBAUnormSrgb;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return wgpu::TextureFormat::Undefined;
}

ezGALTextureWebGPU::ezGALTextureWebGPU(const ezGALTextureCreationDescription& Description)
  : ezGALTexture(Description)
{
}

ezGALTextureWebGPU::~ezGALTextureWebGPU() = default;

ezResult ezGALTextureWebGPU::InitPlatform(ezGALDevice* pDevice0, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  EZ_WEBGPU_TRACE();

  EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "WebGPU can't create resources on a different thread.");

  ezGALDeviceWebGPU* pDevice = static_cast<ezGALDeviceWebGPU*>(pDevice0);

  wgpu::TextureDescriptor desc;

  EZ_ASSERT_DEV(m_Description.m_uiArraySize == 1, "Array textures are not supported");
  EZ_ASSERT_DEV(m_Description.m_uiDepth == 1, "Depth textures are not supported");

  switch (m_Description.m_Type)
  {
    case ezGALTextureType::Texture2D:
      desc.dimension = wgpu::TextureDimension::e2D;
      break;
    case ezGALTextureType::Texture3D:
      desc.dimension = wgpu::TextureDimension::e3D;
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  desc.format = ToWGPU(m_Description.m_Format);
  desc.mipLevelCount = m_Description.m_uiMipLevelCount;
  desc.sampleCount = m_Description.m_SampleCount;
  desc.size.width = m_Description.m_uiWidth;
  desc.size.height = m_Description.m_uiHeight;
  desc.size.depthOrArrayLayers = m_Description.m_uiDepth;
  desc.usage = wgpu::TextureUsage::None;

  if (m_Description.m_bAllowShaderResourceView)
    desc.usage |= wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;

  if (m_Description.m_bCreateRenderTarget)
    desc.usage |= wgpu::TextureUsage::RenderAttachment;

  switch (m_Description.m_ResourceAccess.m_MemoryUsage)
  {
    case ezGALMemoryUsage::Staging:
      desc.usage |= wgpu::BufferUsage::CopySrc;
      desc.usage |= wgpu::BufferUsage::MapWrite;
      break;
    case ezGALMemoryUsage::Readback:
      desc.usage |= wgpu::BufferUsage::CopyDst;
      desc.usage |= wgpu::BufferUsage::MapRead;
      break;
    default:
      desc.usage |= wgpu::BufferUsage::CopySrc;
      desc.usage |= wgpu::BufferUsage::CopyDst;
      break;
  }

  if (!m_Description.m_ResourceAccess.m_bImmutable || !pInitialData.IsEmpty())
    desc.usage |= wgpu::TextureUsage::CopyDst;

  // TODO WebGPU: all the texture flags
  // if (m_Description.m_bAllowUAV)
  //  desc.usage |= wgpu::TextureUsage::StorageBinding;

  m_Texture = pDevice->GetDevice().CreateTexture(&desc);


  if (!pInitialData.IsEmpty())
  {
    EZ_ASSERT_DEV(desc.mipLevelCount == pInitialData.GetCount(), "");

    const ezImageFormat::Enum imgFormat = ezTextureUtils::GalFormatToImageFormat(m_Description.m_Format);
    const ezUInt32 uiMinSize = ezImageFormat::GetBlockWidth(imgFormat);

    wgpu::Extent3D size;
    size.width = m_Description.m_uiWidth;
    size.height = m_Description.m_uiHeight;
    size.depthOrArrayLayers = m_Description.m_uiDepth;

    for (ezUInt32 i = 0; i < desc.mipLevelCount; ++i)
    {
      const auto& src = pInitialData[i];

      wgpu::ImageCopyTexture destination;
      destination.texture = m_Texture;
      destination.mipLevel = i;
      destination.origin = {0, 0, 0};
      destination.aspect = wgpu::TextureAspect::All;

      wgpu::TextureDataLayout source;
      source.offset = 0;
      source.bytesPerRow = src.m_uiRowPitch;
      source.rowsPerImage = src.m_uiSlicePitch / src.m_uiRowPitch;

      wgpu::Queue queue = pDevice->GetDevice().GetQueue();
      queue.WriteTexture(&destination, src.m_pData, src.m_uiSlicePitch, &source, &size);

      size.width = ezMath::Max(uiMinSize, size.width / 2);
      size.height = ezMath::Max(uiMinSize, size.height / 2);
    }
  }

  return EZ_SUCCESS;
}

ezResult ezGALTextureWebGPU::DeInitPlatform(ezGALDevice* pDevice)
{
  if (m_Texture)
  {
    m_Texture.Destroy();
    m_Texture = nullptr;
  }

  return EZ_SUCCESS;
}

void ezGALTextureWebGPU::SetDebugNamePlatform(const char* szName) const
{
  if (m_Texture)
  {
    m_Texture.SetLabel(szName);
  }
}
