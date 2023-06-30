#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGALPlatformSharedHandle, ezNoBase, 1, ezRTTIDefaultAllocator<ezGALPlatformSharedHandle>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ProcessId", m_uiProcessId),
    EZ_MEMBER_PROPERTY("a", a),
    EZ_MEMBER_PROPERTY("b", b),
    EZ_MEMBER_PROPERTY("MemoryTypeIndex", m_uiMemoryTypeIndex),
    EZ_MEMBER_PROPERTY("Size", m_uiSize),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezGALTexture::ezGALTexture(const ezGALTextureCreationDescription& Description)
  : ezGALResource(Description)
{
}

ezGALTexture::~ezGALTexture() = default;

ezGALSharedTexture::~ezGALSharedTexture() = default;



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Texture);
