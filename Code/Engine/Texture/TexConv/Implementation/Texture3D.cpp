#include <TexturePCH.h>

#include <Texture/TexConv/TexConvProcessor.h>
#include <Texture/Image/ImageUtils.h>

ezResult ezTexConvProcessor::Assemble3DTexture(ezImage& dst) const
{
  const auto& images = m_Descriptor.m_InputImages;

  return ezImageUtils::CreateVolumeTextureFromSingleFile(dst, images[0]);
}


EZ_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_Texture3D);

