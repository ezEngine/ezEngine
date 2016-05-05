#include "Main.h"


ezString ezTexConv::ChannelMaskToString(ezUInt8 mask)
{
  ezStringBuilder s("<");

  if ((mask & Channel::Red) != 0)
    s.Append("r");
  if ((mask & Channel::Green) != 0)
    s.Append("g");
  if ((mask & Channel::Blue) != 0)
    s.Append("b");
  if ((mask & Channel::Alpha) != 0)
    s.Append("a");

  s.Append(">");

  return s;
}

float ezTexConv::GetChannelValue(const ChannelMapping& ds, const ezUInt32 rgba)
{
  float numSources = 0.0f;
  float channelValue = 0.0f;

  if ((ds.m_uiChannelMask & Channel::Red) != 0)
  {
    channelValue += ezMath::ColorByteToFloat(rgba & 0x000000FF);
    numSources += 1.0f;
  }

  if ((ds.m_uiChannelMask & Channel::Green) != 0)
  {
    channelValue += ezMath::ColorByteToFloat((rgba & 0x0000FF00) >> 8);
    numSources += 1.0f;
  }

  if ((ds.m_uiChannelMask & Channel::Blue) != 0)
  {
    channelValue += ezMath::ColorByteToFloat((rgba & 0x00FF0000) >> 16);
    numSources += 1.0f;
  }

  if ((ds.m_uiChannelMask & Channel::Alpha) != 0)
  {
    channelValue += ezMath::ColorByteToFloat((rgba & 0xFF000000) >> 24);
    numSources += 1.0f;
  }

  if (numSources > 0.0f)
    channelValue /= numSources;

  return channelValue;
}

bool ezTexConv::IsImageAlphaBinaryMask(const ezImage& img)
{
  EZ_ASSERT_DEV(img.GetImageFormat() == ezImageFormat::R8G8B8A8_UNORM, "Unsupported image format");

  for (ezUInt32 face = 0; face < img.GetNumFaces(); ++face)
  {
    const ezUInt8* pData = img.GetPixelPointer<ezUInt8>(0, face);
    const ezUInt32 uiRowPitch = img.GetRowPitch();

    for (ezUInt32 y = 0; y < img.GetHeight(); ++y)
    {
      for (ezUInt32 x = 0; x < img.GetWidth(); ++x)
      {
        const ezUInt8 alpha = pData[x * 4 + 3];

        if (alpha > 5 && alpha < 250)
          return false;
      }

      pData += uiRowPitch;
    }
  }

  return true;
}