#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Texture.h>

ezGALTexture::ezGALTexture(const ezGALTextureCreationDescription& Description)
  : ezGALResource(Description)
{
}

ezGALTexture::~ezGALTexture() = default;

ezGALSharedTexture::~ezGALSharedTexture() = default;



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Texture);
