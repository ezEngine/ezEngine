#include "Main.h"

void ezTexConv::WriteTexHeader()
{
  if (ezPathUtils::HasExtension(m_sOutputFile, "ezTex"))
  {
    ezAssetFileHeader header;
    header.SetFileHashAndVersion(m_uiAssetHash, m_uiAssetVersion);

    header.Write(m_FileOut);

    m_FileOut << m_bSRGBOutput;
    m_FileOut << m_uiAddressU;
    m_FileOut << m_uiAddressV;
    m_FileOut << m_uiAddressW;
  }
}

ezImageFormat::Enum ezTexConv::ChooseOutputFormat(bool bSRGB, bool bAlphaIsMask) const
{
  if (m_bCompress)
  {
    if (m_uiOutputChannels == 1)
      return ezImageFormat::BC4_UNORM;

    if (m_uiOutputChannels == 2)
      return ezImageFormat::BC5_UNORM;

    if (m_uiOutputChannels == 3)
      return bSRGB ? ezImageFormat::BC1_UNORM_SRGB : ezImageFormat::BC1_UNORM;

    if (m_uiOutputChannels == 4)
    {
      if (bAlphaIsMask)
      {
        /// \todo BC1 seems to get its "1 Bit Alpha" going by premultiplying it into the color value. But might also be a bug in the DDS generator

        // BC1 supports 1 Bit alpha, so that is more efficient for textures that only use a simple mask for alpha
        if (m_bPremultiplyAlpha)
          return bSRGB ? ezImageFormat::BC1_UNORM_SRGB : ezImageFormat::BC1_UNORM;
        else
          return bSRGB ? ezImageFormat::BC2_UNORM_SRGB : ezImageFormat::BC2_UNORM;
      }
      else
      {
        return bSRGB ? ezImageFormat::BC3_UNORM_SRGB : ezImageFormat::BC3_UNORM;
      }

    }
  }
  else
  {
    if (m_uiOutputChannels == 1)
      return ezImageFormat::R8_UNORM;

    if (m_uiOutputChannels == 2)
      return ezImageFormat::R8G8_UNORM;

    if (m_uiOutputChannels == 3)
    {
      /// \todo Use B8G8R8X8 ?? I think it is not properly supported everywhere
      return bSRGB ? ezImageFormat::R8G8B8A8_UNORM_SRGB : ezImageFormat::R8G8B8A8_UNORM;
    }

    if (m_uiOutputChannels == 4)
    {
      return bSRGB ? ezImageFormat::R8G8B8A8_UNORM_SRGB : ezImageFormat::R8G8B8A8_UNORM;
    }
  }

  EZ_REPORT_FAILURE("ChooseOutputFormat: Invalid control flow");
  return ezImageFormat::R8G8B8A8_UNORM_SRGB;
}

bool ezTexConv::CanPassThroughInput() const
{
  if (m_InputImages.GetCount() != 1)
    return false;

  const ezImage& img = m_InputImages[0];
  const bool bAlphaIsMask = (img.GetImageFormat() == ezImageFormat::BC1_UNORM) || (img.GetImageFormat() == ezImageFormat::BC1_UNORM_SRGB);

  const auto format = ChooseOutputFormat(false, bAlphaIsMask);

  if (img.GetImageFormat() != format)
    return false;

  // we just check for ANY mipmaps, not for the correct amount
  const bool bHasMipmaps = img.GetNumMipLevels() > 1;

  if (bHasMipmaps != m_bGeneratedMipmaps)
    return false;

  return true;
}
