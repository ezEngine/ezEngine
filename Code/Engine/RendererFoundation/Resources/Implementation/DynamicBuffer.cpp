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

  ezUInt32 uiOffset = ezInvalidIndex;

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

  if (uiOffset == ezInvalidIndex)
  {
    uiOffset = m_uiNextOffset;
    m_uiNextOffset += uiCount;

    if (m_uiNextOffset > m_uiCapacity)
    {
      AllocateTempData(uiOffset, m_uiNextOffset, pTempAllocator);
    }
  }

  ezUInt32 uiDataIndex = 0;
  const ezUInt32 uiByteEndOffset = (uiOffset + uiCount) * m_Desc.m_uiStructSize;
  if (uiByteEndOffset > m_Data.GetCount())
  {
    for (ezUInt32 i = 0; i < m_TempData.GetCount(); ++i)
    {
      auto& tempData = m_TempData[i];
      if (uiByteEndOffset <= (tempData.m_uiStartByteOffset + tempData.m_uiByteSize))
      {
        uiDataIndex = i + 1;
        break;
      }
    }
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
    m_uiNextOffset = uiOffset;

    // Remove free range in front of the last allocation
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

ezUInt32 ezGALDynamicBuffer::AllocateTempData(ezUInt32 uiStartOffset, ezUInt32 uiNewCount, ezAllocator* pTempAllocator)
{
  constexpr ezUInt32 uiExpGrowthLimit = 16 * 1024 * 1024;

  uiNewCount = ezMath::Max(uiNewCount, 256U);
  if (uiNewCount < uiExpGrowthLimit)
  {
    uiNewCount = ezMath::PowerOfTwo_Ceil(uiNewCount);
  }
  else
  {
    uiNewCount = ezMemoryUtils::AlignSize(uiNewCount, uiExpGrowthLimit);
  }

  m_Desc.m_uiTotalSize = uiNewCount * m_Desc.m_uiStructSize;
  m_uiCapacity = uiNewCount;

  if (pTempAllocator == nullptr)
  {
    pTempAllocator = ezFoundation::GetAlignedAllocator();
  }

  TempData& tempData = m_TempData.ExpandAndGetRef();
  tempData.m_pAllocator = pTempAllocator;
  tempData.m_uiByteSize = (uiNewCount - uiStartOffset) * m_Desc.m_uiStructSize;
  tempData.m_uiStartByteOffset = uiStartOffset * m_Desc.m_uiStructSize;
  tempData.m_pData = static_cast<ezUInt8*>(pTempAllocator->Allocate(tempData.m_uiByteSize, 16));

  m_DirtyRange.SetToIncludeRange(0, (uiNewCount - 1));

  return m_TempData.GetCount();
}

void ezGALDynamicBuffer::UploadChangesForNextFrame()
{
  EZ_LOCK(m_Mutex);

  if (m_DirtyRange.IsValid() == false)
    return;

  // Assemble final data buffer
  m_Data.SetCountUninitialized(m_Desc.m_uiTotalSize);
  for (auto& tempData : m_TempData)
  {
    ezMemoryUtils::Copy(&m_Data[tempData.m_uiStartByteOffset], tempData.m_pData, tempData.m_uiByteSize);
    tempData.m_pAllocator->Deallocate(tempData.m_pData);
  }
  m_TempData.Clear();

  // Patch data indices
  for (auto it = m_Allocations.GetReverseIterator(); it.IsValid(); ++it)
  {
    auto& allocation = it.Value();
    if (allocation.m_uiDataIndex == 0)
      break;

    allocation.m_uiDataIndex = 0;
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

  // Don't try to compact when we still have temporary data
  if (m_TempData.IsEmpty() == false)
    return;

  m_FreeRanges.Sort(CompareRangesByStartReverse());
  EZ_SCOPE_EXIT(m_FreeRanges.Sort(CompareRangesByCount()));

  auto MoveAllocation = [&](const Allocation& allocation, ezUInt32 uiOldOffset, ezUInt32 uiNewOffset)
  {
    out_changedAllocations.PushBack(ChangedAllocation{allocation.m_uiUserData, uiNewOffset});

    const ezUInt32 uiOldByteOffset = uiOldOffset * m_Desc.m_uiStructSize;
    const ezUInt32 uiNewByteOffset = uiNewOffset * m_Desc.m_uiStructSize;
    const ezUInt32 uiByteSize = allocation.m_uiCount * m_Desc.m_uiStructSize;
    ezMemoryUtils::Copy(&m_Data[uiNewByteOffset], &m_Data[uiOldByteOffset], uiByteSize);

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

    // first check whether the last allocation fits into the hole
    auto revIt = m_Allocations.GetReverseIterator();
    if (revIt.IsValid())
    {
      if (revIt.Value().m_uiCount == uiFreeCount)
      {
        m_FreeRanges.PopBack();

        m_uiNextOffset = revIt.Key();
        MoveAllocation(revIt.Value(), revIt.Key(), uiNewOffset);
        continue;
      }
    }

    // if not start to move the allocations forward
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

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  CheckSelf();
#endif
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
void ezGALDynamicBuffer::CheckSelf() const
{
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
  }

  for (auto range : m_FreeRanges)
  {
    const ezUInt32 uiStart = range.m_uiMin;
    const ezUInt32 uiCount = range.GetCount();
    EZ_ASSERT_DEBUG(!check.IsAnyBitSet(uiStart, uiCount), "Overlapping free range detected");
    check.SetBitRange(uiStart, uiCount);
  }

  EZ_ASSERT_DEBUG(check.AreAllBitsSet(), "Some memory is neither allocated nor free");
#  endif
}
#endif
