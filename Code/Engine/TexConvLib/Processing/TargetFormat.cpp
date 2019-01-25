#include <PCH.h>

#include <TexConvLib/Processing/Processor.h>

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

ezResult ezTexConvProcessor::AdjustTargetFormat()
{
  if (m_Descriptor.m_TargetFormat == ezTexConvTargetFormat::Auto)
  {
    if (!m_Descriptor.m_InputFiles.IsEmpty())
    {
      m_Descriptor.m_TargetFormat = DetectTargetFormatFromFilename(m_Descriptor.m_InputFiles[0]);
    }
  }

  if (m_Descriptor.m_TargetFormat == ezTexConvTargetFormat::Auto)
  {
    m_Descriptor.m_TargetFormat = DetectTargetFormatFromImage(m_Descriptor.m_InputImages[0]);
  }

  if (m_Descriptor.m_TargetFormat == ezTexConvTargetFormat::Auto)
  {
    ezLog::Error("Failed to deduce target format.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::DetermineTargetResolution()
{
  EZ_ASSERT_DEV(m_uiTargetResolutionX == 0 && m_uiTargetResolutionY == 0, "Target resolution already determined");

  const ezUInt32 uiOrgResX = m_Descriptor.m_InputImages[0].GetWidth();
  const ezUInt32 uiOrgResY = m_Descriptor.m_InputImages[0].GetHeight();

  m_uiTargetResolutionX = uiOrgResX;
  m_uiTargetResolutionY = uiOrgResY;

  m_uiTargetResolutionX /= (1 << m_Descriptor.m_uiDownscaleSteps);
  m_uiTargetResolutionY /= (1 << m_Descriptor.m_uiDownscaleSteps);

  m_uiTargetResolutionX = ezMath::Clamp(m_uiTargetResolutionX, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution);
  m_uiTargetResolutionY = ezMath::Clamp(m_uiTargetResolutionY, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution);

  EZ_ASSERT_DEV(m_OutputImageFormat != ezImageFormat::UNKNOWN, "Output format must have been determined before this function");
  if (ezImageFormat::IsCompressed(m_OutputImageFormat))
  {
    if (m_uiTargetResolutionX % 4 != 0 || m_uiTargetResolutionY % 4 != 0)
    {
      ezLog::Error("Chosen output image format is compressed, but target resolution is not divisible by 4. {}x{} -> downscale {} / "
                   "clamp({}, {}) -> {}x{}",
                   uiOrgResX, uiOrgResY, m_Descriptor.m_uiDownscaleSteps, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution,
                   m_uiTargetResolutionX, m_uiTargetResolutionY);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}
