#include <TexturePCH.h>

#include <Texture/TexConv/TexConvProcessor.h>

ezResult ezTexConvProcessor::Assemble2DTexture(const ezImageHeader& refImg, ezImage& dst) const
{
  dst.ResetAndAlloc(refImg);

  ezColor* pPixelOut = dst.GetPixelPointer<ezColor>();

  return Assemble2DSlice(m_Descriptor.m_ChannelMappings[0], refImg.GetWidth(), refImg.GetHeight(), pPixelOut);
}

ezResult ezTexConvProcessor::Assemble2DSlice(
  const ezTexConvSliceChannelMapping& mapping, ezUInt32 uiResolutionX, ezUInt32 uiResolutionY, ezColor* pPixelOut) const
{
  ezHybridArray<const ezColor*, 16> pSource;
  for (ezUInt32 i = 0; i < m_Descriptor.m_InputImages.GetCount(); ++i)
  {
    pSource.ExpandAndGetRef() = m_Descriptor.m_InputImages[i].GetPixelPointer<ezColor>();
  }

  const bool bFlip = m_Descriptor.m_bFlipHorizontal;

  for (ezUInt32 y = 0; y < uiResolutionY; ++y)
  {
    const ezUInt32 pixelReadRowOffset = uiResolutionX * y;
    const ezUInt32 pixelWriteRowOffset = uiResolutionX * (bFlip ? (uiResolutionY - y - 1) : y);

    for (ezUInt32 x = 0; x < uiResolutionX; ++x)
    {

      for (ezUInt32 channel = 0; channel < 4; ++channel)
      {
        float fValue = 1.0f;

        const auto& cm = mapping.m_Channel[channel];
        const ezInt32 inputIndex = cm.m_iInputImageIndex;
        if (inputIndex != -1)
        {
          const ezColor* pSourcePixel = pSource[inputIndex] + pixelReadRowOffset + x;

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
              break;
          }
        }
        else
        {
          switch (cm.m_ChannelValue)
          {
            case ezTexConvChannelValue::Black:
              fValue = 0.0f;
              break;

            default:
              fValue = 1.0f;
              break;
          }
        }

        pPixelOut[pixelWriteRowOffset + x].GetData()[channel] = fValue;
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::DetermineTargetResolution(
  const ezImage& image, ezEnum<ezImageFormat> OutputImageFormat, ezUInt32& out_uiTargetResolutionX, ezUInt32& out_uiTargetResolutionY) const
{
  EZ_ASSERT_DEV(out_uiTargetResolutionX == 0 && out_uiTargetResolutionY == 0, "Target resolution already determined");

  const ezUInt32 uiOrgResX = image.GetWidth();
  const ezUInt32 uiOrgResY = image.GetHeight();

  out_uiTargetResolutionX = uiOrgResX;
  out_uiTargetResolutionY = uiOrgResY;

  out_uiTargetResolutionX /= (1 << m_Descriptor.m_uiDownscaleSteps);
  out_uiTargetResolutionY /= (1 << m_Descriptor.m_uiDownscaleSteps);

  out_uiTargetResolutionX = ezMath::Clamp(out_uiTargetResolutionX, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution);
  out_uiTargetResolutionY = ezMath::Clamp(out_uiTargetResolutionY, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution);

  if (m_Descriptor.m_OutputType == ezTexConvOutputType::Volume)
  {
    ezUInt32 uiScaleFactor = uiOrgResY / out_uiTargetResolutionY;
    out_uiTargetResolutionX = uiOrgResX / uiScaleFactor;
  }

  if (OutputImageFormat != ezImageFormat::UNKNOWN && ezImageFormat::RequiresFirstLevelBlockAlignment(OutputImageFormat))
  {
    ezUInt32 blockWidth = ezImageFormat::GetBlockWidth(OutputImageFormat);
    ezUInt32 currentWidth = out_uiTargetResolutionX;
    ezUInt32 currentHeight = out_uiTargetResolutionY;
    bool issueWarning = false;
    if (out_uiTargetResolutionX % blockWidth != 0)
    {
      out_uiTargetResolutionX = ezMath::RoundUp(out_uiTargetResolutionX, static_cast<ezUInt16>(blockWidth));
      issueWarning = true;
    }

    ezUInt32 blockHeight = ezImageFormat::GetBlockHeight(OutputImageFormat);
    if (out_uiTargetResolutionY % blockHeight != 0)
    {
      out_uiTargetResolutionY = ezMath::RoundUp(out_uiTargetResolutionY, static_cast<ezUInt16>(blockHeight));
      issueWarning = true;
    }

    if (issueWarning)
    {
      ezLog::Warning(
        "Chosen output image format is compressed, but target resolution does not fulfill block size requirements. {}x{} -> downscale {} / "
        "clamp({}, {}) -> {}x{}, adjusted to {}x{}",
        uiOrgResX, uiOrgResY, m_Descriptor.m_uiDownscaleSteps, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution, currentWidth,
        currentHeight, out_uiTargetResolutionX, out_uiTargetResolutionY);
    }
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_Texture2D);
