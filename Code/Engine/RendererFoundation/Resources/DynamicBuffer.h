#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>

/// \brief A dynamic buffer can be used when a lot of data needs to be stored in a single large buffer with dynamic size.
///
/// This class supports allocation and deallocation of single elements or ranges of multiple elements.
/// An allocation is identified by an offset and can have additional user data attached.
/// The indented usage patterns is that data is allocated and written to the buffer during game play or extraction code.
/// After all data has been written UploadChangesForNextFrame needs to be called to upload changed data to the GPU buffer.
/// The renderer would then call GetBufferForRendering to get the correct buffer for rendering.
class EZ_RENDERERFOUNDATION_DLL ezGALDynamicBuffer
{
public:
  /// \brief Allocates a single or multiple elements and returns the offset to the first element.
  /// This offset is used to identify the allocation and should also be used in a shader to read the data from the buffer.
  ///
  /// The user data can be used to store additional information, typically the owner of the allocation, like e.g. a component handle.
  template <typename U>
  ezUInt32 Allocate(const U& userData, ezUInt32 uiCount = 1)
  {
    static_assert(sizeof(U) <= sizeof(ezUInt64), "userData is too large");
    ezUInt64 uiUserData = 0;
    *reinterpret_cast<U*>(&uiUserData) = userData;

    return Allocate(uiUserData, uiCount);
  }

  /// \brief Removes an allocation at the given offset. The offset must have been returned by Allocate.
  /// This will create unused space in the buffer that can be filled by subsequent allocations or closed later by compaction.
  void Deallocate(ezUInt32 uiOffset);

  /// \brief Maps a range of elements for writing.
  template <typename T>
  ezArrayPtr<T> MapForWriting(ezUInt32 uiOffset)
  {
    ezUInt32 uiCount = 0;
    ezByteArrayPtr byteData = MapForWriting(uiOffset, uiCount);
    EZ_ASSERT_DEBUG(sizeof(T) == m_Desc.m_uiStructSize, "Invalid Type");
    return ezArrayPtr<T>(reinterpret_cast<T*>(byteData.GetPtr()), uiCount);
  }

  /// \brief Upload all changed data to the GPU buffer for the next rendering frame, aka the next time BeginFrame is called on the GALDevice.
  void UploadChangesForNextFrame();

  struct ChangedAllocation
  {
    ezUInt64 m_uiUserData = 0;
    ezUInt32 m_uiNewOffset = 0;
  };

  /// \brief Tries to compact the buffer by moving allocations to free ranges. All moved allocations are returned in out_changedAllocations.
  ///
  /// The user data can be used to update the owner of the allocation.
  /// To prevent too many changes per frame only uiMaxSteps are executed which corresponds to the number of allocations which can be moved.
  void RunCompactionSteps(ezDynamicArray<ChangedAllocation>& out_changedAllocations, ezUInt32 uiMaxSteps = 16);

  /// \brief This should be called inside the rendering code to retrieve the underlying buffer for rendering.
  ///
  /// It is ensured that it will always return the same buffer until the next time BeginFrame is called on the GALDevice even if the buffer
  /// has been resized due to more allocations on the game play or extraction side.
  const ezGALBufferHandle& GetBufferForRendering() const
  {
    return m_hBufferForRendering;
  }

private:
  friend class ezMemoryUtils;
  friend class ezGALDevice;

  ezGALDynamicBuffer() = default;
  ~ezGALDynamicBuffer();

  void Initialize(const ezGALBufferCreationDescription& desc, ezStringView sDebugName);
  void Deinitialize();

  ezUInt32 Allocate(ezUInt64 uiUserData, ezUInt32 uiCount);
  ezByteArrayPtr MapForWriting(ezUInt32 uiOffset, ezUInt32& out_uiCount);

  void Resize(ezUInt32 uiNewSize);

  void SwapBuffers()
  {
    m_hBufferForRendering = m_hBufferForUpload;
  }

  ezMutex m_Mutex;

  ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper> m_Data;
  ezUInt32 m_uiNextOffset = 0;

  struct Allocation
  {
    ezUInt64 m_uiUserData = 0;
    ezUInt32 m_uiCount = 0;
  };

  ezMap<ezUInt32, Allocation> m_Allocations;

  ezDynamicArray<ezGAL::ModifiedRange> m_FreeRanges;
  ezGAL::ModifiedRange m_DirtyRange;

  ezGALBufferCreationDescription m_Desc;

  ezGALBufferHandle m_hBufferForUpload;
  ezGALBufferHandle m_hBufferForRendering;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezString m_sDebugName;
#endif
};
