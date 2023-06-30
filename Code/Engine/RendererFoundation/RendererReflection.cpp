#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>
// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezGALMSAASampleCount, 1)
  EZ_ENUM_CONSTANTS(ezGALMSAASampleCount::None, ezGALMSAASampleCount::TwoSamples, ezGALMSAASampleCount::FourSamples, ezGALMSAASampleCount::EightSamples)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezGALTextureType, 1)
  EZ_ENUM_CONSTANTS(ezGALTextureType::Invalid, ezGALTextureType::Texture2D, ezGALTextureType::TextureCube, ezGALTextureType::Texture3D, ezGALTextureType::Texture2DProxy)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGALResourceAccess, ezNoBase, 1, ezRTTIDefaultAllocator<ezGALResourceAccess>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ReadBack", m_bReadBack),
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
    EZ_MEMBER_PROPERTY("CreateRenderTarget", m_bCreateRenderTarget),
    EZ_MEMBER_PROPERTY("AllowDynamicMipGeneration", m_bAllowDynamicMipGeneration),
    EZ_MEMBER_PROPERTY("ResourceAccess", m_ResourceAccess),
    // m_pExisitingNativeObject deliberately not reflected as it can't be serialized in any meaningful way.
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_RendererReflection);