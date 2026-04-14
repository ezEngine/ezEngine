#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/TexConv/TexConvProcessor.h>

ezResult ezTexConvProcessor::Assemble2DArrayTexture(ezImage& dst) const
{
  EZ_PROFILE_SCOPE("Assemble2DArrayTexture");

  const auto& mappings = m_Descriptor.m_ChannelMappings;
  const ezUInt32 uiNumSlices = mappings.GetCount();

  if (uiNumSlices == 0)
  {
    ezLog::Error("No channel mappings provided for 2D array texture assembly. Need at least one slice.");
    return EZ_FAILURE;
  }

  const ezUInt32 uiWidth = m_Descriptor.m_InputImages[0].GetWidth();
  const ezUInt32 uiHeight = m_Descriptor.m_InputImages[0].GetHeight();

  ezImageHeader header;
  header.SetWidth(uiWidth);
  header.SetHeight(uiHeight);
  header.SetDepth(1);
  header.SetNumFaces(1);
  header.SetNumMipLevels(1);
  header.SetNumArrayIndices(uiNumSlices);
  header.SetImageFormat(ezImageFormat::R32G32B32A32_FLOAT);

  dst.ResetAndAlloc(header);

  for (ezUInt32 uiSlice = 0; uiSlice < uiNumSlices; ++uiSlice)
  {
    ezColor* pPixelOut = dst.GetPixelPointer<ezColor>(0, 0, uiSlice);
    EZ_SUCCEED_OR_RETURN(Assemble2DSlice(mappings[uiSlice], uiWidth, uiHeight, pPixelOut));
  }

  return EZ_SUCCESS;
}
