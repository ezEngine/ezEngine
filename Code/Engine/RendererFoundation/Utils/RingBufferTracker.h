#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief Tracks memory in a ring buffer on a frame by frame basis.
/// The intended usage pattern is that for frame N, multiple allocations are called. This is followed by a call to SubmitFrame for frame N (optional) which allows the user to handle to memory ranges that were modified in this frame. Eventually at a later point in time Free is called for frame N to allow reuse of the memory.
/// Note that the assumption is that N is always increasing and that free is called for each frame in order.
class EZ_RENDERERFOUNDATION_DLL ezRingBufferTracker
{
public:
  // Used to tag the frameData as submitted to make sure we create a new FrameData bucket and don't upload the same range twice.
  constexpr static ezUInt64 s_FrameDataSubmitted = EZ_BIT(63);

  // \brief A memory range starting at m_uiStartOffset of size m_uiSize that was modified in a frame.
  struct FrameData
  {
    ezUInt64 m_uiFrame = 0;       ///< The frame in which the memory was allocated.
    ezUInt32 m_uiStartOffset = 0; ///< The start offset of the memory that was modified in this frame.
    ezUInt32 m_uiSize = 0;        ///< The size of the memory modification.
  };

public:
  /// Crates a new tracker.
  /// \param uiAlignment All allocations will be aligned to this. Must be power of two.
  /// \param uiTotalSize The size of the memory.
  ezRingBufferTracker(ezUInt32 uiAlignment, ezUInt32 uiTotalSize); // [tested]

  /// Allocates memory of the given size.
  /// \param uiSize Size requested.
  /// \param uiCurrentFrame The current frame for which this allocation is tracked.
  /// \param out_uiAllocatedOffset Will be filled by the start offset of the allocation inside the memory if successful.
  /// \return Returns EZ_FAILURE if not enough memory remains to fulfil the allocation.
  ezResult Allocate(ezUInt32 uiSize, ezUInt64 uiCurrentFrame, ezUInt32& out_uiAllocatedOffset); // [tested]

  /// Makes all memory that was used up to uiUpToFrame available again for allocations.
  /// \param uiUpToFrame All frames below this and this frame itself are safe to be reused.
  void Free(ezUInt64 uiUpToFrame); // [tested]

  /// \brief Retrieves the dirty memory ranges of this frame.
  /// Note that this can be anything between zero and two. In case that the end of the buffer is reached and wraps around within a frame, the end part and the part at the start are split into two FrameData objects. Can be called multiple times per frame. Each dirty range will only be given out once.
  /// \param uiFrame Frame index to submit dirty ranges.
  /// \param out_frameData Will receive the FrameData objects containing the dirty ranges of the memory.
  /// \return Returns EZ_FAILURE if no allocations were made in uiFrame.
  ezResult SubmitFrame(ezUInt64 uiFrame, ezDynamicArray<FrameData>& out_frameData);    // [tested]

  EZ_ALWAYS_INLINE ezUInt32 GetTotalMemory() const { return m_uiTotalSize; }           // [tested]
  EZ_ALWAYS_INLINE ezUInt32 GetUsedMemory() const { return m_uiTotalSize - m_uiFree; } // [tested]
  EZ_ALWAYS_INLINE ezUInt32 GetFreeMemory() const { return m_uiFree; }                 // [tested]

private:
  ezUInt32 m_uiAlignment = 0;
  ezUInt32 m_uiTotalSize = 0;
  ezUInt32 m_uiFree = 0;
  ezUInt32 m_uiCurrentOffset = 0;
  ezHybridArray<FrameData, 4> m_FrameData;
};
