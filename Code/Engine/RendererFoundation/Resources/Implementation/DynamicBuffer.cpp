#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>

namespace
{
  struct CompareRangesByStart
  {
    bool Less(const ezGAL::ModifiedRange& a, const ezGAL::ModifiedRange& b) const
    {
      return a.m_uiMin < b.m_uiMin;
    }
  };

  struct CompareRangesByStartReverse
  {
    bool Less(const ezGAL::ModifiedRange& a, const ezGAL::ModifiedRange& b) const
    {
      return a.m_uiMin > b.m_uiMin;
    }
  };

  struct CompareRangesByCount
  {
    bool Less(const ezGAL::ModifiedRange& a, const ezGAL::ModifiedRange& b) const
    {
      return a.GetCount() < b.GetCount();
    }
  };
} // namespace

////////////////////////////////////////////////////////////////////////

ezGALDynamicBuffer::~ezGALDynamicBuffer()
{
  Deinitialize();
}

void ezGALDynamicBuffer::Initialize(const ezGALBufferCreationDescription& desc, ezStringView sDebugName)
{
  EZ_ASSERT_DEV(desc.m_uiStructSize > 0, "Struct size must be greater than 0");
  EZ_IGNORE_UNUSED(sDebugName);

  m_Desc = desc;

  m_Data.SetCountUninitialized(desc.m_uiTotalSize);
  m_uiCapacity = desc.m_uiTotalSize / desc.m_uiStructSize;

  m_sDebugName = sDebugName;
}

void ezGALDynamicBuffer::Deinitialize()
{
  Clear();

  if (m_hBufferForRendering != m_hBufferForUpload)
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hBufferForRendering);
  }

  ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hBufferForUpload);
}

void ezGALDynamicBuffer::Clear()
{
  m_Data.SetCountUninitialized(m_Desc.m_uiTotalSize);
  m_uiNextOffset = 0;

  for (auto& tempData : m_TempData)
  {
    tempData.m_pAllocator->Deallocate(tempData.m_pData);
  }
  m_TempData.Clear();

  m_Allocations.Clear();
  m_FreeRanges.Clear();
  m_DirtyRange.Reset();
}

ezUInt32 ezGALDynamicBuffer::Allocate(ezUInt64 uiUserData, ezUInt32 uiCount, ezBitflags<AllocateFlags> allocateFlags, ezAllocator* pTempAllocator)
{
  EZ_LOCK(m_Mutex);
  EZ_ASSERT_DEV(uiCount > 0, "Allocation count must be greater than 0");

  ezUInt32 uiOffset = ezInvalidIndex;

  // First try to use a hole. m_FreeRanges is sorted by count, smallest first, so this finds the smallest hole that fits.
  // If the allocation uses only part of a hole, the hole gets smaller but the array is not sorted again.
  // So the order may not be exact until the next sort.
  for (ezUInt32 i = 0; i < m_FreeRanges.GetCount(); ++i)
  {
    auto& freeRange = m_FreeRanges[i];
    const ezUInt32 uiFreeCount = freeRange.GetCount();

    if (uiFreeCount >= uiCount)
    {
      uiOffset = freeRange.m_uiMin;

      if (uiFreeCount == uiCount)
      {
        m_FreeRanges.RemoveAtAndCopy(i);
      }
      else
      {
        freeRange.m_uiMin += uiCount;
      }

      break;
    }
  }

  // No hole fits, so add the allocation at the end
  if (uiOffset == ezInvalidIndex)
  {
    uiOffset = m_uiNextOffset;
    m_uiNextOffset += uiCount;
  }

  // Don't resize m_Data here. Other threads may still have mapped pointers into it.
  // If the full allocation is not inside m_Data or inside one temp data, create new temp data for it.
  // This happens when the buffer has to grow.
  // It also happens when deallocations moved the end of the buffer below the start of the temp data,
  // and the new allocation starts inside m_Data but ends after it.
  ezUInt32 uiDataIndex = FindDataIndex(uiOffset, uiCount);
  if (uiDataIndex == ezInvalidIndex)
  {
    uiDataIndex = AllocateTempData(uiOffset, m_uiNextOffset, pTempAllocator);
  }

  m_Allocations.Insert(uiOffset, Allocation{uiUserData, uiCount, uiDataIndex});

  if (allocateFlags.IsSet(AllocateFlags::ZeroFill))
  {
    ezUInt32 uiDummyCount = 0;
    auto data = MapForWriting(uiOffset, uiDummyCount);
    ezMemoryUtils::ZeroFill(data.GetPtr(), data.GetCount());
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  CheckSelf();
#endif

  return uiOffset;
}

void ezGALDynamicBuffer::Deallocate(ezUInt32 uiOffset)
{
  EZ_LOCK(m_Mutex);

  auto it = m_Allocations.Find(uiOffset);
  EZ_ASSERT_DEV(it.IsValid(), "Invalid offset");

  const ezUInt32 uiCount = it.Value().m_uiCount;

  if (it.Key() == m_Allocations.GetReverseIterator().Key())
  {
    // This is the last allocation. Don't create a free range, make the used part of the buffer smaller instead.
    m_uiNextOffset = uiOffset;

    // If a free range is directly before it, that range is now at the end. Remove it as well.
    // Free ranges are never next to each other, so there is at most one such range.
    for (ezUInt32 i = 0; i < m_FreeRanges.GetCount(); ++i)
    {
      auto& freeRange = m_FreeRanges[i];
      if (freeRange.m_uiMax + 1 == m_uiNextOffset)
      {
        m_uiNextOffset = freeRange.m_uiMin;
        m_FreeRanges.RemoveAtAndCopy(i);
        break;
      }
    }
  }
  else
  {
    m_FreeRanges.PushBack(ezGAL::ModifiedRange{uiOffset, uiOffset + uiCount - 1});

    // Merge adjacent free ranges
    m_FreeRanges.Sort(CompareRangesByStart());
    for (ezUInt32 i = 0; i < m_FreeRanges.GetCount() - 1; ++i)
    {
      auto& currentFreeRange = m_FreeRanges[i];
      auto& nextFreeRange = m_FreeRanges[i + 1];

      if (currentFreeRange.m_uiMax + 1 == nextFreeRange.m_uiMin)
      {
        currentFreeRange.m_uiMax = nextFreeRange.m_uiMax;
        m_FreeRanges.RemoveAtAndCopy(i + 1);
        --i;
      }
    }

    // Sort by count to make sure that the smaller holes are filled first
    m_FreeRanges.Sort(CompareRangesByCount());
  }

  m_Allocations.Remove(it);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  CheckSelf();
#endif
}

ezByteArrayPtr ezGALDynamicBuffer::MapForWriting(ezUInt32 uiOffset, ezUInt32& out_uiCount)
{
  EZ_LOCK(m_Mutex);

  auto it = m_Allocations.Find(uiOffset);
  EZ_ASSERT_DEV(it.IsValid(), "Invalid offset");

  auto& allocation = it.Value();
  out_uiCount = allocation.m_uiCount;

  // Mark dirty
  m_DirtyRange.SetToIncludeRange(uiOffset, uiOffset + out_uiCount - 1);

  const ezUInt32 uiByteOffset = uiOffset * m_Desc.m_uiStructSize;
  const ezUInt32 uiByteSize = out_uiCount * m_Desc.m_uiStructSize;

  if (allocation.m_uiDataIndex > 0)
  {
    auto& tempData = m_TempData[allocation.m_uiDataIndex - 1];
    const ezUInt32 uiLocalByteOffset = uiByteOffset - tempData.m_uiStartByteOffset;
    EZ_ASSERT_DEBUG(uiLocalByteOffset + uiByteSize <= tempData.m_uiByteSize, "Implementation error");
    return ezByteArrayPtr(tempData.m_pData + uiLocalByteOffset, uiByteSize);
  }

  return m_Data.GetByteArrayPtr().GetSubArray(uiByteOffset, uiByteSize);
}

ezConstByteArrayPtr ezGALDynamicBuffer::MapForReading(ezUInt32 uiOffset, ezUInt32& out_uiCount) const
{
  EZ_LOCK(m_Mutex);

  auto it = m_Allocations.Find(uiOffset);
  EZ_ASSERT_DEV(it.IsValid(), "Invalid offset");

  auto& allocation = it.Value();
  out_uiCount = allocation.m_uiCount;

  const ezUInt32 uiByteOffset = uiOffset * m_Desc.m_uiStructSize;
  const ezUInt32 uiByteSize = out_uiCount * m_Desc.m_uiStructSize;

  if (allocation.m_uiDataIndex > 0)
  {
    auto& tempData = m_TempData[allocation.m_uiDataIndex - 1];
    const ezUInt32 uiLocalByteOffset = uiByteOffset - tempData.m_uiStartByteOffset;
    EZ_ASSERT_DEBUG(uiLocalByteOffset + uiByteSize <= tempData.m_uiByteSize, "Implementation error");
    return ezConstByteArrayPtr(tempData.m_pData + uiLocalByteOffset, uiByteSize);
  }

  return m_Data.GetByteArrayPtr().GetSubArray(uiByteOffset, uiByteSize);
}

ezUInt32 ezGALDynamicBuffer::FindDataIndex(ezUInt32 uiOffset, ezUInt32 uiCount) const
{
  const ezUInt32 uiByteOffset = uiOffset * m_Desc.m_uiStructSize;
  const ezUInt32 uiByteEndOffset = (uiOffset + uiCount) * m_Desc.m_uiStructSize;

  if (uiByteEndOffset <= m_Data.GetCount())
    return 0;

  for (ezUInt32 i = m_TempData.GetCount(); i > 0; --i)
  {
    const auto& tempData = m_TempData[i - 1];
    if (uiByteOffset >= tempData.m_uiStartByteOffset && uiByteEndOffset <= tempData.m_uiStartByteOffset + tempData.m_uiByteSize)
      return i;
  }

  return ezInvalidIndex;
}

ezUInt32 ezGALDynamicBuffer::AllocateTempData(ezUInt32 uiStartOffset, ezUInt32 uiMinCapacity, ezAllocator* pTempAllocator)
{
  if (uiMinCapacity > m_uiCapacity)
  {
    constexpr ezUInt32 uiExpGrowthLimit = 16 * 1024 * 1024;

    ezUInt32 uiNewCapacity = ezMath::Max(uiMinCapacity, 256U);
    if (uiNewCapacity < uiExpGrowthLimit)
    {
      uiNewCapacity = ezMath::PowerOfTwo_Ceil(uiNewCapacity);
    }
    else
    {
      uiNewCapacity = ezMemoryUtils::AlignSize(uiNewCapacity, uiExpGrowthLimit);
    }

    // UploadChangesForNextFrame changes the size of m_Data and the GPU buffer to this
    m_Desc.m_uiTotalSize = uiNewCapacity * m_Desc.m_uiStructSize;
    m_uiCapacity = uiNewCapacity;
  }

  if (pTempAllocator == nullptr)
  {
    pTempAllocator = ezFoundation::GetAlignedAllocator();
  }

  // The temp data always goes up to the capacity. Then the next allocations at the end of the buffer also fit into it.
  TempData& tempData = m_TempData.ExpandAndGetRef();
  tempData.m_pAllocator = pTempAllocator;
  tempData.m_uiByteSize = (m_uiCapacity - uiStartOffset) * m_Desc.m_uiStructSize;
  tempData.m_uiStartByteOffset = uiStartOffset * m_Desc.m_uiStructSize;
  tempData.m_pData = static_cast<ezUInt8*>(pTempAllocator->Allocate(tempData.m_uiByteSize, 16));

  // The GPU buffer may be created again with a new size, so all data has to be uploaded
  m_DirtyRange.SetToIncludeRange(0, (m_uiCapacity - 1));

  return m_TempData.GetCount();
}

void ezGALDynamicBuffer::UploadChangesForNextFrame()
{
  EZ_LOCK(m_Mutex);

  if (m_DirtyRange.IsValid() == false)
    return;

  // Build the final data buffer. After this, all pointers from earlier Map calls are invalid.
  m_Data.SetCountUninitialized(m_Desc.m_uiTotalSize);
  if (m_TempData.IsEmpty() == false)
  {
    // Temp data ranges can overlap each other, and they can overlap allocations in m_Data.
    // So don't copy the full temp data. Copy each allocation from the temp data where it is stored.
    for (auto it = m_Allocations.GetIterator(); it.IsValid(); ++it)
    {
      auto& allocation = it.Value();
      if (allocation.m_uiDataIndex == 0)
        continue;

      const auto& tempData = m_TempData[allocation.m_uiDataIndex - 1];
      const ezUInt32 uiByteOffset = it.Key() * m_Desc.m_uiStructSize;
      const ezUInt32 uiByteSize = allocation.m_uiCount * m_Desc.m_uiStructSize;
      ezMemoryUtils::Copy(&m_Data[uiByteOffset], tempData.m_pData + (uiByteOffset - tempData.m_uiStartByteOffset), uiByteSize);

      allocation.m_uiDataIndex = 0;
    }

    for (auto& tempData : m_TempData)
    {
      tempData.m_pAllocator->Deallocate(tempData.m_pData);
    }
    m_TempData.Clear();
  }

  auto pDevice = ezGALDevice::GetDefaultDevice();

  if (m_hBufferForUpload.IsInvalidated() == false && pDevice->GetBuffer(m_hBufferForUpload)->GetDescription().m_uiTotalSize != m_Desc.m_uiTotalSize)
  {
    pDevice->DestroyBuffer(m_hBufferForUpload);
    m_hBufferForUpload.Invalidate();
  }

  if (m_hBufferForUpload.IsInvalidated())
  {
    m_hBufferForUpload = pDevice->CreateBuffer(m_Desc, m_Data);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    pDevice->GetBuffer(m_hBufferForUpload)->SetDebugName(m_sDebugName);
#endif
  }
  else
  {
    const ezUInt32 uiByteOffset = m_DirtyRange.m_uiMin * m_Desc.m_uiStructSize;
    const ezUInt32 uiByteSize = m_DirtyRange.GetCount() * m_Desc.m_uiStructSize;
    auto data = m_Data.GetArrayPtr().GetSubArray(uiByteOffset, uiByteSize);

    pDevice->UpdateBufferForNextFrame(m_hBufferForUpload, data, uiByteOffset);
  }

  m_DirtyRange.Reset();
}

void ezGALDynamicBuffer::RunCompactionSteps(ezDynamicArray<ChangedAllocation>& out_changedAllocations, ezUInt32 uiMaxSteps)
{
  EZ_LOCK(m_Mutex);

  out_changedAllocations.Clear();

  if (m_FreeRanges.IsEmpty() || m_Allocations.IsEmpty())
    return;

  // Don't compact while there is temp data. MoveAllocation only works with m_Data.
  if (m_TempData.IsEmpty() == false)
    return;

  // Each step works on the hole with the lowest offset. Sorting by start in reverse puts this hole at the end of the array.
  m_FreeRanges.Sort(CompareRangesByStartReverse());
  EZ_SCOPE_EXIT(m_FreeRanges.Sort(CompareRangesByCount()));

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  EZ_SCOPE_EXIT(CheckSelf());
#endif

  auto MoveAllocation = [&](const Allocation& allocation, ezUInt32 uiOldOffset, ezUInt32 uiNewOffset)
  {
    out_changedAllocations.PushBack(ChangedAllocation{allocation.m_uiUserData, uiNewOffset});

    const ezUInt32 uiOldByteOffset = uiOldOffset * m_Desc.m_uiStructSize;
    const ezUInt32 uiNewByteOffset = uiNewOffset * m_Desc.m_uiStructSize;
    const ezUInt32 uiByteSize = allocation.m_uiCount * m_Desc.m_uiStructSize;
    // moving an allocation forward by less than its own size overlaps
    ezMemoryUtils::CopyOverlapped(&m_Data[uiNewByteOffset], &m_Data[uiOldByteOffset], uiByteSize);

    m_DirtyRange.SetToIncludeRange(uiNewOffset, uiNewOffset + allocation.m_uiCount - 1);

    m_Allocations.Insert(uiNewOffset, allocation);
    m_Allocations.Remove(uiOldOffset);
  };

  for (ezUInt32 i = 0; i < uiMaxSteps; ++i)
  {
    if (m_FreeRanges.IsEmpty())
      return;

    auto& freeRange = m_FreeRanges.PeekBack();
    const ezUInt32 uiFreeCount = freeRange.GetCount();
    const ezUInt32 uiNewOffset = freeRange.m_uiMin;

    // First check if the last allocation has exactly the size of the hole.
    // If yes, one move fills the hole and makes the used part of the buffer smaller.
    auto revIt = m_Allocations.GetReverseIterator();
    if (revIt.IsValid())
    {
      if (revIt.Value().m_uiCount == uiFreeCount)
      {
        m_FreeRanges.PopBack();

        // The old place of the last allocation is at the end of the buffer, so it does not become a free range
        m_uiNextOffset = revIt.Key();
        MoveAllocation(revIt.Value(), revIt.Key(), uiNewOffset);

        // A free range directly in front of the moved allocation is now at the end of the buffer.
        // The ranges are sorted by start in reverse, so it can only be the first one.
        if (!m_FreeRanges.IsEmpty() && m_FreeRanges[0].m_uiMax + 1 == m_uiNextOffset)
        {
          m_uiNextOffset = m_FreeRanges[0].m_uiMin;
          m_FreeRanges.RemoveAtAndCopy(0);
        }
        continue;
      }
    }

    // If not, move the allocation that comes directly after the hole to the start of the hole.
    // The hole then moves back by the size of that allocation, and it may join the next hole.
    // There is always an allocation directly after a free range, so Find() always succeeds.
    auto it = m_Allocations.Find(freeRange.m_uiMax + 1);
    EZ_ASSERT_DEV(it.IsValid(), "Implementation error");
    {
      const ezUInt32 uiNewFreeRangeMin = uiNewOffset + it.Value().m_uiCount;

      if (it.Key() == revIt.Key())
      {
        // This was the last allocation
        m_uiNextOffset = uiNewFreeRangeMin;
        m_FreeRanges.PopBack();
      }
      else
      {
        freeRange.m_uiMin = uiNewFreeRangeMin;
        freeRange.m_uiMax = freeRange.m_uiMin + uiFreeCount - 1;

        // merge adjacent free ranges
        if (m_FreeRanges.GetCount() > 1)
        {
          const ezUInt32 uiSecondIndex = m_FreeRanges.GetCount() - 2;
          auto& secondFreeRange = m_FreeRanges[uiSecondIndex];
          if (freeRange.m_uiMax + 1 == secondFreeRange.m_uiMin)
          {
            freeRange.m_uiMax = secondFreeRange.m_uiMax;
            m_FreeRanges.RemoveAtAndCopy(uiSecondIndex);
          }
        }
      }

      MoveAllocation(it.Value(), it.Key(), uiNewOffset);
    }
  }
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
void ezGALDynamicBuffer::CheckSelf() const
{
  // Disabled by default, because it checks all allocations on every call. Enable it when you change this class.
#  if 0
  if (m_uiNextOffset == 0 && m_Allocations.IsEmpty() && m_FreeRanges.IsEmpty())
    return;

  ezDynamicBitfield check;
  check.SetCount(m_uiNextOffset, false);

  for (auto it : m_Allocations)
  {
    const ezUInt32 uiStart = it.Key();
    const ezUInt32 uiCount = it.Value().m_uiCount;
    EZ_ASSERT_DEBUG(!check.IsAnyBitSet(uiStart, uiCount), "Overlapping allocation detected");
    check.SetBitRange(uiStart, uiCount);

    const ezUInt32 uiByteOffset = uiStart * m_Desc.m_uiStructSize;
    const ezUInt32 uiByteEndOffset = (uiStart + uiCount) * m_Desc.m_uiStructSize;
    if (it.Value().m_uiDataIndex == 0)
    {
      EZ_ASSERT_DEBUG(uiByteEndOffset <= m_Data.GetCount(), "Allocation is outside of the regular data");
    }
    else
    {
      const auto& tempData = m_TempData[it.Value().m_uiDataIndex - 1];
      EZ_ASSERT_DEBUG(uiByteOffset >= tempData.m_uiStartByteOffset && uiByteEndOffset <= tempData.m_uiStartByteOffset + tempData.m_uiByteSize, "Allocation is outside of its temp data");
    }
  }

  for (auto range : m_FreeRanges)
  {
    const ezUInt32 uiStart = range.m_uiMin;
    const ezUInt32 uiCount = range.GetCount();
    EZ_ASSERT_DEBUG(!check.IsAnyBitSet(uiStart, uiCount), "Overlapping free range detected");
    check.SetBitRange(uiStart, uiCount);

    EZ_ASSERT_DEBUG(m_Allocations.Contains(range.m_uiMax + 1), "Free range is not followed by an allocation");
  }

  EZ_ASSERT_DEBUG(check.AreAllBitsSet(), "Some memory is neither allocated nor free");
#  endif
}
#endif
