#include <PCH.h>

#include <Foundation/Image/ImageUtils.h>

#include <Foundation/Image/ImageConversion.h>
#include <Foundation/Image/ImageFilter.h>
#include <Foundation/SimdMath/SimdVec4f.h>

template <typename TYPE>
static void SetDiff(const ezImageView& ImageA, const ezImageView& ImageB, ezImage& out_Difference, ezUInt32 w, ezUInt32 h, ezUInt32 d,
                    ezUInt32 comp)
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

  ezUInt32 uiRowPitch = DifferenceImage.GetRowPitch();
  ezUInt32 uiDepthPitch = DifferenceImage.GetDepthPitch();
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

void ezImageUtils::Copy(ezImage& dst, ezUInt32 uiPosX, ezUInt32 uiPosY, const ezImageView& src, ezUInt32 uiMipLevel /*= 0*/,
                        ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/)
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

ezResult ezImageUtils::ExtractLowerMipChain(const ezImageView& srcImg, ezImage& dstImg, ezUInt32 uiNumMips)
{
  const ezImageHeader& srcImgHeader = srcImg.GetHeader();

  // 3D textures are not supported
  if (srcImgHeader.GetDepth() != 1)
    return EZ_FAILURE;

  // cubemap textures are not supported
  if (srcImgHeader.GetNumFaces() != 1)
    return EZ_FAILURE;

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
  dstImgHeader.SetNumMipLevels(uiNumMips);

  const void* pDataBegin = srcImg.GetPixelPointer<void>(startMipLevel);
  const void* pDataEnd = srcImg.GetArrayPtr<void>().GetEndPtr();
  const ptrdiff_t dataSize = reinterpret_cast<ptrdiff_t>(pDataEnd) - reinterpret_cast<ptrdiff_t>(pDataBegin);

  const ezArrayPtr<const void> lowResData(pDataBegin, static_cast<ezUInt32>(dataSize));

  ezImageView dataview;
  dataview.ResetAndViewExternalStorage(dstImgHeader, lowResData);

  dstImg.ResetAndCopy(dataview);

  return EZ_SUCCESS;
}

static ezSimdVec4f LoadSample(const ezSimdVec4f* source, ezUInt32 numSourceElements, ezUInt32 stride, ezInt32 index,
                              ezImageAddressMode::Enum addressMode, const ezSimdVec4f& borderColor)
{
  bool useBorderColor = false;
  // result is in the range [-(w-1), (w-1)], bring it to [0, w - 1]
  index = ezImageAddressMode::GetSampleIndex(numSourceElements, index, addressMode, useBorderColor);
  if (useBorderColor)
  {
    return borderColor;
  }
  return source[index * stride];
}

inline static void FilterLine(ezUInt32 numSourceElements, const ezSimdVec4f* __restrict sourceBegin, ezSimdVec4f* __restrict targetBegin,
                              ezUInt32 stride, const ezImageFilterWeights& weights, ezArrayPtr<const ezInt32> firstSampleIndices,
                              ezImageAddressMode::Enum addressMode, const ezSimdVec4f& borderColor)
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
        total = ezSimdVec4f::MulAdd(LoadSample(sourceBegin, numSourceElements, stride, sourceIdx, addressMode, borderColor),
                                    ezSimdVec4f(*nextWeightPtr++), total);
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

static void DownScaleFastLine(ezUInt32 pixelStride, const ezUInt8* src, ezUInt8* dest, ezUInt32 lengthIn, ezUInt32 strideIn,
                              ezUInt32 lengthOut, ezUInt32 strideOut)
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
                          intermediate.GetPixelPointer<ezUInt8>(0, face, arrayIndex, 0, row), originalWidth, pixelStride, width,
                          pixelStride);
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

  for (ezUInt32 arrayIndex = 0; arrayIndex < numArrayElements; arrayIndex++)
  {
    for (ezUInt32 face = 0; face < numFaces; face++)
    {
      for (ezUInt32 col = 0; col < width; col++)
      {
        DownScaleFastLine(pixelStride, intermediate.GetPixelPointer<ezUInt8>(0, face, arrayIndex, col),
                          out_Result.GetPixelPointer<ezUInt8>(0, face, arrayIndex, col), originalHeight, intermediate.GetRowPitch(), height,
                          out_Result.GetRowPitch());
      }
    }
  }
}

static float EvaluateAverageCoverage(ezArrayPtr<const ezColor> colors, float alphaThreshold)
{
  ezUInt32 totalPixels = colors.GetCount();
  ezUInt32 count = 0;
  for (ezUInt32 idx = 0; idx < totalPixels; ++idx)
  {
    count += colors[idx].a >= alphaThreshold;
  }

  return float(count) / float(totalPixels);
}

static void NormalizeCoverage(ezArrayPtr<ezColor> colors, float alphaThreshold, float targetCoverage)
{
  // Based on the idea in http://the-witness.net/news/2010/09/computing-alpha-mipmaps/. Note we're using a histogram
  // to find the new alpha threshold here rather than bisecting.

  // Generate histogram of alpha values
  ezUInt32 totalPixels = colors.GetCount();
  ezUInt32 alphaHistogram[256] = {};
  for (ezUInt32 idx = 0; idx < totalPixels; ++idx)
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
  for (ezUInt32 idx = 0; idx < totalPixels; ++idx)
  {
    colors[idx].a *= alphaScale;
  }
}


ezResult ezImageUtils::Scale(const ezImageView& source, ezImage& target, ezUInt32 width, ezUInt32 height, const ezImageFilter* filter,
                             ezImageAddressMode::Enum addressModeU, ezImageAddressMode::Enum addressModeV, const ezColor& borderColor)
{
  return Scale3D(source, target, width, height, 1, filter, addressModeU, addressModeV, ezImageAddressMode::CLAMP, borderColor);
}

ezResult ezImageUtils::Scale3D(const ezImageView& source, ezImage& target, ezUInt32 width, ezUInt32 height, ezUInt32 depth,
                               const ezImageFilter* filter /*= ez_NULL*/,
                               ezImageAddressMode::Enum addressModeU /*= ezImageAddressMode::CLAMP*/,
                               ezImageAddressMode::Enum addressModeV /*= ezImageAddressMode::CLAMP*/,
                               ezImageAddressMode::Enum addressModeW /*= ezImageAddressMode::CLAMP*/,
                               const ezColor& borderColor /*= ezColors::Black*/)
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
    mipMapOptions.m_addressModeU = ezImageAddressMode::CLAMP;
    mipMapOptions.m_addressModeV = ezImageAddressMode::CLAMP;
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

      auto sourceView = source.GetSubImageView(0, face, arrayIndex).GetArrayPtr<void>();
      auto targetView = target.GetSubImageView(0, face, arrayIndex).GetArrayPtr<void>();

      memcpy(targetView.GetPtr(), sourceView.GetPtr(), targetView.GetCount());

      float targetCoverage = 0.0f;
      if (mipMapOptions.m_preserveCoverage)
      {
        targetCoverage =
            EvaluateAverageCoverage(source.GetSubImageView(0, face, arrayIndex).GetArrayPtr<ezColor>(), mipMapOptions.m_alphaThreshold);
      }

      for (ezUInt32 mipMapLevel = 0; mipMapLevel < numMipMaps - 1; mipMapLevel++)
      {
        ezImageHeader nextMipMapHeader = currentMipMapHeader;
        nextMipMapHeader.SetWidth(ezMath::Max(1u, nextMipMapHeader.GetWidth() / 2));
        nextMipMapHeader.SetHeight(ezMath::Max(1u, nextMipMapHeader.GetHeight() / 2));
        nextMipMapHeader.SetDepth(ezMath::Max(1u, nextMipMapHeader.GetDepth() / 2));

        auto sourceData = target.GetSubImageView(mipMapLevel, face, arrayIndex).GetArrayPtr<void>();
        ezImage currentMipMap;
        currentMipMap.ResetAndUseExternalStorage(currentMipMapHeader, sourceData);

        auto dstData = target.GetSubImageView(mipMapLevel + 1, face, arrayIndex).GetArrayPtr<void>();
        ezImage nextMipMap;
        nextMipMap.ResetAndUseExternalStorage(nextMipMapHeader, dstData);

        ezImageUtils::Scale3D(currentMipMap, nextMipMap, nextMipMapHeader.GetWidth(), nextMipMapHeader.GetHeight(),
                              nextMipMapHeader.GetDepth(), mipMapOptions.m_filter, mipMapOptions.m_addressModeU,
                              mipMapOptions.m_addressModeV, mipMapOptions.m_addressModeW, mipMapOptions.m_borderColor);

        if (mipMapOptions.m_preserveCoverage)
        {
          NormalizeCoverage(nextMipMap.GetArrayPtr<ezColor>(), mipMapOptions.m_alphaThreshold, targetCoverage);
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

  ezSimdVec4f* cur = image.GetArrayPtr<ezSimdVec4f>().GetPtr();
  ezSimdVec4f* const end = image.GetArrayPtr<ezSimdVec4f>().GetEndPtr();

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

  ezSimdVec4f* start = image.GetArrayPtr<ezSimdVec4f>().GetPtr();
  ezSimdVec4f* const end = image.GetArrayPtr<ezSimdVec4f>().GetEndPtr();

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
  EZ_ASSERT_DEV(roughnessMap.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT,
                "This algorithm currently expects a RGBA 32 Float as input");
  EZ_ASSERT_DEV(normalMap.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT,
                "This algorithm currently expects a RGBA 32 Float as input");

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
    ezArrayPtr<ezSimdVec4f> roughnessData = roughnessMap.GetSubImageView(mipLevel, 0, 0).GetArrayPtr<ezSimdVec4f>();
    ezArrayPtr<ezSimdVec4f> normalData = filteredNormalMap.GetSubImageView(mipLevel, 0, 0).GetArrayPtr<ezSimdVec4f>();

    for (ezUInt32 i = 0; i < roughnessData.GetCount(); ++i)
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

/// Converts the cube map 'src' with the Direct3D layout into the cube map 'dst' with the internal ezR layout.
static inline void ConvertCubemapLayout(const ezImageView& src, ezImage& dst)
{
  EZ_ASSERT_DEV(1 == src.GetNumArrayIndices() && 1 == src.GetNumMipLevels() && 6 == src.GetNumFaces() && 1 == src.GetDepth(),
                "The source image is expected to be a cube map without MIP maps.");
  EZ_ASSERT_DEV(src.GetWidth() > 0 && src.GetHeight() > 0, "The source image is expected to be of non-zero dimensions.");
  EZ_ASSERT_DEV(src.GetArrayPtr<void*>() != dst.GetArrayPtr<void*>(),
                "The destination image cannot share its data pointer with the source image.");
  EZ_ASSERT_DEV(src.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "The source image is expected to be 4 x 32-bit float RGBA.");

  // Create the destination image.
  dst.ResetAndAlloc(src.GetHeader());
  // Compute the size of a face of the cube map in QWORDs.
  const ezUInt32 size = src.GetWidth();
  const ezUInt32 faceSizeQWords = size * size * 16;
  // Iterate over the faces of the source cube map.
  for (ezUInt32 f = 0; f < 6; ++f)
  {
    // Mapping from the source face indices to the destination face indices.
    static ezUInt32 faceMap[6] = {5, 4, 3, 2, 0, 1};
    switch (f)
    {
      case 2:
      case 3:
        for (ezUInt32 j = 0; j < size; ++j)
        {
          for (ezUInt32 i = 0; i < size; ++i)
          {
            const ezColor* srcPixel = src.GetPixelPointer<ezColor>(0, f, 0, i, j);
            ezColor* dstPixel;
            if (f == 2)
            {
              // Swap the top and the bottom faces, and rotate the face 90 degrees clockwise.
              dstPixel = dst.GetPixelPointer<ezColor>(0, faceMap[f], 0, (size - 1) - j, i);
            }
            else
            {
              // Swap the top and the bottom faces, and rotate the face 90 degrees counter-clockwise.
              dstPixel = dst.GetPixelPointer<ezColor>(0, faceMap[f], 0, j, (size - 1) - i);
            }
            *dstPixel = *srcPixel;
          }
        }
        break;

      default:
        // Rotate side faces 90 degrees clockwise.
        memcpy(dst.GetPixelPointer<float>(0, faceMap[f], 0, 0, 0), src.GetPixelPointer<float>(0, f, 0, 0, 0), faceSizeQWords);
    }
  }
}

// Spherical map (a.k.a. latitude-longitude or cylindrical).
// Parametrized as { phi: theta } in { X: Y }.
static void ConvertSphericalToCubemap(const ezImageView& src, ezImage& dst, ezUInt32 dstDim)
{
  static const float ez_M_2PI = 6.283185307f;  // 2*pi
  static const float ez_M_PI = 3.141592653f;   // pi
  static const float ez_M_PI_2 = 1.570796326f; // pi/2

  // Get the source image's dimensions.
  const ezUInt32 srcHeight = src.GetHeight();
  const ezUInt32 srcWidth = src.GetWidth();
  // Copy and then patch the metadata for the resulting cube map.
  ezImageHeader dstDesc = src.GetHeader();
  dstDesc.SetWidth(dstDim);
  dstDesc.SetHeight(dstDim);
  dstDesc.SetNumFaces(6);
  // Create a temporary image with the Direct3D layout.
  ezImage img;
  img.ResetAndAlloc(dstDesc);
  // Write (gather) the cube map image data.
  for (ezUInt32 f = 0; f < 6; ++f)
  {
    // Compute the cube map faces axes: right, up, forward.
    ezSimdVec4f axes[3];
    // Face mappings per https://msdn.microsoft.com/en-us/library/windows/desktop/bb204881
    switch (f)
    {
      case 0: // +X: right face.
        axes[0].Set(0.f, 0.f, -1.f, 0.0f);
        axes[1].Set(0.f, 1.f, 0.f, 0.0f);
        axes[2].Set(1.f, 0.f, 0.f, 0.0f);
        break;
      case 1: // -X: left face.
        axes[0].Set(0.f, 0.f, 1.f, 0.0f);
        axes[1].Set(0.f, 1.f, 0.f, 0.0f);
        axes[2].Set(-1.f, 0.f, 0.f, 0.0f);
        break;
      case 2: // +Y: top face.
        axes[0].Set(1.f, 0.f, 0.f, 0.0f);
        axes[1].Set(0.f, 0.f, -1.f, 0.0f);
        axes[2].Set(0.f, 1.f, 0.f, 0.0f);
        break;
      case 3: // -Y: bottom face.
        axes[0].Set(1.f, 0.f, 0.f, 0.0f);
        axes[1].Set(0.f, 0.f, 1.f, 0.0f);
        axes[2].Set(0.f, -1.f, 0.f, 0.0f);
        break;
      case 4: // +Z: back face.
        axes[0].Set(1.f, 0.f, 0.f, 0.0f);
        axes[1].Set(0.f, 1.f, 0.f, 0.0f);
        axes[2].Set(0.f, 0.f, 1.f, 0.0f);
        break;
      default: // -Z: front face.
        axes[0].Set(-1.f, 0.f, 0.f, 0.0f);
        axes[1].Set(0.f, 1.f, 0.f, 0.0f);
        axes[2].Set(0.f, 0.f, -1.f, 0.0f);
    }
    // Compute the bottom left corner of the face.
    const ezSimdFloat simdHalf = 0.5f;
    const ezSimdFloat simdDstDim = dstDim;
    const ezSimdVec4f bottomLeft =
        axes[2] * (simdHalf * simdDstDim) - axes[0] * (simdHalf * simdDstDim) - axes[1] * (simdHalf * simdDstDim);
    // Gather the cube map image data.
    for (ezUInt32 j = 0; j < dstDim; ++j)
    {
      for (ezUInt32 i = 0; i < dstDim; ++i)
      {
        // Compute the normalized cube map direction.
        // Apply the texel center offset of 0.5.
        ezSimdVec4f dir = bottomLeft + axes[0] * ezSimdFloat(i + 0.5f) + axes[1] * ezSimdFloat(j + 0.5f);
        dir.Normalize<3>();
        // Represent 'dir' in the spherical coordinates.
        const float cosTheta = dir.GetComponent<1>();
        const float phi = ezMath::ATan2(dir.GetComponent<2>(), dir.GetComponent<0>()).GetRadian();
        // Bias the phi angle into the (-1/2 * Pi, 3/2 * Pi] range.
        const float biasedPhi = (phi > -ez_M_PI_2) ? phi : phi + ez_M_2PI;
        // Convert (phi, theta) into (x, y) coordinates of the source image.
        // Source image parametrization:
        // X: [0, w) <- [3/2 * Pi, -1/2 * Pi)
        // Y: [0, h) <- [cos(0), cos(Pi)) = [1, -1)
        const float u = ((ez_M_PI + ez_M_PI_2) - biasedPhi) * (1.f / ez_M_2PI);
        // For some reason, 'v' has to be mirrored. Perhaps the D3D9 docs are wrong.
        const float v = 1.f - (ezMath::ACos(cosTheta).GetRadian() * (1.f / ez_M_PI));
        // Rescale to account for the resolution of the source image.
        const float x = u * srcWidth - 0.5f;
        const float y = v * srcHeight - 0.5f;
        // Compute sample positions for bilinear filtering.
        const int xL = ezMath::Clamp(static_cast<int>(ezMath::Floor(x)), 0, static_cast<int>(srcWidth - 1));
        const int xH = ezMath::Min<int>(xL + 1, srcWidth - 1);
        const int yL = ezMath::Clamp(static_cast<int>(ezMath::Floor(y)), 0, static_cast<int>(srcHeight - 1));
        const int yH = ezMath::Min<int>(yL + 1, srcHeight - 1);
        // Fetch 4x samples[x][y].
        ezSimdVec4f samples[2][2];
        samples[0][0].Load<4>(src.GetPixelPointer<float>(0, 0, 0, xL, yL));
        samples[1][0].Load<4>(src.GetPixelPointer<float>(0, 0, 0, xH, yL));
        samples[0][1].Load<4>(src.GetPixelPointer<float>(0, 0, 0, xL, yH));
        samples[1][1].Load<4>(src.GetPixelPointer<float>(0, 0, 0, xH, yH));
        // Compute bilinear weights.
        const ezSimdVec4f wX = ezSimdVec4f(x - xL);
        const ezSimdVec4f wY = ezSimdVec4f(y - yL);
        // Perform bilinear filtering of the source image.
        ezSimdVec4f lerpX0, lerpX1, result;
        lerpX0.Lerp(samples[0][0], samples[1][0], wX);
        lerpX1.Lerp(samples[0][1], samples[1][1], wX);
        result.Lerp(lerpX0, lerpX1, wY);
        // Store the results.
        result.Store<4>(img.GetPixelPointer<float>(0, f, 0, i, j));
      }
    }
  }
  // Perform conversion into the internal ezR layout.
  ConvertCubemapLayout(img, dst);
}

void ezImageUtils::ConvertToCubemap(const ezImageView& src, ezImage& dst)
{
  EZ_ASSERT_DEV(1 == src.GetNumArrayIndices() && 1 == src.GetNumMipLevels() && 1 == src.GetNumFaces() && 1 == src.GetDepth(),
                "The source image is expected to be a 2D texture.");
  EZ_ASSERT_DEV(src.GetWidth() > 0 && src.GetHeight() > 0, "The source image is expected to be of non-zero dimensions.");
  EZ_ASSERT_DEV(src.GetArrayPtr<void*>() != dst.GetArrayPtr<void*>(),
                "The destination image cannot share its data pointer with the source image.");
  EZ_ASSERT_DEV(src.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT, "The source image is expected to be 4 x 32-bit float RGBA.");

  const ezUInt32 srcWidth = src.GetWidth();
  const ezUInt32 srcHeight = src.GetHeight();

  ezUInt32 dstDim;
  ezUInt32 tileCountX;
  ezUInt32 tileCountY;

  const auto init = [&](ezUInt32 a_tileCountX, ezUInt32 a_tileCountY) {
    if (srcWidth % a_tileCountX == 0 && srcHeight % a_tileCountY == 0 && srcWidth / a_tileCountX == srcHeight / a_tileCountY)
    {
      dstDim = srcWidth / a_tileCountX;
      tileCountX = a_tileCountX;
      tileCountY = a_tileCountY;
      return true;
    }
    return false;
  };

  struct Transform
  {
    int srcBaseX;
    int srcBaseY;
    int dstX_mulSrcX, dstX_mulSrcY, dstX_add;
    int dstY_mulSrcX, dstY_mulSrcY, dstY_add;
  };
  enum
  {
    kFaceCount = 6
  };
  Transform transform[kFaceCount];

  const auto setTransform = [&](int face, const int srcTileX, const int srcTileY, const int rotation) {
    Transform& t = transform[face];
    t.srcBaseX = (int)dstDim * srcTileX;
    t.srcBaseY = (int)dstDim * srcTileY;
    switch (rotation & 3)
    {
      case 0: // 0 degrees CCW
        t.dstX_mulSrcX = 1;
        t.dstX_mulSrcY = 0;
        t.dstX_add = 0;
        t.dstY_mulSrcX = 0;
        t.dstY_mulSrcY = 1;
        t.dstY_add = 0;
        break;
      case 1: // 90 degrees CCW
        t.dstX_mulSrcX = 0;
        t.dstX_mulSrcY = 1;
        t.dstX_add = 0;
        t.dstY_mulSrcX = -1;
        t.dstY_mulSrcY = 0;
        t.dstY_add = dstDim - 1;
        break;
      case 2: // 180 degrees CCW
        t.dstX_mulSrcX = -1;
        t.dstX_mulSrcY = 0;
        t.dstX_add = dstDim - 1;
        t.dstY_mulSrcX = 0;
        t.dstY_mulSrcY = -1;
        t.dstY_add = dstDim - 1;
        break;
      default: // 270 degrees CCW
        t.dstX_mulSrcX = 0;
        t.dstX_mulSrcY = -1;
        t.dstX_add = dstDim - 1;
        t.dstY_mulSrcX = 1;
        t.dstY_mulSrcY = 0;
        t.dstY_add = 0;
        break;
    }
  };

  // Determine layout of the input image and act based on it.
  // Note that ezR requires LRUDBF order of faces (Left, Right, Up, Down, Backward, Forward).
  if (init(4, 3))
  {
    // Horizontal cross
    //  U
    // LFRB
    //  D
    setTransform(0, 0, 1, 0);
    setTransform(1, 2, 1, 0);
    setTransform(2, 1, 0, 2);
    setTransform(3, 1, 2, 2);
    setTransform(4, 3, 1, 0);
    setTransform(5, 1, 1, 0);
  }
  else if (init(3, 4))
  {
    // Vertical cross
    //  U
    // LFR
    //  D
    //  B
    setTransform(0, 0, 1, 0);
    setTransform(1, 2, 1, 0);
    setTransform(2, 1, 0, 2);
    setTransform(3, 1, 2, 2);
    setTransform(4, 1, 3, 2);
    setTransform(5, 1, 1, 0);
  }
  else if (init(6, 1))
  {
    // Horizontal strip
    // LRUDBF
    setTransform(0, 0, 0, 0);
    setTransform(1, 1, 0, 0);
    setTransform(2, 2, 0, 2);
    setTransform(3, 3, 0, 2);
    setTransform(4, 4, 0, 0);
    setTransform(5, 5, 0, 0);
  }
  else if (init(1, 6))
  {
    // Vertical strip
    // LRUDBF
    setTransform(0, 0, 0, 0);
    setTransform(1, 0, 1, 0);
    setTransform(2, 0, 2, 2);
    setTransform(3, 0, 3, 2);
    setTransform(4, 0, 4, 0);
    setTransform(5, 0, 5, 0);
  }
  else
  {
    // TODO: add support for angular spherical map (a.k.a. mirror ball)

    // Latitude-Longitude
    ConvertSphericalToCubemap(src, dst, srcHeight);
    return;
  }

  /* Uncomment the following code (and use 'pSrc->' instead of 'src.' in the loop
  below) when/if we support producing cubemaps that have different resolution
  with the input image.

  // Rescale the image to match the output cubemap size
  const ezImage* pSrc = &src;
  ezImage tmp;
  if (srcWidth / tileCountX != dstDim)
  {
      const ezImageFilterBox filter;
      const ezResult result = ezImageUtils::scale(
          src, tmp, dstDim * countWidth, dstDim * countHeight, &filter,
          ezImageAddressMode::CLAMP, ezImageAddressMode::CLAMP, ezColor(0, 0, 0));
      EZ_ASSERT_DEV(0xb8252bc, result.isSuccess(), "Unexpected failure in scaling cubemap image");
      pSrc = &tmp;
  }
  */

  EZ_ASSERT_DEV(srcWidth / tileCountX == dstDim, "Unexpected wrong cubemap size.");

  {
    ezImageHeader dstDesc = src.GetHeader();
    dstDesc.SetWidth(dstDim);
    dstDesc.SetHeight(dstDim);
    dstDesc.SetNumFaces(kFaceCount);
    dst.ResetAndAlloc(dstDesc);
  }

  for (int face = 0; face < kFaceCount; ++face)
  {
    const Transform& t = transform[face];
    ezSimdVec4f sample;
    for (int y = 0; y < (int)dstDim; ++y)
    {
      for (int x = 0; x < (int)dstDim; ++x)
      {
        sample.Load<4>(src.GetPixelPointer<float>(0, 0, 0, ezUInt32(t.srcBaseX + x), ezUInt32(t.srcBaseY + y)));
        const auto xx = ezUInt32(t.dstX_mulSrcX * x + t.dstX_mulSrcY * y + t.dstX_add);
        const auto yy = ezUInt32(t.dstY_mulSrcX * x + t.dstY_mulSrcY * y + t.dstY_add);
        sample.Store<4>(dst.GetPixelPointer<float>(0, face, 0, xx, yy));
      }
    }
  }
}


EZ_STATICLINK_FILE(Foundation, Foundation_Image_Implementation_ImageUtils);
