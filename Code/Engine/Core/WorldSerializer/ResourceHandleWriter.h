#pragma once

#include <Core/Basics.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HashTable.h>

class ezResourceBase;
class ezStreamWriter;

/// \brief Used to write resource references to files. Duplicate references and storage requirements are optimized.
///
/// Every resource reference is written only once. All handles instead only store an ID. A lookup table is written at the
/// end of the stream to store information about which handle ID maps to which actual resource.
///
/// Create an instance of this class and call BeginWritingToStream(). Then call WriteHandle() for every handle to store.
/// The function is static and can be called from everywhere, without the need to pass through this instance.
/// Instead a thread local variable is used to keep track which instance is currently active. Therefore multiple instances
/// can be used on different threads.
/// Finally call EndWritingToStream() to store the gathered lookup table to the stream.
class EZ_CORE_DLL ezResourceHandleWriteContext
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezResourceHandleWriteContext);

public:
  ezResourceHandleWriteContext();
  ~ezResourceHandleWriteContext();

  /// \brief Call this before writing any handles
  void BeginWritingToStream(ezStreamWriter* pStream);

  /// \brief Call this after all handles have been written.
  void EndWritingToStream(ezStreamWriter* pStream);

  /// \brief Use this static function to write a handle to the given stream.
  ///
  /// The function will use the ezResourceHandleWriteContext instance that is currently active.
  /// Ie. the thread local instance on which BeginWritingToStream was called recently.
  ///
  /// Multiple instances can be used on different threads, but on each thread only one instance can be active
  /// at a time.
  template <typename ResourceType>
  static void WriteHandle(ezStreamWriter* pStream, const ezTypedResourceHandle<ResourceType>& hResource)
  {
    WriteHandle(pStream, hResource.m_Typeless.m_pResource);
  }

private:
  static void WriteHandle(ezStreamWriter* pStream, const ezResourceBase* pResource);
  void WriteResourceReference(ezStreamWriter* pStream, const ezResourceBase* pResource);

  ezHashTable<const ezResourceBase*, ezUInt32> m_StoredHandles;
  ezDeque<const ezResourceBase*> m_ResourcesToStore;

  enum class State
  {
    NotStarted,
    Writing,
    Finished,
  };

  State m_State;
};

/// \brief Operator to serialize resource handles
template <typename ResourceType>
void operator<<(ezStreamWriter& Stream, ezTypedResourceHandle<ResourceType> Value)
{
  ezResourceHandleWriteContext::WriteHandle(&Stream, Value);
}
