#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>

/// A dynamic buffer can be used when a lot of data needs to be stored in a single large buffer with dynamic size.
///
/// This class supports allocation and deallocation of single elements or ranges of multiple elements.
/// An allocation is identified by an offset and can have additional user data attached.
/// The indented usage patterns is that data is allocated and written to the buffer during game play or extraction code.
/// After all data has been written UploadChangesForNextFrame needs to be called to upload changed data to the GPU buffer.
/// The renderer would then call GetBufferForRendering to get the correct buffer for rendering.
class EZ_RENDERERFOUNDATION_DLL ezGALDynamicBuffer
{
public:
  /// Deallocates all data.
  void Clear();

  struct AllocateFlags
  {
    using StorageType = ezUInt32;

    enum Enum
    {
      None,
      ZeroFill = EZ_BIT(0),

      Default = None
    };

    struct Bits
    {
      StorageType ZeroFill : 1;
    };
  };

  /// Allocates a single or multiple elements and returns the offset to the first element.
  /// This offset is used to identify the allocation and should also be used in a shader to read the data from the buffer.
  ///
  /// The user data can be used to store additional information, typically the owner of the allocation, like e.g. a component handle.
  template <typename U>
  ezUInt32 Allocate(const U& userData, ezUInt32 uiCount = 1, ezBitflags<AllocateFlags> allocateFlags = AllocateFlags::None, ezAllocator* pTempAllocator = nullptr)
  {
    static_assert(sizeof(U) <= sizeof(ezUInt64), "userData is too large");
    ezUInt64 uiUserData = 0;
    *reinterpret_cast<U*>(&uiUserData) = userData;

    return Allocate(uiUserData, uiCount, allocateFlags, pTempAllocator);
  }

  /// Removes an allocation at the given offset. The offset must have been returned by Allocate.
  /// This will create unused space in the buffer that can be filled by subsequent allocations or closed later by compaction.
  void Deallocate(ezUInt32 uiOffset);

  /// Maps a range of elements for writing.
  template <typename T>
  ezArrayPtr<T> MapForWriting(ezUInt32 uiOffset)
  {
    ezUInt32 uiCount = 0;
    ezByteArrayPtr byteData = MapForWriting(uiOffset, uiCount);
    EZ_ASSERT_DEBUG(sizeof(T) == m_Desc.m_uiStructSize, "Invalid Type");
    return ezArrayPtr<T>(reinterpret_cast<T*>(byteData.GetPtr()), uiCount);
  }

  /// Maps a range of bytes for writing.
  ezByteArrayPtr MapBytesForWriting(ezUInt32 uiOffset)
  {
    ezUInt32 uiCount = 0;
    ezByteArrayPtr byteData = MapForWriting(uiOffset, uiCount);
    EZ_ASSERT_DEBUG(byteData.GetCount() == uiCount * m_Desc.m_uiStructSize, "Implementation error");
    return byteData;
  }

  /// Maps a range of elements for reading.
  template <typename T>
  ezArrayPtr<const T> MapForReading(ezUInt32 uiOffset) const
  {
    ezUInt32 uiCount = 0;
    ezConstByteArrayPtr byteData = MapForReading(uiOffset, uiCount);
    EZ_ASSERT_DEBUG(sizeof(T) == m_Desc.m_uiStructSize, "Invalid Type");
    return ezArrayPtr<const T>(reinterpret_cast<const T*>(byteData.GetPtr()), uiCount);
  }

  /// Upload all changed data to the GPU buffer for the next rendering frame, aka the next time BeginFrame is called on the GALDevice.
  void UploadChangesForNextFrame();

  struct ChangedAllocation
  {
    ezUInt64 m_uiUserData = 0;
    ezUInt32 m_uiNewOffset = 0;
  };

  /// Tries to compact the buffer by moving allocations to free ranges. All moved allocations are returned in out_changedAllocations.
  ///
  /// The user data can be used to update the owner of the allocation.
  /// To prevent too many changes per frame only uiMaxSteps are executed which corresponds to the number of allocations which can be moved.
  void RunCompactionSteps(ezDynamicArray<ChangedAllocation>& out_changedAllocations, ezUInt32 uiMaxSteps = 16);

  /// This should be called inside the rendering code to retrieve the underlying buffer for rendering.
  ///
  /// It is ensured that it will always return the same buffer until the next time BeginFrame is called on the GALDevice even if the buffer
  /// has been resized due to more allocations on the game play or extraction side.
  const ezGALBufferHandle& GetBufferForRendering() const { return m_hBufferForRendering; }

  /// Returns the description that was used to create this dynamic buffer.
  const ezGALBufferCreationDescription& GetDescription() const { return m_Desc; }

  /// Returns the debug name that was used to create this dynamic buffer.
  ezStringView GetDebugName() const { return m_sDebugName; }

private:
  friend class ezMemoryUtils;
  friend class ezGALDevice;

  ezGALDynamicBuffer() = default;
  ~ezGALDynamicBuffer();

  void Initialize(const ezGALBufferCreationDescription& desc, ezStringView sDebugName);
  void Deinitialize();

  ezUInt32 Allocate(ezUInt64 uiUserData, ezUInt32 uiCount, ezBitflags<AllocateFlags> allocateFlags, ezAllocator* pTempAllocator);
  ezByteArrayPtr MapForWriting(ezUInt32 uiOffset, ezUInt32& out_uiCount);
  ezConstByteArrayPtr MapForReading(ezUInt32 uiOffset, ezUInt32& out_uiCount) const;

  /// Returns the m_uiDataIndex for an allocation at this range.
  ///
  /// Returns ezInvalidIndex if neither m_Data nor one temp data contains the full range.
  ezUInt32 FindDataIndex(ezUInt32 uiOffset, ezUInt32 uiCount) const;

  /// Creates new temp data for the range [uiStartOffset, m_uiCapacity) and returns its data index.
  ///
  /// If m_uiCapacity is smaller than uiMinCapacity, the capacity is increased first.
  ezUInt32 AllocateTempData(ezUInt32 uiStartOffset, ezUInt32 uiMinCapacity, ezAllocator* pTempAllocator);

  /// Called by the device in BeginFrame. After this, the renderer uses the buffer of the last upload.
  void SwapBuffers()
  {
    m_hBufferForRendering = m_hBufferForUpload;
  }

  mutable ezMutex m_Mutex;

  /// Number of elements that fit into the buffer without growing.
  ///
  /// When the buffer grows, this value changes immediately, but m_Data only changes in the next upload.
  ezUInt32 m_uiCapacity = 0;

  /// End of the used part of the buffer, in number of elements. A new allocation is placed here if it doesn't fit into a free range.
  ///
  /// Every element in [0, m_uiNextOffset) is either part of an allocation or part of a free range.
  /// The last element always belongs to an allocation. There is never a free range at the end.
  /// Instead, m_uiNextOffset is decreased.
  ezUInt32 m_uiNextOffset = 0;

  /// CPU copy of the GPU buffer data.
  ///
  /// Only UploadChangesForNextFrame and Clear change its size.
  /// This way, mapped pointers stay valid when the buffer grows during a frame.
  ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper> m_Data;

  /// Extra memory for allocations that don't fit into m_Data. It is used until the next UploadChangesForNextFrame.
  ///
  /// The ranges of different temp data can overlap, and they can also overlap m_Data.
  /// Each allocation is stored in exactly one place, see Allocation::m_uiDataIndex.
  struct TempData
  {
    ezAllocator* m_pAllocator = nullptr;
    ezUInt8* m_pData = nullptr;
    ezUInt32 m_uiStartByteOffset = 0; ///< Byte offset in the full buffer where m_pData starts.
    ezUInt32 m_uiByteSize = 0;
  };

  ezSmallArray<TempData, 2> m_TempData;

  struct Allocation
  {
    ezUInt64 m_uiUserData = 0;
    ezUInt32 m_uiCount = 0;     ///< in number of elements
    ezUInt32 m_uiDataIndex = 0; ///< 0 means the data is in m_Data, otherwise it is in m_TempData[m_uiDataIndex - 1]
  };

  /// All allocations. The key is the offset in number of elements.
  ezMap<ezUInt32, Allocation> m_Allocations;

  /// Unused ranges below m_uiNextOffset, in number of elements. Deallocate creates them.
  ///
  /// Two free ranges are never next to each other, they are always merged into one.
  /// Directly after each free range there is an allocation.
  /// The array is sorted by count, so that Allocate uses the smallest hole that fits.
  /// RunCompactionSteps sorts it by start while it runs, and sorts it by count again at the end.
  ezDynamicArray<ezGAL::ModifiedRange> m_FreeRanges;

  /// Range of elements that UploadChangesForNextFrame has to send to the GPU.
  ezGAL::ModifiedRange m_DirtyRange;

  /// m_uiTotalSize changes when the buffer grows. The next upload then uses it for the new size of m_Data and the GPU buffer.
  ezGALBufferCreationDescription m_Desc;

  /// The uploads go into this buffer. In the next BeginFrame it becomes m_hBufferForRendering.
  ezGALBufferHandle m_hBufferForUpload;

  /// The buffer that the renderer uses in the current frame, see GetBufferForRendering().
  ezGALBufferHandle m_hBufferForRendering;

  ezString m_sDebugName;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  void CheckSelf() const;
#endif
};

EZ_DECLARE_FLAGS_OPERATORS(ezGALDynamicBuffer::AllocateFlags);
