#pragma once

#include <Foundation/Memory/LinearAllocator.h>

/// \brief A double buffered stack allocator
class EZ_FOUNDATION_DLL ezDoubleBufferedLinearAllocator
{
public:
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  static constexpr bool OverwriteMemoryOnReset = true;
#else
  static constexpr bool OverwriteMemoryOnReset = false;
#endif
  using StackAllocatorType = ezLinearAllocator<ezAllocatorTrackingMode::Basics, OverwriteMemoryOnReset>;

  ezDoubleBufferedLinearAllocator(ezStringView sName, ezAllocator* pParent);
  ~ezDoubleBufferedLinearAllocator();

  EZ_ALWAYS_INLINE ezAllocator* GetCurrentAllocator() const { return m_pCurrentAllocator; }

  void Swap();
  void Reset();

private:
  StackAllocatorType* m_pCurrentAllocator;
  StackAllocatorType* m_pOtherAllocator;
};

class EZ_FOUNDATION_DLL ezFrameAllocator
{
public:
  EZ_ALWAYS_INLINE static ezAllocator* GetCurrentAllocator() { return s_pAllocator->GetCurrentAllocator(); }

  static void Swap();
  static void Reset();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, FrameAllocator);

  static void Startup();
  static void Shutdown();

  static ezDoubleBufferedLinearAllocator* s_pAllocator;
};
