#pragma once

#include <Foundation/Memory/Allocator.h>

/// \brief Allocator wrapper that should never be used - causes assertion failures.
///
/// This wrapper is used as a template parameter to indicate that no allocator
/// should be used. Any attempt to call GetAllocator() will trigger an assertion.
/// Useful for container types that should never allocate.
struct ezNullAllocatorWrapper
{
  EZ_FORCE_INLINE static ezAllocator* GetAllocator()
  {
    EZ_REPORT_FAILURE("This method should never be called");
    return nullptr;
  }
};

/// \brief Wrapper for the engine's default general-purpose allocator.
struct ezDefaultAllocatorWrapper
{
  EZ_ALWAYS_INLINE static ezAllocator* GetAllocator() { return ezFoundation::GetDefaultAllocator(); }
};

/// \brief Wrapper for the allocator used for static/global objects.
struct ezStaticsAllocatorWrapper
{
  EZ_ALWAYS_INLINE static ezAllocator* GetAllocator() { return ezFoundation::GetStaticsAllocator(); }
};

/// \brief Wrapper for the allocator that provides memory with specific alignment guarantees.
struct ezAlignedAllocatorWrapper
{
  EZ_ALWAYS_INLINE static ezAllocator* GetAllocator() { return ezFoundation::GetAlignedAllocator(); }
};

/// \brief Helper function to facilitate setting the allocator on member containers of a class
/// Allocators can be either template arguments or a ctor parameter. Using the ctor parameter requires the class ctor to reference each member container in the initialization list. This can be very tedious. On the other hand, the template variant only support template parameter so you can't simply pass in a member allocator.
/// This class solves this problem provided the following rules are followed:
/// 1. The `ezAllocator` must be the declared at the earliest in the class, before any container.
/// 2. The `ezLocalAllocatorWrapper` should be declared right afterwards.
/// 3. Any container needs to be declared below these two and must include the `ezLocalAllocatorWrapper` as a template argument to the allocator.
/// 4. In the ctor initializer list, init the ezAllocator first, then the ezLocalAllocatorWrapper. With this approach all containers can be omitted.
/// \code{.cpp}
///   class MyClass
///   {
///     ezAllocator m_SpecialAlloc;
///     ezLocalAllocatorWrapper m_Wrapper;
///
///     ezDynamicArray<int, ezLocalAllocatorWrapper> m_Data;
///
///     MyClass()
///       : m_SpecialAlloc("MySpecialAlloc")
///       , m_Wrapper(&m_SpecialAlloc)
///     {
///     }
///   }
/// \endcode
struct EZ_FOUNDATION_DLL ezLocalAllocatorWrapper
{
  ezLocalAllocatorWrapper(ezAllocator* pAllocator);

  void Reset();

  static ezAllocator* GetAllocator();
};
