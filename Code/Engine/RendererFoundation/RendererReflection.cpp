#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>
// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezGALResourceFormat, 1)
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAFloat),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAUInt),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAInt),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBFloat),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBUInt),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBInt),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::B5G6R5UNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BGRAUByteNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BGRAUByteNormalizedsRGB),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAHalf),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAUShort),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAUShortNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAShort),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAShortNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGFloat),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGUInt),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGInt),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGB10A2UInt),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGB10A2UIntNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RG11B10Float),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAUByteNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAUByteNormalizedsRGB),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAUByte),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAByteNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAByte),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGHalf),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGUShort),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGUShortNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGShort),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGShortNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGUByte),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGUByteNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGByte),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RGByteNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::DFloat),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RFloat),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RUInt),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RInt),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RHalf),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RUShort),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RUShortNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RShort),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RShortNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RUByte),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RUByteNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RByte),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::RByteNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::AUByteNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::D16),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::D24S8),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BC1),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BC1sRGB),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BC2),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BC2sRGB),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BC3),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BC3sRGB),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BC4UNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BC4Normalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BC5UNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BC5Normalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BC6UFloat),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BC6Float),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BC7UNormalized),
    EZ_ENUM_CONSTANT(ezGALResourceFormat::BC7UNormalizedsRGB)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezGALMSAASampleCount, 1)
  EZ_ENUM_CONSTANTS(ezGALMSAASampleCount::None, ezGALMSAASampleCount::TwoSamples, ezGALMSAASampleCount::FourSamples, ezGALMSAASampleCount::EightSamples)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezGALTextureType, 1)
  EZ_ENUM_CONSTANTS(ezGALTextureType::Invalid, ezGALTextureType::Texture2D, ezGALTextureType::TextureCube, ezGALTextureType::Texture3D, ezGALTextureType::Texture2DProxy, ezGALTextureType::Texture2DShared, ezGALTextureType::Texture2DArray, ezGALTextureType::TextureCubeArray)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezGALShaderResourceType, 1)
  EZ_ENUM_CONSTANTS(ezGALShaderResourceType::Unknown,
  ezGALShaderResourceType::Sampler,
  ezGALShaderResourceType::ConstantBuffer,
  ezGALShaderResourceType::PushConstants,
  ezGALShaderResourceType::Texture,
  ezGALShaderResourceType::TextureAndSampler,
  ezGALShaderResourceType::TexelBuffer,
  ezGALShaderResourceType::StructuredBuffer,
  ezGALShaderResourceType::ByteAddressBuffer,
  ezGALShaderResourceType::TextureRW)
  EZ_ENUM_CONSTANTS(ezGALShaderResourceType::TexelBufferRW,
  ezGALShaderResourceType::StructuredBufferRW,
  ezGALShaderResourceType::ByteAddressBufferRW)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGALResourceAccess, ezNoBase, 1, ezRTTIDefaultAllocator<ezGALResourceAccess>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Immutable", m_bImmutable),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGALTextureCreationDescription, ezNoBase, 1, ezRTTIDefaultAllocator<ezGALTextureCreationDescription>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Width", m_uiWidth),
    EZ_MEMBER_PROPERTY("Height", m_uiHeight),
    EZ_MEMBER_PROPERTY("Depth", m_uiDepth),
    EZ_MEMBER_PROPERTY("MipLevelCount", m_uiMipLevelCount),
    EZ_MEMBER_PROPERTY("ArraySize", m_uiArraySize),
    EZ_ENUM_MEMBER_PROPERTY("Format", ezGALResourceFormat, m_Format),
    EZ_ENUM_MEMBER_PROPERTY("SampleCount", ezGALMSAASampleCount, m_SampleCount),
    EZ_ENUM_MEMBER_PROPERTY("Type", ezGALTextureType, m_Type),
    EZ_MEMBER_PROPERTY("AllowShaderResourceView", m_bAllowShaderResourceView),
    EZ_MEMBER_PROPERTY("AllowUAV", m_bAllowUAV),
    EZ_MEMBER_PROPERTY("AllowRenderTarget", m_bAllowRenderTargetView),
    EZ_MEMBER_PROPERTY("AllowDynamicMipGeneration", m_bAllowDynamicMipGeneration),
    EZ_MEMBER_PROPERTY("ResourceAccess", m_ResourceAccess),
    // m_pExisitingNativeObject deliberately not reflected as it can't be serialized in any meaningful way.
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGALPlatformSharedHandle, ezNoBase, 1, ezRTTIDefaultAllocator<ezGALPlatformSharedHandle>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SharedTexture", m_hSharedTexture),
    EZ_MEMBER_PROPERTY("Semaphore", m_hSemaphore),
    EZ_MEMBER_PROPERTY("ProcessId", m_uiProcessId),
    EZ_MEMBER_PROPERTY("MemoryTypeIndex", m_uiMemoryTypeIndex),
    EZ_MEMBER_PROPERTY("Size", m_uiSize),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_RendererReflection);
