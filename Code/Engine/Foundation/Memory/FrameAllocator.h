#pragma once

#include <Foundation/Memory/LinearAllocator.h>

/// \brief A double buffered linear allocator for temporary per-frame allocations.
///
/// This allocator maintains two linear allocators and swaps between them each frame.
/// One allocator is used for the current frame while the previous frame's allocator is reset.
/// This pattern ensures that allocations from the previous frame remain valid until the
/// next frame begins, which is useful for data that needs to persist across frame boundaries.
class EZ_FOUNDATION_DLL ezDoubleBufferedLinearAllocator
{
public:
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  static constexpr bool OverwriteMemoryOnReset = true;
#else
  static constexpr bool OverwriteMemoryOnReset = false;
#endif
  using LinearAllocatorType = ezLinearAllocator<ezAllocatorTrackingMode::Basics, OverwriteMemoryOnReset>;

  ezDoubleBufferedLinearAllocator(ezStringView sName, ezAllocator* pParent);
  ~ezDoubleBufferedLinearAllocator();

  EZ_ALWAYS_INLINE ezAllocator* GetCurrentAllocator() const { return m_pCurrentAllocator; }

  void Swap();
  void Reset();

private:
  LinearAllocatorType* m_pCurrentAllocator;
  LinearAllocatorType* m_pOtherAllocator;
};

/// \brief Global frame allocator for temporary allocations that are reset each frame.
///
/// This is a convenience wrapper around ezDoubleBufferedLinearAllocator that provides a global
/// instance for frame-based allocations. Very efficient for temporary data that only needs to
/// live for one frame (UI layouts, temporary strings, intermediate calculations, etc.).
///
/// Usage pattern:
/// - GetCurrentAllocator() - get allocator for current frame
/// - Swap() - called once per frame to switch buffers (usually by the engine)
/// - Reset() - resets both buffers (usually called during shutdown)
///
/// Performance characteristics:
/// - Allocation: O(1) - just advances a pointer
/// - Deallocation: Not supported individually, entire frame is reset at once
/// - Memory overhead: Minimal, just stack pointers
class EZ_FOUNDATION_DLL ezFrameAllocator
{
public:
  /// \brief Returns the allocator for the current frame.
  ///
  /// All allocations from this allocator will be automatically freed when the frame ends.
  EZ_ALWAYS_INLINE static ezAllocator* GetCurrentAllocator() { return s_pAllocator->GetCurrentAllocator(); }

  /// \brief Swaps the active buffer, should be called once per frame.
  ///
  /// This makes the previous frame's allocations invalid and resets the allocator for new allocations.
  static void Swap();

  /// \brief Resets both buffers, typically called during shutdown.
  static void Reset();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, FrameAllocator);

  static void Startup();
  static void Shutdown();

  static ezDoubleBufferedLinearAllocator* s_pAllocator;
};
