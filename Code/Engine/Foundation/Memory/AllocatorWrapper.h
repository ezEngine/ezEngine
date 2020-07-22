#pragma once

struct ezNullAllocatorWrapper
{
  EZ_FORCE_INLINE static ezAllocatorBase* GetAllocator()
  {
    EZ_REPORT_FAILURE("This method should never be called");
    return nullptr;
  }
};

struct ezDefaultAllocatorWrapper
{
  EZ_ALWAYS_INLINE static ezAllocatorBase* GetAllocator() { return ezFoundation::GetDefaultAllocator(); }
};

struct ezStaticAllocatorWrapper
{
  EZ_ALWAYS_INLINE static ezAllocatorBase* GetAllocator() { return ezFoundation::GetStaticAllocator(); }
};

struct ezAlignedAllocatorWrapper
{
  EZ_ALWAYS_INLINE static ezAllocatorBase* GetAllocator() { return ezFoundation::GetAlignedAllocator(); }
};

struct EZ_FOUNDATION_DLL ezLocalAllocatorWrapper
{
  ezLocalAllocatorWrapper(ezAllocatorBase* pAllocator);

  void Reset();

  static ezAllocatorBase* GetAllocator();
};
