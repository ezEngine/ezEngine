module ez.Foundation.Memory.AllocatorBase;

import ez.Foundation.Types.Types;

extern(C++):

class ezAllocatorBase
{
protected:
  abstract void __dtor(); // destructor member to match c++ vtable layout

public:
  static struct Stats
  {
    ezUInt64 m_uiNumAllocations;      ///< total number of allocations
    ezUInt64 m_uiNumDeallocations;    ///< total number of deallocations
    ezUInt64 m_uiAllocationSize;      ///< total allocation size in bytes
  };

  //ezAllocatorBase();
  //~ezAllocatorBase();

  /// \brief Interface, do not use this directly, always use the new/delete macros below
  abstract void* Allocate(size_t uiSize, size_t uiAlign);
  abstract void Deallocate(void* ptr);
  abstract void* Reallocate(void* ptr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign);
  abstract size_t AllocatedSize(const void* ptr);

  abstract Stats GetStats() const;
};