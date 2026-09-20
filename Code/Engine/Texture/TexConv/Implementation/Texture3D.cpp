#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

ezResult ezTexConvProcessor::Assemble3DTexture(ezImage& dst) const
{
  EZ_PROFILE_SCOPE("Assemble3DTexture");

  const auto& images = m_Descriptor.m_InputImages;

  if (images[0].GetDepth() > 1)
  {
    // The input file is already a genuine volume texture (e.g. a DDS with a 3D resource dimension) -> use it as is.
    dst.ResetAndAlloc(images[0].GetHeader());
    dst.GetByteBlobPtr().CopyFrom(images[0].GetByteBlobPtr());
    return EZ_SUCCESS;
  }

  return ezImageUtils::CreateVolumeTextureFromSingleFile(dst, images[0]);
}
