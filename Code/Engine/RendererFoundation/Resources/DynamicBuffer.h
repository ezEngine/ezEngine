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
  ///
  /// If the buffer has to grow, the data of the allocation is stored in frame allocator memory until the next UploadChangesForNextFrame.
  /// Therefore UploadChangesForNextFrame has to be called in the same or the next frame, before ezFrameAllocator resets that memory.
  template <typename U>
  ezUInt32 Allocate(const U& userData, ezUInt32 uiCount = 1, ezBitflags<AllocateFlags> allocateFlags = AllocateFlags::None)
  {
    static_assert(sizeof(U) <= sizeof(ezUInt64), "userData is too large");
    ezUInt64 uiUserData = 0;
    *reinterpret_cast<U*>(&uiUserData) = userData;

    return Allocate(uiUserData, uiCount, allocateFlags);
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

  ezUInt32 Allocate(ezUInt64 uiUserData, ezUInt32 uiCount, ezBitflags<AllocateFlags> allocateFlags);
  ezByteArrayPtr MapForWriting(ezUInt32 uiOffset, ezUInt32& out_uiCount);
  ezConstByteArrayPtr MapForReading(ezUInt32 uiOffset, ezUInt32& out_uiCount) const;

  /// Allocates temp memory for uiCount elements from the frame allocator. It is never freed explicitly.
  ezUInt8* AllocateTempData(ezUInt32 uiCount);

  /// Makes sure that the last node is free and has at least uiCount elements. Grows the buffer if necessary.
  ///
  /// Growing only changes m_uiCapacity and m_Desc. m_Data and the GPU buffer are resized in the next UploadChangesForNextFrame.
  void EnsureFreeNodeAtEnd(ezUInt32 uiCount);

  /// Returns the index of a node with default values, reusing released nodes first. This can reallocate m_Nodes.
  ezUInt32 AllocateNode();
  void ReleaseNode(ezUInt32 uiNodeIndex);

  /// Adds a free node to the bin for its size. The node's count must not change while it is in a bin.
  void AddToBin(ezUInt32 uiNodeIndex);
  void RemoveFromBin(ezUInt32 uiNodeIndex);

  /// Shrinks a node to uiCount elements and adds a free node for the rest directly after it.
  ///
  /// The node must not be in a bin. Does nothing if the node already has uiCount elements. This can reallocate m_Nodes.
  void SplitNode(ezUInt32 uiNodeIndex, ezUInt32 uiCount);

  /// Marks a node as free, merges it with free neighbors and adds it to its bin.
  ///
  /// The merged neighbors are released, uiNodeIndex stays valid. The node must not be in a bin or in m_OffsetToNodeIndex.
  void FreeNodeAndMerge(ezUInt32 uiNodeIndex);

  /// Returns the index of a free node with at least uiCount elements, or ezInvalidIndex if none was found.
  ///
  /// Only takes nodes from bins where all nodes fit. Nodes in the bin of uiCount itself are not considered, even if they are large enough.
  ezUInt32 FindFreeNode(ezUInt32 uiCount) const;

  /// Called by the device in BeginFrame. After this, the renderer uses the buffer of the last upload.
  void SwapBuffers()
  {
    m_hBufferForRendering = m_hBufferForUpload;
  }

  mutable ezMutex m_Mutex;

  /// CPU copy of the GPU buffer data.
  ///
  /// Only UploadChangesForNextFrame and Clear change its size.
  /// This way, mapped pointers stay valid when the buffer grows during a frame.
  ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper> m_Data;

  /// Number of elements that fit into the buffer without growing.
  ///
  /// When the buffer grows, this value changes immediately, but m_Data only changes in the next upload.
  ezUInt32 m_uiCapacity = 0;

  /// True if any node has temp data, see Node::m_pTempData.
  ///
  /// The temp data is allocated from the frame allocator. UploadChangesForNextFrame copies it into m_Data.
  /// Compaction is skipped while this is set, because it only moves data within m_Data.
  bool m_bHasTempData = false;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  /// ezFrameAllocator::GetSwapCount() when the oldest temp data was allocated.
  ezUInt64 m_uiTempDataSwapCount = 0;
#endif

  struct Node
  {
    // Bit-fields can't have default member initializers in C++17
    Node()
      : m_uiCount(0)
      , m_uiUsed(0)
    {
    }

    ezUInt32 m_uiOffset = 0;                     ///< in number of elements
    ezUInt32 m_uiCount : 31;                     ///< in number of elements
    ezUInt32 m_uiUsed : 1;                       ///< true if this node is used for an allocation, false if it is free

    ezUInt32 m_uiPrevNodeIndex = ezInvalidIndex;
    ezUInt32 m_uiNextNodeIndex = ezInvalidIndex; ///< For released nodes, this is the next released node.

    /// Used and free nodes need different data, so they share the memory. It is not initialized.
    union
    {
      /// Only valid for used nodes
      struct
      {
        ezUInt64 m_uiUserData;

        /// If set, the data of this allocation is stored here instead of in m_Data, until the next UploadChangesForNextFrame.
        ezUInt8* m_pTempData;
      };

      /// Only valid for free nodes. Double linked list of all nodes that are in the same bin.
      struct
      {
        ezUInt32 m_uiPrevNodeIndexInBin;
        ezUInt32 m_uiNextNodeIndexInBin;
      };
    };
  };

  ezDynamicArray<Node> m_Nodes;

  /// Single linked list of released nodes in m_Nodes, through Node::m_uiNextNodeIndex. AllocateNode reuses them first.
  ezUInt32 m_uiFirstReleasedNodeIndex = ezInvalidIndex;

  /// Maps the start offset of an allocation to its index in m_Nodes.
  ezHashTable<ezUInt32, ezUInt32> m_OffsetToNodeIndex;

  /// Number of size bins. BinFloor(0xFFFFFFFF) is 239 and BinCeil can return 240, see DynamicBuffer.cpp.
  static constexpr ezUInt32 s_uiNumBins = 241;

  /// Each bin contains a double linked list of nodes that are in the same bin. The bin is chosen by the size of the allocation.
  ezUInt32 m_FreeNodeBins[s_uiNumBins];

  /// The node with offset 0. The nodes are a double linked list sorted by offset, which covers the full range [0, m_uiCapacity).
  ezUInt32 m_uiFirstNodeIndex = ezInvalidIndex;

  /// The node with the highest offset, it ends at m_uiCapacity.
  ///
  /// When the buffer grows, this node is extended if it is free. Otherwise a new free node is added after it.
  ezUInt32 m_uiLastNodeIndex = ezInvalidIndex;

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
