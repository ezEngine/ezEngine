template<typename Impl>
struct ezImageConversionMixinBase
{
  static void ConvertImage(const ezImage& source, ezImage& target)
  {
    EZ_ASSERT(
      ezImageFormat::GetBitsPerPixel(source.GetImageFormat()) == Impl::s_uiSourceBpp &&
      ezImageFormat::GetBitsPerPixel(target.GetImageFormat()) == Impl::s_uiTargetBpp,
      "Image format pixel size not supported by this conversion routine");

    target.SetWidth(source.GetWidth());
    target.SetHeight(source.GetHeight());
    target.SetDepth(source.GetDepth());
    target.SetNumMipLevels(source.GetNumMipLevels());
    target.SetNumFaces(source.GetNumFaces());
    target.SetNumArrayIndices(source.GetNumArrayIndices());

    target.AllocateImageData();

    for(ezUInt32 uiArrayIndex = 0; uiArrayIndex < source.GetNumArrayIndices(); uiArrayIndex++)
    {
      for(ezUInt32 uiFace = 0; uiFace < source.GetNumFaces(); uiFace++)
      {
        for(ezUInt32 uiMipLevel = 0; uiMipLevel < source.GetNumMipLevels(); uiMipLevel++)
        {
          Impl::ConvertSubImage(source, target, uiFace, uiMipLevel, uiArrayIndex);
        }
      }
    }
  }
};

template<typename Impl>
struct ezImageConversionMixinBlockDecompression : ezImageConversionMixinBase<Impl>
{
  static void ConvertSubImage(const ezImage& source, ezImage& target, ezUInt32 uiFace, ezUInt32 uiMipLevel, ezUInt32 uiArrayIndex)
  {
    const ezUInt32 uiWidth = source.GetWidth(uiMipLevel);
    const ezUInt32 uiHeight = source.GetHeight(uiMipLevel);

    const ezUInt32 uiBlockSize = 4;

    const ezUInt32 uiNumBlocksX = source.GetNumBlocksX(uiMipLevel);
    const ezUInt32 uiNumBlocksY = source.GetNumBlocksY(uiMipLevel);

    // If the row pitch is a multiple of the pixel size, we can transform a whole slice at once
    // instead of converting row-wise.
    const ezImageFormat::Enum sourceFormat = source.GetImageFormat();
    const ezImageFormat::Enum targetFormat = target.GetImageFormat();

    const ezUInt32 uiSourceBytesPerPixel = Impl::s_uiSourceBpp / 8;
    const ezUInt32 uiTargetBytesPerPixel = Impl::s_uiTargetBpp / 8;

    const ezUInt32 uiTargetRowPitch = target.GetRowPitch(uiMipLevel, uiFace, uiArrayIndex);

    const ezUInt32 uiSourceDepthPitch = source.GetDepthPitch(uiMipLevel, uiFace, uiArrayIndex);
    const ezUInt32 uiTargetDepthPitch = target.GetDepthPitch(uiMipLevel, uiFace, uiArrayIndex);

    ezUInt32 uiBytesPerBlock = uiBlockSize * uiBlockSize * uiSourceBytesPerPixel;

    for(ezUInt32 uiSlice = 0; uiSlice < source.GetDepth(uiMipLevel); uiSlice++)
    {
      for(ezUInt32 uiBlockY = 0; uiBlockY < uiNumBlocksY; uiBlockY++)
      {
        for(ezUInt32 uiBlockX = 0; uiBlockX < uiNumBlocksX; uiBlockX++)
        {
          const ezUInt8* pSource = source.GetBlockPointer<ezUInt8>(uiMipLevel, uiFace, uiArrayIndex, uiBlockX, uiBlockY, uiSlice);

          // Decompress into a temp memory block so we don't have to explicitly handle the case where the image is not a multiple of the block size
          ezUInt8 tempBuffer[uiBlockSize * uiBlockSize * uiTargetBytesPerPixel];

          HeapValidate(GetProcessHeap(), 0, 0);

          Impl::DecompressBlock(reinterpret_cast<const Impl::SourceType*>(pSource), reinterpret_cast<Impl::TargetType*>(tempBuffer));

          HeapValidate(GetProcessHeap(), 0, 0);


          ezUInt8* pTarget = target.GetPixelPointer<ezUInt8>(
            uiMipLevel, uiFace, uiArrayIndex,
            uiBlockX * uiBlockSize, uiBlockY * uiBlockSize, uiSlice);
          
          // Copy into actual target, clamping to image dimensions
          ezUInt32 uiCopyWidth = ezMath::Min(uiBlockSize, uiWidth - uiBlockX * uiBlockSize);
          ezUInt32 uiCopyHeight= ezMath::Min(uiBlockSize, uiHeight - uiBlockY * uiBlockSize);
          for(ezUInt32 uiRow = 0; uiRow < uiCopyHeight; uiRow++)
          {
            memcpy(pTarget, &tempBuffer[uiRow * uiBlockSize * uiTargetBytesPerPixel], uiCopyWidth * uiTargetBytesPerPixel);
            pTarget += uiTargetRowPitch;
          }

          HeapValidate(GetProcessHeap(), 0, 0);
          HeapValidate(GetProcessHeap(), 0, 0);

        }
      }
    }
  }
};

template<typename Impl>
struct ezImageConversionMixinLinear : ezImageConversionMixinBase<Impl>
{
  static void ConvertSubImage(const ezImage& source, ezImage& target, ezUInt32 uiFace, ezUInt32 uiMipLevel, ezUInt32 uiArrayIndex)
  {
    const ezUInt32 uiWidth = source.GetWidth();
    const ezUInt32 uiHeight = source.GetHeight(uiMipLevel);

    // If the row pitch is a multiple of the pixel size, we can transform a whole slice at once
    // instead of converting row-wise.
    const ezImageFormat::Enum sourceFormat = source.GetImageFormat();
    const ezImageFormat::Enum targetFormat = target.GetImageFormat();

    const ezUInt32 uiSourceBytesPerPixel = Impl::s_uiSourceBpp / 8;
    const ezUInt32 uiTargetBytesPerPixel = Impl::s_uiTargetBpp / 8;

    const ezUInt32 uiSourceRowPitch = source.GetRowPitch(uiMipLevel, uiFace, uiArrayIndex);
    const ezUInt32 uiTargetRowPitch = target.GetRowPitch(uiMipLevel, uiFace, uiArrayIndex);

    const ezUInt32 uiSourceDepthPitch = source.GetDepthPitch(uiMipLevel, uiFace, uiArrayIndex);
    const ezUInt32 uiTargetDepthPitch = target.GetDepthPitch(uiMipLevel, uiFace, uiArrayIndex);

    const bool bConvertRowWise =
      (uiSourceRowPitch % uiSourceBytesPerPixel != 0) ||
      (uiTargetRowPitch % uiTargetBytesPerPixel != 0);

    for(ezUInt32 uiSlice = 0; uiSlice < source.GetDepth(uiMipLevel); uiSlice++)
    {
      const ezUInt8* pSource = source.GetPixelPointer<ezUInt8>(uiMipLevel, uiFace, uiArrayIndex, 0, 0, uiSlice);
      ezUInt8* pTarget = target.GetPixelPointer<ezUInt8>(uiMipLevel, uiFace, uiArrayIndex, 0, 0, uiSlice);

      if(bConvertRowWise)
      {
        for(ezUInt32 uiRow = 0; uiRow < uiHeight; uiRow++)
        {
          ConvertBatch(pSource, pTarget, uiWidth);
          pSource += uiSourceRowPitch;
          pTarget += uiTargetRowPitch;
        }
      }
      else
      {
        ConvertBatch(pSource, pTarget, uiSourceRowPitch / uiSourceBytesPerPixel * uiHeight);
      }
    }
  }

  static void ConvertBatch(const ezUInt8* pSource, ezUInt8* pTarget, const ezUInt32 uiElements)
  {
    const ezUInt64 uiSource = reinterpret_cast<const ezUInt64&>(pSource);
    const ezUInt64 uiTarget = reinterpret_cast<const ezUInt64&>(pTarget);

    const ezUInt64 uiSource16 = ((uiSource - 1) / 16 + 1) * 16;
    const ezUInt64 uiTarget16 = ((uiTarget - 1) / 16 + 1) * 16;

    const ezUInt32 uiSourceBytesPerPixel = Impl::s_uiSourceBpp / 8;
    const ezUInt32 uiTargetBytesPerPixel = Impl::s_uiTargetBpp / 8;

    const ezUInt32 uiLeadInSource = static_cast<ezUInt32>(uiSource16 - uiSource);
    const ezUInt32 uiLeadInTarget = static_cast<ezUInt32>(uiTarget16 - uiTarget);

    // Check if we can reach a point where source and target are both aligned
    bool bAlignmentMatches =
      (uiLeadInSource % uiSourceBytesPerPixel == 0) &&
      (uiLeadInTarget % uiTargetBytesPerPixel == 0) &&
      (uiLeadInSource / uiSourceBytesPerPixel == uiLeadInTarget / uiTargetBytesPerPixel);

    if(bAlignmentMatches)
    {
      // Convert element-wise until we reach alignment
      const ezUInt32 uiLeadInElements = uiLeadInSource / uiSourceBytesPerPixel;
      for(ezUInt32 uiElement = 0; uiElement < uiLeadInElements; uiElement++)
      {
        Impl::ConvertSingle(
          reinterpret_cast<const Impl::SourceTypeSingle*>(pSource),
          reinterpret_cast<Impl::TargetTypeSingle*>(pTarget));
        pSource += uiSourceBytesPerPixel;
        pTarget += uiTargetBytesPerPixel;
      }

      // Convert multiple elements for as long as possible
      const ezUInt32 uiMiddleElements =
        (uiElements / Impl::s_uiMultiConversionSize) * Impl::s_uiMultiConversionSize;
      for(ezUInt32 uiElement = 0; uiElement < uiMiddleElements; uiElement += Impl::s_uiMultiConversionSize)
      {
        Impl::ConvertMultiple(
          reinterpret_cast<const Impl::SourceTypeMultiple*>(pSource),
          reinterpret_cast<Impl::TargetTypeMultiple*>(pTarget));
        pSource += Impl::s_uiMultiConversionSize * uiSourceBytesPerPixel;
        pTarget += Impl::s_uiMultiConversionSize * uiTargetBytesPerPixel;
      }

      // Convert element-wise until the end
      const ezUInt32 uiLeadOutElements = uiElements - (uiLeadInElements + uiMiddleElements);
      for(ezUInt32 uiElement = 0; uiElement < uiLeadOutElements; uiElement++)
      {
        Impl::ConvertSingle(
          reinterpret_cast<const Impl::SourceTypeSingle*>(pSource),
          reinterpret_cast<Impl::TargetTypeSingle*>(pTarget));
        pSource += uiSourceBytesPerPixel;
        pTarget += uiTargetBytesPerPixel;
      }
    }
    else
    {
      // Slow path: convert each element by itself
      for(ezUInt32 uiElement = 0; uiElement < uiElements; uiElement++)
      {
        Impl::ConvertSingle(
          reinterpret_cast<const Impl::SourceTypeSingle*>(pSource),
          reinterpret_cast<Impl::TargetTypeSingle*>(pTarget));
        pSource += uiSourceBytesPerPixel;
        pTarget += uiTargetBytesPerPixel;
      }
    }
  }
};