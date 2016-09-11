
#include <CoreUtils/PCH.h>
#include <CoreUtils/Basics.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>
#include <CoreUtils/DataProcessing/Stream/StreamProcessor.h>
#include <CoreUtils/DataProcessing/Stream/StreamElementSpawner.h>
#include <CoreUtils/DataProcessing/Stream/Stream.h>
#include <Foundation/Memory/MemoryUtils.h>

ezProcessingStreamGroup::ezProcessingStreamGroup()
{
  Clear();
}

ezProcessingStreamGroup::~ezProcessingStreamGroup()
{
  Clear();
}

void ezProcessingStreamGroup::Clear()
{
  ClearStreamProcessors();
  ClearStreamElementSpawners();

  m_uiPendingNumberOfElementsToSpawn = 0;
  m_uiNumElements = 0;
  m_uiNumActiveElements = 0;
  m_uiHighestNumActiveElements = 0;
  m_bStreamAssignmentDirty = true;

  for (ezProcessingStream* pStream : m_DataStreams)
  {
    EZ_DEFAULT_DELETE(pStream);
  }

  m_DataStreams.Clear();
}

void ezProcessingStreamGroup::AddStreamProcessor(ezProcessingStreamProcessor* pStreamProcessor)
{
  EZ_ASSERT_DEV(pStreamProcessor != nullptr, "Stream processor may not be null!");

  if (pStreamProcessor->m_pStreamGroup != nullptr)
  {
    ezLog::Debug("Stream processor is already assigned to a stream group!");
    return;
  }

  m_StreamProcessors.PushBack(pStreamProcessor);

  pStreamProcessor->m_pStreamGroup = this;

  m_bStreamAssignmentDirty = true;
}

void ezProcessingStreamGroup::RemoveStreamProcessor(ezProcessingStreamProcessor* pStreamProcessor)
{
  m_StreamProcessors.Remove(pStreamProcessor);
  EZ_DEFAULT_DELETE(pStreamProcessor);
}

void ezProcessingStreamGroup::ClearStreamProcessors()
{
  m_bStreamAssignmentDirty = true;

  for (ezProcessingStreamProcessor* pStreamProcessor : m_StreamProcessors)
  {
    EZ_DEFAULT_DELETE(pStreamProcessor);
  }

  m_StreamProcessors.Clear();
}

void ezProcessingStreamGroup::AddStreamElementSpawner(ezProcessingStreamSpawner* pStreamElementSpawner)
{
  EZ_ASSERT_DEV(pStreamElementSpawner != nullptr, "Stream element spawner may not be null!");

  if (pStreamElementSpawner->m_pStreamGroup != nullptr)
  {
    ezLog::Debug("Stream element spawner is already assigned to a stream group!");
    return;
  }

  m_StreamElementSpawners.PushBack(pStreamElementSpawner);

  pStreamElementSpawner->m_pStreamGroup = this;

  m_bStreamAssignmentDirty = true;
}

void ezProcessingStreamGroup::RemoveStreamElementSpawner(ezProcessingStreamSpawner* pStreamElementSpawner)
{
  EZ_VERIFY(m_StreamElementSpawners.Remove(pStreamElementSpawner), "Invalid spawner, not part of this group");
  EZ_DEFAULT_DELETE(pStreamElementSpawner);
}

void ezProcessingStreamGroup::ClearStreamElementSpawners()
{
  m_bStreamAssignmentDirty = true;

  for (ezProcessingStreamSpawner* pStreamElementSpawner : m_StreamElementSpawners)
  {
    EZ_DEFAULT_DELETE(pStreamElementSpawner);
  }

  m_StreamElementSpawners.Clear();
}

ezProcessingStream* ezProcessingStreamGroup::AddStream(const char* szName, ezProcessingStream::DataType Type)
{
  // Treat adding a stream two times as an error (return null)
  if (GetStreamByName(szName))
    return nullptr;

  ezProcessingStream* pStream = EZ_DEFAULT_NEW(ezProcessingStream, szName, Type, 64);

  m_DataStreams.PushBack(pStream);

  m_bStreamAssignmentDirty = true;

  return pStream;
}

void ezProcessingStreamGroup::RemoveStreamByName(const char* szName)
{
  ezHashedString Name;
  Name.Assign(szName);

  for (ezUInt32 i = 0; i < m_DataStreams.GetCount(); ++i)
  {
    if (m_DataStreams[i]->GetName() == Name)
    {
      EZ_DEFAULT_DELETE(m_DataStreams[i]);
      m_DataStreams.RemoveAtSwap(i);

      m_bStreamAssignmentDirty = true;
      break;
    }
  }
}

ezProcessingStream* ezProcessingStreamGroup::GetStreamByName(const char* szName) const
{
  ezHashedString Name;
  Name.Assign(szName);

  for (ezProcessingStream* Stream : m_DataStreams)
  {
    if (Stream->GetName() == Name)
    {
      return Stream;
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

  // Stream processors etc. may have pointers to the stream data for some reason.
  m_bStreamAssignmentDirty = true;
}

/// \brief Removes an element (e.g. due to the death of a particle etc.), this will be enqueued (and thus is safe to be called from within data processors).
void ezProcessingStreamGroup::RemoveElement(ezUInt64 uiElementIndex)
{
  if (m_PendingRemoveIndices.Contains(uiElementIndex))
    return;

  EZ_ASSERT_DEBUG(uiElementIndex < m_uiNumElements, "Element which should be removed is outside of active element range!");

  m_PendingRemoveIndices.PushBack(uiElementIndex);
}

/// \brief Spawns a number of new elements, they will be added as newly initialized stream elements. Safe to call from data processors since the spawning will be queued.
void ezProcessingStreamGroup::SpawnElements(ezUInt64 uiNumElements)
{
  m_uiPendingNumberOfElementsToSpawn += uiNumElements;
}

void ezProcessingStreamGroup::Process()
{
  EnsureStreamAssignmentValid();

  // Run any pending operations the user may have triggered after running Process() the last time
  RunPendingDeletions();
  RunPendingSpawns();

  // TODO: Identify which processors work on which streams and find independent groups and use separate tasks for them?
  for (ezProcessingStreamProcessor* pStreamProcessor : m_StreamProcessors)
  {
    pStreamProcessor->Process(m_uiNumActiveElements);
  }

  // Run any pending deletions which happened due to stream processor execution
  RunPendingDeletions();

  // do not spawn here anymore, delay that to the next round
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

    ezUInt64 uiLastActiveElementIndex = m_uiNumActiveElements - 1;

    ezUInt64 uiElementToRemove = m_PendingRemoveIndices.PeekBack();
    m_PendingRemoveIndices.PopBack();

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
      // Is the pending remove in the array actually the last element we use to swap with? It's simply a matter of updating it to point to the new index.
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
      const void* pSourceData = ezMemoryUtils::AddByteOffsetConst(pStream->GetData(), static_cast<ptrdiff_t>(uiLastActiveElementIndex * uiStreamElementStride));
      void* pTargetData = ezMemoryUtils::AddByteOffset(pStream->GetWritableData(), static_cast<ptrdiff_t>(uiElementToRemove * uiStreamElementStride));

      ezMemoryUtils::Copy<ezUInt8>(static_cast<ezUInt8*>(pTargetData), static_cast<const ezUInt8*>(pSourceData), static_cast<size_t>(uiStreamElementSize));
    }

    // And decrease the size since we swapped the last element to the location of the element we just removed
    m_uiNumActiveElements--;
  }

  m_PendingRemoveIndices.Clear();
}

void ezProcessingStreamGroup::EnsureStreamAssignmentValid()
{
  // If any stream processors or streams were added we may need to inform them.
  if (m_bStreamAssignmentDirty)
  {
    // Set the new size on all stream.
    for (ezProcessingStream* Stream : m_DataStreams)
    {
      Stream->SetSize(m_uiNumElements);
    }

    for (ezProcessingStreamProcessor* pStreamProcessor : m_StreamProcessors)
    {
      pStreamProcessor->UpdateStreamBindings();
    }

    for (ezProcessingStreamSpawner* pStreamElementSpawner : m_StreamElementSpawners)
    {
      pStreamElementSpawner->UpdateStreamBindings();
    }

    m_bStreamAssignmentDirty = false;
  }
}

void ezProcessingStreamGroup::RunPendingSpawns()
{
  // Check if elements need to be spawned. If this is the case spawn them. (This is limited by the maximum number of elements).
  if (m_uiPendingNumberOfElementsToSpawn > 0)
  {
    m_uiPendingNumberOfElementsToSpawn = ezMath::Min(m_uiPendingNumberOfElementsToSpawn, m_uiNumElements - m_uiNumActiveElements);

    if (m_uiPendingNumberOfElementsToSpawn)
    {
      for (ezProcessingStreamSpawner* pStreamElementSpawner : m_StreamElementSpawners)
      {
        pStreamElementSpawner->SpawnElements(m_uiNumActiveElements, m_uiPendingNumberOfElementsToSpawn);
      }
    }

    m_uiNumActiveElements += m_uiPendingNumberOfElementsToSpawn;

    m_uiHighestNumActiveElements = ezMath::Max(m_uiNumActiveElements, m_uiHighestNumActiveElements);

    m_uiPendingNumberOfElementsToSpawn = 0;
  }
}