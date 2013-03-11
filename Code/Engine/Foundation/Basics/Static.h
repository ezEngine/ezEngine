#ifndef EZ_INCLUDE_STATIC_H
#error Please do not include this file directly.
#endif

#pragma once

/// A small helper class to be able to construct global objects and static members that require a memory allocator.
/// By default all accesses to a memory allocator before program startup will trigger an assert.
/// Otherwise global objects would either use the wrong allocator (not yet specified) or they would show false positives
/// in the memory leak tracker, since they are usually not deallocated before application shutdown.
/// Therefore you should generally avoid global objects and static member variables that (indirectly) need to allocate memory.
/// However, to still enable this for code, which would otherwise be difficult to write, this class allows to wrap certain
/// members or global variables and thus have them use the 'static allocator'. This allocator will not report any memory leaks,
/// therefore there won't be any false positives. 
///
/// Usage is like this:
/// ezStatic<ezDynamicArraz> g_MyGlobalData;
///
/// g_MyGlobalData.GetStatic().CallSomeFunction();
template <typename T>
class ezStatic
{
public:
  ezStatic() : m_SetStaticAllocator(), m_Data() // will be initialized in exactly that order, sets the allocators to the static allocator
  {
    ezFoundation::PopStaticAllocator(); // reset the allocators to their original state
  }

  /// Returns the internal object.
  EZ_FORCE_INLINE T& GetStatic() { return m_Data; }

  /// Returns the internal object.
  EZ_FORCE_INLINE const T& GetStatic() const { return m_Data; }

private:
  /// Helper struct, which will execute 'ezFoundation::PushStaticAllocator' upon construction.
  struct ezSetStaticAllocatorHelper
  {
    ezSetStaticAllocatorHelper()
    {
      ezFoundation::PushStaticAllocator();
    }
  };

  /// This must be the first member, it will ensure that the second member will get the static allocator during its construction.
  ezSetStaticAllocatorHelper m_SetStaticAllocator;

  /// The actual object that should be constructed, which is supposed to get the static allocator
  T m_Data;
};


