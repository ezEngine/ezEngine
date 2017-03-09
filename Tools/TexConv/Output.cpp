#include "Main.h"

void ezTexConv::WriteTexHeader()
{
  if (ezPathUtils::HasExtension(m_sOutputFile, "ezTex"))
  {
    ezAssetFileHeader header;
    header.SetFileHashAndVersion(m_uiAssetHash, m_uiAssetVersion);

    header.Write(m_FileOut);

    const ezUInt8 uiTexFileFormatVersion = 2;

    m_FileOut << uiTexFileFormatVersion;
    m_FileOut << m_bSRGBOutput;
    m_FileOut << m_uiAddressU;
    m_FileOut << m_uiAddressV;
    m_FileOut << m_uiAddressW;
    m_FileOut << m_uiFilterSetting;
  }
}

ezImageFormat::Enum ezTexConv::ChooseOutputFormat(bool bSRGB, bool bAlphaIsMask) const
{
  EZ_ASSERT_DEBUG(!bSRGB || !m_bHDROutput, "Output can not be both HDR and SRGB");
  if (m_bCompress)
  {
    if(m_bHDROutput)
    {
      switch (m_uiOutputChannels)
      {
      case 1:
      case 2:
      case 3:
        return ezImageFormat::BC6H_UF16;
      case 4:
        return ezImageFormat::R16G16B16A16_FLOAT;
      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
      }
    }

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
    if (m_bHDROutput)
    {
      // If any of the inputs is 32-bit float use 32-bit float as output, otherwise use 16-bit float as output
      ezImageFormat::Enum hdrFormat = m_InputImages[0].GetImageFormat();
      for (ezUInt32 i = 1; i < m_InputImages.GetCount(); i++)
      {
        ezImageFormat::Enum curFormat = m_InputImages[i].GetImageFormat();
        if (ezImageFormat::GetBitsPerPixel(curFormat) > ezImageFormat::GetBitsPerPixel(hdrFormat))
        {
          hdrFormat = curFormat;
        }
      }
      return hdrFormat;
    }

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

  if (m_TextureType == TextureType::Cubemap)
  {
    // if it is a cubemap, but the single input file does not contain 6 faces,
    // that means it is a texture that encodes the 6 faces in a particular pattern
    const ezImage& img = m_InputImages[0];

    if (img.GetNumFaces() != 6)
      return false;
  }

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
