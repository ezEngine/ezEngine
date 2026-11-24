#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/TexConv/TexConvProcessor.h>

ezResult ezTexConvProcessor::Assemble2DTexture(const ezImageHeader& refImg, ezImage& dst) const
{
  EZ_PROFILE_SCOPE("Assemble2DTexture");

  dst.ResetAndAlloc(refImg);

  ezColor* pPixelOut = dst.GetPixelPointer<ezColor>();

  return Assemble2DSlice(m_Descriptor.m_ChannelMappings[0], refImg.GetWidth(), refImg.GetHeight(), pPixelOut);
}

ezResult ezTexConvProcessor::Assemble2DSlice(const ezTexConvSliceChannelMapping& mapping, ezUInt32 uiResolutionX, ezUInt32 uiResolutionY, ezColor* pPixelOut) const
{
  ezTempHybridArray<const ezColor*, 16> pSource;
  for (ezUInt32 i = 0; i < m_Descriptor.m_InputImages.GetCount(); ++i)
  {
    pSource.ExpandAndGetRef() = m_Descriptor.m_InputImages[i].GetPixelPointer<ezColor>();
  }

  const float fZero = 0.0f;
  const float fOne = 1.0f;
  const float* pSourceValues[4] = {nullptr, nullptr, nullptr, nullptr};
  ezUInt32 uiSourceStrides[4] = {0, 0, 0, 0};

  for (ezUInt32 channel = 0; channel < 4; ++channel)
  {
    const auto& cm = mapping.m_Channel[channel];
    const ezInt32 inputIndex = cm.m_iInputImageIndex;

    if (inputIndex != -1)
    {
      const ezColor* pSourcePixel = pSource[inputIndex];
      uiSourceStrides[channel] = 4;

      switch (cm.m_ChannelValue)
      {
        case ezTexConvChannelValue::Red:
          pSourceValues[channel] = &pSourcePixel->r;
          break;
        case ezTexConvChannelValue::Green:
          pSourceValues[channel] = &pSourcePixel->g;
          break;
        case ezTexConvChannelValue::Blue:
          pSourceValues[channel] = &pSourcePixel->b;
          break;
        case ezTexConvChannelValue::Alpha:
          pSourceValues[channel] = &pSourcePixel->a;
          break;

        default:
          EZ_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }
    else
    {
      uiSourceStrides[channel] = 0; // because of the constant value

      switch (cm.m_ChannelValue)
      {
        case ezTexConvChannelValue::Black:
          pSourceValues[channel] = &fZero;
          break;

        case ezTexConvChannelValue::White:
          pSourceValues[channel] = &fOne;
          break;

        default:
          if (channel == 3)
            pSourceValues[channel] = &fOne;
          else
            pSourceValues[channel] = &fZero;
          break;
      }
    }
  }

  const bool bFlip = m_Descriptor.m_bFlipHorizontal;

  if (!bFlip && (pSourceValues[0] + 1 == pSourceValues[1]) && (pSourceValues[1] + 1 == pSourceValues[2]) &&
      (pSourceValues[2] + 1 == pSourceValues[3]))
  {
    EZ_PROFILE_SCOPE("Assemble2DSlice(memcpy)");

    ezMemoryUtils::Copy<ezColor>(pPixelOut, reinterpret_cast<const ezColor*>(pSourceValues[0]), uiResolutionX * uiResolutionY);
  }
  else
  {
    EZ_PROFILE_SCOPE("Assemble2DSlice(gather)");

    for (ezUInt32 y = 0; y < uiResolutionY; ++y)
    {
      const ezUInt32 pixelWriteRowOffset = uiResolutionX * (bFlip ? (uiResolutionY - y - 1) : y);

      for (ezUInt32 x = 0; x < uiResolutionX; ++x)
      {
        float* dst = &pPixelOut[pixelWriteRowOffset + x].r;

        for (ezUInt32 c = 0; c < 4; ++c)
        {
          dst[c] = *pSourceValues[c];
          pSourceValues[c] += uiSourceStrides[c];
        }
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::DetermineTargetResolution(const ezImage& image, ezEnum<ezImageFormat> OutputImageFormat, ezUInt32& out_uiTargetResolutionX, ezUInt32& out_uiTargetResolutionY) const
{
  EZ_PROFILE_SCOPE("DetermineResolution");

  EZ_ASSERT_DEV(out_uiTargetResolutionX == 0 && out_uiTargetResolutionY == 0, "Target resolution already determined");

  const ezUInt32 uiOrgResX = image.GetWidth();
  const ezUInt32 uiOrgResY = image.GetHeight();

  out_uiTargetResolutionX = uiOrgResX;
  out_uiTargetResolutionY = uiOrgResY;

  if (image.GetDepth() > 1)
  {
    // A genuine volume input (e.g. a 3D DDS or a CUBE LUT). The input is only scaled in X and Y (see ConvertAndScaleImage()),
    // so applying the min / max resolution and downscale steps would distort it. LUTs also typically are smaller than the min resolution
    // and must not be resampled at all. Therefore the original resolution is kept.
    if (uiOrgResX > m_Descriptor.m_uiMaxResolution || uiOrgResY > m_Descriptor.m_uiMaxResolution || m_Descriptor.m_uiDownscaleSteps > 0)
    {
      ezLog::Warning("Max resolution and downscale steps are not applied to volume textures, keeping the resolution of {}x{}x{}.", uiOrgResX, uiOrgResY, image.GetDepth());
    }

    return EZ_SUCCESS;
  }

  out_uiTargetResolutionX /= (1 << m_Descriptor.m_uiDownscaleSteps);
  out_uiTargetResolutionY /= (1 << m_Descriptor.m_uiDownscaleSteps);

  out_uiTargetResolutionX = ezMath::Clamp<ezUInt32>(out_uiTargetResolutionX, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution);
  out_uiTargetResolutionY = ezMath::Clamp<ezUInt32>(out_uiTargetResolutionY, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution);

  // keep original aspect ratio
  if (uiOrgResX > uiOrgResY)
  {
    out_uiTargetResolutionY = (out_uiTargetResolutionX * uiOrgResY) / uiOrgResX;
  }
  else if (uiOrgResX < uiOrgResY)
  {
    out_uiTargetResolutionX = (out_uiTargetResolutionY * uiOrgResX) / uiOrgResY;
  }

  if (m_Descriptor.m_OutputType == ezTexConvOutputType::Volume)
  {
    // slices stored side by side in a 2D image, the depth is uiOrgResX / uiOrgResY
    // the factor is 0 when the min resolution would upscale the image, which then keeps its resolution
    const ezUInt32 uiScaleFactor = ezMath::Max(1u, uiOrgResY / ezMath::Max(1u, out_uiTargetResolutionY));
    out_uiTargetResolutionY = uiOrgResY / uiScaleFactor;
    out_uiTargetResolutionX = uiOrgResX / uiScaleFactor;
  }

  if (OutputImageFormat != ezImageFormat::UNKNOWN && ezImageFormat::RequiresFirstLevelBlockAlignment(OutputImageFormat))
  {
    const ezUInt32 blockWidth = ezImageFormat::GetBlockWidth(OutputImageFormat);

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
