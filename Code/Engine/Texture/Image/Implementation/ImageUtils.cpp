#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageUtils.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Time/Timestamp.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/Image/ImageFilter.h>

template <typename TYPE>
static void SetDiff(const ezImageView& imageA, const ezImageView& imageB, ezImage& out_difference, ezUInt32 w, ezUInt32 h, ezUInt32 d, ezUInt32 uiComp)
{
  const TYPE* pA = imageA.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  const TYPE* pB = imageB.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  TYPE* pR = out_difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  for (ezUInt32 i = 0; i < uiComp; ++i)
    pR[i] = pB[i] > pA[i] ? (pB[i] - pA[i]) : (pA[i] - pB[i]);
}

template <typename TYPE, typename ACCU, int COMP>
static void SetCompMinDiff(const ezImageView& newDifference, ezImage& out_minDifference, ezUInt32 w, ezUInt32 h, ezUInt32 d, ezUInt32 uiComp)
{
  const TYPE* pNew = newDifference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  TYPE* pR = out_minDifference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  for (ezUInt32 i = 0; i < uiComp; i += COMP)
  {
    ACCU minDiff = 0;
    ACCU newDiff = 0;
    for (ezUInt32 c = 0; c < COMP; c++)
    {
      minDiff += pR[i + c];
      newDiff += pNew[i + c];
    }
    if (minDiff > newDiff)
    {
      for (ezUInt32 c = 0; c < COMP; c++)
        pR[i + c] = pNew[i + c];
    }
  }
}

template <typename TYPE>
static ezUInt32 GetError(const ezImageView& difference, ezUInt32 w, ezUInt32 h, ezUInt32 d, ezUInt32 uiComp, ezUInt32 uiPixel)
{
  const TYPE* pR = difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  ezUInt32 uiErrorSum = 0;

  for (ezUInt32 p = 0; p < uiPixel; ++p)
  {
    ezUInt32 error = 0;

    for (ezUInt32 c = 0; c < uiComp; ++c)
    {
      error += *pR;
      ++pR;
    }

    error /= uiComp;
    uiErrorSum += error * error;
  }

  return uiErrorSum;
}

void ezImageUtils::ComputeImageDifferenceABS(const ezImageView& imageA, const ezImageView& imageB, ezImage& out_difference)
{
  EZ_PROFILE_SCOPE("ezImageUtils::ComputeImageDifferenceABS");

  EZ_ASSERT_DEV(imageA.GetWidth() == imageB.GetWidth(), "Dimensions do not match");
  EZ_ASSERT_DEV(imageA.GetHeight() == imageB.GetHeight(), "Dimensions do not match");
  EZ_ASSERT_DEV(imageA.GetDepth() == imageB.GetDepth(), "Dimensions do not match");
  EZ_ASSERT_DEV(imageA.GetImageFormat() == imageB.GetImageFormat(), "Format does not match");

  ezImageHeader differenceHeader;

  differenceHeader.SetWidth(imageA.GetWidth());
  differenceHeader.SetHeight(imageA.GetHeight());
  differenceHeader.SetDepth(imageA.GetDepth());
  differenceHeader.SetImageFormat(imageA.GetImageFormat());
  out_difference.ResetAndAlloc(differenceHeader);

  const ezUInt32 uiSize2D = imageA.GetHeight() * imageA.GetWidth();

  for (ezUInt32 d = 0; d < imageA.GetDepth(); ++d)
  {
    // for (ezUInt32 h = 0; h < ImageA.GetHeight(); ++h)
    {
      // for (ezUInt32 w = 0; w < ImageA.GetWidth(); ++w)
      {
        switch (imageA.GetImageFormat())
        {
          case ezImageFormat::R8G8B8A8_UNORM:
          case ezImageFormat::R8G8B8A8_UNORM_SRGB:
          case ezImageFormat::R8G8B8A8_UINT:
          case ezImageFormat::R8G8B8A8_SNORM:
          case ezImageFormat::R8G8B8A8_SINT:
          case ezImageFormat::B8G8R8A8_UNORM:
          case ezImageFormat::B8G8R8X8_UNORM:
          case ezImageFormat::B8G8R8A8_UNORM_SRGB:
          case ezImageFormat::B8G8R8X8_UNORM_SRGB:
          {
            SetDiff<ezUInt8>(imageA, imageB, out_difference, 0, 0, d, 4 * uiSize2D);
          }
          break;

          case ezImageFormat::B8G8R8_UNORM:
          {
            SetDiff<ezUInt8>(imageA, imageB, out_difference, 0, 0, d, 3 * uiSize2D);
          }
          break;

          default:
            EZ_REPORT_FAILURE("The ezImageFormat {0} is not implemented", (ezUInt32)imageA.GetImageFormat());
            return;
        }
      }
    }
  }
}


void ezImageUtils::ComputeImageDifferenceABSRelaxed(const ezImageView& imageA, const ezImageView& imageB, ezImage& out_difference)
{
  EZ_ASSERT_ALWAYS(imageA.GetDepth() == 1 && imageA.GetNumMipLevels() == 1, "Depth slices and mipmaps are not supported");

  EZ_PROFILE_SCOPE("ezImageUtils::ComputeImageDifferenceABSRelaxed");

  ComputeImageDifferenceABS(imageA, imageB, out_difference);

  ezImage tempB;
  tempB.ResetAndCopy(imageB);
  ezImage tempDiff;
  tempDiff.ResetAndCopy(out_difference);

  for (ezInt32 yOffset = -1; yOffset <= 1; ++yOffset)
  {
    for (ezInt32 xOffset = -1; xOffset <= 1; ++xOffset)
    {
      if (yOffset == 0 && xOffset == 0)
        continue;

      ezImageUtils::Copy(imageB, ezRectU32(ezMath::Max(xOffset, 0), ezMath::Max(yOffset, 0), imageB.GetWidth() - ezMath::Abs(xOffset), imageB.GetHeight() - ezMath::Abs(yOffset)), tempB, ezVec3U32(-ezMath::Min(xOffset, 0), -ezMath::Min(yOffset, 0), 0)).AssertSuccess("");

      ComputeImageDifferenceABS(imageA, tempB, tempDiff);

      const ezUInt32 uiSize2D = imageA.GetHeight() * imageA.GetWidth();
      switch (imageA.GetImageFormat())
      {
        case ezImageFormat::R8G8B8A8_UNORM:
        case ezImageFormat::R8G8B8A8_UNORM_SRGB:
        case ezImageFormat::R8G8B8A8_UINT:
        case ezImageFormat::R8G8B8A8_SNORM:
        case ezImageFormat::R8G8B8A8_SINT:
        case ezImageFormat::B8G8R8A8_UNORM:
        case ezImageFormat::B8G8R8X8_UNORM:
        case ezImageFormat::B8G8R8A8_UNORM_SRGB:
        case ezImageFormat::B8G8R8X8_UNORM_SRGB:
        {
          SetCompMinDiff<ezUInt8, ezUInt32, 4>(tempDiff, out_difference, 0, 0, 0, 4 * uiSize2D);
        }
        break;

        case ezImageFormat::B8G8R8_UNORM:
        {
          SetCompMinDiff<ezUInt8, ezUInt32, 3>(tempDiff, out_difference, 0, 0, 0, 3 * uiSize2D);
        }
        break;

        default:
          EZ_REPORT_FAILURE("The ezImageFormat {0} is not implemented", (ezUInt32)imageA.GetImageFormat());
          return;
      }
    }
  }
}

ezUInt32 ezImageUtils::ComputeMeanSquareError(const ezImageView& differenceImage, ezUInt8 uiBlockSize, ezUInt32 uiOffsetx, ezUInt32 uiOffsety)
{
  EZ_PROFILE_SCOPE("ezImageUtils::ComputeMeanSquareError(detail)");

  EZ_ASSERT_DEV(uiBlockSize > 1, "Blocksize must be at least 2");

  ezUInt32 uiNumComponents = ezImageFormat::GetNumChannels(differenceImage.GetImageFormat());

  ezUInt32 uiWidth = ezMath::Min(differenceImage.GetWidth(), uiOffsetx + uiBlockSize) - uiOffsetx;
  ezUInt32 uiHeight = ezMath::Min(differenceImage.GetHeight(), uiOffsety + uiBlockSize) - uiOffsety;

  // Treat image as single-component format and scale the width instead
  uiWidth *= uiNumComponents;

  if (uiWidth == 0 || uiHeight == 0)
    return 0;

  switch (differenceImage.GetImageFormat())
  {
      // Supported formats
    case ezImageFormat::R8G8B8A8_UNORM:
    case ezImageFormat::R8G8B8A8_UNORM_SRGB:
    case ezImageFormat::R8G8B8A8_UINT:
    case ezImageFormat::R8G8B8A8_SNORM:
    case ezImageFormat::R8G8B8A8_SINT:
    case ezImageFormat::B8G8R8A8_UNORM:
    case ezImageFormat::B8G8R8A8_UNORM_SRGB:
    case ezImageFormat::B8G8R8_UNORM:
      break;

    default:
      EZ_REPORT_FAILURE("The ezImageFormat {0} is not implemented", (ezUInt32)differenceImage.GetImageFormat());
      return 0;
  }


  ezUInt32 error = 0;

  ezUInt64 uiRowPitch = differenceImage.GetRowPitch();
  ezUInt64 uiDepthPitch = differenceImage.GetDepthPitch();

  const ezUInt32 uiSize2D = uiWidth * uiHeight;
  const ezUInt8* pSlicePointer = differenceImage.GetPixelPointer<ezUInt8>(0, 0, 0, uiOffsetx, uiOffsety);

  for (ezUInt32 d = 0; d < differenceImage.GetDepth(); ++d)
  {
    const ezUInt8* pRowPointer = pSlicePointer;

    for (ezUInt32 y = 0; y < uiHeight; ++y)
    {
      const ezUInt8* pPixelPointer = pRowPointer;
      for (ezUInt32 x = 0; x < uiWidth; ++x)
      {
        ezUInt32 uiDiff = *pPixelPointer;
        error += uiDiff * uiDiff;

        pPixelPointer++;
      }

      pRowPointer += uiRowPitch;
    }

    pSlicePointer += uiDepthPitch;
  }

  error /= uiSize2D;
  return error;
}

ezUInt32 ezImageUtils::ComputeMeanSquareError(const ezImageView& differenceImage, ezUInt8 uiBlockSize)
{
  EZ_PROFILE_SCOPE("ezImageUtils::ComputeMeanSquareError");

  EZ_ASSERT_DEV(uiBlockSize > 1, "Blocksize must be at least 2");

  const ezUInt32 uiHalfBlockSize = uiBlockSize / 2;

  const ezUInt32 uiBlocksX = (differenceImage.GetWidth() / uiHalfBlockSize) + 1;
  const ezUInt32 uiBlocksY = (differenceImage.GetHeight() / uiHalfBlockSize) + 1;

  ezUInt32 uiMaxError = 0;

  for (ezUInt32 by = 0; by < uiBlocksY; ++by)
  {
    for (ezUInt32 bx = 0; bx < uiBlocksX; ++bx)
    {
      const ezUInt32 uiBlockError = ComputeMeanSquareError(differenceImage, uiBlockSize, bx * uiHalfBlockSize, by * uiHalfBlockSize);

      uiMaxError = ezMath::Max(uiMaxError, uiBlockError);
    }
  }

  return uiMaxError;
}

template <typename Func, typename ImageType>
static void ApplyFunc(ImageType& inout_image, Func func)
{
  ezUInt32 uiWidth = inout_image.GetWidth();
  ezUInt32 uiHeight = inout_image.GetHeight();
  ezUInt32 uiDepth = inout_image.GetDepth();

  EZ_IGNORE_UNUSED(uiDepth);
  EZ_ASSERT_DEV(uiWidth > 0 && uiHeight > 0 && uiDepth > 0, "The image passed to FindMinMax has illegal dimension {}x{}x{}.", uiWidth, uiHeight, uiDepth);

  ezUInt64 uiRowPitch = inout_image.GetRowPitch();
  ezUInt64 uiDepthPitch = inout_image.GetDepthPitch();
  ezUInt32 uiNumChannels = ezImageFormat::GetNumChannels(inout_image.GetImageFormat());

  auto pSlicePointer = inout_image.template GetPixelPointer<ezUInt8>();

  for (ezUInt32 z = 0; z < inout_image.GetDepth(); ++z)
  {
    auto pRowPointer = pSlicePointer;

    for (ezUInt32 y = 0; y < uiHeight; ++y)
    {
      auto pPixelPointer = pRowPointer;
      for (ezUInt32 x = 0; x < uiWidth; ++x)
      {
        for (ezUInt32 c = 0; c < uiNumChannels; ++c)
        {
          func(pPixelPointer++, x, y, z, c);
        }
      }

      pRowPointer += uiRowPitch;
    }

    pSlicePointer += uiDepthPitch;
  }
}

static void FindMinMax(const ezImageView& image, ezUInt8& out_uiMinRgb, ezUInt8& out_uiMaxRgb, ezUInt8& out_uiMinAlpha, ezUInt8& out_uiMaxAlpha)
{
  ezImageFormat::Enum imageFormat = image.GetImageFormat();
  EZ_IGNORE_UNUSED(imageFormat);
  EZ_ASSERT_DEV(ezImageFormat::GetBitsPerChannel(imageFormat, ezImageFormatChannel::R) == 8 && ezImageFormat::GetDataType(imageFormat) == ezImageFormatDataType::UNORM, "Only 8bpp unorm formats are supported in FindMinMax");

  out_uiMinRgb = 255u;
  out_uiMinAlpha = 255u;
  out_uiMaxRgb = 0u;
  out_uiMaxAlpha = 0u;

  auto minMax = [&](const ezUInt8* pPixel, ezUInt32 /*x*/, ezUInt32 /*y*/, ezUInt32 /*z*/, ezUInt32 c)
  {
    ezUInt8 val = *pPixel;

    if (c < 3)
    {
      out_uiMinRgb = ezMath::Min(out_uiMinRgb, val);
      out_uiMaxRgb = ezMath::Max(out_uiMaxRgb, val);
    }
    else
    {
      out_uiMinAlpha = ezMath::Min(out_uiMinAlpha, val);
      out_uiMaxAlpha = ezMath::Max(out_uiMaxAlpha, val);
    }
  };
  ApplyFunc(image, minMax);
}

void ezImageUtils::Normalize(ezImage& inout_image)
{
  ezUInt8 uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha;
  Normalize(inout_image, uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha);
}

void ezImageUtils::Normalize(ezImage& inout_image, ezUInt8& out_uiMinRgb, ezUInt8& out_uiMaxRgb, ezUInt8& out_uiMinAlpha, ezUInt8& out_uiMaxAlpha)
{
  EZ_PROFILE_SCOPE("ezImageUtils::Normalize");

  ezImageFormat::Enum imageFormat = inout_image.GetImageFormat();

  EZ_ASSERT_DEV(ezImageFormat::GetBitsPerChannel(imageFormat, ezImageFormatChannel::R) == 8 && ezImageFormat::GetDataType(imageFormat) == ezImageFormatDataType::UNORM, "Only 8bpp unorm formats are supported in NormalizeImage");

  bool ignoreAlpha = false;
  if (imageFormat == ezImageFormat::B8G8R8X8_UNORM || imageFormat == ezImageFormat::B8G8R8X8_UNORM_SRGB)
  {
    ignoreAlpha = true;
  }

  FindMinMax(inout_image, out_uiMinRgb, out_uiMaxRgb, out_uiMinAlpha, out_uiMaxAlpha);
  ezUInt8 uiRangeRgb = out_uiMaxRgb - out_uiMinRgb;
  ezUInt8 uiRangeAlpha = out_uiMaxAlpha - out_uiMinAlpha;

  auto normalize = [&](ezUInt8* pPixel, ezUInt32 /*x*/, ezUInt32 /*y*/, ezUInt32 /*z*/, ezUInt32 c)
  {
    ezUInt8 val = *pPixel;
    if (c < 3)
    {
      // color channels are uniform when min == max, in that case keep original value as scaling is not meaningful
      if (uiRangeRgb != 0)
      {
        *pPixel = static_cast<ezUInt8>(255u * (static_cast<float>(val - out_uiMinRgb) / (uiRangeRgb)));
      }
    }
    else
    {
      // alpha is uniform when minAlpha == maxAlpha, in that case keep original alpha as scaling is not meaningful
      if (!ignoreAlpha && uiRangeAlpha != 0)
      {
        *pPixel = static_cast<ezUInt8>(255u * (static_cast<float>(val - out_uiMinAlpha) / (uiRangeAlpha)));
      }
    }
  };
  ApplyFunc(inout_image, normalize);
}

void ezImageUtils::ExtractAlphaChannel(const ezImageView& inputImage, ezImage& inout_outputImage)
{
  EZ_PROFILE_SCOPE("ezImageUtils::ExtractAlphaChannel");

  switch (ezImageFormat::Enum imageFormat = inputImage.GetImageFormat())
  {
    case ezImageFormat::R8G8B8A8_UNORM:
    case ezImageFormat::R8G8B8A8_UNORM_SRGB:
    case ezImageFormat::R8G8B8A8_UINT:
    case ezImageFormat::R8G8B8A8_SNORM:
    case ezImageFormat::R8G8B8A8_SINT:
    case ezImageFormat::B8G8R8A8_UNORM:
    case ezImageFormat::B8G8R8A8_UNORM_SRGB:
      break;
    default:
      EZ_REPORT_FAILURE("ExtractAlpha needs an image with 8bpp and 4 channel. The ezImageFormat {} is not supported.", (ezUInt32)imageFormat);
      return;
  }

  ezImageHeader outputHeader = inputImage.GetHeader();
  outputHeader.SetImageFormat(ezImageFormat::R8_UNORM);
  inout_outputImage.ResetAndAlloc(outputHeader);

  const ezUInt8* pInputSlice = inputImage.GetPixelPointer<ezUInt8>();
  ezUInt8* pOutputSlice = inout_outputImage.GetPixelPointer<ezUInt8>();

  ezUInt64 uiInputRowPitch = inputImage.GetRowPitch();
  ezUInt64 uiInputDepthPitch = inputImage.GetDepthPitch();

  ezUInt64 uiOutputRowPitch = inout_outputImage.GetRowPitch();
  ezUInt64 uiOutputDepthPitch = inout_outputImage.GetDepthPitch();

  for (ezUInt32 d = 0; d < inputImage.GetDepth(); ++d)
  {
    const ezUInt8* pInputRow = pInputSlice;
    ezUInt8* pOutputRow = pOutputSlice;

    for (ezUInt32 y = 0; y < inputImage.GetHeight(); ++y)
    {
      const ezUInt8* pInputPixel = pInputRow;
      ezUInt8* pOutputPixel = pOutputRow;
      for (ezUInt32 x = 0; x < inputImage.GetWidth(); ++x)
      {
        *pOutputPixel = pInputPixel[3];

        pInputPixel += 4;
        ++pOutputPixel;
      }

      pInputRow += uiInputRowPitch;
      pOutputRow += uiOutputRowPitch;
    }

    pInputSlice += uiInputDepthPitch;
    pOutputSlice += uiOutputDepthPitch;
  }
}

void ezImageUtils::CropImage(const ezImageView& input, const ezVec2I32& vOffset, const ezSizeU32& newsize, ezImage& out_output)
{
  EZ_PROFILE_SCOPE("ezImageUtils::CropImage");

  EZ_ASSERT_DEV(vOffset.x >= 0, "Offset is invalid");
  EZ_ASSERT_DEV(vOffset.y >= 0, "Offset is invalid");
  EZ_ASSERT_DEV(vOffset.x < (ezInt32)input.GetWidth(), "Offset is invalid");
  EZ_ASSERT_DEV(vOffset.y < (ezInt32)input.GetHeight(), "Offset is invalid");

  const ezUInt32 uiNewWidth = ezMath::Min(vOffset.x + newsize.width, input.GetWidth()) - vOffset.x;
  const ezUInt32 uiNewHeight = ezMath::Min(vOffset.y + newsize.height, input.GetHeight()) - vOffset.y;

  ezImageHeader outputHeader;
  outputHeader.SetWidth(uiNewWidth);
  outputHeader.SetHeight(uiNewHeight);
  outputHeader.SetImageFormat(input.GetImageFormat());
  out_output.ResetAndAlloc(outputHeader);

  for (ezUInt32 y = 0; y < uiNewHeight; ++y)
  {
    for (ezUInt32 x = 0; x < uiNewWidth; ++x)
    {
      switch (input.GetImageFormat())
      {
        case ezImageFormat::R8G8B8A8_UNORM:
        case ezImageFormat::R8G8B8A8_UNORM_SRGB:
        case ezImageFormat::R8G8B8A8_UINT:
        case ezImageFormat::R8G8B8A8_SNORM:
        case ezImageFormat::R8G8B8A8_SINT:
        case ezImageFormat::B8G8R8A8_UNORM:
        case ezImageFormat::B8G8R8X8_UNORM:
        case ezImageFormat::B8G8R8A8_UNORM_SRGB:
        case ezImageFormat::B8G8R8X8_UNORM_SRGB:
          out_output.GetPixelPointer<ezUInt32>(0, 0, 0, x, y)[0] = input.GetPixelPointer<ezUInt32>(0, 0, 0, vOffset.x + x, vOffset.y + y)[0];
          break;

        case ezImageFormat::B8G8R8_UNORM:
          out_output.GetPixelPointer<ezUInt8>(0, 0, 0, x, y)[0] = input.GetPixelPointer<ezUInt8>(0, 0, 0, vOffset.x + x, vOffset.y + y)[0];
          out_output.GetPixelPointer<ezUInt8>(0, 0, 0, x, y)[1] = input.GetPixelPointer<ezUInt8>(0, 0, 0, vOffset.x + x, vOffset.y + y)[1];
          out_output.GetPixelPointer<ezUInt8>(0, 0, 0, x, y)[2] = input.GetPixelPointer<ezUInt8>(0, 0, 0, vOffset.x + x, vOffset.y + y)[2];
          break;

        default:
          EZ_REPORT_FAILURE("The ezImageFormat {0} is not implemented", (ezUInt32)input.GetImageFormat());
          return;
      }
    }
  }
}

namespace
{
  template <typename T>
  void rotate180(T* pStart, T* pEnd)
  {
    pEnd = pEnd - 1;
    while (pStart < pEnd)
    {
      ezMath::Swap(*pStart, *pEnd);
      pStart++;
      pEnd--;
    }
  }
} // namespace

void ezImageUtils::RotateSubImage180(ezImage& inout_image, ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/)
{
  EZ_PROFILE_SCOPE("ezImageUtils::RotateSubImage180");

  ezUInt8* start = inout_image.GetPixelPointer<ezUInt8>(uiMipLevel, uiFace, uiArrayIndex);
  ezUInt8* end = start + inout_image.GetDepthPitch(uiMipLevel);

  ezUInt32 bytesPerPixel = ezImageFormat::GetBitsPerPixel(inout_image.GetImageFormat()) / 8;

  switch (bytesPerPixel)
  {
    case 4:
      rotate180<ezUInt32>(reinterpret_cast<ezUInt32*>(start), reinterpret_cast<ezUInt32*>(end));
      break;
    case 12:
      rotate180<ezVec3>(reinterpret_cast<ezVec3*>(start), reinterpret_cast<ezVec3*>(end));
      break;
    case 16:
      rotate180<ezVec4>(reinterpret_cast<ezVec4*>(start), reinterpret_cast<ezVec4*>(end));
      break;
    default:
      // fallback version
      {
        end -= bytesPerPixel;
        while (start < end)
        {
          for (ezUInt32 i = 0; i < bytesPerPixel; i++)
          {
            ezMath::Swap(start[i], end[i]);
          }
          start += bytesPerPixel;
          end -= bytesPerPixel;
        }
      }
  }
}

ezResult ezImageUtils::Copy(const ezImageView& srcImg, const ezRectU32& srcRect, ezImage& inout_dstImg, const ezVec3U32& vDstOffset, ezUInt32 uiDstMipLevel /*= 0*/, ezUInt32 uiDstFace /*= 0*/, ezUInt32 uiDstArrayIndex /*= 0*/)
{
  if (inout_dstImg.GetImageFormat() != srcImg.GetImageFormat())   // Can only copy when the image formats are identical
    return EZ_FAILURE;

  if (ezImageFormat::IsCompressed(inout_dstImg.GetImageFormat())) // Compressed formats are not supported
    return EZ_FAILURE;

  EZ_PROFILE_SCOPE("ezImageUtils::Copy");

  const ezUInt64 uiDstRowPitch = inout_dstImg.GetRowPitch(uiDstMipLevel);
  const ezUInt64 uiSrcRowPitch = srcImg.GetRowPitch(uiDstMipLevel);
  const ezUInt32 uiCopyBytesPerRow = ezImageFormat::GetBitsPerPixel(srcImg.GetImageFormat()) * srcRect.width / 8;

  ezUInt8* dstPtr = inout_dstImg.GetPixelPointer<ezUInt8>(uiDstMipLevel, uiDstFace, uiDstArrayIndex, vDstOffset.x, vDstOffset.y, vDstOffset.z);
  const ezUInt8* srcPtr = srcImg.GetPixelPointer<ezUInt8>(0, 0, 0, srcRect.x, srcRect.y);

  for (ezUInt32 y = 0; y < srcRect.height; y++)
  {
    ezMemoryUtils::Copy(dstPtr, srcPtr, uiCopyBytesPerRow);

    dstPtr += uiDstRowPitch;
    srcPtr += uiSrcRowPitch;
  }

  return EZ_SUCCESS;
}

ezResult ezImageUtils::ExtractLowerMipChain(const ezImageView& srcImg, ezImage& ref_dstImg, ezUInt32 uiNumMips)
{
  const ezImageHeader& srcImgHeader = srcImg.GetHeader();

  if (srcImgHeader.GetNumFaces() != 1 || srcImgHeader.GetNumArrayIndices() != 1)
  {
    // Lower mips aren't stored contiguously for array/cube textures and would require copying. This isn't implemented yet.
    return EZ_FAILURE;
  }

  EZ_PROFILE_SCOPE("ezImageUtils::ExtractLowerMipChain");

  uiNumMips = ezMath::Min(uiNumMips, srcImgHeader.GetNumMipLevels());

  ezUInt32 startMipLevel = srcImgHeader.GetNumMipLevels() - uiNumMips;

  ezImageFormat::Enum format = srcImgHeader.GetImageFormat();

  if (ezImageFormat::RequiresFirstLevelBlockAlignment(format))
  {
    // Some block compressed image formats require resolutions that are divisible by block size,
    // therefore adjust startMipLevel accordingly
    while (srcImgHeader.GetWidth(startMipLevel) % ezImageFormat::GetBlockWidth(format) != 0 || srcImgHeader.GetHeight(startMipLevel) % ezImageFormat::GetBlockHeight(format) != 0)
    {
      if (uiNumMips >= srcImgHeader.GetNumMipLevels())
        return EZ_FAILURE;

      if (startMipLevel == 0)
        return EZ_FAILURE;

      ++uiNumMips;
      --startMipLevel;
    }
  }

  ezImageHeader dstImgHeader = srcImgHeader;
  dstImgHeader.SetWidth(srcImgHeader.GetWidth(startMipLevel));
  dstImgHeader.SetHeight(srcImgHeader.GetHeight(startMipLevel));
  dstImgHeader.SetDepth(srcImgHeader.GetDepth(startMipLevel));
  dstImgHeader.SetNumFaces(srcImgHeader.GetNumFaces());
  dstImgHeader.SetNumArrayIndices(srcImgHeader.GetNumArrayIndices());
  dstImgHeader.SetNumMipLevels(uiNumMips);

  const ezUInt8* pDataBegin = srcImg.GetPixelPointer<ezUInt8>(startMipLevel);
  const ezUInt8* pDataEnd = srcImg.GetByteBlobPtr().GetEndPtr();
  const ptrdiff_t dataSize = reinterpret_cast<ptrdiff_t>(pDataEnd) - reinterpret_cast<ptrdiff_t>(pDataBegin);

  const ezConstByteBlobPtr lowResData(pDataBegin, static_cast<ezUInt64>(dataSize));

  ezImageView dataview;
  dataview.ResetAndViewExternalStorage(dstImgHeader, lowResData);

  ref_dstImg.ResetAndCopy(dataview);

  return EZ_SUCCESS;
}

ezUInt32 ezImageUtils::GetSampleIndex(ezUInt32 uiNumTexels, ezInt32 iIndex, ezImageAddressMode::Enum addressMode, bool& out_bUseBorderColor)
{
  out_bUseBorderColor = false;
  if (ezUInt32(iIndex) >= uiNumTexels)
  {
    switch (addressMode)
    {
      case ezImageAddressMode::Repeat:
        iIndex %= uiNumTexels;

        if (iIndex < 0)
        {
          iIndex += uiNumTexels;
        }
        return iIndex;

      case ezImageAddressMode::Mirror:
      {
        if (iIndex < 0)
        {
          iIndex = -iIndex - 1;
        }
        bool flip = (iIndex / uiNumTexels) & 1;
        iIndex %= uiNumTexels;
        if (flip)
        {
          iIndex = uiNumTexels - iIndex - 1;
        }
        return iIndex;
      }

      case ezImageAddressMode::Clamp:
        return ezMath::Clamp<ezInt32>(iIndex, 0, uiNumTexels - 1);

      case ezImageAddressMode::ClampBorder:
        out_bUseBorderColor = true;
        return 0;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED
        return 0;
    }
  }
  return iIndex;
}

static ezSimdVec4f LoadSample(const ezSimdVec4f* pSource, ezUInt32 uiNumSourceElements, ezUInt32 uiStride, ezInt32 iIndex, ezImageAddressMode::Enum addressMode, const ezSimdVec4f& vBorderColor)
{
  bool useBorderColor = false;
  // result is in the range [-(w-1), (w-1)], bring it to [0, w - 1]
  iIndex = ezImageUtils::GetSampleIndex(uiNumSourceElements, iIndex, addressMode, useBorderColor);
  if (useBorderColor)
  {
    return vBorderColor;
  }
  return pSource[iIndex * uiStride];
}

inline static void FilterLine(
  ezUInt32 uiNumSourceElements, const ezSimdVec4f* __restrict pSourceBegin, ezSimdVec4f* __restrict pTargetBegin, ezUInt32 uiStride, const ezImageFilterWeights& weights, ezArrayPtr<const ezInt32> firstSampleIndices, ezImageAddressMode::Enum addressMode, const ezSimdVec4f& vBorderColor)
{
  // Convolve the image using the precomputed weights
  const ezUInt32 numWeights = weights.GetNumWeights();

  // When the first source index for the output is between 0 and this value,
  // we can fetch all numWeights inputs without taking addressMode into consideration,
  // which makes the inner loop a lot faster.
  const ezInt32 trivialSourceIndicesEnd = static_cast<ezInt32>(uiNumSourceElements) - static_cast<ezInt32>(numWeights);
  const auto weightsView = weights.ViewWeights();
  const float* __restrict nextWeightPtr = weightsView.GetPtr();
  EZ_ASSERT_DEBUG((static_cast<ezUInt32>(weightsView.GetCount()) % numWeights) == 0, "");
  for (ezInt32 firstSourceIdx : firstSampleIndices)
  {
    ezSimdVec4f total(0.0f, 0.0f, 0.0f, 0.0f);

    if (firstSourceIdx >= 0 && firstSourceIdx < trivialSourceIndicesEnd)
    {
      const auto* __restrict sourcePtr = pSourceBegin + firstSourceIdx * uiStride;
      for (ezUInt32 weightIdx = 0; weightIdx < numWeights; ++weightIdx)
      {
        total = ezSimdVec4f::MulAdd(*sourcePtr, ezSimdVec4f(*nextWeightPtr++), total);
        sourcePtr += uiStride;
      }
    }
    else
    {
      // Very slow fallback case that respects the addressMode
      // (not a lot of pixels are taking this path, so it's probably fine)
      ezInt32 sourceIdx = firstSourceIdx;
      for (ezUInt32 weightIdx = 0; weightIdx < numWeights; ++weightIdx)
      {
        total = ezSimdVec4f::MulAdd(LoadSample(pSourceBegin, uiNumSourceElements, uiStride, sourceIdx, addressMode, vBorderColor), ezSimdVec4f(*nextWeightPtr++), total);
        sourceIdx++;
      }
    }
    // It's ok to check this once per source index, see the assert above
    // (number of weights in weightsView is divisible by numWeights)
    if (nextWeightPtr == weightsView.GetEndPtr())
    {
      nextWeightPtr = weightsView.GetPtr();
    }
    *pTargetBegin = total;
    pTargetBegin += uiStride;
  }
}

static void DownScaleFastLine(ezUInt32 uiPixelStride, const ezUInt8* pSrc, ezUInt8* pDest, ezUInt32 uiLengthIn, ezUInt32 uiStrideIn, ezUInt32 uiLengthOut, ezUInt32 uiStrideOut)
{
  const ezUInt32 downScaleFactor = uiLengthIn / uiLengthOut;
  EZ_ASSERT_DEBUG(downScaleFactor >= 1, "Can't upscale");

  const ezUInt32 downScaleFactorLog2 = ezMath::Log2i(static_cast<ezUInt32>(downScaleFactor));
  const ezUInt32 roundOffset = downScaleFactor / 2;

  for (ezUInt32 offset = 0; offset < uiLengthOut; ++offset)
  {
    for (ezUInt32 channel = 0; channel < uiPixelStride; ++channel)
    {
      const ezUInt32 destOffset = offset * uiStrideOut + channel;

      ezUInt32 curChannel = roundOffset;
      for (ezUInt32 index = 0; index < downScaleFactor; ++index)
      {
        curChannel += static_cast<ezUInt32>(pSrc[channel + index * uiStrideIn]);
      }

      curChannel = curChannel >> downScaleFactorLog2;
      pDest[destOffset] = static_cast<ezUInt8>(curChannel);
    }

    pSrc += downScaleFactor * uiStrideIn;
  }
}

static void DownScaleFast(const ezImageView& image, ezImage& out_result, ezUInt32 uiWidth, ezUInt32 uiHeight)
{
  ezImageFormat::Enum format = image.GetImageFormat();

  ezUInt32 originalWidth = image.GetWidth();
  ezUInt32 originalHeight = image.GetHeight();
  ezUInt32 numArrayElements = image.GetNumArrayIndices();
  ezUInt32 numFaces = image.GetNumFaces();

  ezUInt32 pixelStride = ezImageFormat::GetBitsPerPixel(format) / 8;

  ezImageHeader intermediateHeader;
  intermediateHeader.SetWidth(uiWidth);
  intermediateHeader.SetHeight(originalHeight);
  intermediateHeader.SetNumArrayIndices(numArrayElements);
  intermediateHeader.SetNumFaces(numFaces);
  intermediateHeader.SetImageFormat(format);

  ezImage intermediate;
  intermediate.ResetAndAlloc(intermediateHeader);

  for (ezUInt32 arrayIndex = 0; arrayIndex < numArrayElements; arrayIndex++)
  {
    for (ezUInt32 face = 0; face < numFaces; face++)
    {
      for (ezUInt32 row = 0; row < originalHeight; row++)
      {
        DownScaleFastLine(pixelStride, image.GetPixelPointer<ezUInt8>(0, face, arrayIndex, 0, row), intermediate.GetPixelPointer<ezUInt8>(0, face, arrayIndex, 0, row), originalWidth, pixelStride, uiWidth, pixelStride);
      }
    }
  }

  // input and output images may be the same, so we can't access the original image below this point

  ezImageHeader outHeader;
  outHeader.SetWidth(uiWidth);
  outHeader.SetHeight(uiHeight);
  outHeader.SetNumArrayIndices(numArrayElements);
  outHeader.SetNumArrayIndices(numFaces);
  outHeader.SetImageFormat(format);

  out_result.ResetAndAlloc(outHeader);

  EZ_ASSERT_DEBUG(intermediate.GetRowPitch() < ezMath::MaxValue<ezUInt32>(), "Row pitch exceeds ezUInt32 max value.");
  EZ_ASSERT_DEBUG(out_result.GetRowPitch() < ezMath::MaxValue<ezUInt32>(), "Row pitch exceeds ezUInt32 max value.");

  for (ezUInt32 arrayIndex = 0; arrayIndex < numArrayElements; arrayIndex++)
  {
    for (ezUInt32 face = 0; face < numFaces; face++)
    {
      for (ezUInt32 col = 0; col < uiWidth; col++)
      {
        DownScaleFastLine(pixelStride, intermediate.GetPixelPointer<ezUInt8>(0, face, arrayIndex, col), out_result.GetPixelPointer<ezUInt8>(0, face, arrayIndex, col), originalHeight, static_cast<ezUInt32>(intermediate.GetRowPitch()), uiHeight, static_cast<ezUInt32>(out_result.GetRowPitch()));
      }
    }
  }
}

static float EvaluateAverageCoverage(ezBlobPtr<const ezColor> colors, float fAlphaThreshold)
{
  EZ_PROFILE_SCOPE("EvaluateAverageCoverage");

  ezUInt64 totalPixels = colors.GetCount();
  ezUInt64 count = 0;
  for (ezUInt64 idx = 0; idx < totalPixels; ++idx)
  {
    count += colors[idx].a >= fAlphaThreshold;
  }

  return float(count) / float(totalPixels);
}

static void NormalizeCoverage(ezBlobPtr<ezColor> colors, float fAlphaThreshold, float fTargetCoverage)
{
  EZ_PROFILE_SCOPE("NormalizeCoverage");

  // Based on the idea in http://the-witness.net/news/2010/09/computing-alpha-mipmaps/. Note we're using a histogram
  // to find the new alpha threshold here rather than bisecting.

  // Generate histogram of alpha values
  ezUInt64 totalPixels = colors.GetCount();
  ezUInt32 alphaHistogram[256] = {};
  for (ezUInt64 idx = 0; idx < totalPixels; ++idx)
  {
    alphaHistogram[ezMath::ColorFloatToByte(colors[idx].a)]++;
  }

  // Find range of alpha thresholds so the number of covered pixels matches by summing up the histogram
  ezInt32 targetCount = ezInt32(fTargetCoverage * totalPixels);
  ezInt32 coverageCount = 0;
  ezInt32 maxThreshold = 255;
  for (; maxThreshold >= 0; maxThreshold--)
  {
    coverageCount += alphaHistogram[maxThreshold];

    if (coverageCount >= targetCount)
    {
      break;
    }
  }

  coverageCount = targetCount;
  ezInt32 minThreshold = 0;
  for (; minThreshold < 256; minThreshold++)
  {
    coverageCount -= alphaHistogram[maxThreshold];

    if (coverageCount <= targetCount)
    {
      break;
    }
  }

  ezInt32 currentThreshold = ezMath::ColorFloatToByte(fAlphaThreshold);

  // Each of the alpha test thresholds in the range [minThreshold; maxThreshold] will result in the same coverage. Pick a new threshold
  // close to the old one so we scale by the smallest necessary amount.
  ezInt32 newThreshold;
  if (currentThreshold < minThreshold)
  {
    newThreshold = minThreshold;
  }
  else if (currentThreshold > maxThreshold)
  {
    newThreshold = maxThreshold;
  }
  else
  {
    // Avoid rescaling altogether if the current threshold already preserves coverage
    return;
  }

  // Rescale alpha values
  float alphaScale = fAlphaThreshold / (newThreshold / 255.0f);
  for (ezUInt64 idx = 0; idx < totalPixels; ++idx)
  {
    colors[idx].a *= alphaScale;
  }
}


ezResult ezImageUtils::Scale(const ezImageView& source, ezImage& ref_target, ezUInt32 uiWidth, ezUInt32 uiHeight, const ezImageFilter* pFilter, ezImageAddressMode::Enum addressModeU, ezImageAddressMode::Enum addressModeV, const ezColor& borderColor)
{
  return Scale3D(source, ref_target, uiWidth, uiHeight, 1, pFilter, addressModeU, addressModeV, ezImageAddressMode::Clamp, borderColor);
}

ezResult ezImageUtils::Scale3D(const ezImageView& source, ezImage& ref_target, ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiDepth, const ezImageFilter* pFilter /*= ez_NULL*/, ezImageAddressMode::Enum addressModeU /*= ezImageAddressMode::Clamp*/,
  ezImageAddressMode::Enum addressModeV /*= ezImageAddressMode::Clamp*/, ezImageAddressMode::Enum addressModeW /*= ezImageAddressMode::Clamp*/, const ezColor& borderColor /*= ezColors::Black*/)
{
  EZ_PROFILE_SCOPE("ezImageUtils::Scale3D");

  if (uiWidth == 0 || uiHeight == 0 || uiDepth == 0)
  {
    ezImageHeader header;
    header.SetImageFormat(source.GetImageFormat());
    ref_target.ResetAndAlloc(header);
    return EZ_SUCCESS;
  }

  const ezImageFormat::Enum format = source.GetImageFormat();

  const ezUInt32 originalWidth = source.GetWidth();
  const ezUInt32 originalHeight = source.GetHeight();
  const ezUInt32 originalDepth = source.GetDepth();
  const ezUInt32 numFaces = source.GetNumFaces();
  const ezUInt32 numArrayElements = source.GetNumArrayIndices();

  if (originalWidth == uiWidth && originalHeight == uiHeight && originalDepth == uiDepth)
  {
    ref_target.ResetAndCopy(source);
    return EZ_SUCCESS;
  }

  // Scaling down by an even factor?
  const ezUInt32 downScaleFactorX = originalWidth / uiWidth;
  const ezUInt32 downScaleFactorY = originalHeight / uiHeight;

  if (pFilter == nullptr && (format == ezImageFormat::R8G8B8A8_UNORM || format == ezImageFormat::B8G8R8A8_UNORM || format == ezImageFormat::B8G8R8_UNORM) && downScaleFactorX * uiWidth == originalWidth && downScaleFactorY * uiHeight == originalHeight && uiDepth == 1 && originalDepth == 1 &&
      ezMath::IsPowerOf2(downScaleFactorX) && ezMath::IsPowerOf2(downScaleFactorY))
  {
    DownScaleFast(source, ref_target, uiWidth, uiHeight);
    return EZ_SUCCESS;
  }

  // Fallback to default filter
  ezImageFilterTriangle defaultFilter;
  if (!pFilter)
  {
    pFilter = &defaultFilter;
  }

  const ezImageView* stepSource;

  // Manage scratch images for intermediate conversion or filtering
  const ezUInt32 maxNumScratchImages = 2;
  ezImage scratch[maxNumScratchImages];
  bool scratchUsed[maxNumScratchImages] = {};
  auto allocateScratch = [&]() -> ezImage&
  {
    for (ezUInt32 i = 0;; ++i)
    {
      EZ_ASSERT_DEV(i < maxNumScratchImages, "Failed to allocate scratch image");
      if (!scratchUsed[i])
      {
        scratchUsed[i] = true;
        return scratch[i];
      }
    }
  };
  auto releaseScratch = [&](const ezImageView& image)
  {
    for (ezUInt32 i = 0; i < maxNumScratchImages; ++i)
    {
      if (&scratch[i] == &image)
      {
        scratchUsed[i] = false;
        return;
      }
    }
  };

  if (format == ezImageFormat::R32G32B32A32_FLOAT)
  {
    stepSource = &source;
  }
  else
  {
    ezImage& conversionScratch = allocateScratch();
    if (ezImageConversion::Convert(source, conversionScratch, ezImageFormat::R32G32B32A32_FLOAT).Failed())
    {
      return EZ_FAILURE;
    }

    stepSource = &conversionScratch;
  };

  ezHybridArray<ezInt32, 256> firstSampleIndices;
  firstSampleIndices.Reserve(ezMath::Max(uiWidth, uiHeight, uiDepth));

  if (uiWidth != originalWidth)
  {
    ezImageFilterWeights weights(*pFilter, originalWidth, uiWidth);
    firstSampleIndices.SetCountUninitialized(uiWidth);
    for (ezUInt32 x = 0; x < uiWidth; ++x)
    {
      firstSampleIndices[x] = weights.GetFirstSourceSampleIndex(x);
    }

    ezImage* stepTarget;
    if (uiHeight == originalHeight && uiDepth == originalDepth && format == ezImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &ref_target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    ezImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetWidth(uiWidth);
    stepTarget->ResetAndAlloc(stepHeader);

    for (ezUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (ezUInt32 face = 0; face < numFaces; ++face)
      {
        for (ezUInt32 z = 0; z < originalDepth; ++z)
        {
          for (ezUInt32 y = 0; y < originalHeight; ++y)
          {
            const ezSimdVec4f* filterSource = stepSource->GetPixelPointer<ezSimdVec4f>(0, face, arrayIndex, 0, y, z);
            ezSimdVec4f* filterTarget = stepTarget->GetPixelPointer<ezSimdVec4f>(0, face, arrayIndex, 0, y, z);
            FilterLine(originalWidth, filterSource, filterTarget, 1, weights, firstSampleIndices, addressModeU, ezSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  if (uiHeight != originalHeight)
  {
    ezImageFilterWeights weights(*pFilter, originalHeight, uiHeight);
    firstSampleIndices.SetCount(uiHeight);
    for (ezUInt32 y = 0; y < uiHeight; ++y)
    {
      firstSampleIndices[y] = weights.GetFirstSourceSampleIndex(y);
    }

    ezImage* stepTarget;
    if (uiDepth == originalDepth && format == ezImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &ref_target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    ezImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetHeight(uiHeight);
    stepTarget->ResetAndAlloc(stepHeader);

    for (ezUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (ezUInt32 face = 0; face < numFaces; ++face)
      {
        for (ezUInt32 z = 0; z < originalDepth; ++z)
        {
          for (ezUInt32 x = 0; x < uiWidth; ++x)
          {
            const ezSimdVec4f* filterSource = stepSource->GetPixelPointer<ezSimdVec4f>(0, face, arrayIndex, x, 0, z);
            ezSimdVec4f* filterTarget = stepTarget->GetPixelPointer<ezSimdVec4f>(0, face, arrayIndex, x, 0, z);
            FilterLine(originalHeight, filterSource, filterTarget, uiWidth, weights, firstSampleIndices, addressModeV, ezSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  if (uiDepth != originalDepth)
  {
    ezImageFilterWeights weights(*pFilter, originalDepth, uiDepth);
    firstSampleIndices.SetCount(uiDepth);
    for (ezUInt32 z = 0; z < uiDepth; ++z)
    {
      firstSampleIndices[z] = weights.GetFirstSourceSampleIndex(z);
    }

    ezImage* stepTarget;
    if (format == ezImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &ref_target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    ezImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetDepth(uiDepth);
    stepTarget->ResetAndAlloc(stepHeader);

    for (ezUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (ezUInt32 face = 0; face < numFaces; ++face)
      {
        for (ezUInt32 y = 0; y < uiHeight; ++y)
        {
          for (ezUInt32 x = 0; x < uiWidth; ++x)
          {
            const ezSimdVec4f* filterSource = stepSource->GetPixelPointer<ezSimdVec4f>(0, face, arrayIndex, x, y, 0);
            ezSimdVec4f* filterTarget = stepTarget->GetPixelPointer<ezSimdVec4f>(0, face, arrayIndex, x, y, 0);
            FilterLine(originalHeight, filterSource, filterTarget, uiWidth * uiHeight, weights, firstSampleIndices, addressModeW, ezSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  // Convert back to original format - no-op if stepSource and target are the same
  return ezImageConversion::Convert(*stepSource, ref_target, format);
}

void ezImageUtils::GenerateMipMaps(const ezImageView& source, ezImage& ref_target, const MipMapOptions& options)
{
  EZ_PROFILE_SCOPE("ezImageUtils::GenerateMipMaps");

  ezImageHeader header = source.GetHeader();
  EZ_ASSERT_DEV(header.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "The source image must be a RGBA 32-bit float format.");
  EZ_ASSERT_DEV(&source != &ref_target, "Source and target must not be the same image.");

  // Make a local copy to be able to tweak some of the options
  ezImageUtils::MipMapOptions mipMapOptions = options;

  // alpha thresholds with extreme values are not supported at the moment
  mipMapOptions.m_alphaThreshold = ezMath::Clamp(mipMapOptions.m_alphaThreshold, 0.05f, 0.95f);

  // Enforce CLAMP addressing mode for cubemaps
  if (source.GetNumFaces() == 6)
  {
    mipMapOptions.m_addressModeU = ezImageAddressMode::Clamp;
    mipMapOptions.m_addressModeV = ezImageAddressMode::Clamp;
  }

  ezUInt32 numMipMaps = header.ComputeNumberOfMipMaps();
  if (mipMapOptions.m_numMipMaps > 0 && mipMapOptions.m_numMipMaps < numMipMaps)
  {
    numMipMaps = mipMapOptions.m_numMipMaps;
  }
  header.SetNumMipLevels(numMipMaps);

  ref_target.ResetAndAlloc(header);

  for (ezUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (ezUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      ezImageHeader currentMipMapHeader = header;
      currentMipMapHeader.SetNumMipLevels(1);
      currentMipMapHeader.SetNumFaces(1);
      currentMipMapHeader.SetNumArrayIndices(1);

      auto sourceView = source.GetSubImageView(0, face, arrayIndex).GetByteBlobPtr();
      auto targetView = ref_target.GetSubImageView(0, face, arrayIndex).GetByteBlobPtr();

      memcpy(targetView.GetPtr(), sourceView.GetPtr(), static_cast<size_t>(targetView.GetCount()));

      float targetCoverage = 0.0f;
      if (mipMapOptions.m_preserveCoverage)
      {
        targetCoverage = EvaluateAverageCoverage(source.GetSubImageView(0, face, arrayIndex).GetBlobPtr<ezColor>(), mipMapOptions.m_alphaThreshold);
      }

      for (ezUInt32 mipMapLevel = 0; mipMapLevel < numMipMaps - 1; mipMapLevel++)
      {
        ezImageHeader nextMipMapHeader = currentMipMapHeader;
        nextMipMapHeader.SetWidth(ezMath::Max(1u, nextMipMapHeader.GetWidth() / 2));
        nextMipMapHeader.SetHeight(ezMath::Max(1u, nextMipMapHeader.GetHeight() / 2));
        nextMipMapHeader.SetDepth(ezMath::Max(1u, nextMipMapHeader.GetDepth() / 2));

        auto sourceData = ref_target.GetSubImageView(mipMapLevel, face, arrayIndex).GetByteBlobPtr();
        ezImage currentMipMap;
        currentMipMap.ResetAndUseExternalStorage(currentMipMapHeader, sourceData);

        auto dstData = ref_target.GetSubImageView(mipMapLevel + 1, face, arrayIndex).GetByteBlobPtr();
        ezImage nextMipMap;
        nextMipMap.ResetAndUseExternalStorage(nextMipMapHeader, dstData);

        ezImageUtils::Scale3D(currentMipMap, nextMipMap, nextMipMapHeader.GetWidth(), nextMipMapHeader.GetHeight(), nextMipMapHeader.GetDepth(), mipMapOptions.m_filter, mipMapOptions.m_addressModeU, mipMapOptions.m_addressModeV, mipMapOptions.m_addressModeW, mipMapOptions.m_borderColor)
          .IgnoreResult();

        if (mipMapOptions.m_preserveCoverage)
        {
          NormalizeCoverage(nextMipMap.GetBlobPtr<ezColor>(), mipMapOptions.m_alphaThreshold, targetCoverage);
        }

        if (mipMapOptions.m_renormalizeNormals)
        {
          RenormalizeNormalMap(nextMipMap);
        }

        currentMipMapHeader = nextMipMapHeader;
      }
    }
  }
}

void ezImageUtils::ReconstructNormalZ(ezImage& ref_image)
{
  EZ_PROFILE_SCOPE("ezImageUtils::ReconstructNormalZ");

  EZ_ASSERT_DEV(ref_image.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  ezSimdVec4f* cur = ref_image.GetBlobPtr<ezSimdVec4f>().GetPtr();
  ezSimdVec4f* const end = ref_image.GetBlobPtr<ezSimdVec4f>().GetEndPtr();

  ezSimdFloat oneScalar = 1.0f;

  ezSimdVec4f two(2.0f);

  ezSimdVec4f minusOne(-1.0f);

  ezSimdVec4f half(0.5f);

  for (; cur < end; cur++)
  {
    ezSimdVec4f normal;
    // unpack from [0,1] to [-1, 1]
    normal = ezSimdVec4f::MulAdd(*cur, two, minusOne);

    // compute Z component
    normal.SetZ((oneScalar - normal.Dot<2>(normal)).GetSqrt());

    // pack back to [0,1]
    *cur = ezSimdVec4f::MulAdd(half, normal, half);
  }
}

void ezImageUtils::RenormalizeNormalMap(ezImage& ref_image)
{
  EZ_PROFILE_SCOPE("ezImageUtils::RenormalizeNormalMap");

  EZ_ASSERT_DEV(ref_image.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  ezSimdVec4f* start = ref_image.GetBlobPtr<ezSimdVec4f>().GetPtr();
  ezSimdVec4f* const end = ref_image.GetBlobPtr<ezSimdVec4f>().GetEndPtr();

  ezSimdVec4f two(2.0f);

  ezSimdVec4f minusOne(-1.0f);

  ezSimdVec4f half(0.5f);

  for (; start < end; start++)
  {
    ezSimdVec4f normal;
    normal = ezSimdVec4f::MulAdd(*start, two, minusOne);
    normal.Normalize<3>();
    *start = ezSimdVec4f::MulAdd(half, normal, half);
  }
}

void ezImageUtils::AdjustRoughness(ezImage& ref_roughnessMap, const ezImageView& normalMap)
{
  EZ_PROFILE_SCOPE("ezImageUtils::AdjustRoughness");

  EZ_ASSERT_DEV(ref_roughnessMap.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");
  EZ_ASSERT_DEV(normalMap.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  EZ_ASSERT_DEV(ref_roughnessMap.GetWidth() >= normalMap.GetWidth() && ref_roughnessMap.GetHeight() >= normalMap.GetHeight(), "The roughness map needs to be bigger or same size than the normal map.");

  ezImage filteredNormalMap;
  ezImageUtils::MipMapOptions options;

  // Box filter normal map without re-normalization so we have the average normal length in each mip map.
  if (ref_roughnessMap.GetWidth() != normalMap.GetWidth() || ref_roughnessMap.GetHeight() != normalMap.GetHeight())
  {
    ezImage temp;
    ezImageUtils::Scale(normalMap, temp, ref_roughnessMap.GetWidth(), ref_roughnessMap.GetHeight()).IgnoreResult();
    ezImageUtils::RenormalizeNormalMap(temp);
    ezImageUtils::GenerateMipMaps(temp, filteredNormalMap, options);
  }
  else
  {
    ezImageUtils::GenerateMipMaps(normalMap, filteredNormalMap, options);
  }

  EZ_ASSERT_DEV(ref_roughnessMap.GetNumMipLevels() == filteredNormalMap.GetNumMipLevels(), "Roughness and normal map must have the same number of mip maps");

  ezSimdVec4f two(2.0f);
  ezSimdVec4f minusOne(-1.0f);

  ezUInt32 numMipLevels = ref_roughnessMap.GetNumMipLevels();
  for (ezUInt32 mipLevel = 1; mipLevel < numMipLevels; ++mipLevel)
  {
    ezBlobPtr<ezSimdVec4f> roughnessData = ref_roughnessMap.GetSubImageView(mipLevel, 0, 0).GetBlobPtr<ezSimdVec4f>();
    ezBlobPtr<ezSimdVec4f> normalData = filteredNormalMap.GetSubImageView(mipLevel, 0, 0).GetBlobPtr<ezSimdVec4f>();

    for (ezUInt64 i = 0; i < roughnessData.GetCount(); ++i)
    {
      ezSimdVec4f normal = ezSimdVec4f::MulAdd(normalData[i], two, minusOne);

      float avgNormalLength = normal.GetLength<3>();
      if (avgNormalLength < 1.0f)
      {
        float avgNormalLengthSquare = avgNormalLength * avgNormalLength;
        float kappa = (3.0f * avgNormalLength - avgNormalLength * avgNormalLengthSquare) / (1.0f - avgNormalLengthSquare);
        float variance = 1.0f / (2.0f * kappa);

        float oldRoughness = roughnessData[i].GetComponent<0>();
        float newRoughness = ezMath::Sqrt(oldRoughness * oldRoughness + variance);

        roughnessData[i].Set(newRoughness);
      }
    }
  }
}

void ezImageUtils::ChangeExposure(ezImage& ref_image, float fBias)
{
  EZ_ASSERT_DEV(ref_image.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "This function expects an RGBA 32 float image as input");

  if (fBias == 0.0f)
    return;

  EZ_PROFILE_SCOPE("ezImageUtils::ChangeExposure");

  const float multiplier = ezMath::Pow2(fBias);

  for (ezColor& col : ref_image.GetBlobPtr<ezColor>())
  {
    col = multiplier * col;
  }
}

static ezResult CopyImageRectToFace(ezImage& ref_dstImg, const ezImageView& srcImg, ezUInt32 uiOffsetX, ezUInt32 uiOffsetY, ezUInt32 uiFaceIndex)
{
  ezRectU32 r;
  r.x = uiOffsetX;
  r.y = uiOffsetY;
  r.width = ref_dstImg.GetWidth();
  r.height = r.width;

  return ezImageUtils::Copy(srcImg, r, ref_dstImg, ezVec3U32(0), 0, uiFaceIndex);
}

ezResult ezImageUtils::CreateCubemapFromSingleFile(ezImage& ref_dstImg, const ezImageView& srcImg)
{
  EZ_PROFILE_SCOPE("ezImageUtils::CreateCubemapFromSingleFile");

  if (srcImg.GetNumFaces() == 6)
  {
    ref_dstImg.ResetAndCopy(srcImg);
    return EZ_SUCCESS;
  }
  else if (srcImg.GetNumFaces() == 1)
  {
    if (srcImg.GetWidth() % 3 == 0 && srcImg.GetHeight() % 4 == 0 && srcImg.GetWidth() / 3 == srcImg.GetHeight() / 4)
    {
      // Vertical cube map layout
      //     +---+
      //     | Y+|
      // +---+---+---+
      // | X-| Z+| X+|
      // +---+---+---+
      //     | Y-|
      //     +---+
      //     | Z-|
      //     +---+
      const ezUInt32 faceSize = srcImg.GetWidth() / 3;

      ezImageHeader imgHeader;
      imgHeader.SetWidth(faceSize);
      imgHeader.SetHeight(faceSize);
      imgHeader.SetImageFormat(srcImg.GetImageFormat());
      imgHeader.SetDepth(1);
      imgHeader.SetNumFaces(6);
      imgHeader.SetNumMipLevels(1);
      imgHeader.SetNumArrayIndices(1);

      ref_dstImg.ResetAndAlloc(imgHeader);

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      EZ_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize * 2, faceSize, 0));

      // Negative X face
      EZ_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, 0, faceSize, 1));

      // Positive Y face
      EZ_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, 0, 2));

      // Negative Y face
      EZ_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize * 2, 3));

      // Positive Z face
      EZ_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize, 4));

      // Negative Z face
      EZ_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize * 3, 5));
      ezImageUtils::RotateSubImage180(ref_dstImg, 0, 5);
    }
    else if (srcImg.GetWidth() % 4 == 0 && srcImg.GetHeight() % 3 == 0 && srcImg.GetWidth() / 4 == srcImg.GetHeight() / 3)
    {
      // Horizontal cube map layout
      //     +---+
      //     | Y+|
      // +---+---+---+---+
      // | X-| Z+| X+| Z-|
      // +---+---+---+---+
      //     | Y-|
      //     +---+
      const ezUInt32 faceSize = srcImg.GetWidth() / 4;

      ezImageHeader imgHeader;
      imgHeader.SetWidth(faceSize);
      imgHeader.SetHeight(faceSize);
      imgHeader.SetImageFormat(srcImg.GetImageFormat());
      imgHeader.SetDepth(1);
      imgHeader.SetNumFaces(6);
      imgHeader.SetNumMipLevels(1);
      imgHeader.SetNumArrayIndices(1);

      ref_dstImg.ResetAndAlloc(imgHeader);

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      EZ_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize * 2, faceSize, 0));

      // Negative X face
      EZ_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, 0, faceSize, 1));

      // Positive Y face
      EZ_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, 0, 2));

      // Negative Y face
      EZ_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize * 2, 3));

      // Positive Z face
      EZ_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize, 4));

      // Negative Z face
      EZ_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize * 3, faceSize, 5));
    }
    else
    {
      // Spherical mapping
      if (srcImg.GetWidth() % 4 != 0)
      {
        ezLog::Error("Width of the input image should be a multiple of 4");
        return EZ_FAILURE;
      }

      const ezUInt32 faceSize = srcImg.GetWidth() / 4;

      ezImageHeader imgHeader;
      imgHeader.SetWidth(faceSize);
      imgHeader.SetHeight(faceSize);
      imgHeader.SetImageFormat(srcImg.GetImageFormat());
      imgHeader.SetDepth(1);
      imgHeader.SetNumFaces(6);
      imgHeader.SetNumMipLevels(1);
      imgHeader.SetNumArrayIndices(1);

      ref_dstImg.ResetAndAlloc(imgHeader);

      // Corners of the UV space for the respective faces in model space
      const ezVec3 faceCorners[] = {
        ezVec3(0.5, 0.5, 0.5),   // X+
        ezVec3(-0.5, 0.5, -0.5), // X-
        ezVec3(-0.5, 0.5, -0.5), // Y+
        ezVec3(-0.5, -0.5, 0.5), // Y-
        ezVec3(-0.5, 0.5, 0.5),  // Z+
        ezVec3(0.5, 0.5, -0.5)   // Z-
      };

      // UV Axis of the respective faces in model space
      const ezVec3 faceAxis[] = {
        ezVec3(0, 0, -1), ezVec3(0, -1, 0), // X+
        ezVec3(0, 0, 1), ezVec3(0, -1, 0),  // X-
        ezVec3(1, 0, 0), ezVec3(0, 0, 1),   // Y+
        ezVec3(1, 0, 0), ezVec3(0, 0, -1),  // Y-
        ezVec3(1, 0, 0), ezVec3(0, -1, 0),  // Z+
        ezVec3(-1, 0, 0), ezVec3(0, -1, 0)  // Z-
      };

      const float fFaceSize = (float)faceSize;
      const float fHalfPixel = 0.5f / fFaceSize;
      const float fPixel = 1.0f / fFaceSize;

      const float fHalfSrcWidth = srcImg.GetWidth() / 2.0f;
      const float fSrcHeight = (float)srcImg.GetHeight();

      const ezUInt32 srcWidthMinus1 = srcImg.GetWidth() - 1;
      const ezUInt32 srcHeightMinus1 = srcImg.GetHeight() - 1;

      EZ_ASSERT_DEBUG(srcImg.GetRowPitch() % sizeof(ezColor) == 0, "Row pitch should be a multiple of sizeof(ezColor)");
      const ezUInt64 srcRowPitch = srcImg.GetRowPitch() / sizeof(ezColor);

      EZ_ASSERT_DEBUG(ref_dstImg.GetRowPitch() % sizeof(ezColor) == 0, "Row pitch should be a multiple of sizeof(ezColor)");
      const ezUInt64 faceRowPitch = ref_dstImg.GetRowPitch() / sizeof(ezColor);

      const ezColor* srcData = srcImg.GetPixelPointer<ezColor>();
      const float InvPi = 1.0f / ezMath::Pi<float>();

      for (ezUInt32 faceIndex = 0; faceIndex < 6; faceIndex++)
      {
        ezColor* faceData = ref_dstImg.GetPixelPointer<ezColor>(0, faceIndex);
        for (ezUInt32 y = 0; y < faceSize; y++)
        {
          const float dstV = (float)y * fPixel + fHalfPixel;

          for (ezUInt32 x = 0; x < faceSize; x++)
          {
            const float dstU = (float)x * fPixel + fHalfPixel;
            const ezVec3 modelSpacePos = faceCorners[faceIndex] + dstU * faceAxis[faceIndex * 2] + dstV * faceAxis[faceIndex * 2 + 1];
            const ezVec3 modelSpaceDir = modelSpacePos.GetNormalized();

            const float phi = ezMath::ATan2(modelSpaceDir.x, modelSpaceDir.z).GetRadian() + ezMath::Pi<float>();
            const float r = ezMath::Sqrt(modelSpaceDir.x * modelSpaceDir.x + modelSpaceDir.z * modelSpaceDir.z);
            const float theta = ezMath::ATan2(modelSpaceDir.y, r).GetRadian() + ezMath::Pi<float>() * 0.5f;

            EZ_ASSERT_DEBUG(phi >= 0.0f && phi <= 2.0f * ezMath::Pi<float>(), "");
            EZ_ASSERT_DEBUG(theta >= 0.0f && theta <= ezMath::Pi<float>(), "");

            const float srcU = phi * InvPi * fHalfSrcWidth;
            const float srcV = (1.0f - theta * InvPi) * fSrcHeight;

            ezUInt32 x1 = (ezUInt32)ezMath::Floor(srcU);
            ezUInt32 x2 = x1 + 1;
            ezUInt32 y1 = (ezUInt32)ezMath::Floor(srcV);
            ezUInt32 y2 = y1 + 1;

            const float fracX = srcU - x1;
            const float fracY = srcV - y1;

            x1 = ezMath::Clamp(x1, 0u, srcWidthMinus1);
            x2 = ezMath::Clamp(x2, 0u, srcWidthMinus1);
            y1 = ezMath::Clamp(y1, 0u, srcHeightMinus1);
            y2 = ezMath::Clamp(y2, 0u, srcHeightMinus1);

            ezColor A = srcData[x1 + y1 * srcRowPitch];
            ezColor B = srcData[x2 + y1 * srcRowPitch];
            ezColor C = srcData[x1 + y2 * srcRowPitch];
            ezColor D = srcData[x2 + y2 * srcRowPitch];

            ezColor interpolated = A * (1 - fracX) * (1 - fracY) + B * (fracX) * (1 - fracY) + C * (1 - fracX) * fracY + D * fracX * fracY;
            faceData[x + y * faceRowPitch] = interpolated;
          }
        }
      }
    }

    return EZ_SUCCESS;
  }

  ezLog::Error("Unexpected number of faces in cubemap input image.");
  return EZ_FAILURE;
}

ezResult ezImageUtils::CreateCubemapFrom6Files(ezImage& ref_dstImg, const ezImageView* pSourceImages)
{
  EZ_PROFILE_SCOPE("ezImageUtils::CreateCubemapFrom6Files");

  ezImageHeader header = pSourceImages[0].GetHeader();
  header.SetNumFaces(6);

  if (header.GetWidth() != header.GetHeight())
    return EZ_FAILURE;

  if (!ezMath::IsPowerOf2(header.GetWidth()))
    return EZ_FAILURE;

  ref_dstImg.ResetAndAlloc(header);

  for (ezUInt32 i = 0; i < 6; ++i)
  {
    if (pSourceImages[i].GetImageFormat() != ref_dstImg.GetImageFormat())
      return EZ_FAILURE;

    if (pSourceImages[i].GetWidth() != ref_dstImg.GetWidth())
      return EZ_FAILURE;

    if (pSourceImages[i].GetHeight() != ref_dstImg.GetHeight())
      return EZ_FAILURE;

    EZ_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, pSourceImages[i], 0, 0, i));
  }

  return EZ_SUCCESS;
}

ezResult ezImageUtils::CreateVolumeTextureFromSingleFile(ezImage& ref_dstImg, const ezImageView& srcImg)
{
  EZ_PROFILE_SCOPE("ezImageUtils::CreateVolumeTextureFromSingleFile");

  const ezUInt32 uiWidthHeight = srcImg.GetHeight();
  const ezUInt32 uiDepth = srcImg.GetWidth() / uiWidthHeight;

  if (!ezMath::IsPowerOf2(uiWidthHeight))
    return EZ_FAILURE;
  if (!ezMath::IsPowerOf2(uiDepth))
    return EZ_FAILURE;

  ezImageHeader header;
  header.SetWidth(uiWidthHeight);
  header.SetHeight(uiWidthHeight);
  header.SetDepth(uiDepth);
  header.SetImageFormat(srcImg.GetImageFormat());

  ref_dstImg.ResetAndAlloc(header);

  const ezImageView view = srcImg.GetSubImageView();

  for (ezUInt32 d = 0; d < uiDepth; ++d)
  {
    ezRectU32 r;
    r.x = uiWidthHeight * d;
    r.y = 0;
    r.width = uiWidthHeight;
    r.height = uiWidthHeight;

    EZ_SUCCEED_OR_RETURN(Copy(view, r, ref_dstImg, ezVec3U32(0, 0, d)));
  }

  return EZ_SUCCESS;
}

ezColor ezImageUtils::NearestSample(const ezImageView& image, ezImageAddressMode::Enum addressMode, ezVec2 vUv)
{
  EZ_ASSERT_DEBUG(image.GetDepth() == 1 && image.GetNumFaces() == 1 && image.GetNumArrayIndices() == 1, "Only 2d images are supported");
  EZ_ASSERT_DEBUG(image.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "Unsupported format");

  return NearestSample(image.GetPixelPointer<ezColor>(), image.GetWidth(), image.GetHeight(), addressMode, vUv);
}

ezColor ezImageUtils::NearestSample(const ezColor* pPixelPointer, ezUInt32 uiWidth, ezUInt32 uiHeight, ezImageAddressMode::Enum addressMode, ezVec2 vUv)
{
  const ezInt32 w = uiWidth;
  const ezInt32 h = uiHeight;

  vUv = vUv.CompMul(ezVec2(static_cast<float>(w), static_cast<float>(h)));
  const ezInt32 intX = (ezInt32)ezMath::Floor(vUv.x);
  const ezInt32 intY = (ezInt32)ezMath::Floor(vUv.y);

  ezInt32 x = intX;
  ezInt32 y = intY;

  if (addressMode == ezImageAddressMode::Clamp)
  {
    x = ezMath::Clamp(x, 0, w - 1);
    y = ezMath::Clamp(y, 0, h - 1);
  }
  else if (addressMode == ezImageAddressMode::Repeat)
  {
    x = x % w;
    x = x < 0 ? x + w : x;
    y = y % h;
    y = y < 0 ? y + h : y;
  }
  else
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return *(pPixelPointer + (y * w) + x);
}

ezColor ezImageUtils::BilinearSample(const ezImageView& image, ezImageAddressMode::Enum addressMode, ezVec2 vUv)
{
  EZ_ASSERT_DEBUG(image.GetDepth() == 1 && image.GetNumFaces() == 1 && image.GetNumArrayIndices() == 1, "Only 2d images are supported");
  EZ_ASSERT_DEBUG(image.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "Unsupported format");

  return BilinearSample(image.GetPixelPointer<ezColor>(), image.GetWidth(), image.GetHeight(), addressMode, vUv);
}

ezColor ezImageUtils::BilinearSample(const ezColor* pData, ezUInt32 uiWidth, ezUInt32 uiHeight, ezImageAddressMode::Enum addressMode, ezVec2 vUv)
{
  ezInt32 w = uiWidth;
  ezInt32 h = uiHeight;

  vUv = vUv.CompMul(ezVec2(static_cast<float>(w), static_cast<float>(h))) - ezVec2(0.5f);
  const float floorX = ezMath::Floor(vUv.x);
  const float floorY = ezMath::Floor(vUv.y);
  const float fractionX = vUv.x - floorX;
  const float fractionY = vUv.y - floorY;
  const ezInt32 intX = (ezInt32)floorX;
  const ezInt32 intY = (ezInt32)floorY;

  ezColor c[4];
  for (ezUInt32 i = 0; i < 4; ++i)
  {
    ezInt32 x = intX + (i % 2);
    ezInt32 y = intY + (i / 2);

    if (addressMode == ezImageAddressMode::Clamp)
    {
      x = ezMath::Clamp(x, 0, w - 1);
      y = ezMath::Clamp(y, 0, h - 1);
    }
    else if (addressMode == ezImageAddressMode::Repeat)
    {
      x = x % w;
      x = x < 0 ? x + w : x;
      y = y % h;
      y = y < 0 ? y + h : y;
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }

    c[i] = *(pData + (y * w) + x);
  }

  const ezColor cr0 = ezMath::Lerp(c[0], c[1], fractionX);
  const ezColor cr1 = ezMath::Lerp(c[2], c[3], fractionX);

  return ezMath::Lerp(cr0, cr1, fractionY);
}

ezResult ezImageUtils::CopyChannel(ezImage& ref_dstImg, ezUInt8 uiDstChannelIdx, const ezImage& srcImg, ezUInt8 uiSrcChannelIdx)
{
  EZ_PROFILE_SCOPE("ezImageUtils::CopyChannel");

  if (uiSrcChannelIdx >= 4 || uiDstChannelIdx >= 4)
    return EZ_FAILURE;

  if (ref_dstImg.GetImageFormat() != ezImageFormat::R32G32B32A32_FLOAT)
    return EZ_FAILURE;

  if (srcImg.GetImageFormat() != ref_dstImg.GetImageFormat())
    return EZ_FAILURE;

  if (srcImg.GetWidth() != ref_dstImg.GetWidth())
    return EZ_FAILURE;

  if (srcImg.GetHeight() != ref_dstImg.GetHeight())
    return EZ_FAILURE;

  const ezUInt32 uiNumPixels = srcImg.GetWidth() * srcImg.GetHeight();
  const float* pSrcPixel = srcImg.GetPixelPointer<float>();
  float* pDstPixel = ref_dstImg.GetPixelPointer<float>();

  pSrcPixel += uiSrcChannelIdx;
  pDstPixel += uiDstChannelIdx;

  for (ezUInt32 i = 0; i < uiNumPixels; ++i)
  {
    *pDstPixel = *pSrcPixel;

    pSrcPixel += 4;
    pDstPixel += 4;
  }

  return EZ_SUCCESS;
}

static const ezUInt8 s_Base64EncodingTable[64] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
  'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

static const ezUInt8 BASE64_CHARS_PER_LINE = 76;

static ezUInt32 GetBase64EncodedLength(ezUInt32 uiInputLength, bool bInsertLineBreaks)
{
  ezUInt32 outputLength = (uiInputLength + 2) / 3 * 4;

  if (bInsertLineBreaks)
  {
    outputLength += outputLength / BASE64_CHARS_PER_LINE;
  }

  return outputLength;
}

static ezDynamicArray<char> ArrayToBase64(ezArrayPtr<const ezUInt8> in, bool bInsertLineBreaks = true)
{
  ezDynamicArray<char> out;
  out.SetCountUninitialized(GetBase64EncodedLength(in.GetCount(), bInsertLineBreaks));

  ezUInt32 offsetIn = 0;
  ezUInt32 offsetOut = 0;

  ezUInt32 blocksTillNewline = BASE64_CHARS_PER_LINE / 4;
  while (offsetIn < in.GetCount())
  {
    ezUInt8 ibuf[3] = {0};

    ezUInt32 ibuflen = ezMath::Min(in.GetCount() - offsetIn, 3u);

    for (ezUInt32 i = 0; i < ibuflen; ++i)
    {
      ibuf[i] = in[offsetIn++];
    }

    char obuf[4];
    obuf[0] = s_Base64EncodingTable[(ibuf[0] >> 2)];
    obuf[1] = s_Base64EncodingTable[((ibuf[0] << 4) & 0x30) | (ibuf[1] >> 4)];
    obuf[2] = s_Base64EncodingTable[((ibuf[1] << 2) & 0x3c) | (ibuf[2] >> 6)];
    obuf[3] = s_Base64EncodingTable[(ibuf[2] & 0x3f)];

    if (ibuflen >= 3)
    {
      out[offsetOut++] = obuf[0];
      out[offsetOut++] = obuf[1];
      out[offsetOut++] = obuf[2];
      out[offsetOut++] = obuf[3];
    }
    else // need to pad up to 4
    {
      switch (ibuflen)
      {
        case 1:
          out[offsetOut++] = obuf[0];
          out[offsetOut++] = obuf[1];
          out[offsetOut++] = '=';
          out[offsetOut++] = '=';
          break;
        case 2:
          out[offsetOut++] = obuf[0];
          out[offsetOut++] = obuf[1];
          out[offsetOut++] = obuf[2];
          out[offsetOut++] = '=';
          break;
      }
    }

    if (--blocksTillNewline == 0)
    {
      if (bInsertLineBreaks)
      {
        out[offsetOut++] = '\n';
      }
      blocksTillNewline = 19;
    }
  }

  EZ_ASSERT_DEV(offsetOut == out.GetCount(), "All output data should have been written");
  return out;
}

void ezImageUtils::EmbedImageData(ezStringBuilder& out_sHtml, const ezImage& image)
{
  ezImageFileFormat* format = ezImageFileFormat::GetWriterFormat("png");
  EZ_ASSERT_DEV(format != nullptr, "No PNG writer found");

  ezDynamicArray<ezUInt8> imgData;
  ezMemoryStreamContainerWrapperStorage<ezDynamicArray<ezUInt8>> storage(&imgData);
  ezMemoryStreamWriter writer(&storage);
  format->WriteImage(writer, image, "png").IgnoreResult();

  ezDynamicArray<char> imgDataBase64 = ArrayToBase64(imgData.GetArrayPtr());
  ezStringView imgDataBase64StringView(imgDataBase64.GetArrayPtr().GetPtr(), imgDataBase64.GetArrayPtr().GetEndPtr());
  out_sHtml.AppendFormat("data:image/png;base64,{0}", imgDataBase64StringView);
}

void ezImageUtils::CreateImageDiffHtml(ezStringBuilder& out_sHtml, ezStringView sTitle, const ezImage& referenceImgRgb, const ezImage& referenceImgAlpha, const ezImage& capturedImgRgb, const ezImage& capturedImgAlpha, const ezImage& diffImgRgb, const ezImage& diffImgAlpha, ezUInt32 uiError, ezUInt32 uiThreshold, ezUInt8 uiMinDiffRgb, ezUInt8 uiMaxDiffRgb, ezUInt8 uiMinDiffAlpha, ezUInt8 uiMaxDiffAlpha)
{
  ezStringBuilder& output = out_sHtml;
  output.Append("<!DOCTYPE html PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
                "<!DOCTYPE html PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
                "<HTML> <HEAD>\n");

  output.AppendFormat("<TITLE>{}</TITLE>\n", sTitle);
  output.Append("<script type = \"text/javascript\">\n"
                "function showReferenceImage()\n"
                "{\n"
                "    document.getElementById('image_current_rgb').style.display = 'none'\n"
                "    document.getElementById('image_current_a').style.display = 'none'\n"
                "    document.getElementById('image_reference_rgb').style.display = 'inline-block'\n"
                "    document.getElementById('image_reference_a').style.display = 'inline-block'\n"
                "    document.getElementById('image_caption_rgb').innerHTML = 'Displaying: Reference Image RGB'\n"
                "    document.getElementById('image_caption_a').innerHTML = 'Displaying: Reference Image Alpha'\n"
                "}\n"
                "function showCurrentImage()\n"
                "{\n"
                "    document.getElementById('image_current_rgb').style.display = 'inline-block'\n"
                "    document.getElementById('image_current_a').style.display = 'inline-block'\n"
                "    document.getElementById('image_reference_rgb').style.display = 'none'\n"
                "    document.getElementById('image_reference_a').style.display = 'none'\n"
                "    document.getElementById('image_caption_rgb').innerHTML = 'Displaying: Current Image RGB'\n"
                "    document.getElementById('image_caption_a').innerHTML = 'Displaying: Current Image Alpha'\n"
                "}\n"
                "function imageover()\n"
                "{\n"
                "    var mode = document.querySelector('input[name=\"image_interaction_mode\"]:checked').value\n"
                "    if (mode == 'interactive')\n"
                "    {\n"
                "        showReferenceImage()\n"
                "    }\n"
                "}\n"
                "function imageout()\n"
                "{\n"
                "    var mode = document.querySelector('input[name=\"image_interaction_mode\"]:checked').value\n"
                "    if (mode == 'interactive')\n"
                "    {\n"
                "        showCurrentImage()\n"
                "    }\n"
                "}\n"
                "function handleModeClick(clickedItem)\n"
                "{\n"
                "    if (clickedItem.value == 'current_image' || clickedItem.value == 'interactive')\n"
                "    {\n"
                "        showCurrentImage()\n"
                "    }\n"
                "    else if (clickedItem.value == 'reference_image')\n"
                "    {\n"
                "        showReferenceImage()\n"
                "    }\n"
                "}\n"
                "</script>\n"
                "</HEAD>\n"
                "<BODY bgcolor=\"#ccdddd\">\n"
                "<div style=\"line-height: 1.5; margin-top: 0px; margin-left: 10px; font-family: sans-serif;\">\n");

  output.AppendFormat("<b>Test result for \"{}\" from ", sTitle);
  ezDateTime dateTime = ezDateTime::MakeFromTimestamp(ezTimestamp::CurrentTimestamp());
  output.AppendFormat("{}-{}-{} {}:{}:{}</b><br>\n", dateTime.GetYear(), ezArgI(dateTime.GetMonth(), 2, true), ezArgI(dateTime.GetDay(), 2, true), ezArgI(dateTime.GetHour(), 2, true), ezArgI(dateTime.GetMinute(), 2, true), ezArgI(dateTime.GetSecond(), 2, true));

  output.Append("<table cellpadding=\"0\" cellspacing=\"0\" border=\"0\">\n");

  output.Append("<!-- STATS-TABLE-START -->\n");

  output.AppendFormat("<tr>\n"
                      "<td>Error metric:</td>\n"
                      "<td align=\"right\" style=\"padding-left: 2em;\">{}</td>\n"
                      "</tr>\n",
    uiError);
  output.AppendFormat("<tr>\n"
                      "<td>Error threshold:</td>\n"
                      "<td align=\"right\" style=\"padding-left: 2em;\">{}</td>\n"
                      "</tr>\n",
    uiThreshold);

  output.Append("<!-- STATS-TABLE-END -->\n");

  output.Append("</table>\n"
                "<div style=\"margin-top: 0.5em; margin-bottom: -0.75em\">\n"
                "    <input type=\"radio\" name=\"image_interaction_mode\" onclick=\"handleModeClick(this)\" value=\"interactive\" "
                "checked=\"checked\"> Mouse-Over Image Switching\n"
                "    <input type=\"radio\" name=\"image_interaction_mode\" onclick=\"handleModeClick(this)\" value=\"current_image\"> "
                "Current Image\n"
                "    <input type=\"radio\" name=\"image_interaction_mode\" onclick=\"handleModeClick(this)\" value=\"reference_image\"> "
                "Reference Image\n"
                "</div>\n");

  output.AppendFormat("<div style=\"width:{}px;display: inline-block;\">\n", capturedImgRgb.GetWidth());

  output.Append("<p id=\"image_caption_rgb\">Displaying: Current Image RGB</p>\n"

                "<div style=\"block;\" onmouseover=\"imageover()\" onmouseout=\"imageout()\">\n"
                "<img id=\"image_current_rgb\" alt=\"Captured Image RGB\" src=\"");
  EmbedImageData(output, capturedImgRgb);
  output.Append("\" />\n"
                "<img id=\"image_reference_rgb\" style=\"display: none\" alt=\"Reference Image RGB\" src=\"");
  EmbedImageData(output, referenceImgRgb);
  output.Append("\" />\n"
                "</div>\n"
                "<div style=\"display: block;\">\n");
  output.AppendFormat("<p>RGB Difference (min: {}, max: {}):</p>\n", uiMinDiffRgb, uiMaxDiffRgb);
  output.Append("<img alt=\"Diff Image RGB\" src=\"");
  EmbedImageData(output, diffImgRgb);
  output.Append("\" />\n"
                "</div>\n"
                "</div>\n");

  output.AppendFormat("<div style=\"width:{}px;display: inline-block;\">\n", capturedImgAlpha.GetWidth());

  output.Append("<p id=\"image_caption_a\">Displaying: Current Image Alpha</p>\n"
                "<div style=\"display: block;\" onmouseover=\"imageover()\" onmouseout=\"imageout()\">\n"
                "<img id=\"image_current_a\" alt=\"Captured Image Alpha\" src=\"");
  EmbedImageData(output, capturedImgAlpha);
  output.Append("\" />\n"
                "<img id=\"image_reference_a\" style=\"display: none\" alt=\"Reference Image Alpha\" src=\"");
  EmbedImageData(output, referenceImgAlpha);
  output.Append("\" />\n"
                "</div>\n"
                "<div style=\"px;display: block;\">\n");
  output.AppendFormat("<p>Alpha Difference (min: {}, max: {}):</p>\n", uiMinDiffAlpha, uiMaxDiffAlpha);
  output.Append("<img alt=\"Diff Image Alpha\" src=\"");
  EmbedImageData(output, diffImgAlpha);
  output.Append("\" />\n"
                "</div>\n"
                "</div>\n"
                "</div>\n"
                "</BODY> </HTML>");
}


