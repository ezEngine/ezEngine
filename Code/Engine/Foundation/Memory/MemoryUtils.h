#pragma once

#include <Foundation/Basics.h>
#include <cstdint> // for uintptr_t

enum ezConstructionMode
{
  ConstructAll = 0,     /// < default initialize all types, including std::is_trivial types
  SkipTrivialTypes = 1, ///< If the given type to construct is trivial, nothing will be done
};

/// \brief This class provides functions to work on raw memory.
///
/// \details
///   The following concepts are realized:
///   Copy: Copying a object from a to b means that two equivalent objects will exists in both a and b.
///   Move: Moving an object from a to b means that the object will exist in b afterwards but a will be empty afterwards, but not
///   destructed. This strictly requires an available move constructor (compile error otherwise).
///   Relocate: Relocating an object from a to b
///   means that the object will exist in b afterwards but will no longer exist in a, which means a will be moved if available or copied,
///   but destructed afterwards in any case.
///   Construct: Constructing assumes that the destination does not contain a valid object.
///   Overlapped: The source and destination range may overlap for the operation to be performed.
///   The above mentioned concepts can be combined, e.g. RelocateConstruct for relocating to an uninitialized buffer.
class ezMemoryUtils
{
public:
  using ConstructorFunction = void (*)(void* pDestination);
  using CopyConstructorFunction = void (*)(void* pDestination, const void* pSource);
  using DestructorFunction = void (*)(void* pDestination);

  /// \brief Constructs \a uiCount objects of type T in a raw buffer at \a pDestination.
  ///
  /// The ezConstructionMode template argument determines whether trivial types will be skipped.
  template <ezConstructionMode mode, typename T>
  static void Construct(T* pDestination, size_t uiCount = 1); // [tested]

  /// \brief Returns a function pointer to construct an instance of T. Returns nullptr for trivial types.
  template <ezConstructionMode mode, typename T>
  static ConstructorFunction MakeConstructorFunction(); // [tested]

  /// \brief Constructs \a uiCount objects of type T in a raw buffer at \a pDestination, by creating \a uiCount copies of \a copy.
  template <typename Destination, typename Source>
  static void CopyConstruct(Destination* pDestination, const Source& copy, size_t uiCount = 1); // [tested]

  /// \brief Constructs \a uiCount objects of type T in a raw buffer at \a pDestination from an existing array of objects at \a pSource by
  /// using copy construction.
  template <typename T>
  static void CopyConstructArray(T* pDestination, const T* pSource, size_t uiCount); // [tested]

  /// \brief Returns a function pointer to copy construct an instance of T.
  template <typename T>
  static CopyConstructorFunction MakeCopyConstructorFunction(); // [tested]

  /// \brief Constructs an object of type T in a raw buffer at \a pDestination, by using move construction from \a source.
  template <typename T>
  static void MoveConstruct(T* pDestination, T&& source); // [tested]

  /// \brief Constructs \a uiCount objects of type T in a raw buffer at \a pDestination from an existing array of objects at \a pSource by
  /// using move construction.
  template <typename T>
  static void MoveConstruct(T* pDestination, T* pSource, size_t uiCount = 1);

  /// \brief This function will either move call MoveConstruct or CopyConstruct for a single element \a source, depending on whether it was
  /// called with a rvalue reference or a const reference to \a source.
  template <typename Destination, typename Source>
  static void CopyOrMoveConstruct(Destination* pDestination, Source&& source);

  /// \brief Constructs \a uiCount objects of type T in a raw buffer at \a pDestination from an existing array of objects at \a pSource by
  /// using move construction if availble, otherwise by copy construction. Calls destructor of source elements in any case (if it is a non
  /// primitive or mem-relocatable type).
  template <typename T>
  static void RelocateConstruct(T* pDestination, T* pSource, size_t uiCount = 1);

  /// \brief Destructs \a uiCount objects of type T at \a pDestination.
  template <typename T>
  static void Destruct(T* pDestination, size_t uiCount = 1); // [tested]

  /// \brief Returns a function pointer to destruct an instance of T. Returns nullptr for POD-types.
  template <typename T>
  static DestructorFunction MakeDestructorFunction(); // [tested]

  /// \brief Copies objects of type T from \a pSource to \a pDestination.
  ///
  /// If the two buffers overlap use CopyOverlapped instead.
  template <typename T>
  static void Copy(T* pDestination, const T* pSource, size_t uiCount = 1); // [tested]

  /// \brief Copies exactly \a uiNumBytesToCopy from \a pSource to \a pDestination, independent of the involved types and their sizes.
  static void RawByteCopy(void* pDestination, const void* pSource, size_t uiNumBytesToCopy);

  /// \brief Copies objects of type T from \a pSource to \a pDestination.
  ///
  /// The two buffers may overlap when using this method.
  template <typename T>
  static void CopyOverlapped(T* pDestination, const T* pSource, size_t uiCount = 1);

  /// \brief Moves objects of type T from \a pSource to \a pDestination.
  template <typename T>
  static void Relocate(T* pDestination, T* pSource, size_t uiCount = 1); // [tested]

  /// \brief Moves objects of type T from \a pSource to \a pDestination.
  ///
  /// The two buffers may overlap when using this method.
  template <typename T>
  static void RelocateOverlapped(T* pDestination, T* pSource, size_t uiCount = 1); // [tested]

  /// \brief Moves \a uiCount objects in \a pDestination by one object and copies \a source to the free space.
  template <typename T>
  static void Prepend(T* pDestination, const T& source, size_t uiCount);

  /// \brief Moves \a uiCount objects in \a pDestination by one object and moves \a source to the free space.
  template <typename T>
  static void Prepend(T* pDestination, T&& source, size_t uiCount);

  /// \brief Moves \a uiCount objects in \a pDestination by \a uiSourceCount objects and copies \a source to the free space.
  template <typename T>
  static void Prepend(T* pDestination, const T* pSource, size_t uiSourceCount, size_t uiCount);

  /// \brief Tests if objects of type T from \a pSource and \a pDestination are equal.
  template <typename T>
  static bool IsEqual(const T* a, const T* b, size_t uiCount = 1); // [tested]

  /// \brief Zeros every byte in the provided memory buffer.
  template <typename T>
  static void ZeroFill(T* pDestination, size_t uiCount = 1); // [tested]

  /// \brief Overload to prevent confusing calling this on a single object or a static array of objects. Use ZeroFillArray() instead.
  template <typename T, size_t N>
  static void ZeroFill(T (&destination)[N]) = delete;

  /// \brief Zeros every byte in the provided memory buffer.
  template <typename T, size_t N>
  static void ZeroFillArray(T (&destination)[N]); // [tested]

  /// \brief Fills every byte of the provided buffer with the given value
  template <typename T>
  static void PatternFill(T* pDestination, ezUInt8 uiBytePattern, size_t uiCount = 1); // [tested]

  /// \brief Overload to prevent confusing calling this on a single object or a static array of objects. Use PatternFillArray() instead.
  template <typename T, size_t N>
  static void PatternFill(T (&destination)[N], ezUInt8 uiBytePattern) = delete;

  /// \brief Fills every byte of the provided buffer with the given value
  template <typename T, size_t N>
  static void PatternFillArray(T (&destination)[N], ezUInt8 uiBytePattern); // [tested]

  /// \brief Compares two buffers of raw memory byte wise.
  template <typename T>
  static ezInt32 Compare(const T* a, const T* b, size_t uiCount = 1); // [tested]

  /// \brief Compares exactly \a uiNumBytesToCompare from \a a and \a b, independent of the involved types and their sizes.
  static ezInt32 RawByteCompare(const void* a, const void* b, size_t uiNumBytesToCompare);

  /// \brief Returns the address stored in \a ptr plus the given byte offset \a iOffset, cast to type \a T.
  ///
  /// This is useful when working with raw memory, to safely modify a pointer without having to take care of the
  /// details of pointer arithmetic.
  template <typename T>
  static T* AddByteOffset(T* pPtr, std::ptrdiff_t offset); // [tested]

  /// \brief Aligns the pointer \a ptr by moving its address backwards to the previous multiple of \a uiAlignment.
  template <typename T>
  static T* AlignBackwards(T* pPtr, size_t uiAlignment); // [tested]

  /// \brief Aligns the pointer \a ptr by moving its address forwards to the next multiple of \a uiAlignment.
  template <typename T>
  static T* AlignForwards(T* pPtr, size_t uiAlignment); // [tested]

  /// \brief Aligns the given size \a uiSize by rounding up to the next multiple of the size.
  template <typename T>
  static T AlignSize(T uiSize, T uiAlignment); // [tested]

  /// \brief Checks whether \a ptr is aligned to a memory address that is a multiple of \a uiAlignment.
  template <typename T>
  static bool IsAligned(const T* pPtr, size_t uiAlignment); // [tested]

  /// \brief Checks whether the given size is aligned.
  template <typename T>
  static bool IsSizeAligned(T uiSize, T uiAlignment); // [tested]

  /// \brief Reserves the lower 4GB of address space in 64-bit builds to ensure all allocations start above 4GB.
  ///
  /// \note Note that this does NOT reserve 4GB of RAM, only address space.
  ///       This can help to detect pointer truncation. In 32-bit builds it does nothing.
  ///
  /// Currently only implemented on Windows.
  static void ReserveLower4GBAddressSpace();
};

#include <Foundation/Memory/Implementation/MemoryUtils_inl.h>
