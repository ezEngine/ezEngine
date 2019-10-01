#include <TexturePCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Texture/Image/ImageConversion.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezImageConversionStep);

namespace
{
  struct TableEntry
  {
    TableEntry() = default;

    TableEntry(const ezImageConversionStep* pStep, const ezImageConversionEntry& entry)
    {
      m_step = pStep;
      m_sourceFormat = entry.m_sourceFormat;
      m_targetFormat = entry.m_targetFormat;
      m_numChannels = ezMath::Min(ezImageFormat::GetNumChannels(entry.m_sourceFormat), ezImageFormat::GetNumChannels(entry.m_targetFormat));

      float sourceBpp = ezImageFormat::GetExactBitsPerPixel(m_sourceFormat);
      float targetBpp = ezImageFormat::GetExactBitsPerPixel(m_targetFormat);

      m_flags = entry.m_flags;

      // Base cost is amount of bits processed
      m_cost = sourceBpp + targetBpp;

      // Penalty for non-inplace conversion
      if ((m_flags & ezImageConversionFlags::InPlace) == 0)
      {
        m_cost *= 2;
      }

      // Penalize formats that aren't aligned to powers of two
      if (!ezImageFormat::IsCompressed(m_sourceFormat) && !ezImageFormat::IsCompressed(m_targetFormat))
      {
        auto sourceBppInt = static_cast<ezUInt32>(sourceBpp);
        auto targetBppInt = static_cast<ezUInt32>(targetBpp);
        if (!ezMath::IsPowerOf2(sourceBppInt) || !ezMath::IsPowerOf2(targetBppInt))
        {
          m_cost *= 2;
        }
      }
    }

    const ezImageConversionStep* m_step = nullptr;
    ezImageFormat::Enum m_sourceFormat = ezImageFormat::UNKNOWN;
    ezImageFormat::Enum m_targetFormat = ezImageFormat::UNKNOWN;
    ezBitflags<ezImageConversionFlags> m_flags;
    float m_cost = ezMath::MaxValue<float>();
    ezUInt32 m_numChannels = 0;

    static TableEntry chain(const TableEntry& a, const TableEntry& b)
    {
      if (ezImageFormat::GetExactBitsPerPixel(a.m_sourceFormat) > ezImageFormat::GetExactBitsPerPixel(a.m_targetFormat) &&
          ezImageFormat::GetExactBitsPerPixel(b.m_sourceFormat) < ezImageFormat::GetExactBitsPerPixel(b.m_targetFormat))
      {
        // Disallow chaining conversions which first reduce to a smaller intermediate and then go back to a larger one, since
        // we end up throwing away information.
        return {};
      }

      TableEntry entry;
      entry.m_step = a.m_step;
      entry.m_cost = a.m_cost + b.m_cost;
      entry.m_sourceFormat = a.m_sourceFormat;
      entry.m_targetFormat = a.m_targetFormat;
      entry.m_flags = a.m_flags;
      entry.m_numChannels = ezMath::Min(a.m_numChannels, b.m_numChannels);
      return entry;
    }

    bool operator<(const TableEntry& other) const
    {
      if (m_numChannels > other.m_numChannels)
        return true;

      if (m_numChannels < other.m_numChannels)
        return false;

      return m_cost < other.m_cost;
    }

    bool isAdmissible() const
    {
      if (m_numChannels == 0)
        return false;

      return m_cost < ezMath::MaxValue<float>();
    }
  };

  ezMutex s_conversionTableLock;
  ezHashTable<ezUInt32, TableEntry> s_conversionTable;
  bool s_conversionTableValid = false;

  ezUInt32 MakeKey(ezImageFormat::Enum a, ezImageFormat::Enum b) { return a * ezImageFormat::NUM_FORMATS + b; }

  struct IntermediateBuffer
  {
    IntermediateBuffer(ezUInt32 bitsPerBlock)
        : m_bitsPerBlock(bitsPerBlock)
    {
    }
    ezUInt32 m_bitsPerBlock;
  };

  ezUInt32 allocateScratchBufferIndex(ezHybridArray<IntermediateBuffer, 16>& scratchBuffers, ezUInt32 bitsPerBlock, ezUInt32 excludedIndex)
  {
    int foundIndex = -1;

    for (ezUInt32 bufferIndex = 0; bufferIndex < ezUInt32(scratchBuffers.GetCount()); ++bufferIndex)
    {
      if (bufferIndex == excludedIndex)
      {
        continue;
      }

      if (scratchBuffers[bufferIndex].m_bitsPerBlock == bitsPerBlock)
      {
        foundIndex = bufferIndex;
        break;
      }
    }

    if (foundIndex >= 0)
    {
      // Reuse existing scratch buffer
      return foundIndex;
    }
    else
    {
      // Allocate new scratch buffer
      scratchBuffers.PushBack(IntermediateBuffer(bitsPerBlock));
      return scratchBuffers.GetCount() - 1;
    }
  }
} // namespace

ezImageConversionStep::ezImageConversionStep()
{
  s_conversionTableValid = false;
}

ezImageConversionStep::~ezImageConversionStep()
{
  s_conversionTableValid = false;
}

ezResult ezImageConversion::BuildPath(ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat, bool bSourceEqualsTarget,
                                      ezHybridArray<ezImageConversion::ConversionPathNode, 16>& path_out, ezUInt32& numScratchBuffers_out)
{
  EZ_LOCK(s_conversionTableLock);

  path_out.Clear();
  numScratchBuffers_out = 0;

  if (sourceFormat == targetFormat)
  {
    ConversionPathNode node;
    node.m_sourceFormat = sourceFormat;
    node.m_targetFormat = targetFormat;
    node.m_inPlace = bSourceEqualsTarget;
    node.m_sourceBufferIndex = 0;
    node.m_targetBufferIndex = 0;
    node.m_step = nullptr;
    path_out.PushBack(node);
    return EZ_SUCCESS;
  }

  if (!s_conversionTableValid)
  {
    RebuildConversionTable();
  }

  for (ezImageFormat::Enum current = sourceFormat; current != targetFormat;)
  {
    ezUInt32 currentTableIndex = MakeKey(current, targetFormat);

    TableEntry entry;

    if (!s_conversionTable.TryGetValue(currentTableIndex, entry))
    {
      return EZ_FAILURE;
    }

    ezImageConversion::ConversionPathNode step;
    step.m_sourceFormat = entry.m_sourceFormat;
    step.m_targetFormat = entry.m_targetFormat;
    step.m_inPlace = entry.m_flags.IsAnySet(ezImageConversionFlags::InPlace);
    step.m_step = entry.m_step;

    current = entry.m_targetFormat;

    path_out.PushBack(step);
  }

  ezHybridArray<IntermediateBuffer, 16> scratchBuffers;
  scratchBuffers.PushBack(IntermediateBuffer(ezImageFormat::GetBitsPerBlock(targetFormat)));

  for (int i = path_out.GetCount() - 1; i >= 0; --i)
  {
    if (i == path_out.GetCount() - 1)
      path_out[i].m_targetBufferIndex = 0;
    else
      path_out[i].m_targetBufferIndex = path_out[i + 1].m_sourceBufferIndex;

    if (i > 0)
    {
      if (path_out[i].m_inPlace)
      {
        path_out[i].m_sourceBufferIndex = path_out[i].m_targetBufferIndex;
      }
      else
      {
        ezUInt32 bitsPerBlock = ezImageFormat::GetBitsPerBlock(path_out[i].m_sourceFormat);

        path_out[i].m_sourceBufferIndex = allocateScratchBufferIndex(scratchBuffers, bitsPerBlock, path_out[i].m_targetBufferIndex);
      }
    }
  }

  if (bSourceEqualsTarget)
  {
    // Enforce constraint that source == target
    path_out[0].m_sourceBufferIndex = 0;

    // Did we accidentally break the in-place invariant?
    if (path_out[0].m_sourceBufferIndex == path_out[0].m_targetBufferIndex && !path_out[0].m_inPlace)
    {
      if (path_out.GetCount() == 1)
      {
        // Only a single step, so we need to add a copy step
        ezImageConversion::ConversionPathNode copy;
        copy.m_inPlace = false;
        copy.m_sourceFormat = sourceFormat;
        copy.m_targetFormat = sourceFormat;
        copy.m_sourceBufferIndex = path_out[0].m_sourceBufferIndex;
        copy.m_targetBufferIndex = allocateScratchBufferIndex(scratchBuffers, ezImageFormat::GetBitsPerBlock(path_out[0].m_sourceFormat),
                                                              path_out[0].m_sourceBufferIndex);
        path_out[0].m_sourceBufferIndex = copy.m_targetBufferIndex;
        copy.m_step = nullptr;
        path_out.Insert(copy, 0);
      }
      else
      {
        // Turn second step to non-inplace
        path_out[1].m_inPlace = false;
        path_out[1].m_sourceBufferIndex = allocateScratchBufferIndex(
            scratchBuffers, ezImageFormat::GetBitsPerBlock(path_out[1].m_sourceFormat), path_out[0].m_sourceBufferIndex);
        path_out[0].m_targetBufferIndex = path_out[1].m_sourceBufferIndex;
      }
    }
  }
  else
  {
    path_out[0].m_sourceBufferIndex = scratchBuffers.GetCount();
  }

  numScratchBuffers_out = scratchBuffers.GetCount() - 1;

  return EZ_SUCCESS;
}

void ezImageConversion::RebuildConversionTable()
{
  EZ_LOCK(s_conversionTableLock);

  s_conversionTable.Clear();

  // Prime conversion table with known conversions
  for (ezImageConversionStep* conversion = ezImageConversionStep::GetFirstInstance(); conversion;
       conversion = conversion->GetNextInstance())
  {
    ezArrayPtr<const ezImageConversionEntry> entries = conversion->GetSupportedConversions();

    for (ezUInt32 subIndex = 0; subIndex < (ezUInt32)entries.GetCount(); subIndex++)
    {
      const ezImageConversionEntry& subConversion = entries[subIndex];

      if (subConversion.m_flags.IsAnySet(ezImageConversionFlags::InPlace))
      {
        EZ_ASSERT_DEV(ezImageFormat::IsCompressed(subConversion.m_sourceFormat) ==
                              ezImageFormat::IsCompressed(subConversion.m_targetFormat) &&
                          ezImageFormat::GetBitsPerBlock(subConversion.m_sourceFormat) ==
                              ezImageFormat::GetBitsPerBlock(subConversion.m_targetFormat),
                      "In-place conversions are only allowed between formats of the same number of bits per pixel and compressedness");
      }


      ezUInt32 tableIndex = MakeKey(subConversion.m_sourceFormat, subConversion.m_targetFormat);

      // Use the cheapest known conversion for each combination in case there are multiple ones
      TableEntry candidate(conversion, subConversion);

      TableEntry existing;

      if (!s_conversionTable.TryGetValue(tableIndex, existing) || candidate < existing)
      {
        s_conversionTable.Insert(tableIndex, candidate);
      }
    }
  }

  for (ezUInt32 i = 0; i < ezImageFormat::NUM_FORMATS; i++)
  {
    const ezImageFormat::Enum format = static_cast<ezImageFormat::Enum>(i);
    // Add copy-conversion (from and to same format)
    s_conversionTable.Insert(MakeKey(format, format),
      TableEntry(nullptr, ezImageConversionEntry(ezImageConversionEntry(format, format, ezImageConversionFlags::InPlace))));
  }

  // Straight from http://en.wikipedia.org/wiki/Floyd-Warshall_algorithm
  for (ezUInt32 k = 1; k < ezImageFormat::NUM_FORMATS; k++)
  {
    for (ezUInt32 i = 1; i < ezImageFormat::NUM_FORMATS; i++)
    {
      if (k == i)
      {
        continue;
      }

      ezUInt32 tableIndexIK = MakeKey(static_cast<ezImageFormat::Enum>(i), static_cast<ezImageFormat::Enum>(k));

      TableEntry entryIK;
      if (!s_conversionTable.TryGetValue(tableIndexIK, entryIK))
      {
        continue;
      }

      for (ezUInt32 j = 1; j < ezImageFormat::NUM_FORMATS; j++)
      {
        if (j == i || j == k)
        {
          continue;
        }

        ezUInt32 tableIndexIJ = MakeKey(static_cast<ezImageFormat::Enum>(i), static_cast<ezImageFormat::Enum>(j));
        ezUInt32 tableIndexKJ = MakeKey(static_cast<ezImageFormat::Enum>(k), static_cast<ezImageFormat::Enum>(j));

        TableEntry entryKJ;
        if (!s_conversionTable.TryGetValue(tableIndexKJ, entryKJ))
        {
          continue;
        }

        TableEntry candidate = TableEntry::chain(entryIK, entryKJ);

        TableEntry existing;
        if (candidate.isAdmissible() && candidate < s_conversionTable[tableIndexIJ])
        {
          // To Convert from format I to format J, first Convert from I to K
          s_conversionTable[tableIndexIJ] = candidate;
        }
      }
    }
  }

  s_conversionTableValid = true;
}

ezResult ezImageConversion::Convert(const ezImageView& source, ezImage& target, ezImageFormat::Enum targetFormat)
{
  ezImageFormat::Enum sourceFormat = source.GetImageFormat();

  // Trivial copy
  if (sourceFormat == targetFormat)
  {
    if (&source != &target)
    {
      // copy if not already the same
      target.ResetAndCopy(source);
    }
    return EZ_SUCCESS;
  }

  ezHybridArray<ConversionPathNode, 16> path;
  ezUInt32 numScratchBuffers = 0;
  if (BuildPath(sourceFormat, targetFormat, &source == &target, path, numScratchBuffers).Failed())
  {
    return EZ_FAILURE;
  }

  return Convert(source, target, path, numScratchBuffers);
}

ezResult ezImageConversion::Convert(const ezImageView& source, ezImage& target, ezArrayPtr<ConversionPathNode> path,
                                    ezUInt32 numScratchBuffers)
{
  EZ_ASSERT_DEV(path.GetCount() > 0, "Invalid conversion path");
  EZ_ASSERT_DEV(path[0].m_sourceFormat == source.GetImageFormat(), "Invalid conversion path");

  ezHybridArray<ezImage, 16> intermediates;
  intermediates.SetCount(numScratchBuffers);

  const ezImageView* pSource = &source;

  for (ezUInt32 i = 0; i < path.GetCount(); ++i)
  {
    ezUInt32 targetIndex = path[i].m_targetBufferIndex;

    ezImage* pTarget = targetIndex == 0 ? &target : &intermediates[targetIndex - 1];

    if (ConvertSingleStep(path[i].m_step, *pSource, *pTarget, path[i].m_targetFormat).Failed())
    {
      return EZ_FAILURE;
    }

    pSource = pTarget;
  }

  return EZ_SUCCESS;
}

ezResult ezImageConversion::ConvertRaw(ezConstByteBlobPtr source, ezByteBlobPtr target, ezUInt32 numElements,
                                       ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat)
{
  if (numElements == 0)
  {
    return EZ_SUCCESS;
  }

  // Trivial copy
  if (sourceFormat == targetFormat)
  {
    if (target.GetPtr() != source.GetPtr())
      memcpy(target.GetPtr(), source.GetPtr(), numElements * ezUInt64(ezImageFormat::GetBitsPerPixel(sourceFormat)) / 8);
    return EZ_SUCCESS;
  }

  if (ezImageFormat::IsCompressed(sourceFormat) || ezImageFormat::IsCompressed(targetFormat))
  {
    return EZ_FAILURE;
  }

  ezHybridArray<ConversionPathNode, 16> path;
  ezUInt32 numScratchBuffers;
  if (BuildPath(sourceFormat, targetFormat, source.GetPtr() == target.GetPtr(), path, numScratchBuffers).Failed())
  {
    return EZ_FAILURE;
  }

  return ConvertRaw(source, target, numElements, path, numScratchBuffers);
}

ezResult ezImageConversion::ConvertRaw(ezConstByteBlobPtr source, ezByteBlobPtr target, ezUInt32 numElements,
                                       ezArrayPtr<ConversionPathNode> path, ezUInt32 numScratchBuffers)
{
  EZ_ASSERT_DEV(path.GetCount() > 0, "Path of length 0 is invalid.");

  if (numElements == 0)
  {
    return EZ_SUCCESS;
  }

  if (ezImageFormat::IsCompressed(path.GetPtr()->m_sourceFormat) || ezImageFormat::IsCompressed((path.GetEndPtr() - 1)->m_targetFormat))
  {
    return EZ_FAILURE;
  }

  ezHybridArray<ezBlob, 16> intermediates;
  intermediates.SetCount(numScratchBuffers);

  for (ezUInt32 i = 0; i < path.GetCount(); ++i)
  {
    ezUInt32 targetIndex = path[i].m_targetBufferIndex;
    ezUInt32 targetBpp = ezImageFormat::GetBitsPerPixel(path[i].m_targetFormat);

    ezByteBlobPtr stepTarget;
    if (targetIndex == 0)
    {
      stepTarget = target;
    }
    else
    {
      ezUInt32 expectedSize = static_cast<ezUInt32>(targetBpp * numElements / 8);
      intermediates[targetIndex - 1].SetCountUninitialized(expectedSize);
      stepTarget = intermediates[targetIndex - 1].GetByteBlobPtr();
    }

    if (path[i].m_step == nullptr)
    {
      memcpy(stepTarget.GetPtr(), source.GetPtr(), numElements * targetBpp / 8);
    }
    else
    {
      if (static_cast<const ezImageConversionStepLinear*>(path[i].m_step)
              ->ConvertPixels(source, stepTarget, numElements, path[i].m_sourceFormat, path[i].m_targetFormat)
              .Failed())
      {
        return EZ_FAILURE;
      }
    }

    source = stepTarget;
  }

  return EZ_SUCCESS;
}

ezResult ezImageConversion::ConvertSingleStep(const ezImageConversionStep* pStep, const ezImageView& source, ezImage& target,
                                              ezImageFormat::Enum targetFormat)
{
  if (!pStep)
  {
    target.ResetAndCopy(source);
    return EZ_SUCCESS;
  }

  ezImageFormat::Enum sourceFormat = source.GetImageFormat();

  ezImageHeader header = source.GetHeader();
  header.SetImageFormat(targetFormat);
  target.ResetAndAlloc(header);

  if (!ezImageFormat::IsCompressed(sourceFormat))
  {
    if (!ezImageFormat::IsCompressed(targetFormat))
    {
      // we have to do the computation in 64-bit otherwise it might overflow for very large textures (8k x 4k or bigger).
      ezUInt64 numElements =
          ezUInt64(8) * target.GetByteBlobPtr().GetCount() / (ezUInt64)ezImageFormat::GetBitsPerPixel(targetFormat);
      return static_cast<const ezImageConversionStepLinear*>(pStep)->ConvertPixels(source.GetByteBlobPtr(), target.GetByteBlobPtr(),
                                                                                   (ezUInt32)numElements, sourceFormat, targetFormat);
    }
    else
    {
      return ConvertSingleStepCompress(source, target, sourceFormat, targetFormat, pStep);
    }
  }
  else
  {
    return ConvertSingleStepDecompress(source, target, sourceFormat, targetFormat, pStep);
  }
}

ezResult ezImageConversion::ConvertSingleStepDecompress(const ezImageView& source, ezImage& target, ezImageFormat::Enum sourceFormat,
                                                        ezImageFormat::Enum targetFormat, const ezImageConversionStep* pStep)
{
  for (ezUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (ezUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (ezUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const ezUInt32 width = target.GetWidth(mipLevel);
        const ezUInt32 height = target.GetHeight(mipLevel);

        const ezUInt32 blockSizeX = ezImageFormat::GetBlockWidth(sourceFormat);
        const ezUInt32 blockSizeY = ezImageFormat::GetBlockHeight(sourceFormat);

        const ezUInt32 numBlocksX = source.GetNumBlocksX(mipLevel);
        const ezUInt32 numBlocksY = source.GetNumBlocksY(mipLevel);

        const ezUInt64 targetRowPitch = target.GetRowPitch(mipLevel);
        const ezUInt32 targetBytesPerPixel = ezImageFormat::GetBitsPerPixel(targetFormat) / 8;

        const ezUInt32 blockSizeInBytes = ezImageFormat::GetBitsPerBlock(sourceFormat) / 8;

        // Decompress into a temp memory block so we don't have to explicitly handle the case where the image is not a multiple of the block
        // size
        ezHybridArray<ezUInt8, 256> tempBuffer;
        tempBuffer.SetCount(numBlocksX * blockSizeX * blockSizeY * targetBytesPerPixel);

        for (ezUInt32 slice = 0; slice < source.GetDepth(mipLevel); slice++)
        {
          for (ezUInt32 blockY = 0; blockY < numBlocksY; blockY++)
          {
            ezImageView sourceRowView = source.GetRowView(mipLevel, face, arrayIndex, blockY, slice);

            if (static_cast<const ezImageConversionStepDecompressBlocks*>(pStep)
                    ->DecompressBlocks(sourceRowView.GetByteBlobPtr(), ezByteBlobPtr(tempBuffer.GetData(), tempBuffer.GetCount()),
                                       numBlocksX, sourceFormat, targetFormat)
                    .Failed())
            {
              return EZ_FAILURE;
            }

            for (ezUInt32 blockX = 0; blockX < numBlocksX; blockX++)
            {
              ezUInt8* targetPointer =
                  target.GetPixelPointer<ezUInt8>(mipLevel, face, arrayIndex, blockX * blockSizeX, blockY * blockSizeY, slice);

              // Copy into actual target, clamping to image dimensions
              ezUInt32 copyWidth = ezMath::Min(blockSizeX, width - blockX * blockSizeX);
              ezUInt32 copyHeight = ezMath::Min(blockSizeY, height - blockY * blockSizeY);
              for (ezUInt32 row = 0; row < copyHeight; row++)
              {
                memcpy(targetPointer, &tempBuffer[(blockX * blockSizeX + row) * blockSizeY * targetBytesPerPixel],
                       copyWidth * targetBytesPerPixel);
                targetPointer += targetRowPitch;
              }
            }
          }
        }
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezImageConversion::ConvertSingleStepCompress(const ezImageView& source, ezImage& target, ezImageFormat::Enum sourceFormat,
                                                      ezImageFormat::Enum targetFormat, const ezImageConversionStep* pStep)
{
  for (ezUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (ezUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (ezUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const ezUInt32 sourceWidth = source.GetWidth(mipLevel);
        const ezUInt32 sourceHeight = source.GetHeight(mipLevel);

        const ezUInt32 numBlocksX = target.GetNumBlocksX(mipLevel);
        const ezUInt32 numBlocksY = target.GetNumBlocksY(mipLevel);

        const ezUInt32 targetWidth = numBlocksX * ezImageFormat::GetBlockWidth(targetFormat);
        const ezUInt32 targetHeight = numBlocksY * ezImageFormat::GetBlockHeight(targetFormat);

        const ezUInt64 sourceRowPitch = source.GetRowPitch(mipLevel);
        const ezUInt32 sourceBytesPerPixel = ezImageFormat::GetBitsPerPixel(sourceFormat) / 8;

        const ezUInt32 blockSizeInBytes = ezImageFormat::GetBitsPerBlock(targetFormat) / 8;

        // Pad image to multiple of block size for compression
        ezImageHeader paddedSliceHeader;
        paddedSliceHeader.SetWidth(targetWidth);
        paddedSliceHeader.SetHeight(targetHeight);
        paddedSliceHeader.SetImageFormat(sourceFormat);

        ezImage paddedSlice;
        paddedSlice.ResetAndAlloc(paddedSliceHeader);

        for (ezUInt32 slice = 0; slice < source.GetDepth(mipLevel); slice++)
        {
          for (ezUInt32 y = 0; y < targetHeight; ++y)
          {
            ezUInt32 sourceY = ezMath::Min(y, sourceHeight - 1);

            memcpy(paddedSlice.GetPixelPointer<void>(0, 0, 0, 0, y),
                   source.GetPixelPointer<void>(mipLevel, face, arrayIndex, 0, sourceY, slice), static_cast<size_t>(sourceRowPitch));

            for (ezUInt32 x = sourceWidth; x < targetWidth; ++x)
            {
              memcpy(paddedSlice.GetPixelPointer<void>(0, 0, 0, x, y),
                     source.GetPixelPointer<void>(mipLevel, face, arrayIndex, sourceWidth - 1, sourceY, slice), sourceBytesPerPixel);
            }
          }

          ezResult result = static_cast<const ezImageConversionStepCompressBlocks*>(pStep)->CompressBlocks(
              paddedSlice.GetByteBlobPtr(), target.GetSliceView(mipLevel, face, arrayIndex, slice).GetByteBlobPtr(), numBlocksX, numBlocksY,
              sourceFormat, targetFormat);

          if (result.Failed())
          {
            return EZ_FAILURE;
          }
        }
      }
    }
  }

  return EZ_SUCCESS;
}


bool ezImageConversion::IsConvertible(ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat)
{
  EZ_LOCK(s_conversionTableLock);

  if (!s_conversionTableValid)
  {
    RebuildConversionTable();
  }

  ezUInt32 tableIndex = MakeKey(sourceFormat, targetFormat);
  return s_conversionTable.Contains(tableIndex);
}

ezImageFormat::Enum ezImageConversion::FindClosestCompatibleFormat(
  ezImageFormat::Enum format, ezArrayPtr<const ezImageFormat::Enum> compatibleFormats)
{
  EZ_LOCK(s_conversionTableLock);

  if (!s_conversionTableValid)
  {
    RebuildConversionTable();
  }

  TableEntry bestEntry;
  ezImageFormat::Enum bestFormat = ezImageFormat::UNKNOWN;

  for (ezUInt32 targetIndex = 0; targetIndex < ezUInt32(compatibleFormats.GetCount()); targetIndex++)
  {
    ezUInt32 tableIndex = MakeKey(format, compatibleFormats[targetIndex]);
    TableEntry candidate;
    if (s_conversionTable.TryGetValue(tableIndex, candidate) && candidate < bestEntry)
    {
      bestEntry = candidate;
      bestFormat = compatibleFormats[targetIndex];
    }
  }

  return bestFormat;
}

EZ_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageConversion);

