#include <PCH.h>

#include <TexConvLib/Configuration/TexConvState.h>

#include <Foundation/Image/Image.h>
#include <Foundation/Image/ImageConversion.h>

struct NameToFormat
{
  const char* name;
  const ezTexConvTargetFormat::Enum format;
};

// TODO (texconv): review this list
static NameToFormat nameToFormatMap[] = {{"nrm", ezTexConvTargetFormat::NormalMap},    {"norm", ezTexConvTargetFormat::NormalMap},
                                         {"_nb", ezTexConvTargetFormat::NormalMap},    {"ibl", ezTexConvTargetFormat::Color_Hdr},
                                         {"diff", ezTexConvTargetFormat::Color},       {"albedo", ezTexConvTargetFormat::Color},
                                         {"emissive", ezTexConvTargetFormat::Color},   {"emit", ezTexConvTargetFormat::Color},
                                         {"rough", ezTexConvTargetFormat::Grayscale},  {"metallic", ezTexConvTargetFormat::Grayscale},
                                         {"_metal", ezTexConvTargetFormat::Grayscale}, {"_ao", ezTexConvTargetFormat::Grayscale},
                                         {"height", ezTexConvTargetFormat::Grayscale}};


static ezTexConvTargetFormat::Enum DetectTargetFormatFromFilename(const char* szFile)
{
  ezStringBuilder name = ezPathUtils::GetFileName(szFile);
  name.ToLower();

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(nameToFormatMap); ++i)
  {
    if (name.FindSubString(nameToFormatMap[i].name) != nullptr)
    {
      return nameToFormatMap[i].format;
    }
  }

  return ezTexConvTargetFormat::Auto;
}

static ezTexConvTargetFormat::Enum DetectTargetFormatFromImage(const ezImage& image)
{
  const ezImageHeader& header = image.GetHeader();
  const ezImageFormat::Enum format = header.GetImageFormat();

  if (header.GetDepth() > 1)
  {
    // unsupported
    return ezTexConvTargetFormat::Auto;
  }

  if (ezImageFormat::IsSrgb(format))
  {
    // already sRGB so must be color
    return ezTexConvTargetFormat::Color;
  }

  if (format == ezImageFormat::BC5_UNORM)
  {
    return ezTexConvTargetFormat::NormalMap;
  }

  if (ezImageFormat::GetNumChannels(format) == 1)
  {
    if (ezImageFormat::GetBitsPerChannel(format, ezImageFormatChannel::R) > 8)
    {
      return ezTexConvTargetFormat::Grayscale_Hdr;
    }

    return ezTexConvTargetFormat::Grayscale;
  }

  if (ezImageFormat::GetBitsPerChannel(format, ezImageFormatChannel::R) >= 16 || format == ezImageFormat::BC6H_SF16 ||
      format == ezImageFormat::BC6H_UF16)
  {
    return ezTexConvTargetFormat::Color_Hdr;
  }

  ezImage converted;
  if (ezImageConversion::Convert(image, converted, ezImageFormat::R8G8B8A8_UNORM).Failed())
  {
    // cannot convert to RGBA -> maybe some weird lookup table format
    return ezTexConvTargetFormat::Auto;
  }

  // analyze the image content
  {
    ezUInt32 sr = 0;
    ezUInt32 sg = 0;
    ezUInt32 sb = 0;

    ezUInt32 uiExtremeNormals = 0;

    ezUInt32 uiNumPixels = header.GetWidth() * header.GetHeight();

    // Sample no more than 10000 pixels
    ezUInt32 uiStride = ezMath::Max(1U, uiNumPixels / 10000);
    uiNumPixels /= uiStride;

    const ezUInt8* pPixel = converted.GetPixelPointer<ezUInt8>();

    for (ezUInt32 uiPixel = 0; uiPixel < uiNumPixels; ++uiPixel)
    {
      // definitely not a normal map, if any Z vector points that much backwards
      uiExtremeNormals += (pPixel[2] < 90) ? 1 : 0;

      sr += pPixel[0];
      sg += pPixel[1];
      sb += pPixel[2];

      pPixel += 4 * uiStride;
    }

    // the average color in the image
    sr /= uiNumPixels;
    sg /= uiNumPixels;
    sb /= uiNumPixels;

    if (sb < 230 || sr < 128 - 60 || sr > 128 + 60 || sg < 128 - 60 || sg > 128 + 60)
    {
      // if the average color is not a proper hue of blue, it cannot be a normal map
      return ezTexConvTargetFormat::Color;
    }

    if (uiExtremeNormals > uiNumPixels / 100)
    {
      // more than 1 percent of normals pointing backwards ? => probably not a normalmap
      return ezTexConvTargetFormat::Color;
    }

    // it might just be a normal map, it does have the proper hue of blue
    return ezTexConvTargetFormat::NormalMap;
  }
}

void ezTexConvState::AdjustTargetFormat()
{
  if (m_Descriptor.m_TargetFormat == ezTexConvTargetFormat::Auto)
  {
    m_Descriptor.m_TargetFormat = DetectTargetFormatFromFilename(m_Descriptor.m_sOutputFile);
  }

  if (m_Descriptor.m_TargetFormat == ezTexConvTargetFormat::Auto)
  {
    // TODO: do not load the image just for this (load inputs once)
    ezImage img;
    if (img.LoadFrom(m_Descriptor.m_InputFiles[0]).Failed())
      return;

    m_Descriptor.m_TargetFormat = DetectTargetFormatFromImage(img);
  }
}

