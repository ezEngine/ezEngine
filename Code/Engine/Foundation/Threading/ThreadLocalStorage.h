#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/StaticArray.h>

/// \brief User-configurable: How many pointer values can be stored in thread local variables
///
/// \note Note that increasing the value increases the size of each ezThread object accordingly)
#define EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT 16

#define EZ_THREAD_LOCAL_STORAGE_INVALID_SLOT EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT

typedef ezStaticArray<void*, EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT> ezThreadLocalPointerTable;

/// \brief Collection of basic methods for interaction with thread local storage
struct EZ_FOUNDATION_DLL ezThreadLocalStorage
{
public:

  /// \brief Allocates a slot from the TLS variable pool.
  ///
  /// Returns EZ_THREAD_LOCAL_STORAGE_INVALID_SLOT on failure (may also be checked with IsValidSlot()).
  /// Note that the preferred way of interaction is ezThreadLocal which encapsulates the logic to allocate and free TLS slots
  static ezUInt32 AllocateSlot();

  /// \brief Frees a slot from the TLS variable pool.
  static void FreeSlot(ezUInt32 uiSlotIndex);

  /// \brief Sets the pointer value for the given slot.
  static void SetValueForSlot(ezUInt32 uiSlotIndex, void* pValue);

  /// \brief Returns the pointer value stored in the given slot.
  static void* GetValueForSlot(ezUInt32 uiSlotIndex);

  /// \brief Helper function to check if the given slot is a valid one.
  static inline bool IsValidSlot(ezUInt32 uiSlotIndex)
  {
    return uiSlotIndex < EZ_THREAD_LOCAL_STORAGE_SLOT_COUNT;
  }

  /// \brief Sets the pointer to the per thread table used to handle an arbitrary amount of TLS variables.
  ///
  /// \note Note that all ezThread derived classes will set up this correctly before calling Run()
  static void SetPerThreadPointerTable(ezThreadLocalPointerTable* pPerThreadPointerTable);

  /// \brief Returns the per thread pointer table used to store TLS values internally
  static ezThreadLocalPointerTable* GetPerThreadPointerTable();

private:

  friend class FoundationThreadUtilsSubSystem;

  static bool s_bInitialized;

  /// \brief Initialization functionality of the thread local storage system (called by foundation startup and thus private)
  static void Initialize();

  /// \brief Cleanup functionality of the thread local storage system (called by foundation shutdown and thus private)
  static void Shutdown();
};

