#pragma once

#include <Foundation/Types/TypeTraits.h>

#include <Foundation/Threading/AtomicUtils.h>

template <int T>
struct ezAtomicStorageType
{
};

template <>
struct ezAtomicStorageType<1>
{
  using Type = ezInt32;
};

template <>
struct ezAtomicStorageType<2>
{
  using Type = ezInt32;
};

template <>
struct ezAtomicStorageType<4>
{
  using Type = ezInt32;
};

template <>
struct ezAtomicStorageType<8>
{
  using Type = ezInt64;
};

/// \brief Thread-safe atomic integer with lock-free operations
///
/// Provides atomic (thread-safe) operations on integer types without requiring explicit locking.
/// All operations are lock-free and use hardware atomic instructions where available.
/// Supports common atomic operations like increment, decrement, compare-and-swap, and bitwise operations.
/// The class is templated to work with various integer types while ensuring proper atomic alignment.
/// Use ezAtomicInteger32 or ezAtomicInteger64 typedefs for common cases.
template <typename T>
class ezAtomicInteger
{
  using UnderlyingType = typename ezAtomicStorageType<sizeof(T)>::Type;

public:
  EZ_DECLARE_POD_TYPE();

  /// \brief Initializes the value to zero.
  ezAtomicInteger(); // [tested]

  /// \brief Initializes the object with a value
  ezAtomicInteger(const T value); // [tested]

  /// \brief Copy-constructor
  ezAtomicInteger(const ezAtomicInteger<T>& value); // [tested]

  /// \brief Assigns a new integer value to this object
  ezAtomicInteger& operator=(T value); // [tested]

  /// \brief Assignment operator
  ezAtomicInteger& operator=(const ezAtomicInteger& value); // [tested]

  /// \brief Increments the internal value and returns the incremented value
  T Increment(); // [tested]

  /// \brief Decrements the internal value and returns the decremented value
  T Decrement(); // [tested]

  /// \brief Increments the internal value and returns the value immediately before the increment
  T PostIncrement(); // [tested]

  /// \brief Decrements the internal value and returns the value immediately before the decrement
  T PostDecrement();  // [tested]

  void Add(T x);      // [tested]
  void Subtract(T x); // [tested]

  void And(T x);      // [tested]
  void Or(T x);       // [tested]
  void Xor(T x);      // [tested]

  void Min(T x);      // [tested]
  void Max(T x);      // [tested]

  /// \brief Sets the internal value to x and returns the original internal value.
  T Set(T x); // [tested]

  /// \brief Atomic conditional assignment operation
  ///
  /// Sets the value to x only if the current value equals expected. Returns true if the assignment
  /// occurred, false otherwise. This is a fundamental building block for lock-free algorithms.
  bool TestAndSet(T expected, T x); // [tested]

  /// \brief Atomic compare-and-swap operation returning the previous value
  ///
  /// If the current value equals expected, it is atomically replaced with x.
  /// Always returns the value that was present before the operation, regardless of whether
  /// the swap occurred. This enables retry loops in lock-free algorithms.
  T CompareAndSwap(T expected, T x); // [tested]

  operator T() const;                // [tested]

private:
  UnderlyingType m_Value;
};

/// \brief An atomic boolean variable. This is just a wrapper around an atomic int32 for convenience.
class ezAtomicBool
{
public:
  /// \brief Initializes the bool to 'false'.
  ezAtomicBool(); // [tested]
  ~ezAtomicBool();

  /// \brief Initializes the object with a value
  ezAtomicBool(bool value); // [tested]

  /// \brief Copy-constructor
  ezAtomicBool(const ezAtomicBool& rhs);

  /// \brief Sets the bool to the given value and returns its previous value.
  bool Set(bool value); // [tested]

  /// \brief Sets the bool to the given value.
  void operator=(bool value); // [tested]

  /// \brief Sets the bool to the given value.
  void operator=(const ezAtomicBool& rhs);

  /// \brief Returns the current value.
  operator bool() const; // [tested]

  /// \brief Atomic conditional assignment for boolean values
  ///
  /// Sets the value to newValue only if the current value equals expected.
  /// Returns true if the assignment occurred, false otherwise.
  bool TestAndSet(bool bExpected, bool bNewValue);

private:
  ezAtomicInteger<ezInt32> m_iAtomicInt;
};

// Include inline file
#include <Foundation/Threading/Implementation/AtomicInteger_inl.h>

using ezAtomicInteger32 = ezAtomicInteger<ezInt32>; // [tested]
using ezAtomicInteger64 = ezAtomicInteger<ezInt64>; // [tested]
static_assert(sizeof(ezAtomicInteger32) == sizeof(ezInt32));
static_assert(sizeof(ezAtomicInteger64) == sizeof(ezInt64));
