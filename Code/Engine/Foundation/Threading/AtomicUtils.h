#pragma once

#include <Foundation/Basics.h>

/// \brief Low-level platform-independent atomic operations for thread-safe programming
///
/// Provides atomic (indivisible) operations that are faster than mutexes for simple operations
/// but slower than regular operations. Use only when thread safety is required.
///
/// Important considerations:
/// - Individual operations are atomic, but sequences of operations are not
/// - Only use in code that requires thread safety - atomic ops have performance overhead
/// - For higher-level usage, prefer ezAtomicInteger which wraps these utilities
/// - All operations use lock-free hardware instructions where available
///
/// These functions form the foundation for lock-free data structures and algorithms.
struct EZ_FOUNDATION_DLL ezAtomicUtils
{
  /// \brief Atomically reads a 32-bit integer value
  ///
  /// Ensures the read operation is atomic and not subject to partial reads on all platforms.
  static ezInt32 Read(const ezInt32& iSrc); // [tested]

  /// \brief Atomically reads a 64-bit integer value
  ///
  /// Ensures the read operation is atomic and not subject to partial reads on all platforms.
  static ezInt64 Read(const ezInt64& iSrc); // [tested]

  /// \brief Increments dest as an atomic operation and returns the new value.
  static ezInt32 Increment(ezInt32& ref_iDest); // [tested]

  /// \brief Increments dest as an atomic operation and returns the new value.
  static ezInt64 Increment(ezInt64& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the new value.
  static ezInt32 Decrement(ezInt32& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the new value.
  static ezInt64 Decrement(ezInt64& ref_iDest); // [tested]

  /// \brief Increments dest as an atomic operation and returns the old value.
  static ezInt32 PostIncrement(ezInt32& ref_iDest); // [tested]

  /// \brief Increments dest as an atomic operation and returns the old value.
  static ezInt64 PostIncrement(ezInt64& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the old value.
  static ezInt32 PostDecrement(ezInt32& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the old value.
  static ezInt64 PostDecrement(ezInt64& ref_iDest); // [tested]

  /// \brief Adds value to dest as an atomic operation.
  static void Add(ezInt32& ref_iDest, ezInt32 value); // [tested]

  /// \brief Adds value to dest as an atomic operation.
  static void Add(ezInt64& ref_iDest, ezInt64 value); // [tested]

  /// \brief Performs an atomic bitwise AND on dest using value.
  static void And(ezInt32& ref_iDest, ezInt32 value); // [tested]

  /// \brief Performs an atomic bitwise AND on dest using value.
  static void And(ezInt64& ref_iDest, ezInt64 value); // [tested]

  /// \brief Performs an atomic bitwise OR on dest using value.
  static void Or(ezInt32& ref_iDest, ezInt32 value); // [tested]

  /// \brief Performs an atomic bitwise OR on dest using value.
  static void Or(ezInt64& ref_iDest, ezInt64 value); // [tested]

  /// \brief Performs an atomic bitwise XOR on dest using value.
  static void Xor(ezInt32& ref_iDest, ezInt32 value); // [tested]

  /// \brief Performs an atomic bitwise XOR on dest using value.
  static void Xor(ezInt64& ref_iDest, ezInt64 value); // [tested]

  /// \brief Performs an atomic min operation on dest using value.
  static void Min(ezInt32& ref_iDest, ezInt32 value); // [tested]

  /// \brief Performs an atomic min operation on dest using value.
  static void Min(ezInt64& ref_iDest, ezInt64 value); // [tested]

  /// \brief Performs an atomic max operation on dest using value.
  static void Max(ezInt32& ref_iDest, ezInt32 value); // [tested]

  /// \brief Performs an atomic max operation on dest using value.
  static void Max(ezInt64& ref_iDest, ezInt64 value); // [tested]

  /// \brief Sets dest to value as an atomic operation and returns the original value of dest.
  static ezInt32 Set(ezInt32& ref_iDest, ezInt32 value); // [tested]

  /// \brief Sets dest to value as an atomic operation and returns the original value of dest.
  static ezInt64 Set(ezInt64& ref_iDest, ezInt64 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(ezInt32& ref_iDest, ezInt32 iExpected, ezInt32 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(ezInt64& ref_iDest, ezInt64 iExpected, ezInt64 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(void** pDest, void* pExpected, void* value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value*. Otherwise *dest* will not be modified. Always returns the value
  /// of *dest* before the modification.
  static ezInt32 CompareAndSwap(ezInt32& ref_iDest, ezInt32 iExpected, ezInt32 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value*. Otherwise *dest* will not be modified. Always returns the value
  /// of *dest* before the modification.
  static ezInt64 CompareAndSwap(ezInt64& ref_iDest, ezInt64 iExpected, ezInt64 value); // [tested]
};

// include platforma specific implementation
#include <AtomicUtils_Platform.h>
