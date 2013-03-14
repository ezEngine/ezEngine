#pragma once

#include <Foundation/Basics/TypeTraits.h>
#include <Foundation/Threading/AtomicUtils.h>

/// Integer class that can be manipulated in an atomic (i.e. thread-safe) fashion.
template <typename T>
class ezAtomicInteger
{
public:
  EZ_DECLARE_POD_TYPE();

  /// Default constructor
  ezAtomicInteger(); // [tested]

  /// Initializes the object with a value
  ezAtomicInteger(const T value); // [tested]

  /// Copy-constructor
  ezAtomicInteger(const ezAtomicInteger<T>& value); // [tested]

  /// Assigns a new integer value to this object
  ezAtomicInteger& operator=(T value); // [tested]

  /// Assignment operator
  ezAtomicInteger& operator=(const ezAtomicInteger& value); // [tested]

  /// Increments the internal value and returns the incremented value
  T Increment(); // [tested]

  /// Decrements the internal value and returns the decremented value
  T Decrement(); // [tested]

  void Add(T x); // [tested]
  void Subtract(T x); // [tested]

  void And(T x); // [tested]
  void Or(T x); // [tested]
  void Xor(T x); // [tested]

  void Min(T x); // [tested]
  void Max(T x); // [tested]

  /// Replaces the internal value with x and returns the original internal value.
  T Swap(T x);

  operator T() const;

private:
  volatile T m_value;
};

// Include inline file
#include <Foundation/Threading/Implementation/AtomicInteger_inl.h>

typedef ezAtomicInteger<ezInt32> ezAtomicInteger32; // [tested]
typedef ezAtomicInteger<ezInt64> ezAtomicInteger64; // [tested]
