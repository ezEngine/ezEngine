#include <PCH.h>

#include <Texture/TexConv/TexConvProcessor.h>

#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

struct NameToUsage
{
  const char* name;
  const ezTexConvUsage::Enum usage;
};

// TODO (texconv): review this list
static NameToUsage nameToUsageMap[] = {{"nrm", ezTexConvUsage::NormalMap}, {"norm", ezTexConvUsage::NormalMap},
  {"_nb", ezTexConvUsage::NormalMap}, {"ibl", ezTexConvUsage::Hdr}, {"diff", ezTexConvUsage::Color}, {"albedo", ezTexConvUsage::Color},
  {"emissive", ezTexConvUsage::Color}, {"emit", ezTexConvUsage::Color}, {"rough", ezTexConvUsage::Linear},
  {"metallic", ezTexConvUsage::Linear}, {"_metal", ezTexConvUsage::Linear}, {"_ao", ezTexConvUsage::Linear},
  {"height", ezTexConvUsage::Linear}};


static ezTexConvUsage::Enum DetectUsageFromFilename(const char* szFile)
{
  ezStringBuilder name = ezPathUtils::GetFileName(szFile);
  name.ToLower();

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(nameToUsageMap); ++i)
  {
    if (name.FindSubString(nameToUsageMap[i].name) != nullptr)
    {
      return nameToUsageMap[i].usage;
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

  // TODO (texconv): review

  if (ezImageFormat::GetNumChannels(format) == 1)
  {
    if (ezImageFormat::GetBitsPerChannel(format, ezImageFormatChannel::R) > 8)
    {
      return ezTexConvUsage::Hdr;
    }

    return ezTexConvUsage::Linear;
  }

  if (ezImageFormat::GetBitsPerChannel(format, ezImageFormatChannel::R) >= 16 || format == ezImageFormat::BC6H_SF16 ||
      format == ezImageFormat::BC6H_UF16)
  {
    return ezTexConvUsage::Hdr;
  }

  const ezImage* pImgRGBA = &image;
  ezImage convertedRGBA;

  if (image.GetImageFormat() != ezImageFormat::R8G8B8A8_UNORM && image.GetImageFormat() != ezImageFormat::R8G8B8A8_UNORM_SRGB)
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
