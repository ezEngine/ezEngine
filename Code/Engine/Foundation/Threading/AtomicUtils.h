#pragma once

#include <Foundation/Basics.h>

/// \brief This class provides functions to do atomic operations.
///
/// Atomic operations are generally faster than mutexes, and should therefore be preferred whenever possible.
/// However only the operations in themselves are atomic, once you execute several of them in sequence,
/// the sequence will not be atomic.
/// Also atomic operations are a lot slower than non-atomic operations, thus you should not use them in code that
/// does not need to be thread-safe.
/// ezAtomicInteger is built on top of ezAtomicUtils and provides a more convenient interface to use atomic
/// integer instructions.
struct EZ_FOUNDATION_DLL ezAtomicUtils
{
  /// \brief Returns src as an atomic operation and returns its value.
  static ezInt32 Read(const ezInt32& iSrc); // [tested]

  /// \brief Returns src as an atomic operation and returns its value.
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
