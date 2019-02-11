#include <TexturePCH.h>

#include <Texture/TexConv/TexConvProcessor.h>

#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

struct FileSuffixToUsage
{
  const char* m_szSuffix = nullptr;
  const ezTexConvUsage::Enum m_Usage = ezTexConvUsage::Auto;
};

static FileSuffixToUsage suffixToUsageMap[] = {
  //
  {"_d", ezTexConvUsage::Color},       //
  {"diff", ezTexConvUsage::Color},     //
  {"diffuse", ezTexConvUsage::Color},  //
  {"albedo", ezTexConvUsage::Color},   //
  {"col", ezTexConvUsage::Color},      //
  {"color", ezTexConvUsage::Color},    //
  {"emissive", ezTexConvUsage::Color}, //
  {"emit", ezTexConvUsage::Color},     //

  {"_n", ezTexConvUsage::NormalMap},      //
  {"nrm", ezTexConvUsage::NormalMap},     //
  {"norm", ezTexConvUsage::NormalMap},    //
  {"normal", ezTexConvUsage::NormalMap},  //
  {"normals", ezTexConvUsage::NormalMap}, //

  {"_rgh", ezTexConvUsage::Linear},      //
  {"_rough", ezTexConvUsage::Linear},    //
  {"roughness", ezTexConvUsage::Linear}, //

  {"_met", ezTexConvUsage::Linear},     //
  {"_metal", ezTexConvUsage::Linear},   //
  {"metallic", ezTexConvUsage::Linear}, //

  {"height", ezTexConvUsage::Linear},   //
  {"_disp", ezTexConvUsage::Linear},    //

  {"_ao", ezTexConvUsage::Linear},      //

  {"_alpha", ezTexConvUsage::Linear},   //
};


static ezTexConvUsage::Enum DetectUsageFromFilename(const char* szFile)
{
  ezStringBuilder name = ezPathUtils::GetFileName(szFile);
  name.ToLower();

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(suffixToUsageMap); ++i)
  {
    if (name.EndsWith_NoCase(suffixToUsageMap[i].m_szSuffix))
    {
      return suffixToUsageMap[i].m_Usage;
    }
  }

  return ezTexConvUsage::Auto;
}

static ezTexConvUsage::Enum DetectUsageFromImage(const ezImage& image)
{
  const ezImageHeader& header = image.GetHeader();
  const ezImageFormat::Enum format = header.GetImageFormat();

  if (header.GetDepth() > 1)
  {
    // unsupported
    return ezTexConvUsage::Auto;
  }

  if (ezImageFormat::IsSrgb(format))
  {
    // already sRGB so must be color
    return ezTexConvUsage::Color;
  }

  if (format == ezImageFormat::BC5_UNORM)
  {
    return ezTexConvUsage::NormalMap;
  }

  if (ezImageFormat::GetBitsPerChannel(format, ezImageFormatChannel::R) > 8 || format == ezImageFormat::BC6H_SF16 ||
      format == ezImageFormat::BC6H_UF16)
  {
    return ezTexConvUsage::Hdr;
  }

  if (ezImageFormat::GetNumChannels(format) <= 2)
  {
    return ezTexConvUsage::Linear;
  }

  const ezImage* pImgRGBA = &image;
  ezImage convertedRGBA;

  if (image.GetImageFormat() != ezImageFormat::R8G8B8A8_UNORM)
  {
    pImgRGBA = &convertedRGBA;
    if (ezImageConversion::Convert(image, convertedRGBA, ezImageFormat::R8G8B8A8_UNORM).Failed())
    {
      // cannot convert to RGBA -> maybe some weird lookup table format
      return ezTexConvUsage::Auto;
    }
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

    const ezUInt8* pPixel = pImgRGBA->GetPixelPointer<ezUInt8>();

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
      return ezTexConvUsage::Color;
    }

    if (uiExtremeNormals > uiNumPixels / 100)
    {
      // more than 1 percent of normals pointing backwards ? => probably not a normalmap
      return ezTexConvUsage::Color;
    }

    // it might just be a normal map, it does have the proper hue of blue
    return ezTexConvUsage::NormalMap;
  }
}

ezResult ezTexConvProcessor::AdjustUsage(const char* szFilename, const ezImage& srcImg, ezEnum<ezTexConvUsage>& inout_Usage)
{
  if (inout_Usage == ezTexConvUsage::Auto)
  {
    inout_Usage = DetectUsageFromFilename(szFilename);
  }

  if (inout_Usage == ezTexConvUsage::Auto)
  {
    inout_Usage = DetectUsageFromImage(srcImg);
  }

  if (inout_Usage == ezTexConvUsage::Auto)
  {
    ezLog::Error("Failed to deduce target format.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_TargetFormat);
