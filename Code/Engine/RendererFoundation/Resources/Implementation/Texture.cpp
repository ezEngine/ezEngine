#include <RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Texture.h>

const ezGALTexture* ezGALTexture::GetParentTexture() const
{
  return this;
}

ezGALTexture::ezGALTexture(const ezGALTextureCreationDescription& Description)
  : ezGALResource(Description)
{
}

ezGALTexture::~ezGALTexture() {}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Texture);
