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
  static ezInt32 Read(volatile const ezInt32& src); // [tested]

  /// \brief Returns src as an atomic operation and returns its value.
  static ezInt64 Read(volatile const ezInt64& src); // [tested]

  /// \brief Increments dest as an atomic operation and returns the new value.
  static ezInt32 Increment(volatile ezInt32& dest); // [tested]
  
  /// \brief Increments dest as an atomic operation and returns the new value.
  static ezInt64 Increment(volatile ezInt64& dest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the new value.
  static ezInt32 Decrement(volatile ezInt32& dest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the new value.
  static ezInt64 Decrement(volatile ezInt64& dest); // [tested]

  /// \brief Adds value to dest as an atomic operation.
  static void Add(volatile ezInt32& dest, ezInt32 value); // [tested]

  /// \brief Adds value to dest as an atomic operation.
  static void Add(volatile ezInt64& dest, ezInt64 value); // [tested]

  /// \brief Performs an atomic bitwise AND on dest using value.
  static void And(volatile ezInt32& dest, ezInt32 value); // [tested]

  /// \brief Performs an atomic bitwise AND on dest using value.
  static void And(volatile ezInt64& dest, ezInt64 value); // [tested]

  /// \brief Performs an atomic bitwise OR on dest using value.
  static void Or(volatile ezInt32& dest, ezInt32 value); // [tested]

  /// \brief Performs an atomic bitwise OR on dest using value.
  static void Or(volatile ezInt64& dest, ezInt64 value); // [tested]

  /// \brief Performs an atomic bitwise XOR on dest using value.
  static void Xor(volatile ezInt32& dest, ezInt32 value); // [tested]

  /// \brief Performs an atomic bitwise XOR on dest using value.
  static void Xor(volatile ezInt64& dest, ezInt64 value); // [tested]

  /// \brief Performs an atomic min operation on dest using value.
  static void Min(volatile ezInt32& dest, ezInt32 value); // [tested]

  /// \brief Performs an atomic min operation on dest using value.
  static void Min(volatile ezInt64& dest, ezInt64 value); // [tested]

  /// \brief Performs an atomic max operation on dest using value.
  static void Max(volatile ezInt32& dest, ezInt32 value); // [tested]

  /// \brief Performs an atomic max operation on dest using value.
  static void Max(volatile ezInt64& dest, ezInt64 value); // [tested]

  /// \brief Sets dest to value as an atomic operation and returns the original value of dest.
  static ezInt32 Set(volatile ezInt32& dest, ezInt32 value); // [tested]

  /// \brief Sets dest to value as an atomic operation and returns the original value of dest.
  static ezInt64 Set(volatile ezInt64& dest, ezInt64 value); // [tested]

  /// \brief Sets dest to value if dest is equal to expected and returns true, otherwise does nothing and returns false.
  static bool TestAndSet(volatile ezInt32& dest, ezInt32 expected, ezInt32 value); // [tested]

  /// \brief Sets dest to value if dest is equal to expected and returns true, otherwise does nothing and returns false.
  static bool TestAndSet(volatile ezInt64& dest, ezInt64 expected, ezInt64 value); // [tested]

  /// \brief Sets dest to value if dest is equal to expected and returns true, otherwise does nothing and returns false.
  static bool TestAndSet(void** volatile dest, void* expected, void* value); // [tested]
};

  // Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Threading/Implementation/Win/AtomicUtils_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
  #include <Foundation/Threading/Implementation/Posix/AtomicUtils_posix.h>
#else
  #error "Atomics are not implemented on current platform"
#endif

