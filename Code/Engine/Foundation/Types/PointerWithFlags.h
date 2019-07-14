#pragma once

#include <Foundation/Basics.h>

/// \brief A wrapper around a raw pointer that allows to use the lower N bits for flags
///
/// When accessing the pointer, the lower N bits are masked off.
/// Typically one can safely store 3 bits in the lower bits of a pointer as most data is 8 byte aligned,
/// especially when it was heap allocated.
template <typename PtrType, uint8_t NumFlagBits = 2>
class ezPointerWithFlags
{
private:
  enum : size_t
  {
    AllOnes = (std::size_t)(-1),
    PtrBits = sizeof(void*) * 8,
    FlagsMask = (AllOnes >> (PtrBits - NumFlagBits)),
    PtrMask = ~FlagsMask,
  };

  void* m_ptr = nullptr;

public:
  /// \brief Initializes the pointer and flags with zero
  ezPointerWithFlags() = default;

  /// \brief Initializes the pointer and flags
  explicit ezPointerWithFlags(PtrType* ptr, uint8_t flags = 0) { SetPtrAndFlags(ptr, flags); }

  /// \brief Changes the pointer and flags
  void SetPtrAndFlags(PtrType* ptr, uint8_t flags)
  {
    const std::uintptr_t isrc = *reinterpret_cast<std::uintptr_t*>(&ptr);
    std::uintptr_t& iptr = *reinterpret_cast<std::uintptr_t*>(&m_ptr);

    iptr = (isrc & PtrMask) | (flags & FlagsMask);
  }

  /// \brief Returns the masked off pointer value
  const PtrType* GetPtr() const
  {
    const std::uintptr_t& iptr = *reinterpret_cast<const std::uintptr_t*>(&m_ptr);
    return reinterpret_cast<const PtrType*>(iptr & PtrMask); // mask off lower N bits
  }

  /// \brief Returns the masked off pointer value
  PtrType* GetPtr()
  {
    std::uintptr_t& iptr = *reinterpret_cast<std::uintptr_t*>(&m_ptr);
    return reinterpret_cast<PtrType*>(iptr & PtrMask); // mask off lower N bits
  }

  /// \brief Changes the pointer value only. Flags stay unchanged.
  void SetPtr(PtrType* ptr)
  {
    const std::uintptr_t isrc = *reinterpret_cast<std::uintptr_t*>(&ptr);
    EZ_ASSERT_DEBUG((isrc & FlagsMask) == 0, "The given pointer does not have an {} byte alignment and thus cannot be stored lossless.", 1u << NumFlagBits);

    std::uintptr_t& iptr = *reinterpret_cast<std::uintptr_t*>(&m_ptr);

    iptr = (isrc & PtrMask) | (iptr & FlagsMask);
  }
  /// \brief Returns the flags value only
  uint8_t GetFlags() const
  {
    const std::uintptr_t& iptr = *reinterpret_cast<const std::uintptr_t*>(&m_ptr);
    return static_cast<uint8_t>(iptr & FlagsMask);
  }

  /// \brief Changes only the flags value. The given value must fit into the reserved bits.
  void SetFlags(uint8_t flags)
  {
    EZ_ASSERT_DEBUG(flags <= FlagsMask, "The flag value {} requires more than {} bits", flags, NumFlagBits);

    std::uintptr_t& iptr = *reinterpret_cast<std::uintptr_t*>(&m_ptr);

    iptr = (iptr & PtrMask) | (flags & FlagsMask);
  }

  /// \brief Returns the masked off pointer value
  operator PtrType*() { return GetPtr(); }

  /// \brief Returns the masked off pointer value
  operator const PtrType*() const { return GetPtr(); }

  /// \brief Changes the pointer value only. Flags stay unchanged.
  void operator=(PtrType* ptr)
  {
    SetPtr(ptr);
  }

  /// \brief Compares the pointer part for equality (flags are ignored)
  bool operator==(const PtrType* ptr) const { return GetPtr() == ptr; }

  /// \brief Compares the pointer part for inequality (flags are ignored)
  bool operator!=(const PtrType* ptr) const { return !(*this == ptr); }

  /// \brief Compares the pointer part for equality (flags are ignored)
  bool operator==(PtrType* ptr) const { return GetPtr() == ptr; }

  /// \brief Compares the pointer part for inequality (flags are ignored)
  bool operator!=(PtrType* ptr) const { return !(*this == ptr); }

  /// \brief Compares the pointer part for equality (flags are ignored)
  bool operator==(std::nullptr_t) const { return GetPtr() == nullptr; }

  /// \brief Compares the pointer part for inequality (flags are ignored)
  bool operator!=(std::nullptr_t) const { return !(*this == nullptr); }

  /// \brief Checks whether the pointer part is not nullptr (flags are ignored)
  operator bool() const { return GetPtr() != nullptr; }

  /// \brief Dereferences the pointer
  const PtrType* operator->() const { return GetPtr(); }

  /// \brief Dereferences the pointer
  PtrType* operator->() { return GetPtr(); }

  /// \brief Dereferences the pointer
  const PtrType& operator*() const { return *GetPtr(); }

  /// \brief Dereferences the pointer
  PtrType& operator*() { return *GetPtr(); }
};
