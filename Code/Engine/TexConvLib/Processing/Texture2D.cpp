#include <PCH.h>

#include <TexConvLib/Processing/Processor.h>

ezResult ezTexConvProcessor::Assemble2DTexture()
{
  m_pCurrentScratchImage->ResetAndAlloc(m_Descriptor.m_InputImages[0].GetHeader());

  ezColor* pPixelOut = m_pCurrentScratchImage->GetPixelPointer<ezColor>();

  if (m_Descriptor.m_ChannelMappings.IsEmpty())
  {
    ezTexConvSliceChannelMapping mapping;
    mapping.m_Channel[0].m_iInputImageIndex = 0;
    mapping.m_Channel[1].m_iInputImageIndex = 0;
    mapping.m_Channel[2].m_iInputImageIndex = 0;
    mapping.m_Channel[3].m_iInputImageIndex = 0;

    return Assemble2DSlice(mapping, pPixelOut);
  }
  else
    return Assemble2DSlice(m_Descriptor.m_ChannelMappings[0], pPixelOut);
}

ezResult ezTexConvProcessor::Assemble2DSlice(const ezTexConvSliceChannelMapping& mapping, ezColor* pPixelOut)
{
  ezHybridArray<const ezColor*, 16> pSource;
  for (ezUInt32 i = 0; i < m_Descriptor.m_InputImages.GetCount(); ++i)
  {
    pSource.ExpandAndGetRef() = m_Descriptor.m_InputImages[i].GetPixelPointer<ezColor>();
  }

  for (ezUInt32 y = 0; y < m_uiTargetResolutionY; ++y)
  {
    for (ezUInt32 x = 0; x < m_uiTargetResolutionX; ++x)
    {
      const ezUInt32 pixelOffset = (y * m_uiTargetResolutionX) + x;
      ezColor* pThisPixelOut = pPixelOut + pixelOffset;

      for (ezUInt32 channel = 0; channel < 4; ++channel)
      {
        float fValue;

        const auto& cm = mapping.m_Channel[channel];
        const ezInt32 inputIndex = cm.m_iInputImageIndex;
        if (inputIndex != -1)
        {
          const ezColor* pSourcePixel = pSource[inputIndex] + pixelOffset;

          switch (cm.m_ChannelValue)
          {
            case ezTexConvChannelValue::Red:
              fValue = pSourcePixel->r;
              break;
            case ezTexConvChannelValue::Green:
              fValue = pSourcePixel->g;
              break;
            case ezTexConvChannelValue::Blue:
              fValue = pSourcePixel->b;
              break;
            case ezTexConvChannelValue::Alpha:
              fValue = pSourcePixel->a;
              break;

            default:
              EZ_ASSERT_NOT_IMPLEMENTED;
          }
        }
        else
        {
          switch (cm.m_ChannelValue)
          {
            case ezTexConvChannelValue::Black:
              fValue = 0.0f;
              break;
            case ezTexConvChannelValue::White:
            default:
              fValue = 1.0f;
              break;
          }
        }

        pThisPixelOut->GetData()[channel] = fValue;
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::GenerateMipmaps()
{
  ezImageUtils::MipMapOptions opt;

  ezImageFilterBox filterLinear;
  ezImageFilterSincWithKaiserWindow filterKaiser;

  switch (m_Descriptor.m_MipmapMode)
  {
    case ezTexConvMipmapMode::None:
      return EZ_SUCCESS;

    case ezTexConvMipmapMode::Linear:
      opt.m_filter = &filterLinear;
      break;

    case ezTexConvMipmapMode::Kaiser:
      opt.m_filter = &filterKaiser;
      break;
  }

  switch (m_Descriptor.m_WrapModes[0])
  {
    case ezTexConvWrapMode::Repeat:
      opt.m_addressModeU = ezImageAddressMode::WRAP;
      break;
    case ezTexConvWrapMode::Clamp:
      opt.m_addressModeU = ezImageAddressMode::CLAMP;
      break;
    case ezTexConvWrapMode::Mirror:
      opt.m_addressModeU = ezImageAddressMode::MIRROR;
      break;
  }

  switch (m_Descriptor.m_WrapModes[1])
  {
    case ezTexConvWrapMode::Repeat:
      opt.m_addressModeV = ezImageAddressMode::WRAP;
      break;
    case ezTexConvWrapMode::Clamp:
      opt.m_addressModeV = ezImageAddressMode::CLAMP;
      break;
    case ezTexConvWrapMode::Mirror:
      opt.m_addressModeV = ezImageAddressMode::MIRROR;
      break;
  }

  switch (m_Descriptor.m_WrapModes[2])
  {
    case ezTexConvWrapMode::Repeat:
      opt.m_addressModeW = ezImageAddressMode::WRAP;
      break;
    case ezTexConvWrapMode::Clamp:
      opt.m_addressModeW = ezImageAddressMode::CLAMP;
      break;
    case ezTexConvWrapMode::Mirror:
      opt.m_addressModeW = ezImageAddressMode::MIRROR;
      break;
  }

  opt.m_preserveCoverage = m_Descriptor.m_bPreserveMipmapCoverage;
  opt.m_alphaThreshold = m_Descriptor.m_fMipmapAlphaThreshold;

  opt.m_renormalizeNormals = m_Descriptor.m_Usage == ezTexConvUsage::NormalMap ||
                             m_Descriptor.m_Usage == ezTexConvUsage::NormalMap_Inverted;

  ezImageUtils::GenerateMipMaps(*m_pCurrentScratchImage, *m_pOtherScratchImage, opt);
  ezMath::Swap(m_pCurrentScratchImage, m_pOtherScratchImage);

  if (m_pCurrentScratchImage->GetNumMipLevels() <= 1)
  {
    ezLog::Error("Mipmap generation failed.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}
