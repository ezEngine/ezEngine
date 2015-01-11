
#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Resources/Texture.h>

ezGALTexture::ezGALTexture(const ezGALTextureCreationDescription& Description)
  : ezGALObjectBase(Description)
{
}

ezGALTexture::~ezGALTexture()
{
}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Texture);

