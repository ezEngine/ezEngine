#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/ThreadLocalStorage.h>

/// Template class to interact with thread local variables
/// The thread local variable will only store pointers to the type T given 
template <typename T> 
class ezThreadLocalPointer
{
public:

  /// Initialization function
  /// Note that the constructor doesn't allocate the TLS index since that is only safe to do after initialization of the
  /// thread local storage system and the common use case for thread local pointers is a globally shared variable (static)
  ezThreadLocalPointer()
    : m_uiThreadLocalSlotIndex(EZ_THREAD_LOCAL_STORAGE_INVALID_SLOT)
  {
  }

  /// Cleanup function
  /// Frees the associated thread local storage index for this variable
  ~ezThreadLocalPointer()
  {
    if(ezThreadLocalStorage::IsValidSlot(m_uiThreadLocalSlotIndex))
      ezThreadLocalStorage::FreeSlot(m_uiThreadLocalSlotIndex);
  }

  /// Returns the pointer stored in the thread pointer table for the current thread
  /// Note that a variable which hasn't been assigned yet will return NULL
  operator T* ()
  {
    if(!ezThreadLocalStorage::IsValidSlot(m_uiThreadLocalSlotIndex))
      return NULL;
    else
      return reinterpret_cast<T*>(ezThreadLocalStorage::GetValueForSlot(m_uiThreadLocalSlotIndex));
  }

  /// Returns the const pointer stored in the thread pointer table for the current thread
  /// Note that a variable which hasn't been assigned yet will return NULL
  operator const T* () const
  {
    if(!ezThreadLocalStorage::IsValidSlot(m_uiThreadLocalSlotIndex))
      return NULL;
    else
      return reinterpret_cast<const T*>(ezThreadLocalStorage::GetValueForSlot(m_uiThreadLocalSlotIndex));
  }

  /// Assigns the pointer value to the variable for this thread
  /// Note that this triggers on first access the allocation of the TLS slot (see constructor description)
  void operator = (T* pValue)
  {
    if(!ezThreadLocalStorage::IsValidSlot(m_uiThreadLocalSlotIndex))
    {
      m_uiThreadLocalSlotIndex = ezThreadLocalStorage::AllocateSlot();
    }

    ezThreadLocalStorage::SetValueForSlot(m_uiThreadLocalSlotIndex, pValue);
  }

private:

  ezUInt32 m_uiThreadLocalSlotIndex;

  EZ_DISALLOW_COPY_AND_ASSIGN(ezThreadLocalPointer);
};