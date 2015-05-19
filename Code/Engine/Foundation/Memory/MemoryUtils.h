#pragma once

#include <Foundation/Basics.h>

/// \brief This class provides functions to work on raw memory.
///
/// \details
///   The following concepts are realized:
///   Copy: Copying a object from a to b means that two equivalent objects will exists in both a and b.
///   Relocate: Relocating an object from a to b means that the object will exist in b afterwards but will no longer exist in a, which means a will be destructed.
///   Move: Moving an object from a to b menas that the object will exist in b afterwards but a will be empty afterwards, but not destructed.
///   Construct: Constructing assumes that the destination does not contain a valid object.
///   Overlapped: The source and destination range may overlap for the operation to be performed.
///   The above mentioned concepts can be combined, e.g. RelocateConstruct for relocating to an uninitialized buffer.
class ezMemoryUtils
{
public:
  typedef void(*ConstructorFunction)(void* pDestination);
  typedef void(*CopyConstructorFunction)(void* pDestination, const void* pSource);
  typedef void(*DestructorFunction)(void* pDestination);

  /// \brief Constructs \a uiCount objects of type T in a raw buffer at \a pDestination.
  ///
  /// You should use 'DefaultConstruct' instead if default construction is needed for trivial types as well.
  template <typename T>
  static void Construct(T* pDestination, size_t uiCount); // [tested]

  /// \brief Returns a function pointer to construct an instance of T. Returns nullptr for trivial types.
  template <typename T>
  static ConstructorFunction MakeConstructorFunction(); // [tested]

  /// \brief Default constructs \a uiCount objects of type T in a raw buffer at \a pDestination regardless of T being a class, POD or trivial.
  template <typename T>
  static void DefaultConstruct(T* pDestination, size_t uiCount); // [tested]

  /// \brief Returns a function pointer to construct an instance of T. Always returns a constructor function regardless of T being a class, POD or trivial.
  template <typename T>
  static ConstructorFunction MakeDefaultConstructorFunction(); // [tested]

  /// \brief Constructs \a uiCount objects of type T in a raw buffer at \a pDestination, by creating \a uiCount copies of \a copy.
  template <typename T>
  static void CopyConstruct(T* pDestination, const T& copy, size_t uiCount); // [tested]

  /// \brief Constructs \a uiCount objects of type T in a raw buffer at \a pDestination from an existing array of objects at \a pSource by using copy construction.
  template <typename T>
  static void CopyConstruct(T* pDestination, const T* pSource, size_t uiCount); // [tested]

  /// \brief Returns a function pointer to copy construct an instance of T.
  template <typename T>
  static CopyConstructorFunction MakeCopyConstructorFunction(); // [tested]

  /// \brief Constructs \a uiCount objects of type T in a raw buffer at \a pDestionation from an existing array of objects at \a pSource by using move construction.
  template <typename T>
  static void RelocateConstruct(T* pDestination, T* pSource, size_t uiCount);

  /// \brief Constructs an object of type T in a raw buffer at \a pDestionation, by using move construction from \a source.
  template <typename T>
  static void MoveConstruct(T* pDestination, T&& source); // [tested]

  /// \brief Destructs \a uiCount objects of type T at \a pDestination.
  template <typename T>
  static void Destruct(T* pDestination, size_t uiCount); // [tested]

  /// \brief Returns a function pointer to destruct an instance of T. Returns nullptr for POD-types.
  template <typename T>
  static DestructorFunction MakeDestructorFunction(); // [tested]

  /// \brief Copies objects of type T from \a pSource to \a pDestination.
  ///
  /// If the two buffers overlap use CopyOverlapped instead.
  template <typename T>
  static void Copy(T* pDestination, const T* pSource, size_t uiCount); // [tested]

  /// \brief Copies objects of type T from \a pSource to \a pDestination.
  ///
  /// The two buffers may overlap when using this method.
  template <typename T>
  static void CopyOverlapped(T* pDestination, const T* pSource, size_t uiCount);

  /// \brief Moves objects of type T from \a pSource to \a pDestination.
  template <typename T>
  static void Relocate(T* pDestination, T* pSource, size_t uiCount); // [tested]

  /// \brief Moves objects of type T from \a pSource to \a pDestination.
  ///
  /// The two buffers may overlap when using this method.
  template <typename T>
  static void RelocateOverlapped(T* pDestination, T* pSource, size_t uiCount); // [tested]

  /// \brief Moves \a uiCount objects in \a pDestination by one object and copies \a source to the free space.
  template <typename T>
  static void Prepend(T* pDestination, const T& source, size_t uiCount);

  /// \brief Moves \a uiCount objects in \a pDestination by one object and moves \a source to the free space.
  template <typename T>
  static void Prepend(T* pDestination, T&& source, size_t uiCount);

  /// \brief Tests if objects of type T from \a pSource and \a pDestination are equal.
  template <typename T>
  static bool IsEqual(const T* a, const T* b, size_t uiCount = 1); // [tested]

  /// \brief Zeros out buffer of a raw memory.
  template <typename T>
  static void ZeroFill(T* pDestination, size_t uiCount = 1); // [tested]

  /// \brief Compares two buffers of raw memory byte wise.
  template <typename T>
  static ezInt32 ByteCompare(const T* a, const T* b, size_t uiCount = 1); // [tested]

  /// \brief Returns the address stored in \a ptr plus the given byte offset \a iOffset, cast to type \a T.
  ///
  /// This is useful when working with raw memory, to safely modify a pointer without having to take care of the
  /// details of pointer arithmetic.
  template <typename T>
  static T* AddByteOffset(T* ptr, ptrdiff_t iOffset); // [tested]

  /// \brief Returns the address stored in \a ptr plus the given byte offset \a iOffset, cast to type \a const \a T.
  ///
  /// This is useful when working with raw memory, to safely modify a pointer without having to take care of the
  /// details of pointer arithmetic. Also it allows you to modify the pointer without having to do a const_cast.
  template <typename T>
  static const T* AddByteOffsetConst(const T* ptr, ptrdiff_t iOffset); // [tested]

  /// \brief Aligns the pointer \a ptr by moving its address backwards to the previous multiple of \a uiAlignment.  
  template <typename T>
  static T* Align(T* ptr, size_t uiAlignment); // [tested]

  /// \brief Aligns the given size \a uiSize by rounding up to the next multiple of the size. 
  template <typename T>
  static T AlignSize(T uiSize, T uiAlignment); // [tested]

  /// \brief Checks whether \a ptr is aligned to a memory address that is a multiple of \a uiAlignment.
  template <typename T>
  static bool IsAligned(const T* ptr, size_t uiAlignment); // [tested]

  /// \brief Reserves the lower 4GB of address space in 64-bit builds to ensure all allocations start above 4GB.
  ///
  /// \note Note that this does NOT reserve 4GB of RAM, only address space.
  ///       This can help to detect pointer truncation. In 32-bit builds it does nothing.
  ///
  /// Currently only implemented on Windows.
  static void ReserveLower4GBAddressSpace();

private:
  template <typename T>
  static void Construct(T* pDestination, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void Construct(T* pDestination, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static ConstructorFunction MakeConstructorFunction(ezTypeIsPod);
  template <typename T>
  static ConstructorFunction MakeConstructorFunction(ezTypeIsClass);

  template <typename T>
  static void CopyConstruct(T* pDestination, const T& copy, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void CopyConstruct(T* pDestination, const T& copy, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static void CopyConstruct(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void CopyConstruct(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static void RelocateConstruct(T* pDestination, T* pSource, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void RelocateConstruct(T* pDestination, T* pSource, size_t uiCount, ezTypeIsMemRelocatable);
  template <typename T>
  static void RelocateConstruct(T* pDestination, T* pSource, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static void Destruct(T* pDestination, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void Destruct(T* pDestination, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static DestructorFunction MakeDestructorFunction(ezTypeIsPod);
  template <typename T>
  static DestructorFunction MakeDestructorFunction(ezTypeIsClass);

  template <typename T>
  static void Copy(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void Copy(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static void CopyOverlapped(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void CopyOverlapped(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static void Relocate(T* pDestination, T* pSource, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void Relocate(T* pDestination, T* pSource, size_t uiCount, ezTypeIsMemRelocatable);
  template <typename T>
  static void Relocate(T* pDestination, T* pSource, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static void RelocateOverlapped(T* pDestination, T* pSource, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void RelocateOverlapped(T* pDestination, T* pSource, size_t uiCount, ezTypeIsMemRelocatable);
  template <typename T>
  static void RelocateOverlapped(T* pDestination, T* pSource, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static void Prepend(T* pDestination, const T& source, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void Prepend(T* pDestination, const T& source, size_t uiCount, ezTypeIsMemRelocatable);
  template <typename T>
  static void Prepend(T* pDestination, const T& source, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static void Prepend(T* pDestination, T&& source, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static void Prepend(T* pDestination, T&& source, size_t uiCount, ezTypeIsMemRelocatable);
  template <typename T>
  static void Prepend(T* pDestination, T&& source, size_t uiCount, ezTypeIsClass);

  template <typename T>
  static bool IsEqual(const T* a, const T* b, size_t uiCount, ezTypeIsPod);
  template <typename T>
  static bool IsEqual(const T* a, const T* b, size_t uiCount, ezTypeIsClass);
};

#include <Foundation/Memory/Implementation/MemoryUtils_inl.h>

