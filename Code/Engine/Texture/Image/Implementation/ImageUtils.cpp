#include <TexturePCH.h>

#include <Texture/Image/ImageUtils.h>

#include <Foundation/SimdMath/SimdVec4f.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/Image/ImageFilter.h>

template <typename TYPE>
static void SetDiff(
  const ezImageView& ImageA, const ezImageView& ImageB, ezImage& out_Difference, ezUInt32 w, ezUInt32 h, ezUInt32 d, ezUInt32 comp)
{
  const TYPE* pA = ImageA.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  const TYPE* pB = ImageB.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  TYPE* pR = out_Difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  for (ezUInt32 i = 0; i < comp; ++i)
    pR[i] = pB[i] > pA[i] ? (pB[i] - pA[i]) : (pA[i] - pB[i]);
}

template <typename TYPE>
static ezUInt32 GetError(const ezImageView& Difference, ezUInt32 w, ezUInt32 h, ezUInt32 d, ezUInt32 comp, ezUInt32 pixel)
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

void ezImageUtils::ComputeImageDifferenceABS(const ezImageView& ImageA, const ezImageView& ImageB, ezImage& out_Difference)
{
  EZ_ASSERT_DEV(ImageA.GetWidth() == ImageB.GetWidth(), "Dimensions do not match");
  EZ_ASSERT_DEV(ImageA.GetHeight() == ImageB.GetHeight(), "Dimensions do not match");
  EZ_ASSERT_DEV(ImageA.GetDepth() == ImageB.GetDepth(), "Dimensions do not match");
  EZ_ASSERT_DEV(ImageA.GetImageFormat() == ImageB.GetImageFormat(), "Format does not match");

  ezImageHeader differenceHeader;

  differenceHeader.SetWidth(ImageA.GetWidth());
  differenceHeader.SetHeight(ImageA.GetHeight());
  differenceHeader.SetDepth(ImageA.GetDepth());
  differenceHeader.SetImageFormat(ImageA.GetImageFormat());
  out_Difference.ResetAndAlloc(differenceHeader);

  const ezUInt32 uiSize2D = ImageA.GetHeight() * ImageA.GetWidth();

  for (ezUInt32 d = 0; d < ImageA.GetDepth(); ++d)
  {
    // for (ezUInt32 h = 0; h < ImageA.GetHeight(); ++h)
    {
      // for (ezUInt32 w = 0; w < ImageA.GetWidth(); ++w)
      {
        switch (ImageA.GetImageFormat())
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
            SetDiff<ezUInt8>(ImageA, ImageB, out_Difference, 0, 0, d, 4 * uiSize2D);
          }
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

ezUInt32 ezImageUtils::ComputeMeanSquareError(const ezImageView& DifferenceImage, ezUInt8 uiBlockSize, ezUInt32 offsetx, ezUInt32 offsety)
{
  EZ_ASSERT_DEV(uiBlockSize > 1, "Blocksize must be at least 2");

  ezUInt32 uiWidth = ezMath::Min(DifferenceImage.GetWidth(), offsetx + uiBlockSize) - offsetx;
  ezUInt32 uiHeight = ezMath::Min(DifferenceImage.GetHeight(), offsety + uiBlockSize) - offsety;

  if (uiWidth == 0 || uiHeight == 0)
    return 0;

  switch (DifferenceImage.GetImageFormat())
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
      EZ_REPORT_FAILURE("The ezImageFormat {0} is not implemented", (ezUInt32)DifferenceImage.GetImageFormat());
      return 0;
  }


  ezUInt32 error = 0;

  ezUInt64 uiRowPitch = DifferenceImage.GetRowPitch();
  ezUInt64 uiDepthPitch = DifferenceImage.GetDepthPitch();
  ezUInt32 uiNumComponents = ezImageFormat::GetNumChannels(DifferenceImage.GetImageFormat());

  // Treat image as single-component format and scale the width instead
  uiWidth *= uiNumComponents;

  const ezUInt32 uiSize2D = uiWidth * uiHeight;
  const ezUInt8* pSlicePointer = DifferenceImage.GetPixelPointer<ezUInt8>(0, 0, 0, offsetx, offsety);

  for (ezUInt32 d = 0; d < DifferenceImage.GetDepth(); ++d)
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

ezUInt32 ezImageUtils::ComputeMeanSquareError(const ezImageView& DifferenceImage, ezUInt8 uiBlockSize)
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

template <typename Func, typename ImageType>
static void ApplyFunc(ImageType& image, Func func)
{
  ezUInt32 uiWidth = image.GetWidth();
  ezUInt32 uiHeight = image.GetHeight();
  ezUInt32 uiDepth = image.GetDepth();

  EZ_ASSERT_DEV(uiWidth > 0 && uiHeight > 0 && uiDepth > 0, "The image passed to FindMinMax has illegal dimension {}x{}x{}.", uiWidth,
    uiHeight, uiDepth);

  ezUInt64 uiRowPitch = image.GetRowPitch();
  ezUInt64 uiDepthPitch = image.GetDepthPitch();
  ezUInt32 uiNumChannels = ezImageFormat::GetNumChannels(image.GetImageFormat());

  auto pSlicePointer = image.template GetPixelPointer<ezUInt8>();

  for (ezUInt32 z = 0; z < image.GetDepth(); ++z)
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

static void FindMinMax(const ezImageView& image, ezUInt8& uiMinRgb, ezUInt8& uiMaxRgb, ezUInt8& uiMinAlpha, ezUInt8& uiMaxAlpha)
{
  ezImageFormat::Enum imageFormat = image.GetImageFormat();
  EZ_ASSERT_DEV(ezImageFormat::GetBitsPerChannel(imageFormat, ezImageFormatChannel::R) == 8 &&
                  ezImageFormat::GetDataType(imageFormat) == ezImageFormatDataType::UNORM,
    "Only 8bpp unorm formats are supported in FindMinMax");

  uiMinRgb = 255u;
  uiMinAlpha = 255u;
  uiMaxRgb = 0u;
  uiMaxAlpha = 0u;

  auto minMax = [&](const ezUInt8* pixel, ezUInt32 /*x*/, ezUInt32 /*y*/, ezUInt32 /*z*/, ezUInt32 c) {
    ezUInt8 val = *pixel;

    if (c < 3)
    {
      uiMinRgb = ezMath::Min(uiMinRgb, val);
      uiMaxRgb = ezMath::Max(uiMaxRgb, val);
    }
    else
    {
      uiMinAlpha = ezMath::Min(uiMinAlpha, val);
      uiMaxAlpha = ezMath::Max(uiMaxAlpha, val);
    }
  };
  ApplyFunc(image, minMax);
}

void ezImageUtils::Normalize(ezImage& image)
{
  ezUInt8 uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha;
  Normalize(image, uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha);
}

void ezImageUtils::Normalize(ezImage& image, ezUInt8& uiMinRgb, ezUInt8& uiMaxRgb, ezUInt8& uiMinAlpha, ezUInt8& uiMaxAlpha)
{
  ezImageFormat::Enum imageFormat = image.GetImageFormat();

  EZ_ASSERT_DEV(ezImageFormat::GetBitsPerChannel(imageFormat, ezImageFormatChannel::R) == 8 &&
                  ezImageFormat::GetDataType(imageFormat) == ezImageFormatDataType::UNORM,
    "Only 8bpp unorm formats are supported in NormalizeImage");

  bool ignoreAlpha = false;
  if (imageFormat == ezImageFormat::B8G8R8X8_UNORM || imageFormat == ezImageFormat::B8G8R8X8_UNORM_SRGB)
  {
    ignoreAlpha = true;
  }

  FindMinMax(image, uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha);
  ezUInt8 uiRangeRgb = uiMaxRgb - uiMinRgb;
  ezUInt8 uiRangeAlpha = uiMaxAlpha - uiMinAlpha;

  auto normalize = [&](ezUInt8* pixel, ezUInt32 /*x*/, ezUInt32 /*y*/, ezUInt32 /*z*/, ezUInt32 c) {
    ezUInt8 val = *pixel;
    if (c < 3)
    {
      // color channels are uniform when min == max, in that case keep original value as scaling is not meaningful
      if (uiRangeRgb != 0)
      {
        *pixel = static_cast<ezUInt8>(255u * (static_cast<float>(val - uiMinRgb) / (uiRangeRgb)));
      }
    }
    else
    {
      // alpha is uniform when minAlpha == maxAlpha, in that case keep original alpha as scaling is not meaningful
      if (!ignoreAlpha && uiRangeAlpha != 0)
      {
        *pixel = static_cast<ezUInt8>(255u * (static_cast<float>(val - uiMinAlpha) / (uiRangeAlpha)));
      }
    }
  };
  ApplyFunc(image, normalize);
}

void ezImageUtils::ExtractAlphaChannel(const ezImageView& inputImage, ezImage& outputImage)
{
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
      EZ_REPORT_FAILURE(
        "ExtractAlpha needs an image with 8bpp and 4 channel. The ezImageFormat {} is not supported.", (ezUInt32)imageFormat);
      return;
  }

  ezImageHeader outputHeader = inputImage.GetHeader();
  outputHeader.SetImageFormat(ezImageFormat::R8_UNORM);
  outputImage.ResetAndAlloc(outputHeader);

  const ezUInt8* pInputSlice = inputImage.GetPixelPointer<ezUInt8>();
  ezUInt8* pOutputSlice = outputImage.GetPixelPointer<ezUInt8>();

  ezUInt64 uiInputRowPitch = inputImage.GetRowPitch();
  ezUInt64 uiInputDepthPitch = inputImage.GetDepthPitch();

  ezUInt64 uiOutputRowPitch = outputImage.GetRowPitch();
  ezUInt64 uiOutputDepthPitch = outputImage.GetDepthPitch();

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

void ezImageUtils::CropImage(const ezImageView& input, const ezVec2I32& offset, const ezSizeU32& newsize, ezImage& output)
{
  EZ_ASSERT_DEV(offset.x >= 0, "Offset is invalid");
  EZ_ASSERT_DEV(offset.y >= 0, "Offset is invalid");
  EZ_ASSERT_DEV(offset.x < (ezInt32)input.GetWidth(), "Offset is invalid");
  EZ_ASSERT_DEV(offset.y < (ezInt32)input.GetHeight(), "Offset is invalid");

  const ezUInt32 uiNewWidth = ezMath::Min(offset.x + newsize.width, input.GetWidth()) - offset.x;
  const ezUInt32 uiNewHeight = ezMath::Min(offset.y + newsize.height, input.GetHeight()) - offset.y;

  ezImageHeader outputHeader;
  outputHeader.SetWidth(uiNewWidth);
  outputHeader.SetHeight(uiNewHeight);
  outputHeader.SetImageFormat(input.GetImageFormat());
  output.ResetAndAlloc(outputHeader);

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
          output.GetPixelPointer<ezUInt32>(0, 0, 0, x, y)[0] = input.GetPixelPointer<ezUInt32>(0, 0, 0, offset.x + x, offset.y + y)[0];
          break;

        case ezImageFormat::B8G8R8_UNORM:
          output.GetPixelPointer<ezUInt8>(0, 0, 0, x, y)[0] = input.GetPixelPointer<ezUInt8>(0, 0, 0, offset.x + x, offset.y + y)[0];
          output.GetPixelPointer<ezUInt8>(0, 0, 0, x, y)[1] = input.GetPixelPointer<ezUInt8>(0, 0, 0, offset.x + x, offset.y + y)[1];
          output.GetPixelPointer<ezUInt8>(0, 0, 0, x, y)[2] = input.GetPixelPointer<ezUInt8>(0, 0, 0, offset.x + x, offset.y + y)[2];
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
} // namespace

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

ezResult ezImageUtils::Copy(const ezImageView& srcImg, const ezRectU32& srcRect, ezImage& dstImg, const ezVec3U32& dstOffset,
  ezUInt32 uiDstMipLevel /*= 0*/, ezUInt32 uiDstFace /*= 0*/, ezUInt32 uiDstArrayIndex /*= 0*/)
{
  if (dstImg.GetImageFormat() != srcImg.GetImageFormat()) // Can only copy when the image formats are identical
    return EZ_FAILURE;

  if (ezImageFormat::IsCompressed(dstImg.GetImageFormat())) // Compressed formats are not supported
    return EZ_FAILURE;

  const ezUInt64 uiDstRowPitch = dstImg.GetRowPitch(uiDstMipLevel);
  const ezUInt64 uiSrcRowPitch = srcImg.GetRowPitch(uiDstMipLevel);
  const ezUInt32 uiCopyBytesPerRow = ezImageFormat::GetBitsPerPixel(srcImg.GetImageFormat()) * srcRect.width / 8;

  ezUInt8* dstPtr = dstImg.GetPixelPointer<ezUInt8>(uiDstMipLevel, uiDstFace, uiDstArrayIndex, dstOffset.x, dstOffset.y, dstOffset.z);
  const ezUInt8* srcPtr = srcImg.GetPixelPointer<ezUInt8>(0, 0, 0, srcRect.x, srcRect.y);

  for (ezUInt32 y = 0; y < srcRect.height; y++)
  {
    ezMemoryUtils::Copy(dstPtr, srcPtr, uiCopyBytesPerRow);

    dstPtr += uiDstRowPitch;
    srcPtr += uiSrcRowPitch;
  }

  return EZ_SUCCESS;
}

ezResult ezImageUtils::ExtractLowerMipChain(const ezImageView& srcImg, ezImage& dstImg, ezUInt32 uiNumMips)
{
  const ezImageHeader& srcImgHeader = srcImg.GetHeader();

  // only power-of-two resolutions are supported atm
  if (!ezMath::IsPowerOf2(srcImgHeader.GetWidth()) || !ezMath::IsPowerOf2(srcImgHeader.GetHeight()))
    return EZ_FAILURE;

  uiNumMips = ezMath::Min(uiNumMips, srcImgHeader.GetNumMipLevels());

  ezUInt32 startMipLevel = srcImgHeader.GetNumMipLevels() - uiNumMips;

  // block compressed image formats require resolutions that are divisible by 4
  // therefore, adjust startMipLevel accordingly
  while (srcImgHeader.GetWidth(startMipLevel) % 4 != 0 || srcImgHeader.GetHeight(startMipLevel) % 4 != 0)
  {
    if (uiNumMips >= srcImgHeader.GetNumMipLevels())
      return EZ_FAILURE;

    if (startMipLevel == 0)
      return EZ_FAILURE;

    ++uiNumMips;
    --startMipLevel;
  }

  ezImageHeader dstImgHeader = srcImgHeader;
  dstImgHeader.SetWidth(srcImgHeader.GetWidth(startMipLevel));
  dstImgHeader.SetHeight(srcImgHeader.GetHeight(startMipLevel));
  dstImgHeader.SetDepth(srcImgHeader.GetDepth(startMipLevel));
  dstImgHeader.SetNumFaces(srcImgHeader.GetNumFaces());
  dstImgHeader.SetNumArrayIndices(srcImgHeader.GetNumArrayIndices());
  dstImgHeader.SetNumMipLevels(uiNumMips);

  const void* pDataBegin = srcImg.GetPixelPointer<void>(startMipLevel);
  const void* pDataEnd = srcImg.GetBlobPtr<void>().GetEndPtr();
  const ptrdiff_t dataSize = reinterpret_cast<ptrdiff_t>(pDataEnd) - reinterpret_cast<ptrdiff_t>(pDataBegin);

  const ezBlobPtr<const void> lowResData(pDataBegin, static_cast<ezUInt64>(dataSize));

  ezImageView dataview;
  dataview.ResetAndViewExternalStorage(dstImgHeader, lowResData);

  dstImg.ResetAndCopy(dataview);

  return EZ_SUCCESS;
}



ezUInt32 ezImageUtils::GetSampleIndex(ezUInt32 numTexels, ezInt32 index, ezImageAddressMode::Enum addressMode, bool& outUseBorderColor)
{
  outUseBorderColor = false;
  if (ezUInt32(index) >= numTexels)
  {
    switch (addressMode)
    {
      case ezImageAddressMode::Repeat:
        index %= numTexels;

        if (index < 0)
        {
          index += numTexels;
        }
        return index;

      case ezImageAddressMode::Mirror:
      {
        if (index < 0)
        {
          index = -index - 1;
        }
        bool flip = (index / numTexels) & 1;
        index %= numTexels;
        if (flip)
        {
          index = numTexels - index - 1;
        }
        return index;
      }

      case ezImageAddressMode::Clamp:
        return ezMath::Clamp<ezInt32>(index, 0, numTexels - 1);

      case ezImageAddressMode::ClampBorder:
        outUseBorderColor = true;
        return 0;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED
        return 0;
    }
  }
  return index;
}

static ezSimdVec4f LoadSample(const ezSimdVec4f* source, ezUInt32 numSourceElements, ezUInt32 stride, ezInt32 index,
  ezImageAddressMode::Enum addressMode, const ezSimdVec4f& borderColor)
{
  bool useBorderColor = false;
  // result is in the range [-(w-1), (w-1)], bring it to [0, w - 1]
  index = ezImageUtils::GetSampleIndex(numSourceElements, index, addressMode, useBorderColor);
  if (useBorderColor)
  {
    return borderColor;
  }
  return source[index * stride];
}

inline static void FilterLine(ezUInt32 numSourceElements, const ezSimdVec4f* __restrict sourceBegin, ezSimdVec4f* __restrict targetBegin,
  ezUInt32 stride, const ezImageFilterWeights& weights, ezArrayPtr<const ezInt32> firstSampleIndices, ezImageAddressMode::Enum addressMode,
  const ezSimdVec4f& borderColor)
{
  // Convolve the image using the precomputed weights
  const ezUInt32 numWeights = weights.GetNumWeights();

  // When the first source index for the output is between 0 and this value,
  // we can fetch all numWeights inputs without taking addressMode into consideration,
  // which makes the inner loop a lot faster.
  const ezInt32 trivialSourceIndicesEnd = static_cast<ezInt32>(numSourceElements) - static_cast<ezInt32>(numWeights);
  const auto weightsView = weights.ViewWeights();
  const float* __restrict nextWeightPtr = weightsView.GetPtr();
  EZ_ASSERT_DEBUG((static_cast<ezUInt32>(weightsView.GetCount()) % numWeights) == 0, "");
  for (ezInt32 firstSourceIdx : firstSampleIndices)
  {
    ezSimdVec4f total(0.0f, 0.0f, 0.0f, 0.0f);

    if (firstSourceIdx >= 0 && firstSourceIdx < trivialSourceIndicesEnd)
    {
      const auto* __restrict sourcePtr = sourceBegin + firstSourceIdx * stride;
      for (ezUInt32 weightIdx = 0; weightIdx < numWeights; ++weightIdx)
      {
        total = ezSimdVec4f::MulAdd(*sourcePtr, ezSimdVec4f(*nextWeightPtr++), total);
        sourcePtr += stride;
      }
    }
    else
    {
      // Very slow fallback case that respects the addressMode
      // (not a lot of pixels are taking this path, so it's probably fine)
      ezInt32 sourceIdx = firstSourceIdx;
      for (ezUInt32 weightIdx = 0; weightIdx < numWeights; ++weightIdx)
      {
        total = ezSimdVec4f::MulAdd(
          LoadSample(sourceBegin, numSourceElements, stride, sourceIdx, addressMode, borderColor), ezSimdVec4f(*nextWeightPtr++), total);
        sourceIdx++;
      }
    }
    // It's ok to check this once per source index, see the assert above
    // (number of weights in weightsView is divisible by numWeights)
    if (nextWeightPtr == weightsView.GetEndPtr())
    {
      nextWeightPtr = weightsView.GetPtr();
    }
    *targetBegin = total;
    targetBegin += stride;
  }
}

static void DownScaleFastLine(
  ezUInt32 pixelStride, const ezUInt8* src, ezUInt8* dest, ezUInt32 lengthIn, ezUInt32 strideIn, ezUInt32 lengthOut, ezUInt32 strideOut)
{
  const ezUInt32 downScaleFactor = lengthIn / lengthOut;

  const ezUInt32 downScaleFactorLog2 = ezMath::Log2i(static_cast<ezUInt32>(downScaleFactor));
  const ezUInt32 roundOffset = downScaleFactor / 2;

  for (ezUInt32 offset = 0; offset < lengthOut; ++offset)
  {
    for (ezUInt32 channel = 0; channel < pixelStride; ++channel)
    {
      const ezUInt32 destOffset = offset * strideOut + channel;

      ezUInt32 curChannel = roundOffset;
      for (ezUInt32 index = 0; index < downScaleFactor; ++index)
      {
        curChannel += static_cast<ezUInt32>(src[channel + index * strideIn]);
      }

      curChannel = curChannel >> downScaleFactorLog2;
      dest[destOffset] = static_cast<ezUInt8>(curChannel);
    }

    src += downScaleFactor * strideIn;
  }
}

static void DownScaleFast(const ezImageView& image, ezImage& out_Result, ezUInt32 width, ezUInt32 height)
{
  ezImageFormat::Enum format = image.GetImageFormat();

  ezUInt32 originalWidth = image.GetWidth();
  ezUInt32 originalHeight = image.GetHeight();
  ezUInt32 numArrayElements = image.GetNumArrayIndices();
  ezUInt32 numFaces = image.GetNumFaces();

  ezUInt32 pixelStride = ezImageFormat::GetBitsPerPixel(format) / 8;

  ezImageHeader intermediateHeader;
  intermediateHeader.SetWidth(width);
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
        DownScaleFastLine(pixelStride, image.GetPixelPointer<ezUInt8>(0, face, arrayIndex, 0, row),
          intermediate.GetPixelPointer<ezUInt8>(0, face, arrayIndex, 0, row), originalWidth, pixelStride, width, pixelStride);
      }
    }
  }

  // input and output images may be the same, so we can't access the original image below this point

  ezImageHeader outHeader;
  outHeader.SetWidth(width);
  outHeader.SetHeight(height);
  outHeader.SetNumArrayIndices(numArrayElements);
  outHeader.SetNumArrayIndices(numFaces);
  outHeader.SetImageFormat(format);

  out_Result.ResetAndAlloc(outHeader);

  EZ_ASSERT_DEBUG(intermediate.GetRowPitch() < ezMath::BasicType<ezUInt32>::MaxValue(), "Row pitch exceeds ezUInt32 max value.");
  EZ_ASSERT_DEBUG(out_Result.GetRowPitch() < ezMath::BasicType<ezUInt32>::MaxValue(), "Row pitch exceeds ezUInt32 max value.");

  for (ezUInt32 arrayIndex = 0; arrayIndex < numArrayElements; arrayIndex++)
  {
    for (ezUInt32 face = 0; face < numFaces; face++)
    {
      for (ezUInt32 col = 0; col < width; col++)
      {
        DownScaleFastLine(pixelStride, intermediate.GetPixelPointer<ezUInt8>(0, face, arrayIndex, col),
          out_Result.GetPixelPointer<ezUInt8>(0, face, arrayIndex, col), originalHeight, static_cast<ezUInt32>(intermediate.GetRowPitch()), height,
          static_cast<ezUInt32>(out_Result.GetRowPitch()));
      }
    }
  }
}

static float EvaluateAverageCoverage(ezBlobPtr<const ezColor> colors, float alphaThreshold)
{
  ezUInt64 totalPixels = colors.GetCount();
  ezUInt64 count = 0;
  for (ezUInt32 idx = 0; idx < totalPixels; ++idx)
  {
    count += colors[idx].a >= alphaThreshold;
  }

  return float(count) / float(totalPixels);
}

static void NormalizeCoverage(ezBlobPtr<ezColor> colors, float alphaThreshold, float targetCoverage)
{
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
  ezInt32 targetCount = ezInt32(targetCoverage * totalPixels);
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

  ezInt32 currentThreshold = ezMath::ColorFloatToByte(alphaThreshold);

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
  float alphaScale = alphaThreshold / (newThreshold / 255.0f);
  for (ezUInt64 idx = 0; idx < totalPixels; ++idx)
  {
    colors[idx].a *= alphaScale;
  }
}


ezResult ezImageUtils::Scale(const ezImageView& source, ezImage& target, ezUInt32 width, ezUInt32 height, const ezImageFilter* filter,
  ezImageAddressMode::Enum addressModeU, ezImageAddressMode::Enum addressModeV, const ezColor& borderColor)
{
  return Scale3D(source, target, width, height, 1, filter, addressModeU, addressModeV, ezImageAddressMode::Clamp, borderColor);
}

ezResult ezImageUtils::Scale3D(const ezImageView& source, ezImage& target, ezUInt32 width, ezUInt32 height, ezUInt32 depth,
  const ezImageFilter* filter /*= ez_NULL*/, ezImageAddressMode::Enum addressModeU /*= ezImageAddressMode::Clamp*/,
  ezImageAddressMode::Enum addressModeV /*= ezImageAddressMode::Clamp*/,
  ezImageAddressMode::Enum addressModeW /*= ezImageAddressMode::Clamp*/, const ezColor& borderColor /*= ezColors::Black*/)
{
  if (width == 0 || height == 0 || depth == 0)
  {
    ezImageHeader header;
    header.SetImageFormat(source.GetImageFormat());
    target.ResetAndAlloc(header);
    return EZ_SUCCESS;
  }

  const ezImageFormat::Enum format = source.GetImageFormat();

  const ezUInt32 originalWidth = source.GetWidth();
  const ezUInt32 originalHeight = source.GetHeight();
  const ezUInt32 originalDepth = source.GetDepth();
  const ezUInt32 numFaces = source.GetNumFaces();
  const ezUInt32 numArrayElements = source.GetNumArrayIndices();

  if (originalWidth == width && originalHeight == height && originalDepth == depth)
  {
    target.ResetAndCopy(source);
    return EZ_SUCCESS;
  }

  // Scaling down by an even factor?
  const ezUInt32 downScaleFactorX = originalWidth / width;
  const ezUInt32 downScaleFactorY = originalHeight / height;

  if (filter == nullptr &&
      (format == ezImageFormat::R8G8B8A8_UNORM || format == ezImageFormat::B8G8R8A8_UNORM || format == ezImageFormat::B8G8R8_UNORM) &&
      downScaleFactorX * width == originalWidth && downScaleFactorY * height == originalHeight && depth == 1 && originalDepth == 1 &&
      ezMath::IsPowerOf2(downScaleFactorX) && ezMath::IsPowerOf2(downScaleFactorY))
  {
    DownScaleFast(source, target, width, height);
    return EZ_SUCCESS;
  }

  // Fallback to default filter
  ezImageFilterTriangle defaultFilter;
  if (!filter)
  {
    filter = &defaultFilter;
  }

  const ezImageView* stepSource;

  // Manage scratch images for intermediate conversion or filtering
  const ezUInt32 maxNumScratchImages = 2;
  ezImage scratch[maxNumScratchImages];
  bool scratchUsed[maxNumScratchImages] = {};
  auto allocateScratch = [&]() -> ezImage& {
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
  auto releaseScratch = [&](const ezImageView& image) {
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
  firstSampleIndices.Reserve(ezMath::Max(width, height, depth));

  if (width != originalWidth)
  {
    ezImageFilterWeights weights(*filter, originalWidth, width);
    firstSampleIndices.SetCountUninitialized(width);
    for (ezUInt32 x = 0; x < width; ++x)
    {
      firstSampleIndices[x] = weights.GetFirstSourceSampleIndex(x);
    }

    ezImage* stepTarget;
    if (height == originalHeight && depth == originalDepth && format == ezImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    ezImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetWidth(width);
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
            FilterLine(originalWidth, filterSource, filterTarget, 1, weights, firstSampleIndices, addressModeU,
              ezSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  if (height != originalHeight)
  {
    ezImageFilterWeights weights(*filter, originalHeight, height);
    firstSampleIndices.SetCount(height);
    for (ezUInt32 y = 0; y < height; ++y)
    {
      firstSampleIndices[y] = weights.GetFirstSourceSampleIndex(y);
    }

    ezImage* stepTarget;
    if (depth == originalDepth && format == ezImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    ezImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetHeight(height);
    stepTarget->ResetAndAlloc(stepHeader);

    for (ezUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (ezUInt32 face = 0; face < numFaces; ++face)
      {
        for (ezUInt32 z = 0; z < originalDepth; ++z)
        {
          for (ezUInt32 x = 0; x < width; ++x)
          {
            const ezSimdVec4f* filterSource = stepSource->GetPixelPointer<ezSimdVec4f>(0, face, arrayIndex, x, 0, z);
            ezSimdVec4f* filterTarget = stepTarget->GetPixelPointer<ezSimdVec4f>(0, face, arrayIndex, x, 0, z);
            FilterLine(originalHeight, filterSource, filterTarget, width, weights, firstSampleIndices, addressModeV,
              ezSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  if (depth != originalDepth)
  {
    ezImageFilterWeights weights(*filter, originalDepth, depth);
    firstSampleIndices.SetCount(depth);
    for (ezUInt32 z = 0; z < depth; ++z)
    {
      firstSampleIndices[z] = weights.GetFirstSourceSampleIndex(z);
    }

    ezImage* stepTarget;
    if (format == ezImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    ezImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetDepth(depth);
    stepTarget->ResetAndAlloc(stepHeader);

    for (ezUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (ezUInt32 face = 0; face < numFaces; ++face)
      {
        for (ezUInt32 y = 0; y < height; ++y)
        {
          for (ezUInt32 x = 0; x < width; ++x)
          {
            const ezSimdVec4f* filterSource = stepSource->GetPixelPointer<ezSimdVec4f>(0, face, arrayIndex, x, y, 0);
            ezSimdVec4f* filterTarget = stepTarget->GetPixelPointer<ezSimdVec4f>(0, face, arrayIndex, x, y, 0);
            FilterLine(originalHeight, filterSource, filterTarget, width * height, weights, firstSampleIndices, addressModeW,
              ezSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  // Convert back to original format - no-op if stepSource and target are the same
  return ezImageConversion::Convert(*stepSource, target, format);
}

void ezImageUtils::GenerateMipMaps(const ezImageView& source, ezImage& target, const MipMapOptions& options)
{
  ezImageHeader header = source.GetHeader();
  EZ_ASSERT_DEV(header.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "The source image must be a RGBA 32-bit float format.");
  EZ_ASSERT_DEV(&source != &target, "Source and target must not be the same image.");

  // Make a local copy to be able to tweak some of the options
  ezImageUtils::MipMapOptions mipMapOptions = options;

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

  target.ResetAndAlloc(header);

  for (ezUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (ezUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      ezImageHeader currentMipMapHeader = header;
      currentMipMapHeader.SetNumMipLevels(1);
      currentMipMapHeader.SetNumFaces(1);
      currentMipMapHeader.SetNumArrayIndices(1);

      auto sourceView = source.GetSubImageView(0, face, arrayIndex).GetBlobPtr<void>();
      auto targetView = target.GetSubImageView(0, face, arrayIndex).GetBlobPtr<ezUInt8>();

      memcpy(targetView.GetPtr(), sourceView.GetPtr(), targetView.GetCount());

      float targetCoverage = 0.0f;
      if (mipMapOptions.m_preserveCoverage)
      {
        targetCoverage =
          EvaluateAverageCoverage(source.GetSubImageView(0, face, arrayIndex).GetBlobPtr<ezColor>(), mipMapOptions.m_alphaThreshold);
      }

      for (ezUInt32 mipMapLevel = 0; mipMapLevel < numMipMaps - 1; mipMapLevel++)
      {
        ezImageHeader nextMipMapHeader = currentMipMapHeader;
        nextMipMapHeader.SetWidth(ezMath::Max(1u, nextMipMapHeader.GetWidth() / 2));
        nextMipMapHeader.SetHeight(ezMath::Max(1u, nextMipMapHeader.GetHeight() / 2));
        nextMipMapHeader.SetDepth(ezMath::Max(1u, nextMipMapHeader.GetDepth() / 2));

        auto sourceData = target.GetSubImageView(mipMapLevel, face, arrayIndex).GetBlobPtr<void>();
        ezImage currentMipMap;
        currentMipMap.ResetAndUseExternalStorage(currentMipMapHeader, sourceData);

        auto dstData = target.GetSubImageView(mipMapLevel + 1, face, arrayIndex).GetBlobPtr<void>();
        ezImage nextMipMap;
        nextMipMap.ResetAndUseExternalStorage(nextMipMapHeader, dstData);

        ezImageUtils::Scale3D(currentMipMap, nextMipMap, nextMipMapHeader.GetWidth(), nextMipMapHeader.GetHeight(),
          nextMipMapHeader.GetDepth(), mipMapOptions.m_filter, mipMapOptions.m_addressModeU, mipMapOptions.m_addressModeV,
          mipMapOptions.m_addressModeW, mipMapOptions.m_borderColor);

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

void ezImageUtils::ReconstructNormalZ(ezImage& image)
{
  EZ_ASSERT_DEV(image.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  ezSimdVec4f* cur = image.GetBlobPtr<ezSimdVec4f>().GetPtr();
  ezSimdVec4f* const end = image.GetBlobPtr<ezSimdVec4f>().GetEndPtr();

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

void ezImageUtils::RenormalizeNormalMap(ezImage& image)
{
  EZ_ASSERT_DEV(image.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  ezSimdVec4f* start = image.GetBlobPtr<ezSimdVec4f>().GetPtr();
  ezSimdVec4f* const end = image.GetBlobPtr<ezSimdVec4f>().GetEndPtr();

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

void ezImageUtils::AdjustRoughness(ezImage& roughnessMap, const ezImageView& normalMap)
{
  EZ_ASSERT_DEV(
    roughnessMap.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");
  EZ_ASSERT_DEV(
    normalMap.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  EZ_ASSERT_DEV(roughnessMap.GetWidth() >= normalMap.GetWidth() && roughnessMap.GetHeight() >= normalMap.GetHeight(),
    "The roughness map needs to be bigger or same size than the normal map.");

  ezImage filteredNormalMap;
  ezImageUtils::MipMapOptions options;

  // Box filter normal map without re-normalization so we have the average normal length in each mip map.
  if (roughnessMap.GetWidth() != normalMap.GetWidth() || roughnessMap.GetHeight() != normalMap.GetHeight())
  {
    ezImage temp;
    ezImageUtils::Scale(normalMap, temp, roughnessMap.GetWidth(), roughnessMap.GetHeight());
    ezImageUtils::RenormalizeNormalMap(temp);
    ezImageUtils::GenerateMipMaps(temp, filteredNormalMap, options);
  }
  else
  {
    ezImageUtils::GenerateMipMaps(normalMap, filteredNormalMap, options);
  }

  EZ_ASSERT_DEV(roughnessMap.GetNumMipLevels() == filteredNormalMap.GetNumMipLevels(),
    "Roughness and normal map must have the same number of mip maps");

  ezSimdVec4f two(2.0f);
  ezSimdVec4f minusOne(-1.0f);

  ezUInt32 numMipLevels = roughnessMap.GetNumMipLevels();
  for (ezUInt32 mipLevel = 1; mipLevel < numMipLevels; ++mipLevel)
  {
    ezBlobPtr<ezSimdVec4f> roughnessData = roughnessMap.GetSubImageView(mipLevel, 0, 0).GetBlobPtr<ezSimdVec4f>();
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

void ezImageUtils::ChangeExposure(ezImage& image, float bias)
{
  EZ_ASSERT_DEV(image.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "This function expects an RGBA 32 float image as input");

  if (bias == 0.0f)
    return;

  const float multiplier = ezMath::Pow2(bias);

  for (ezColor& col : image.GetBlobPtr<ezColor>())
  {
    col = multiplier * col;
  }
}

static void CopyImageRectToFace(ezImage& dstImg, const ezImageView& srcImg, ezUInt32 offsetX, ezUInt32 offsetY, ezUInt32 faceIndex)
{
  ezRectU32 r;
  r.x = offsetX;
  r.y = offsetY;
  r.width = dstImg.GetWidth();
  r.height = r.width;

  ezImageUtils::Copy(srcImg, r, dstImg, ezVec3U32(0), 0, faceIndex);
}

ezResult ezImageUtils::CreateCubemapFromSingleFile(ezImage& dstImg, const ezImageView& srcImg)
{
  if (srcImg.GetNumFaces() == 6)
  {
    dstImg.ResetAndCopy(srcImg);
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

      dstImg.ResetAndAlloc(imgHeader);

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      CopyImageRectToFace(dstImg, srcImg, faceSize * 2, faceSize, 0);

      // Negative X face
      CopyImageRectToFace(dstImg, srcImg, 0, faceSize, 1);

      // Positive Y face
      CopyImageRectToFace(dstImg, srcImg, faceSize, 0, 2);

      // Negative Y face
      CopyImageRectToFace(dstImg, srcImg, faceSize, faceSize * 2, 3);

      // Positive Z face
      CopyImageRectToFace(dstImg, srcImg, faceSize, faceSize, 4);

      // Negative Z face
      CopyImageRectToFace(dstImg, srcImg, faceSize, faceSize * 3, 5);
      ezImageUtils::RotateSubImage180(dstImg, 0, 5);
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

      dstImg.ResetAndAlloc(imgHeader);

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      CopyImageRectToFace(dstImg, srcImg, faceSize * 2, faceSize, 0);

      // Negative X face
      CopyImageRectToFace(dstImg, srcImg, 0, faceSize, 1);

      // Positive Y face
      CopyImageRectToFace(dstImg, srcImg, faceSize, 0, 2);

      // Negative Y face
      CopyImageRectToFace(dstImg, srcImg, faceSize, faceSize * 2, 3);

      // Positive Z face
      CopyImageRectToFace(dstImg, srcImg, faceSize, faceSize, 4);

      // Negative Z face
      CopyImageRectToFace(dstImg, srcImg, faceSize * 3, faceSize, 5);
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

      dstImg.ResetAndAlloc(imgHeader);

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

      EZ_ASSERT_DEBUG(dstImg.GetRowPitch() % sizeof(ezColor) == 0, "Row pitch should be a multiple of sizeof(ezColor)");
      const ezUInt64 faceRowPitch = dstImg.GetRowPitch() / sizeof(ezColor);

      const ezColor* srcData = srcImg.GetPixelPointer<ezColor>();
      const float InvPi = 1.0f / ezMath::BasicType<float>::Pi();

      for (ezUInt32 faceIndex = 0; faceIndex < 6; faceIndex++)
      {
        ezColor* faceData = dstImg.GetPixelPointer<ezColor>(0, faceIndex);
        for (ezUInt32 y = 0; y < faceSize; y++)
        {
          const float dstV = (float)y * fPixel + fHalfPixel;

          for (ezUInt32 x = 0; x < faceSize; x++)
          {
            const float dstU = (float)x * fPixel + fHalfPixel;
            const ezVec3 modelSpacePos = faceCorners[faceIndex] + dstU * faceAxis[faceIndex * 2] + dstV * faceAxis[faceIndex * 2 + 1];
            const ezVec3 modelSpaceDir = modelSpacePos.GetNormalized();

            const float phi = ezMath::ATan2(modelSpaceDir.x, modelSpaceDir.z).GetRadian() + ezMath::BasicType<float>::Pi();
            const float r = ezMath::Sqrt(modelSpaceDir.x * modelSpaceDir.x + modelSpaceDir.z * modelSpaceDir.z);
            const float theta = ezMath::ATan2(modelSpaceDir.y, r).GetRadian() + ezMath::BasicType<float>::Pi() * 0.5f;

            EZ_ASSERT_DEBUG(phi >= 0.0f && phi <= 2.0f * ezMath::BasicType<float>::Pi(), "");
            EZ_ASSERT_DEBUG(theta >= 0.0f && theta <= ezMath::BasicType<float>::Pi(), "");

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

ezResult ezImageUtils::CreateCubemapFrom6Files(ezImage& dstImg, const ezImageView* pSourceImages)
{
  ezImageHeader header = pSourceImages[0].GetHeader();
  header.SetNumFaces(6);

  if (header.GetWidth() != header.GetHeight())
    return EZ_FAILURE;

  if (!ezMath::IsPowerOf2(header.GetWidth()))
    return EZ_FAILURE;

  dstImg.ResetAndAlloc(header);

  for (ezUInt32 i = 0; i < 6; ++i)
  {
    if (pSourceImages[i].GetImageFormat() != dstImg.GetImageFormat())
      return EZ_FAILURE;

    if (pSourceImages[i].GetWidth() != dstImg.GetWidth())
      return EZ_FAILURE;

    if (pSourceImages[i].GetHeight() != dstImg.GetHeight())
      return EZ_FAILURE;

    CopyImageRectToFace(dstImg, pSourceImages[i], 0, 0, i);
  }

  return EZ_SUCCESS;
}

ezResult ezImageUtils::CreateVolumeTextureFromSingleFile(ezImage& dstImg, const ezImageView& srcImg)
{
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

  dstImg.ResetAndAlloc(header);

  const ezImageView view = srcImg.GetSubImageView();

  for (ezUInt32 d = 0; d < uiDepth; ++d)
  {
    ezRectU32 r;
    r.x = uiWidthHeight * d;
    r.y = 0;
    r.width = uiWidthHeight;
    r.height = uiWidthHeight;

    EZ_SUCCEED_OR_RETURN(Copy(view, r, dstImg, ezVec3U32(0, 0, d)));
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageUtils);
