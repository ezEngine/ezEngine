#include "Main.h"


ezString ezTexConv::ChannelMaskToString(ezUInt8 mask)
{
  ezStringBuilder s("<");

  if ((mask & Channel::RedSRGB) != 0)
    s.Append("r");
  if ((mask & Channel::GreenSRGB) != 0)
    s.Append("g");
  if ((mask & Channel::BlueSRGB) != 0)
    s.Append("b");
  if ((mask & Channel::AlphaSRGB) != 0)
    s.Append("a");
  if ((mask & Channel::RedLinear) != 0)
    s.Append("x");
  if ((mask & Channel::GreenLinear) != 0)
    s.Append("y");
  if ((mask & Channel::BlueLinear) != 0)
    s.Append("z");
  if ((mask & Channel::AlphaLinear) != 0)
    s.Append("w");

  s.Append(">");

  return s;
}

float ezTexConv::GetChannelValueSRGB(ezUInt32 rawColor, float& numSources)
{
  //float fRaw = ezMath::ColorByteToFloat(rawColor);
  //fRaw = ezColor::GammaToLinear(fRaw);

  const float fRaw = ezMath::ColorByteToFloat(rawColor);

  numSources += 1.0f;
  return fRaw;
}

float ezTexConv::GetChannelValueLinear(ezUInt32 rawColor, float& numSources)
{
  const float fRaw = ezMath::ColorByteToFloat(rawColor);
  numSources += 1.0f;
  return fRaw;
}

float ezTexConv::GetChannelValue(const ChannelMapping& ds, const ezUInt32 rgba)
{
  float numSources = 0.0f;
  float channelValue = 0.0f;

  if ((ds.m_uiChannelMask & Channel::RedSRGB) != 0)
    channelValue += GetChannelValueSRGB((rgba & 0x000000FF), numSources);

  if ((ds.m_uiChannelMask & Channel::GreenSRGB) != 0)
    channelValue += GetChannelValueSRGB((rgba & 0x0000FF00) >> 8, numSources);

  if ((ds.m_uiChannelMask & Channel::BlueSRGB) != 0)
    channelValue += GetChannelValueSRGB((rgba & 0x00FF0000) >> 16, numSources);

  if ((ds.m_uiChannelMask & Channel::AlphaSRGB) != 0)
    channelValue += GetChannelValueSRGB((rgba & 0xFF000000) >> 24, numSources);

  if ((ds.m_uiChannelMask & Channel::RedLinear) != 0)
    channelValue += GetChannelValueLinear((rgba & 0x000000FF), numSources);

  if ((ds.m_uiChannelMask & Channel::GreenLinear) != 0)
    channelValue += GetChannelValueLinear((rgba & 0x0000FF00) >> 8, numSources);

  if ((ds.m_uiChannelMask & Channel::BlueLinear) != 0)
    channelValue += GetChannelValueLinear((rgba & 0x00FF0000) >> 16, numSources);

  if ((ds.m_uiChannelMask & Channel::AlphaLinear) != 0)
    channelValue += GetChannelValueLinear((rgba & 0xFF000000) >> 24, numSources);

  if (numSources > 0.0f)
    channelValue /= numSources;

  return channelValue;
}