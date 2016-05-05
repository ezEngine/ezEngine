#include "Main.h"

void ezTexConv::WriteTexHeader(ezFileWriter& fileOut)
{
  if (ezPathUtils::HasExtension(m_sOutputFile, "ezTex"))
  {
    ezAssetFileHeader header;
    header.SetFileHashAndVersion(m_uiAssetHash, m_uiAssetVersion);

    header.Write(fileOut);

    fileOut << m_bSRGBOutput;
  }
}

ezImageFormat::Enum ezTexConv::ChooseOutputFormat(bool bSRGB) const
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
      /// \todo Use BC1 if entire alpha channel is either 0 or 255 (mask)
      return bSRGB ? ezImageFormat::BC3_UNORM_SRGB : ezImageFormat::BC3_UNORM;
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

  const auto format = ChooseOutputFormat(false);

  if (img.GetImageFormat() != format)
    return false;

  // we just check for ANY mipmaps, not for the correct amount
  const bool bHasMipmaps = img.GetNumMipLevels() > 0;

  if (bHasMipmaps != m_bGeneratedMipmaps)
    return false;

  return true;
}
