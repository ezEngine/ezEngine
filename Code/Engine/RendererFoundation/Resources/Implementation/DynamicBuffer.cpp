#include <RendererFoundation/RendererFoundationPCH.h>

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

  m_Desc = desc;
  m_Data.SetCountUninitialized(desc.m_uiTotalSize);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName = sDebugName;
#endif
}

void ezGALDynamicBuffer::Deinitialize()
{
  m_Data.Clear();
  m_Allocations.Clear();
  m_FreeRanges.Clear();
  m_DirtyRange.Reset();

  if (m_hBufferForUpload.IsInvalidated() == false)
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hBufferForUpload);
  }

  if (m_hBufferForRendering.IsInvalidated() == false && m_hBufferForRendering != m_hBufferForUpload)
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hBufferForRendering);
  }

  m_hBufferForUpload.Invalidate();
  m_hBufferForRendering.Invalidate();
}

ezUInt32 ezGALDynamicBuffer::Allocate(ezUInt64 uiUserData, ezUInt32 uiCount)
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

    const ezUInt32 uiTotalByteSize = m_uiNextOffset * m_Desc.m_uiStructSize;
    if (uiTotalByteSize > m_Desc.m_uiTotalSize)
    {
      Resize(uiTotalByteSize);
    }
  }

  m_Allocations.Insert(uiOffset, Allocation{uiUserData, uiCount});
  return uiOffset;
}

void ezGALDynamicBuffer::Deallocate(ezUInt32 uiOffset)
{
  EZ_LOCK(m_Mutex);

  auto it = m_Allocations.Find(uiOffset);
  EZ_ASSERT_DEV(it.IsValid(), "Invalid offset");

  const ezUInt32 uiCount = it.Value().m_uiCount;
  m_Allocations.Remove(it);

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

ezByteArrayPtr ezGALDynamicBuffer::MapForWriting(ezUInt32 uiOffset, ezUInt32& out_uiCount)
{
  EZ_LOCK(m_Mutex);

  auto it = m_Allocations.Find(uiOffset);
  EZ_ASSERT_DEV(it.IsValid(), "Invalid offset");

  out_uiCount = it.Value().m_uiCount;
  m_DirtyRange.SetToIncludeRange(uiOffset, uiOffset + out_uiCount - 1);

  const ezUInt32 uiByteOffset = uiOffset * m_Desc.m_uiStructSize;
  const ezUInt32 uiByteSize = out_uiCount * m_Desc.m_uiStructSize;
  return m_Data.GetByteArrayPtr().GetSubArray(uiByteOffset, uiByteSize);
}

void ezGALDynamicBuffer::UploadChanges()
{
  EZ_LOCK(m_Mutex);

  if (m_DirtyRange.IsValid() == false)
    return;

  auto pDevice = ezGALDevice::GetDefaultDevice();

  if (m_hBufferForUpload.IsInvalidated() == false && pDevice->GetBuffer(m_hBufferForUpload)->GetDescription().m_uiTotalSize != m_Desc.m_uiTotalSize)
  {
    pDevice->DestroyBuffer(m_hBufferForUpload);
    m_hBufferForUpload.Invalidate();
  }

  if (m_hBufferForUpload.IsInvalidated())
  {
    m_hBufferForUpload = pDevice->CreateBuffer(m_Desc, m_Data);
    pDevice->GetBuffer(m_hBufferForUpload)->SetDebugName(m_sDebugName);
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

  // EZ_ASSERT_NOT_IMPLEMENTED;
  EZ_IGNORE_UNUSED(uiMaxSteps);
}

void ezGALDynamicBuffer::Resize(ezUInt32 uiNewSize)
{
  constexpr ezUInt32 uiExpGrowthLimit = 16 * 1024 * 1024;

  ezUInt32 uiSize = ezMath::Max(uiNewSize, 256U);
  if (uiSize < uiExpGrowthLimit)
  {
    uiSize = ezMath::PowerOfTwo_Ceil(uiSize);
  }
  else
  {
    uiSize = ezMemoryUtils::AlignSize(uiSize, uiExpGrowthLimit);
  }

  m_Desc.m_uiTotalSize = uiSize;
  m_Data.SetCountUninitialized(uiSize);

  m_DirtyRange.SetToIncludeRange(0, (uiSize / m_Desc.m_uiStructSize) - 1);
}
