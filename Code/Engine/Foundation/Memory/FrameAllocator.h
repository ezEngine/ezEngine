#pragma once

#include <Foundation/Memory/LinearAllocator.h>

/// \brief A double buffered stack allocator
class EZ_FOUNDATION_DLL ezDoubleBufferedStackAllocator
{
public:
  using StackAllocatorType = ezLinearAllocator<ezAllocatorTrackingMode::Basics>;

  ezDoubleBufferedStackAllocator(ezStringView sName, ezAllocator* pParent);
  ~ezDoubleBufferedStackAllocator();

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

  static ezDoubleBufferedStackAllocator* s_pAllocator;
};
