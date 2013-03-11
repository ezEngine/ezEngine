#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Basics/Types/ArrayPtr.h>

#ifdef new
  #undef new
#endif

#ifdef delete
  #undef delete
#endif

/// Interface for all memory allocators including helper methods
class ezIAllocator
{
public:
  struct Stats
  {
    ezUInt64 m_uiNumAllocations;      // total number of allocations
    ezUInt64 m_uiNumDeallocations;    // total number of deallocations
    ezUInt64 m_uiNumLiveAllocations;  // total number of live allocations
    ezUInt64 m_uiAllocationSize;      // total allocation size in bytes
    ezUInt64 m_uiUsedMemorySize;      // total used memory size in bytes including overhead due to alignment etc
  };

  ezIAllocator(const char* szName);
  virtual ~ezIAllocator();

  /// Interface, do not use this directly, always use the new/delete macros below
  virtual void* Allocate(size_t uiSize, size_t uiAlign) = 0;
  virtual void Deallocate(void* ptr) = 0;
  virtual size_t AllocatedSize(const void* ptr) = 0;
  virtual size_t UsedMemorySize(const void* ptr) = 0;
  virtual void GetStats(Stats& stats) = 0;

  const char* GetName() const;

  /// If this is a proxy allocator this method returns the "real" allocator or another proxy 
  /// allocator, otherwise this method returns NULL.
  virtual ezIAllocator* GetParent() const;

protected:
  const char* m_szName;

  EZ_DISALLOW_COPY_AND_ASSIGN(ezIAllocator);
};

#include <Foundation/Memory/Implementation/IAllocator_inl.h>

/// creates a new instance of type using the given allocator
#define EZ_NEW(allocator, type) \
  new (allocator->Allocate(sizeof(type), EZ_ALIGNMENT_OF(type))) type

/// deletes the instance stored in ptr using the given allocator and sets ptr to NULL
#define EZ_DELETE(allocator, ptr) \
  { ezInternal::Delete(allocator, ptr); ptr = NULL; }

/// creates a new array of type using the given allocator with count elements,
/// calls default constructor for non-POD types
#define EZ_NEW_ARRAY(allocator, type, count) \
  ezInternal::CreateArray<type>(allocator, count)

/// calls destructor on every element for non-POD types and deletes the array
/// stored in arrayPtr using the given allocator
#define EZ_DELETE_ARRAY(allocator, arrayPtr) \
  { ezInternal::DeleteArray(allocator, arrayPtr); arrayPtr.Reset(); }

/// creates a raw buffer of type using the given allocator with count elements, 
/// but does NOT call the default constructor
#define EZ_NEW_RAW_BUFFER(allocator, type, count) \
  ezInternal::CreateRawBuffer<type>(allocator, count)

/// deletes a raw buffer stored in ptr using the given allocator, but does NOT call destructor
#define EZ_DELETE_RAW_BUFFER(allocator, ptr) \
  { ezInternal::DeleteRawBuffer(allocator, ptr); ptr = NULL; }



/// creates a new instance of type using the default allocator
#define EZ_DEFAULT_NEW(type) EZ_NEW(ezFoundation::GetDefaultAllocator(), type)

/// deletes the instance stored in ptr using the default allocator and sets ptr to NULL
#define EZ_DEFAULT_DELETE(ptr) EZ_DELETE(ezFoundation::GetDefaultAllocator(), ptr)

/// creates a new array of type using the default allocator with count elements, 
/// calls default constructor for non-POD types
#define EZ_DEFAULT_NEW_ARRAY(type, count) EZ_NEW_ARRAY(ezFoundation::GetDefaultAllocator(), type, count)

/// calls destructor on every element for non-POD types and deletes the array 
/// stored in arrayPtr using the default allocator
#define EZ_DEFAULT_DELETE_ARRAY(arrayPtr) EZ_DELETE_ARRAY(ezFoundation::GetDefaultAllocator(), arrayPtr)

/// creates a raw buffer of type using the default allocator with count elements, 
/// but does NOT call the default constructor
#define EZ_DEFAULT_NEW_RAW_BUFFER(type, count) EZ_NEW_RAW_BUFFER(ezFoundation::GetDefaultAllocator(), type, count)

/// deletes a raw buffer stored in ptr using the default allocator, but does NOT call destructor
#define EZ_DEFAULT_DELETE_RAW_BUFFER(ptr) EZ_DELETE_RAW_BUFFER(ezFoundation::GetDefaultAllocator(), ptr)

// todo: find a better solution for that
// Disallow the use of new, always use the macros above
//#if !(defined(EZ_ALLOW_STANDARD_NEW) && EZ_ALLOW_STANDARD_NEW)
//  #define new DISALLOWED_USAGE_OF_NEW
//  #define delete DISALLOWED_USAGE_OF_DELETE
//#endif
