#include <PCH.h>

#include <Foundation/Image/ImageUtils.h>

template <typename TYPE>
static void SetDiff(const ezImage& ImageA, const ezImage& ImageB, ezImage& out_Difference, ezUInt32 w, ezUInt32 h, ezUInt32 d, ezUInt32 comp)
{
  const TYPE* pA = ImageA.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  const TYPE* pB = ImageB.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  TYPE* pR = out_Difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  for (ezUInt32 i = 0; i < comp; ++i)
    pR[i] = pB[i] > pA[i] ? (pB[i] - pA[i]) : (pA[i] - pB[i]);
}

template <typename TYPE>
static ezUInt32 GetError(const ezImage& Difference, ezUInt32 w, ezUInt32 h, ezUInt32 d, ezUInt32 comp, ezUInt32 pixel)
{
  const TYPE* pR = Difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  ezUInt32 uiErrorSum = 0;

  for (ezUInt32 p = 0; p < pixel; ++p)
  {
    ezUInt32 error = 0;

    for (ezUInt32 c = 0; c < comp; ++c)
    {
      error += *pR;
      ++pR;
    }

    error /= comp;
    uiErrorSum += error * error;
  }

  return uiErrorSum;
}

void ezImageUtils::ComputeImageDifferenceABS(const ezImage& ImageA, const ezImage& ImageB, ezImage& out_Difference)
{
  EZ_ASSERT_DEV(ImageA.GetWidth() == ImageB.GetWidth(), "Dimensions do not match");
  EZ_ASSERT_DEV(ImageA.GetHeight() == ImageB.GetHeight(), "Dimensions do not match");
  EZ_ASSERT_DEV(ImageA.GetDepth() == ImageB.GetDepth(), "Dimensions do not match");
  EZ_ASSERT_DEV(ImageA.GetImageFormat() == ImageB.GetImageFormat(), "Format does not match");

  out_Difference.SetWidth(ImageA.GetWidth());
  out_Difference.SetHeight(ImageA.GetHeight());
  out_Difference.SetDepth(ImageA.GetDepth());
  out_Difference.SetImageFormat(ImageA.GetImageFormat());
  out_Difference.AllocateImageData();

  const ezUInt32 uiSize2D = ImageA.GetHeight() * ImageA.GetWidth();

  for (ezUInt32 d = 0; d < ImageA.GetDepth(); ++d)
  {
    //for (ezUInt32 h = 0; h < ImageA.GetHeight(); ++h)
    {
      //for (ezUInt32 w = 0; w < ImageA.GetWidth(); ++w)
      {
        switch (ImageA.GetImageFormat())
        {
          case ezImageFormat::R8G8B8A8_UNORM:
          case ezImageFormat::R8G8B8A8_TYPELESS:
          case ezImageFormat::R8G8B8A8_UNORM_SRGB:
          case ezImageFormat::R8G8B8A8_UINT:
          case ezImageFormat::R8G8B8A8_SNORM:
          case ezImageFormat::R8G8B8A8_SINT:
          case ezImageFormat::B8G8R8A8_UNORM:
          case ezImageFormat::B8G8R8X8_UNORM:
          case ezImageFormat::B8G8R8A8_TYPELESS:
          case ezImageFormat::B8G8R8A8_UNORM_SRGB:
          case ezImageFormat::B8G8R8X8_TYPELESS:
          case ezImageFormat::B8G8R8X8_UNORM_SRGB:
            SetDiff<ezUInt32>(ImageA, ImageB, out_Difference, 0, 0, d, uiSize2D);
            break;

          case ezImageFormat::B8G8R8_UNORM:
          {
            SetDiff<ezUInt8>(ImageA, ImageB, out_Difference, 0, 0, d, 3 * uiSize2D);
          }
          break;

          default:
            EZ_REPORT_FAILURE("The ezImageFormat {0} is not implemented", (ezUInt32)ImageA.GetImageFormat());
            return;
        }
      }
    }
  }
}

ezUInt32 ezImageUtils::ComputeMeanSquareError(const ezImage& DifferenceImage, ezUInt8 uiBlockSize, ezUInt32 offsetx, ezUInt32 offsety)
{
  EZ_ASSERT_DEV(uiBlockSize > 1, "Blocksize must be at least 2");

  const ezUInt32 uiWidth = ezMath::Min(DifferenceImage.GetWidth(), offsetx + uiBlockSize) - offsetx;
  const ezUInt32 uiHeight = ezMath::Min(DifferenceImage.GetHeight(), offsety + uiBlockSize) - offsety;

  if (uiWidth == 0 || uiHeight == 0)
    return 0;

  const ezUInt32 uiSize2D = uiWidth * uiHeight;

  ezUInt32 error = 0;

  for (ezUInt32 d = 0; d < DifferenceImage.GetDepth(); ++d)
  {
    for (ezUInt32 y = 0; y < uiHeight; ++y)
    {
      for (ezUInt32 x = 0; x < uiWidth; ++x)
      {
        switch (DifferenceImage.GetImageFormat())
        {
          case ezImageFormat::R8G8B8A8_UNORM:
          case ezImageFormat::R8G8B8A8_TYPELESS:
          case ezImageFormat::R8G8B8A8_UNORM_SRGB:
          case ezImageFormat::R8G8B8A8_UINT:
          case ezImageFormat::R8G8B8A8_SNORM:
          case ezImageFormat::R8G8B8A8_SINT:
          case ezImageFormat::B8G8R8A8_UNORM:
          case ezImageFormat::B8G8R8X8_UNORM:
          case ezImageFormat::B8G8R8A8_TYPELESS:
          case ezImageFormat::B8G8R8A8_UNORM_SRGB:
          case ezImageFormat::B8G8R8X8_TYPELESS:
          case ezImageFormat::B8G8R8X8_UNORM_SRGB:
            error += GetError<ezUInt8>(DifferenceImage, offsetx + x, offsety + y, d, 4, 1);
            break;

          case ezImageFormat::B8G8R8_UNORM:
            error += GetError<ezUInt8>(DifferenceImage, offsetx + x, offsety + y, d, 3, 1);
            break;

          default:
            EZ_REPORT_FAILURE("The ezImageFormat {0} is not implemented", (ezUInt32)DifferenceImage.GetImageFormat());
            return 0;
        }
      }
    }
  }

  error /= uiSize2D;
  return error;
}

ezUInt32 ezImageUtils::ComputeMeanSquareError(const ezImage& DifferenceImage, ezUInt8 uiBlockSize)
{
  EZ_ASSERT_DEV(uiBlockSize > 1, "Blocksize must be at least 2");

  const ezUInt32 uiHalfBlockSize = uiBlockSize / 2;

  const ezUInt32 uiBlocksX = (DifferenceImage.GetWidth() / uiHalfBlockSize) + 1;
  const ezUInt32 uiBlocksY = (DifferenceImage.GetHeight() / uiHalfBlockSize) + 1;

  ezUInt32 uiMaxError = 0;

  for (ezUInt32 by = 0; by < uiBlocksY; ++by)
  {
    for (ezUInt32 bx = 0; bx < uiBlocksX; ++bx)
    {
      const ezUInt32 uiBlockError = ComputeMeanSquareError(DifferenceImage, uiBlockSize, bx * uiHalfBlockSize, by * uiHalfBlockSize);

      uiMaxError = ezMath::Max(uiMaxError, uiBlockError);
    }
  }

  return uiMaxError;
}

void ezImageUtils::CropImage(const ezImage& input, const ezVec2I32& offset, const ezSizeU32& newsize, ezImage& output)
{
  EZ_ASSERT_DEV(offset.x >= 0, "Offset is invalid");
  EZ_ASSERT_DEV(offset.y >= 0, "Offset is invalid");
  EZ_ASSERT_DEV(offset.x < (ezInt32)input.GetWidth(), "Offset is invalid");
  EZ_ASSERT_DEV(offset.y < (ezInt32)input.GetHeight(), "Offset is invalid");

  const ezUInt32 uiNewWidth = ezMath::Min(offset.x + newsize.width, input.GetWidth()) - offset.x;
  const ezUInt32 uiNewHeight = ezMath::Min(offset.y + newsize.height, input.GetHeight()) - offset.y;

  output.SetWidth(uiNewWidth);
  output.SetHeight(uiNewHeight);
  output.SetImageFormat(input.GetImageFormat());
  output.AllocateImageData();

  for (ezUInt32 y = 0; y < uiNewHeight; ++y)
  {
    for (ezUInt32 x = 0; x < uiNewWidth; ++x)
    {
      switch (input.GetImageFormat())
      {
        case ezImageFormat::R8G8B8A8_UNORM:
        case ezImageFormat::R8G8B8A8_TYPELESS:
        case ezImageFormat::R8G8B8A8_UNORM_SRGB:
        case ezImageFormat::R8G8B8A8_UINT:
        case ezImageFormat::R8G8B8A8_SNORM:
        case ezImageFormat::R8G8B8A8_SINT:
        case ezImageFormat::B8G8R8A8_UNORM:
        case ezImageFormat::B8G8R8X8_UNORM:
        case ezImageFormat::B8G8R8A8_TYPELESS:
        case ezImageFormat::B8G8R8A8_UNORM_SRGB:
        case ezImageFormat::B8G8R8X8_TYPELESS:
        case ezImageFormat::B8G8R8X8_UNORM_SRGB:
          output.GetPixelPointer<ezUInt32>(0, 0, 0, x, y)[0] = input.GetPixelPointer<ezUInt32>(0, 0, 0, offset.x + x, offset.y + y)[0];
          break;

        case ezImageFormat::B8G8R8_UNORM:
          output.GetPixelPointer<ezUInt8>(0, 0, 0, x, y)[0] = input.GetPixelPointer<ezUInt32>(0, 0, 0, offset.x + x, offset.y + y)[0];
          output.GetPixelPointer<ezUInt8>(0, 0, 0, x, y)[1] = input.GetPixelPointer<ezUInt32>(0, 0, 0, offset.x + x, offset.y + y)[1];
          output.GetPixelPointer<ezUInt8>(0, 0, 0, x, y)[2] = input.GetPixelPointer<ezUInt32>(0, 0, 0, offset.x + x, offset.y + y)[2];
          break;

        default:
          EZ_REPORT_FAILURE("The ezImageFormat {0} is not implemented", (ezUInt32)input.GetImageFormat());
          return;
      }
    }
  }
}

void ezImageUtils::ScaleDownHalf(const ezImage& Image, ezImage& out_Result)
{
  const ezUInt32 uiNewWidth = Image.GetWidth() / 2;
  const ezUInt32 uiNewHeight = Image.GetHeight() / 2;

  out_Result.SetWidth(uiNewWidth);
  out_Result.SetHeight(uiNewHeight);
  out_Result.SetImageFormat(Image.GetImageFormat());
  out_Result.AllocateImageData();

  const ezUInt32 uiRowPitchImg = Image.GetRowPitch();
  const ezUInt32 uiRowPitchRes = out_Result.GetRowPitch();

  ezUInt32 uiPixelBytes = 0;

  if (Image.GetImageFormat() == ezImageFormat::R8G8B8A8_UNORM ||
      Image.GetImageFormat() == ezImageFormat::R8G8B8A8_TYPELESS ||
      Image.GetImageFormat() == ezImageFormat::R8G8B8A8_UINT ||
      Image.GetImageFormat() == ezImageFormat::R8G8B8A8_SNORM ||
      Image.GetImageFormat() == ezImageFormat::R8G8B8A8_SINT ||
      Image.GetImageFormat() == ezImageFormat::B8G8R8A8_UNORM ||
      Image.GetImageFormat() == ezImageFormat::B8G8R8X8_UNORM ||
      Image.GetImageFormat() == ezImageFormat::B8G8R8A8_TYPELESS ||
      Image.GetImageFormat() == ezImageFormat::B8G8R8X8_TYPELESS)
  {
    uiPixelBytes = 4;
  }
  else if (Image.GetImageFormat() == ezImageFormat::B8G8R8_UNORM)
  {
    uiPixelBytes = 3;
  }

  EZ_ASSERT_DEV(uiPixelBytes > 0, "The image format '{0}' is not supported", Image.GetImageFormat());

  ezUInt8* pDataRes = out_Result.GetPixelPointer<ezUInt8>(0, 0, 0, 0, 0);
  const ezUInt8* pDataImg = Image.GetPixelPointer<ezUInt8>(0, 0, 0, 0, 0);

  for (ezUInt32 y = 0; y < uiNewHeight; ++y)
  {
    for (ezUInt32 x = 0; x < uiNewWidth; ++x)
    {
      ezUInt32 uiPixelImg0 = (uiRowPitchImg * y * 2) + (x * 2) * uiPixelBytes;
      ezUInt32 uiPixelImg1 = (uiRowPitchImg * y * 2) + (x * 2 + 1) * uiPixelBytes;
      ezUInt32 uiPixelImg2 = (uiRowPitchImg * (y * 2 + 1)) + (x * 2 + 1) * uiPixelBytes;
      ezUInt32 uiPixelImg3 = (uiRowPitchImg * (y * 2 + 1)) + (x * 2 + 1) * uiPixelBytes;

      ezUInt32 uiPixelRes = (uiRowPitchRes * y) + x * uiPixelBytes;

      for (ezUInt32 p = 0; p < uiPixelBytes; ++p)
        pDataRes[uiPixelRes + p] = (((ezUInt32)pDataImg[uiPixelImg0 + p] + (ezUInt32)pDataImg[uiPixelImg1 + p] + (ezUInt32)pDataImg[uiPixelImg2 + p] + (ezUInt32)pDataImg[uiPixelImg3 + p]) / 4) & 0xFF;
    }
  }
}

namespace
{
  template <typename T>
  void rotate180(T* start, T* end)
  {
    end = end - 1;
    while (start < end)
    {
      ezMath::Swap(*start, *end);
      start++;
      end--;
    }
  }
}

void ezImageUtils::RotateSubImage180(ezImage& image, ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/)
{
  ezUInt8* start = image.GetPixelPointer<ezUInt8>(uiMipLevel, uiFace, uiArrayIndex);
  ezUInt8* end = start + image.GetDepthPitch(uiMipLevel);

  ezUInt32 bytesPerPixel = ezImageFormat::GetBitsPerPixel(image.GetImageFormat()) / 8;

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

void ezImageUtils::Copy(ezImage& dst, ezUInt32 uiPosX, ezUInt32 uiPosY, const ezImage& src, ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/)
{
  EZ_ASSERT_DEV(dst.GetImageFormat() == src.GetImageFormat(), "Can only copy when the image formats are identical");

  const ezUInt32 uiBitsPerPixel = ezImageFormat::GetBitsPerPixel(src.GetImageFormat());
  const ezUInt32 uiBytesPerPixel = uiBitsPerPixel / 8;
  EZ_ASSERT_DEV(uiBytesPerPixel > 0 && uiBitsPerPixel % 8 == 0, "Only uncompressed formats can be copied");

  const ezUInt32 width = src.GetWidth(uiMipLevel);
  const ezUInt32 height = src.GetHeight(uiMipLevel);

  for (ezUInt32 y = 0; y < height; ++y)
  {
    for (ezUInt32 x = 0; x < width; ++x)
    {
      ezUInt8* pDstData = dst.GetPixelPointer<ezUInt8>(uiMipLevel, uiFace, uiArrayIndex, uiPosX + x, uiPosY + y);
      const ezUInt8* pSrcData = src.GetPixelPointer<ezUInt8>(uiMipLevel, uiFace, uiArrayIndex, x, y);

      ezMemoryUtils::Copy<ezUInt8>(pDstData, pSrcData, uiBytesPerPixel);
    }
  }
}

void ezImageUtils::ScaleDownArbitrary(const ezImage& Image, ezUInt32 uiNewWidth, ezUInt32 uiNewHeight, ezImage& out_Result)
{
  if (uiNewWidth == Image.GetWidth() / 2 && uiNewHeight == Image.GetHeight() / 2)
  {
    ScaleDownHalf(Image, out_Result);
    return;
  }

  out_Result.SetWidth(uiNewWidth);
  out_Result.SetHeight(uiNewHeight);
  out_Result.SetImageFormat(Image.GetImageFormat());
  out_Result.AllocateImageData();

  const ezUInt32 uiRowPitchImg = Image.GetRowPitch();
  const ezUInt32 uiRowPitchRes = out_Result.GetRowPitch();

  ezUInt32 uiPixelBytes = 0;

  if (Image.GetImageFormat() == ezImageFormat::R8G8B8A8_UNORM ||
      Image.GetImageFormat() == ezImageFormat::R8G8B8A8_TYPELESS ||
      Image.GetImageFormat() == ezImageFormat::R8G8B8A8_UINT ||
      Image.GetImageFormat() == ezImageFormat::R8G8B8A8_SNORM ||
      Image.GetImageFormat() == ezImageFormat::R8G8B8A8_SINT ||
      Image.GetImageFormat() == ezImageFormat::B8G8R8A8_UNORM ||
      Image.GetImageFormat() == ezImageFormat::B8G8R8X8_UNORM ||
      Image.GetImageFormat() == ezImageFormat::B8G8R8A8_TYPELESS ||
      Image.GetImageFormat() == ezImageFormat::B8G8R8X8_TYPELESS)
  {
    uiPixelBytes = 4;
  }
  else if (Image.GetImageFormat() == ezImageFormat::B8G8R8_UNORM)
  {
    uiPixelBytes = 3;
  }

  /// \todo support 32 Bit float format

  EZ_ASSERT_DEV(uiPixelBytes > 0, "The image format '{0}' is not supported", Image.GetImageFormat());

  ezUInt8* pDataRes = out_Result.GetPixelPointer<ezUInt8>(0, 0, 0, 0, 0);
  const ezUInt8* pDataImg = Image.GetPixelPointer<ezUInt8>(0, 0, 0, 0, 0);

  const double ratioX = Image.GetWidth() / (double)uiNewWidth;
  const double ratioY = Image.GetHeight() / (double)uiNewHeight;

  for (ezUInt32 y = 0; y < uiNewHeight; ++y)
  {
    for (ezUInt32 x = 0; x < uiNewWidth; ++x)
    {
      const double startX = x * ratioX;
      const double endX = (x + 1) * ratioX;
      const double startY = y * ratioY;
      const double endY = (y + 1) * ratioY;

      double avgValue[4] = {0, 0, 0, 0};
      double numPixels = 0;

      for (ezUInt32 srcY = (ezUInt32)ezMath::Floor(startY); srcY < (ezUInt32)ezMath::Ceil(endY); ++srcY)
      {
        for (ezUInt32 srcX = (ezUInt32)ezMath::Floor(startX); srcX < (ezUInt32)ezMath::Ceil(endX); ++srcX)
        {
          const ezUInt32 uiSrcPixelIdx = uiRowPitchImg * srcY + srcX * uiPixelBytes;

          const double distToEdgeX = ezMath::Min(1.0 + (srcX - startX), (endX - srcX), 1.0);
          const double distToEdgeY = ezMath::Min(1.0 + (srcY - startY), (endY - srcY), 1.0);

          const double pixelCoverage = distToEdgeX * distToEdgeY;
          numPixels += pixelCoverage;

          for (ezUInt32 p = 0; p < uiPixelBytes; ++p)
          {
            avgValue[p] += pDataImg[uiSrcPixelIdx + p] * pixelCoverage;
          }
        }
      }

      avgValue[0] /= numPixels;
      avgValue[1] /= numPixels;
      avgValue[2] /= numPixels;
      avgValue[3] /= numPixels;

      const ezUInt32 uiPixelRes = (uiRowPitchRes * y) + x * uiPixelBytes;

      for (ezUInt32 p = 0; p < uiPixelBytes; ++p)
      {
        pDataRes[uiPixelRes + p] = ((ezInt32)ezMath::Clamp<double>(avgValue[p], 0.0, 255.0)) & 0xFF;
      }
    }
  }
}

ezResult ezImageUtils::ExtractLowerMipChain(const ezImage& src, ezImage& dst, ezUInt8 uiNumMips)
{
  if (src.GetDepth() != 1 || src.GetNumFaces() != 1 || src.GetNumArrayIndices() != 1)
    return EZ_FAILURE;

  uiNumMips = ezMath::Min<ezUInt8>(uiNumMips, src.GetNumMipLevels());
  const ezUInt8 uiMaxMip = src.GetNumMipLevels() - uiNumMips;

  dst.SetImageFormat(src.GetImageFormat());
  dst.SetWidth(src.GetWidth(uiMaxMip));
  dst.SetHeight(src.GetHeight(uiMaxMip));
  dst.SetDepth(1);
  dst.SetNumArrayIndices(1);
  dst.SetNumFaces(1);
  dst.SetNumMipLevels(uiNumMips);
  dst.AllocateImageData();

  const ezUInt8* pStart = src.GetDataPointer<ezUInt8>();

  const ezUInt32 uiStartOffset = src.GetDataOffSet(uiMaxMip);
  const ezUInt32 uiEndOffset = src.GetDataSize();

  ezUInt8* pTarget = dst.GetDataPointer<ezUInt8>();

  ezMemoryUtils::Copy<ezUInt8>(pTarget, pStart + uiStartOffset, (size_t)(uiEndOffset - uiStartOffset));

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Image_Implementation_ImageUtils);
