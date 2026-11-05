
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>

class ezProcessingStreamProcessor;
class ezProcessingStreamGroup;

struct ezStreamGroupElementRemovedEvent
{
  ezProcessingStreamGroup* m_pStreamGroup;
  ezUInt64 m_uiElementIndex;
};

struct ezStreamGroupElementsClearedEvent
{
  ezProcessingStreamGroup* m_pStreamGroup;
};

/// A stream group encapsulates the streams and the corresponding data processors.
class EZ_FOUNDATION_DLL ezProcessingStreamGroup
{
public:
  /// Constructor
  ezProcessingStreamGroup();

  /// Destructor
  ~ezProcessingStreamGroup();

  /// Removes all streams and processors and frees the stream data.
  ///
  /// The stream objects are kept and reused by later AddStream() calls, so an old pointer may end up referencing a different stream.
  void Clear();

  /// Adds a stream processor to the stream group.
  /// Ownership is transferred to the stream group and the processor will be deallocated using the RTTI deallocator on destruction.
  /// Processors are executed in the order they are added to the stream group.
  void AddProcessor(ezProcessingStreamProcessor* pProcessor);

  /// Removes the given stream processor from the group.
  void RemoveProcessor(ezProcessingStreamProcessor* pProcessor);

  /// Removes all stream processors from the group.
  void ClearProcessors();

  /// Adds a stream with the given name to the stream group. Adding a stream two times with the same name will return nullptr for the second
  /// attempt to signal an error.
  ///
  /// The returned pointer stays valid until the stream is removed or Clear() is called, also when the group gets resized.
  ///
  /// All streams share one memory block, which gets allocated by the next Process().
  /// So until then the new stream has no data, and pointers to the data of all other streams become invalid at that point.
  /// The active elements of the other streams are preserved. For the new stream they are uninitialized.
  ezProcessingStream* AddStream(const ezHashedString& sName, ezProcessingStream::DataType type);

  /// Removes the stream with the given name, if it exists.
  ///
  /// The memory of the stream stays unused until the memory block gets reallocated. Data pointers of other streams stay valid.
  void RemoveStreamByName(ezTempHashedString sName);

  /// Returns the stream by it's name, returns nullptr if not existent. More efficient since direct use of ezHashedString.
  ezProcessingStream* GetStreamByName(ezTempHashedString sName) const;

  /// Resizes all streams to contain storage for uiNumElements. Any pending remove and spawn operations will be reset!
  void SetSize(ezUInt64 uiNumElements);

  /// Removes an element (e.g. due to the death of a particle etc.), this will be enqueued (and thus is safe to be called from within data
  /// processors).
  void RemoveElement(ezUInt64 uiElementIndex);

  /// Spawns a number of new elements, they will be added as newly initialized stream elements. Safe to call from data processors since the
  /// spawning will be queued.
  void InitializeElements(ezUInt64 uiNumElements);

  /// Runs the stream processors which have been added to the stream group.
  void Process();

  /// Returns the number of elements the streams store.
  inline ezUInt64 GetNumElements() const { return m_uiNumElements; }

  /// Returns the number of currently active elements.
  inline ezUInt64 GetNumActiveElements() const { return m_uiNumActiveElements; }

  /// Returns the highest number of active elements since the last SetSize() call.
  inline ezUInt64 GetHighestNumActiveElements() const { return m_uiHighestNumActiveElements; }

  /// Subscribe to this event to be informed when (shortly before) items are deleted.
  ezEvent<const ezStreamGroupElementRemovedEvent&> m_ElementRemovedEvent;

private:
  /// Internal helper function which removes any pending elements and spawns new elements as needed
  void RunPendingDeletions();

  void RunPendingSpawns();

  void SortProcessorsByPriority();

  /// Allocates one memory block that exactly fits all streams and moves the active elements of the existing streams into it.
  void UpdateStreamLayout();

  ezHybridArray<ezProcessingStreamProcessor*, 8> m_Processors;

  ezHybridArray<ezProcessingStream*, 8> m_DataStreams;

  /// Stream objects from removed streams. Those are reused, since the group gets cleared and set up again every time a particle system is reused.
  ezHybridArray<ezProcessingStream*, 8> m_UnusedStreams;

  void* m_pStreamData = nullptr;

  ezHybridArray<ezUInt64, 64> m_PendingRemoveIndices;

  ezUInt64 m_uiPendingNumberOfElementsToSpawn;

  ezUInt64 m_uiNumElements;

  ezUInt64 m_uiNumActiveElements;

  ezUInt64 m_uiHighestNumActiveElements;

  bool m_bProcessorOrderDirty;

  bool m_bStreamLayoutDirty;
};
