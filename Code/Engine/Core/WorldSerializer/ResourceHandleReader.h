#pragma once

#include <Core/Basics.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>

class ezResource;
class ezStreamReader;

/// \brief Used in conjunction with ezResoruceHandleWriteContext to restore serialized resource handles
///
/// Only one instance can be 'active' on each thread, ie. BeginRestoringHandles() can only have been called on one instance recently.
/// However, multiple instances can be used to do parallel loading on multiple threads.
///
/// Reading the vital information for restoring handles, and actually restoring handles is separated into two operations
/// which can be done interleaved (standard sequential reading from a file), or in separate steps, to allow to apply the handle
/// restoration multiple times (for instantiating the same data over and over, ie. for prefab creation).
class EZ_CORE_DLL ezResourceHandleReadContext
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezResourceHandleReadContext);

public:
  ezResourceHandleReadContext();
  ~ezResourceHandleReadContext();

  /// \brief Call this at the equivalent point where ezResourceHandleWriteContext::BeginWritingToStream() was called.
  void BeginReadingFromStream(ezStreamReader* pStream);

  /// \brief Call this at the equivalent point where ezResourceHandleWriteContext::EndWritingToStream() was called.
  void EndReadingFromStream(ezStreamReader* pStream);

  /// \brief Call this to queue a resource handle for restoration, that was written with ezResourceHandleWriteContext::WriteHandle().
  ///
  /// Note that the handle is NOT valid after this call. Resource handles are only restored during EndRestoringHandles() and thus
  /// will only become valid after that.
  template <typename ResourceType>
  static void ReadHandle(ezStreamReader* pStream, ezTypedResourceHandle<ResourceType>& ResourceHandle)
  {
    ReadHandle(pStream, &ResourceHandle.m_Typeless);
  }

  /// \brief Call this before doing the first call to ReadHandle(). In a typical scenario that is immediately after
  /// BeginReadingFromStream().
  ///
  /// In a non-typical scenario, where the data between BeginReadingFromStream() and EndReadingFromStream() is cached and potentially
  /// read only later (multiple times), BeginRestoringHandles() can be called every time the cached memory block is re-read.
  /// This is what ezWorldReader does to allow for instantiating the same world multiple times (as a prefab).
  void BeginRestoringHandles(ezStreamReader* pStream);

  /// \brief Call this after all handles have been read to finalize the resource restoration.
  ///
  /// Note: This must be called AFTER EndReadingFromStream() was called, as it requires the data read by that function.
  void EndRestoringHandles();

  /// \brief Resets all internal state such that the reader can be reused.
  void Reset();

private:
  static void ReadHandle(ezStreamReader* pStream, ezTypelessResourceHandle* pResourceHandle);
  void ReadResourceReference(ezStreamReader* pStream, ezTypelessResourceHandle* pResourceHandle);

  struct HandleData
  {
    EZ_DECLARE_POD_TYPE();

    ezTypelessResourceHandle* m_pHandle;
    ezUInt32 m_uiResourceID;
  };

  ezDeque<HandleData> m_StoredHandles;
  ezDynamicArray<ezTypelessResourceHandle> m_AllResources;
  ezUInt8 m_uiVersion;
  bool m_bReadData;
};


/// \brief Operator to serialize resource handles
template <typename ResourceType>
void operator>>(ezStreamReader& Stream, ezTypedResourceHandle<ResourceType>& Value)
{
  ezResourceHandleReadContext::ReadHandle(&Stream, Value);
}

