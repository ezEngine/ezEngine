#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ReadbackTexture.h>

ezGALReadbackTexture::ezGALReadbackTexture(const ezGALTextureCreationDescription& Description)
  : ezGALResource(Description)
{
}

ezGALReadbackTexture::~ezGALReadbackTexture() = default;
