#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/MemoryUtils.h>

namespace
{
  // Some users access the data of a stream with SIMD types.
  constexpr ezUInt64 s_uiStreamAlignment = 16;
} // namespace

ezProcessingStreamGroup::ezProcessingStreamGroup()
{
  Clear();
}

ezProcessingStreamGroup::~ezProcessingStreamGroup()
{
  Clear();

  for (ezProcessingStream* pStream : m_UnusedStreams)
  {
    EZ_DEFAULT_DELETE(pStream);
  }
}

void ezProcessingStreamGroup::Clear()
{
  ClearProcessors();

  m_uiPendingNumberOfElementsToSpawn = 0;
  m_uiNumElements = 0;
  m_uiNumActiveElements = 0;
  m_uiHighestNumActiveElements = 0;
  m_bProcessorOrderDirty = true;
  m_bStreamLayoutDirty = false;

  m_UnusedStreams.PushBackRange(m_DataStreams);
  m_DataStreams.Clear();

  if (m_pStreamData != nullptr)
  {
    ezFoundation::GetAlignedAllocator()->Deallocate(m_pStreamData);
    m_pStreamData = nullptr;
  }
}

void ezProcessingStreamGroup::AddProcessor(ezProcessingStreamProcessor* pProcessor)
{
  EZ_ASSERT_DEV(pProcessor != nullptr, "Stream processor may not be null!");

  if (pProcessor->m_pStreamGroup != nullptr)
  {
    ezLog::Debug("Stream processor is already assigned to a stream group!");
    return;
  }

  m_Processors.PushBack(pProcessor);

  pProcessor->m_pStreamGroup = this;

  m_bProcessorOrderDirty = true;
}

void ezProcessingStreamGroup::RemoveProcessor(ezProcessingStreamProcessor* pProcessor)
{
  m_Processors.RemoveAndCopy(pProcessor);
  pProcessor->GetDynamicRTTI()->GetAllocator()->Deallocate(pProcessor);
}

void ezProcessingStreamGroup::ClearProcessors()
{
  m_bProcessorOrderDirty = true;

  for (ezProcessingStreamProcessor* pProcessor : m_Processors)
  {
    pProcessor->GetDynamicRTTI()->GetAllocator()->Deallocate(pProcessor);
  }

  m_Processors.Clear();
}

ezProcessingStream* ezProcessingStreamGroup::AddStream(const ezHashedString& sName, ezProcessingStream::DataType type)
{
  // Treat adding a stream two times as an error (return null)
  if (GetStreamByName(sName))
    return nullptr;

  ezProcessingStream* pStream = nullptr;

  if (!m_UnusedStreams.IsEmpty())
  {
    pStream = m_UnusedStreams.PeekBack();
    m_UnusedStreams.PopBack();

    *pStream = ezProcessingStream(sName, ezArrayPtr<ezUInt8>(), type);
  }
  else
  {
    pStream = EZ_DEFAULT_NEW(ezProcessingStream, sName, ezArrayPtr<ezUInt8>(), type);
  }

  m_DataStreams.PushBack(pStream);

  m_bStreamLayoutDirty = true;

  return pStream;
}

void ezProcessingStreamGroup::RemoveStreamByName(ezTempHashedString sName)
{
  for (ezUInt32 i = 0; i < m_DataStreams.GetCount(); ++i)
  {
    if (m_DataStreams[i]->GetName() == sName)
    {
      m_UnusedStreams.PushBack(m_DataStreams[i]);
      m_DataStreams.RemoveAtAndSwap(i);
      break;
    }
  }
}

ezProcessingStream* ezProcessingStreamGroup::GetStreamByName(ezTempHashedString sName) const
{
  for (ezProcessingStream* pStream : m_DataStreams)
  {
    if (pStream->GetName() == sName)
    {
      return pStream;
    }
  }

  return nullptr;
}

void ezProcessingStreamGroup::SetSize(ezUInt64 uiNumElements)
{
  if (m_uiNumElements == uiNumElements)
    return;

  m_uiNumElements = uiNumElements;

  // Also reset any pending remove and spawn operations since they refer to the old size and content
  m_PendingRemoveIndices.Clear();
  m_uiPendingNumberOfElementsToSpawn = 0;

  m_uiHighestNumActiveElements = 0;

  // The memory gets reallocated in Process().
  m_bStreamLayoutDirty = true;
}

/// Removes an element (e.g. due to the death of a particle etc.), this will be enqueued (and thus is safe to be called from within data
/// processors).
void ezProcessingStreamGroup::RemoveElement(ezUInt64 uiElementIndex)
{
  if (m_PendingRemoveIndices.Contains(uiElementIndex))
    return;

  EZ_ASSERT_DEBUG(uiElementIndex < m_uiNumActiveElements, "Element which should be removed is outside of active element range!");

  m_PendingRemoveIndices.PushBack(uiElementIndex);
}

/// Spawns a number of new elements, they will be added as newly initialized stream elements. Safe to call from data processors since the
/// spawning will be queued.
void ezProcessingStreamGroup::InitializeElements(ezUInt64 uiNumElements)
{
  m_uiPendingNumberOfElementsToSpawn += uiNumElements;
}

void ezProcessingStreamGroup::Process()
{
  if (m_bProcessorOrderDirty)
  {
    SortProcessorsByPriority();
    m_bProcessorOrderDirty = false;
  }

  if (m_bStreamLayoutDirty)
  {
    UpdateStreamLayout();
    m_bStreamLayoutDirty = false;
  }

  // TODO: Identify which processors work on which streams and find independent groups and use separate tasks for them?
  for (ezProcessingStreamProcessor* pStreamProcessor : m_Processors)
  {
    pStreamProcessor->Process(m_uiNumActiveElements);
  }

  // Run any pending deletions which happened due to stream processor execution
  RunPendingDeletions();

  // spawning here (instead of before processing) allows for particles to exist for exactly one frame
  // they will be created, initialized, then rendered, and the next Process() will already delete them
  RunPendingSpawns();
}


void ezProcessingStreamGroup::RunPendingDeletions()
{
  ezStreamGroupElementRemovedEvent e;
  e.m_pStreamGroup = this;

  // Remove elements
  while (!m_PendingRemoveIndices.IsEmpty())
  {
    if (m_uiNumActiveElements == 0)
      break;

    const ezUInt64 uiLastActiveElementIndex = m_uiNumActiveElements - 1;

    const ezUInt64 uiElementToRemove = m_PendingRemoveIndices.PeekBack();
    m_PendingRemoveIndices.PopBack();

    EZ_ASSERT_DEBUG(uiElementToRemove < m_uiNumActiveElements, "Invalid index to remove");

    // inform any interested party about the tragic death
    e.m_uiElementIndex = uiElementToRemove;
    m_ElementRemovedEvent.Broadcast(e);

    // If the element which should be removed is the last element we can just decrement the number of active elements
    // and no further work needs to be done
    if (uiElementToRemove == uiLastActiveElementIndex)
    {
      m_uiNumActiveElements--;
      continue;
    }

    // Since we swap with the last element we need to make sure that any pending removals of the (current) last element are updated
    // and point to the place where we moved the data to.
    for (ezUInt32 i = 0; i < m_PendingRemoveIndices.GetCount(); ++i)
    {
      // Is the pending remove in the array actually the last element we use to swap with? It's simply a matter of updating it to point to the new
      // index.
      if (m_PendingRemoveIndices[i] == uiLastActiveElementIndex)
      {
        m_PendingRemoveIndices[i] = uiElementToRemove;

        // We can break since the RemoveElement() operation takes care that each index can be in the array only once
        break;
      }
    }

    // Move the data
    for (ezProcessingStream* pStream : m_DataStreams)
    {
      const ezUInt64 uiStreamElementStride = pStream->GetElementStride();
      const ezUInt64 uiStreamElementSize = pStream->GetElementSize();
      const void* pSourceData = ezMemoryUtils::AddByteOffset(pStream->GetData(), static_cast<std::ptrdiff_t>(uiLastActiveElementIndex * uiStreamElementStride));
      void* pTargetData = ezMemoryUtils::AddByteOffset(pStream->GetWritableData(), static_cast<std::ptrdiff_t>(uiElementToRemove * uiStreamElementStride));

      ezMemoryUtils::Copy<ezUInt8>(static_cast<ezUInt8*>(pTargetData), static_cast<const ezUInt8*>(pSourceData), static_cast<size_t>(uiStreamElementSize));
    }

    // And decrease the size since we swapped the last element to the location of the element we just removed
    m_uiNumActiveElements--;
  }

  m_PendingRemoveIndices.Clear();
}

void ezProcessingStreamGroup::UpdateStreamLayout()
{
  ezUInt64 uiTotalSize = 0;
  for (const ezProcessingStream* pStream : m_DataStreams)
  {
    uiTotalSize = ezMemoryUtils::AlignSize(uiTotalSize, s_uiStreamAlignment) + m_uiNumElements * pStream->GetElementSize();
  }

  void* pNewData = nullptr;
  if (uiTotalSize > 0)
  {
    pNewData = ezFoundation::GetAlignedAllocator()->Allocate(static_cast<size_t>(uiTotalSize), static_cast<size_t>(s_uiStreamAlignment));
  }

  // Elements that don't fit anymore are lost.
  const ezUInt64 uiNumElementsToKeep = ezMath::Min(m_uiNumActiveElements, m_uiNumElements);

  ezUInt64 uiOffset = 0;
  for (ezProcessingStream* pStream : m_DataStreams)
  {
    uiOffset = ezMemoryUtils::AlignSize(uiOffset, s_uiStreamAlignment);

    const ezUInt64 uiElementSize = pStream->GetElementSize();
    const ezUInt64 uiStreamSize = m_uiNumElements * uiElementSize;

    void* pStreamData = uiStreamSize > 0 ? ezMemoryUtils::AddByteOffset(pNewData, static_cast<std::ptrdiff_t>(uiOffset)) : nullptr;

    // Particles stay alive when a reloaded effect needs a different set of streams. New streams don't have any data yet.
    if (pStream->m_pData != nullptr && pStreamData != nullptr)
    {
      ezMemoryUtils::RawByteCopy(pStreamData, pStream->m_pData, static_cast<size_t>(uiNumElementsToKeep * uiElementSize));
    }

    pStream->m_pData = pStreamData;
    pStream->m_uiDataSize = uiStreamSize;

    uiOffset += uiStreamSize;
  }

  if (m_pStreamData != nullptr)
  {
    ezFoundation::GetAlignedAllocator()->Deallocate(m_pStreamData);
  }

  m_pStreamData = pNewData;
}

void ezProcessingStreamGroup::RunPendingSpawns()
{
  // Check if elements need to be spawned. If this is the case spawn them. (This is limited by the maximum number of elements).
  if (m_uiPendingNumberOfElementsToSpawn > 0)
  {
    m_uiPendingNumberOfElementsToSpawn = ezMath::Min(m_uiPendingNumberOfElementsToSpawn, m_uiNumElements - m_uiNumActiveElements);

    if (m_uiPendingNumberOfElementsToSpawn)
    {
      for (ezProcessingStreamProcessor* pSpawner : m_Processors)
      {
        pSpawner->InitializeElements(m_uiNumActiveElements, m_uiPendingNumberOfElementsToSpawn);
      }
    }

    m_uiNumActiveElements += m_uiPendingNumberOfElementsToSpawn;

    m_uiHighestNumActiveElements = ezMath::Max(m_uiNumActiveElements, m_uiHighestNumActiveElements);

    m_uiPendingNumberOfElementsToSpawn = 0;
  }
}

struct ProcessorComparer
{
  EZ_ALWAYS_INLINE bool Less(const ezProcessingStreamProcessor* a, const ezProcessingStreamProcessor* b) const { return a->m_fPriority < b->m_fPriority; }
};

void ezProcessingStreamGroup::SortProcessorsByPriority()
{
  ProcessorComparer cmp;
  m_Processors.Sort(cmp);
}
