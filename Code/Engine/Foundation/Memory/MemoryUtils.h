#pragma once

#include <Foundation/Basics.h>

/// \brief This class provides functions to work on raw memory.
class ezMemoryUtils
{
public:
  /// \brief constructs objects of type T in a raw buffer
  template <typename T>
  static void Construct(T* pDestination, size_t uiCount);

  /// \brief constructs objects of type T in a raw buffer using the copy constructor
  template <typename T>
  static void Construct(T* pDestination, const T& copy, size_t uiCount);

  /// \brief constructs objects of type T in a raw buffer from an existing array using the copy constructor
  template <typename T>
  static void Construct(T* pDestination, const T* pSource, size_t uiCount);

  /// \brief destructs objects of type T in pDestination
  template <typename T>
  static void Destruct(T* pDestination, size_t uiCount);

  /// \brief copies objects of type T from pSource to pDestination, pDestination must not be a raw buffer
  template <typename T>
  static void Copy(T* pDestination, const T* pSource, size_t uiCount);

  /// \brief moves objects of type T from pSource to pDestination, pDestination must not be a raw buffer
  template <typename T>
  static void Move(T* pDestination, const T* pSource, size_t uiCount);

  /// \brief tests if objects of type T from pSource and pDestination are equal
  template <typename T>
  static bool IsEqual(const T* a, const T* b, size_t uiCount = 1);

  /// \brief zeros out a raw buffer
  template <typename T>
  static void ZeroFill(T* pDestination, size_t uiCount = 1);

  template <typename T>
  static T* AddByteOffset(T* ptr, ptrdiff_t iOffset);

  template <typename T>
  static const T* AddByteOffsetConst(const T* ptr, ptrdiff_t iOffset);

  /// \brief Alignes the pointer ptr by moving its address backwards to the previous multiple of uiAlignment.  
  template <typename T>
  static T* Align(T* ptr, size_t uiAlignment);

  /// \brief Checks whether ptr is aligned to a memory address that is a multiple of uiAlignment.
  template <typename T>
  static bool IsAligned(const T* ptr, size_t uiAlignment);

  /// \brief Reserves the lower 4GB of address space in 64-bit builds to ensure all allocations start above 4GB.
  ///
  /// \note Note that this does NOT reserve 4GB of RAM, only address space.
  ///       This can help to detect pointer truncation. In 32-bit builds it does nothing.
  static void ReserveLower4GBAddressSpace();

private:
  template <typename T>
  static void Construct(T* pDestination, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void Construct(T* pDestination, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static void Construct(T* pDestination, const T& copy, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void Construct(T* pDestination, const T& copy, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static void Construct(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void Construct(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static void Destruct(T* pDestination, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void Destruct(T* pDestination, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static void Copy(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void Copy(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static void Move(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void Move(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static bool IsEqual(const T* a, const T* b, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static bool IsEqual(const T* a, const T* b, size_t uiCount, ezTypeIsClass);
};

#include <Foundation/Memory/Implementation/MemoryUtils_inl.h>

