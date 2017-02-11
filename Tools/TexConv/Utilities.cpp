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

float ezTexConv::GetChannelValue(const ChannelMapping& ds, const ezColor& rgba)
{
  float numSources = 0.0f;
  float channelValue = 0.0f;

  if ((ds.m_uiChannelMask & Channel::Red) != 0)
  {
    channelValue += rgba.r;
    numSources += 1.0f;
  }

  if ((ds.m_uiChannelMask & Channel::Green) != 0)
  {
    channelValue += rgba.g;
    numSources += 1.0f;
  }

  if ((ds.m_uiChannelMask & Channel::Blue) != 0)
  {
    channelValue += rgba.b;
    numSources += 1.0f;
  }

  if ((ds.m_uiChannelMask & Channel::Alpha) != 0)
  {
    channelValue += rgba.a;
    numSources += 1.0f;
  }

  if (numSources > 0.0f)
    channelValue /= numSources;

  return channelValue;
}

bool ezTexConv::IsImageAlphaBinaryMask(const ezImage& img)
{
  EZ_ASSERT_DEV(img.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "Unsupported image format");

  for (ezUInt32 array = 0; array < img.GetNumArrayIndices(); ++array)
  {
    for (ezUInt32 face = 0; face < img.GetNumFaces(); ++face)
    {
      const float* pData = img.GetPixelPointer<float>(0, face, array);
      EZ_ASSERT_DEBUG(img.GetRowPitch() % sizeof(float) == 0, "Row pitch should be a multiple of sizeof(float)");
      const ezUInt32 uiRowPitch = img.GetRowPitch() / sizeof(float);

      for (ezUInt32 y = 0; y < img.GetHeight(); ++y)
      {
        for (ezUInt32 x = 0; x < img.GetWidth(); ++x)
        {
          const float alpha = pData[x * 4 + 3];

          if (alpha > 0.02 && alpha < 0.98)
            return false;
        }

        pData += uiRowPitch;
      }
    }
  }

  return true;
}
