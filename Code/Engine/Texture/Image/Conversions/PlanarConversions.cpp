#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageConversion.h>

namespace
{
  // https://docs.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering#converting-8-bit-yuv-to-rgb888
  ezVec3I32 RGB2YUV(ezVec3I32 vRgb)
  {
    ezVec3I32 yuv;
    yuv.x = ((66 * vRgb.x + 129 * vRgb.y + 25 * vRgb.z + 128) >> 8) + 16;
    yuv.y = ((-38 * vRgb.x - 74 * vRgb.y + 112 * vRgb.z + 128) >> 8) + 128;
    yuv.z = ((112 * vRgb.x - 94 * vRgb.y - 18 * vRgb.z + 128) >> 8) + 128;
    return yuv;
  }

  ezVec3I32 YUV2RGB(ezVec3I32 vYuv)
  {
    ezVec3I32 rgb;

    ezInt32 C = vYuv.x - 16;
    ezInt32 D = vYuv.y - 128;
    ezInt32 E = vYuv.z - 128;

    rgb.x = ezMath::Clamp((298 * C + 409 * E + 128) >> 8, 0, 255);
    rgb.y = ezMath::Clamp((298 * C - 100 * D - 208 * E + 128) >> 8, 0, 255);
    rgb.z = ezMath::Clamp((298 * C + 516 * D + 128) >> 8, 0, 255);
    return rgb;
  }
} // namespace

struct ezImageConversion_NV12_sRGB : public ezImageConversionStepDeplanarize
{
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
      ezImageConversionEntry(ezImageFormat::NV12, ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<ezImageView> source, ezImage target, ezUInt32 uiNumPixelsX, ezUInt32 uiNumPixelsY, ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    EZ_IGNORE_UNUSED(sourceFormat);
    EZ_IGNORE_UNUSED(targetFormat);

    for (ezUInt32 y = 0; y < uiNumPixelsY; y += 2)
    {
      const ezUInt8* luma0 = source[0].GetPixelPointer<ezUInt8>(0, 0, 0, 0, y);
      const ezUInt8* luma1 = source[0].GetPixelPointer<ezUInt8>(0, 0, 0, 0, y + 1);
      const ezUInt8* chroma = source[1].GetPixelPointer<ezUInt8>(0, 0, 0, 0, y / 2);

      ezUInt8* rgba0 = target.GetPixelPointer<ezUInt8>(0, 0, 0, 0, y);
      ezUInt8* rgba1 = target.GetPixelPointer<ezUInt8>(0, 0, 0, 0, y + 1);

      for (ezUInt32 x = 0; x < uiNumPixelsX; x += 2)
      {
        ezVec3I32 p00 = YUV2RGB(ezVec3I32(luma0[0], chroma[0], chroma[1]));
        ezVec3I32 p01 = YUV2RGB(ezVec3I32(luma0[1], chroma[0], chroma[1]));
        ezVec3I32 p10 = YUV2RGB(ezVec3I32(luma1[0], chroma[0], chroma[1]));
        ezVec3I32 p11 = YUV2RGB(ezVec3I32(luma1[1], chroma[0], chroma[1]));

        rgba0[0] = static_cast<ezUInt8>(p00.x);
        rgba0[1] = static_cast<ezUInt8>(p00.y);
        rgba0[2] = static_cast<ezUInt8>(p00.z);
        rgba0[3] = static_cast<ezUInt8>(0xff);
        rgba0[4] = static_cast<ezUInt8>(p01.x);
        rgba0[5] = static_cast<ezUInt8>(p01.y);
        rgba0[6] = static_cast<ezUInt8>(p01.z);
        rgba0[7] = static_cast<ezUInt8>(0xff);

        rgba1[0] = static_cast<ezUInt8>(p10.x);
        rgba1[1] = static_cast<ezUInt8>(p10.y);
        rgba1[2] = static_cast<ezUInt8>(p10.z);
        rgba1[3] = static_cast<ezUInt8>(0xff);
        rgba1[4] = static_cast<ezUInt8>(p11.x);
        rgba1[5] = static_cast<ezUInt8>(p11.y);
        rgba1[6] = static_cast<ezUInt8>(p11.z);
        rgba1[7] = static_cast<ezUInt8>(0xff);

        luma0 += 2;
        luma1 += 2;
        chroma += 2;

        rgba0 += 8;
        rgba1 += 8;
      }
    }

    return EZ_SUCCESS;
  }
};

struct ezImageConversion_sRGB_NV12 : public ezImageConversionStepPlanarize
{
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
      ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::NV12, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(const ezImageView& source, ezArrayPtr<ezImage> target, ezUInt32 uiNumPixelsX, ezUInt32 uiNumPixelsY, ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    EZ_IGNORE_UNUSED(sourceFormat);
    EZ_IGNORE_UNUSED(targetFormat);

    for (ezUInt32 y = 0; y < uiNumPixelsY; y += 2)
    {
      const ezUInt8* rgba0 = source.GetPixelPointer<ezUInt8>(0, 0, 0, 0, y);
      const ezUInt8* rgba1 = source.GetPixelPointer<ezUInt8>(0, 0, 0, 0, y + 1);

      ezUInt8* luma0 = target[0].GetPixelPointer<ezUInt8>(0, 0, 0, 0, y);
      ezUInt8* luma1 = target[0].GetPixelPointer<ezUInt8>(0, 0, 0, 0, y + 1);
      ezUInt8* chroma = target[1].GetPixelPointer<ezUInt8>(0, 0, 0, 0, y / 2);

      for (ezUInt32 x = 0; x < uiNumPixelsX; x += 2)
      {
        ezVec3I32 p00 = RGB2YUV(ezVec3I32(rgba0[0], rgba0[1], rgba0[2]));
        ezVec3I32 p01 = RGB2YUV(ezVec3I32(rgba0[4], rgba0[5], rgba0[6]));
        ezVec3I32 p10 = RGB2YUV(ezVec3I32(rgba1[0], rgba1[1], rgba1[2]));
        ezVec3I32 p11 = RGB2YUV(ezVec3I32(rgba1[4], rgba1[5], rgba1[6]));

        luma0[0] = static_cast<ezUInt8>(p00.x);
        luma0[1] = static_cast<ezUInt8>(p01.x);
        luma1[0] = static_cast<ezUInt8>(p10.x);
        luma1[1] = static_cast<ezUInt8>(p11.x);

        ezVec3I32 c = (p00 + p01 + p10 + p11);

        chroma[0] = static_cast<ezUInt8>(c.y >> 2);
        chroma[1] = static_cast<ezUInt8>(c.z >> 2);

        luma0 += 2;
        luma1 += 2;
        chroma += 2;

        rgba0 += 8;
        rgba1 += 8;
      }
    }

    return EZ_SUCCESS;
  }
};

// EZ_STATICLINK_FORCE
static ezImageConversion_NV12_sRGB s_conversion_NV12_sRGB;
static ezImageConversion_sRGB_NV12 s_conversion_sRGB_NV12;



EZ_STATICLINK_FILE(Texture, Texture_Image_Conversions_PlanarConversions);
