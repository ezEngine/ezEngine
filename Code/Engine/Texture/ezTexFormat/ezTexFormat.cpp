#include <TexturePCH.h>

#include <Foundation/IO/Stream.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

void ezTexFormat::WriteTextureHeader(ezStreamWriter& stream) const
{
  ezUInt8 uiFileFormatVersion = 2;
  stream << uiFileFormatVersion;

  stream << m_bSRGB;
  stream << m_AddressModeU;
  stream << m_AddressModeV;
  stream << m_AddressModeW;
  stream << m_TextureFilter;
}

void ezTexFormat::WriteRenderTargetHeader(ezStreamWriter& stream) const
{
  ezUInt8 uiFileFormatVersion = 5;
  stream << uiFileFormatVersion;

  // version 2
  stream << m_bSRGB;
  stream << m_AddressModeU;
  stream << m_AddressModeV;
  stream << m_AddressModeW;
  stream << m_TextureFilter;

  // version 3
  stream << m_iRenderTargetResolutionX;
  stream << m_iRenderTargetResolutionY;

  // version 4
  stream << m_fResolutionScale;

  // version 5
  stream << m_GalRenderTargetFormat;
}

void ezTexFormat::ReadHeader(ezStreamReader& stream)
{
  ezUInt8 uiFileFormatVersion = 0;
  stream >> uiFileFormatVersion;

  // version 2
  if (uiFileFormatVersion >= 2)
  {
    stream >> m_bSRGB;
    stream >> m_AddressModeU;
    stream >> m_AddressModeV;
    stream >> m_AddressModeW;
    stream >> m_TextureFilter;
  }

  // version 3
  if (uiFileFormatVersion >= 3)
  {
    stream >> m_iRenderTargetResolutionX;
    stream >> m_iRenderTargetResolutionY;
  }

  // version 4
  if (uiFileFormatVersion >= 4)
  {
    stream >> m_fResolutionScale;
  }

  // version 5
  if (uiFileFormatVersion >= 5)
  {
    stream >> m_GalRenderTargetFormat;
  }
}



EZ_STATICLINK_FILE(Texture, Texture_ezTexFormat_ezTexFormat);
