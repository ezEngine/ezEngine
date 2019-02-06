#include <PCH.h>

#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

ezResult ezTexConvProcessor::AssembleCubemap(ezImage& dst) const
{
  const auto& cm = m_Descriptor.m_ChannelMappings;
  const auto& images = m_Descriptor.m_InputImages;

  if (m_Descriptor.m_ChannelMappings.GetCount() == 6)
  {
    ezImageView faces[6];
    faces[0] = images[cm[0].m_Channel[0].m_iInputImageIndex];
    faces[1] = images[cm[1].m_Channel[0].m_iInputImageIndex];
    faces[2] = images[cm[2].m_Channel[0].m_iInputImageIndex];
    faces[3] = images[cm[3].m_Channel[0].m_iInputImageIndex];
    faces[4] = images[cm[4].m_Channel[0].m_iInputImageIndex];
    faces[5] = images[cm[5].m_Channel[0].m_iInputImageIndex];

    if (ezImageUtils::CreateCubemapFrom6Files(dst, faces).Failed())
    {
      ezLog::Error("Failed to assemble cubemap from 6 images. Images must be square, with power-of-two resolutions.");
      return EZ_FAILURE;
    }
  }
  else
  {
    if (ezImageUtils::CreateCubemapFromSingleFile(dst, images[0]).Failed())
    {
      ezLog::Error("Failed to assemble cubemap from single image.");
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_TextureCube);
