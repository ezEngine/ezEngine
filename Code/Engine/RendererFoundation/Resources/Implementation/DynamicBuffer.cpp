#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/DynamicBuffer.h>

ezIdTable<ezGALDynamicBufferHandle::IdType, ezGALDynamicBuffer*> ezGALDynamicBuffer::s_Buffers;
static bool s_bInitialized = false;
static ezMutex s_Mutex;

// static
ezGALDynamicBufferHandle ezGALDynamicBuffer::Create(const ezGALBufferCreationDescription& desc, ezStringView sDebugName)
{
  EZ_LOCK(s_Mutex);

  if (!s_bInitialized)
  {
    ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&ezGALDynamicBuffer::DeviceEventHandler));
    s_bInitialized = true;
  }

  auto pBuffer = EZ_DEFAULT_NEW(ezGALDynamicBuffer);
  pBuffer->Initialize(desc, sDebugName);

  return ezGALDynamicBufferHandle(s_Buffers.Insert(pBuffer));
}

// static
void ezGALDynamicBuffer::Destroy(ezGALDynamicBufferHandle& inout_hBuffer)
{
  if (inout_hBuffer.IsInvalidated())
    return;

  EZ_LOCK(s_Mutex);

  ezGALDynamicBuffer* pOldBuffer = nullptr;
  s_Buffers.Remove(inout_hBuffer, &pOldBuffer);

  EZ_DEFAULT_DELETE(pOldBuffer);

  inout_hBuffer.Invalidate();
}

// static
ezGALDynamicBuffer* ezGALDynamicBuffer::Get(ezGALDynamicBufferHandle hBuffer)
{
  EZ_LOCK(s_Mutex);

  ezGALDynamicBuffer* pBuffer = nullptr;
  s_Buffers.TryGetValue(hBuffer, pBuffer);
  return pBuffer;
}

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
  ezUInt32 uiOffset = ezInvalidIndex;

  for (ezUInt32 i = 0; i < m_FreeRanges.GetCount(); ++i)
  {
    auto& freeRange = m_FreeRanges[i];
    const ezUInt32 uiFreeCount = freeRange.GetCount();

    if (uiFreeCount >= uiCount)
    {
      if (uiFreeCount == uiCount)
      {
        m_FreeRanges.RemoveAtAndCopy(i);
      }
      else
      {
        freeRange.m_uiMin += uiCount;
      }

      uiOffset = freeRange.m_uiMin;
      break;
    }
  }

  if (uiOffset == ezInvalidIndex)
  {
    uiOffset = m_uiNextOffset;
    m_uiNextOffset += uiCount;

    const ezUInt32 uiTotalByteSize = m_Data.GetCount() + (uiCount * m_Desc.m_uiStructSize);
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
  auto it = m_Allocations.Find(uiOffset);
  EZ_ASSERT_DEV(it.IsValid(), "Invalid offset");

  // const ezUInt32 uiCount = it.Value().m_uiCount;
  m_Allocations.Remove(it);

  // First check for adjacent free ranges
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezByteArrayPtr ezGALDynamicBuffer::MapForWriting(ezUInt32 uiOffset, ezUInt32& out_uiCount)
{
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
  out_changedAllocations.Clear();

  //EZ_ASSERT_NOT_IMPLEMENTED;
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

void ezGALDynamicBuffer::DeviceEventHandler(const ezGALDeviceEvent& e)
{
  if (e.m_Type != ezGALDeviceEvent::AfterBeginFrame)
    return;

  EZ_LOCK(s_Mutex);

  for (auto it = s_Buffers.GetIterator(); it.IsValid(); ++it)
  {
    auto pBuffer = it.Value();
    pBuffer->m_hBufferForRendering = pBuffer->m_hBufferForUpload;
  }
}
