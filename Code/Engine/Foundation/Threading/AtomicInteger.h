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
  ezAtomicInteger();

  /// Initializes the object with a value
  ezAtomicInteger(const T value);

  /// Copy-constructor
  ezAtomicInteger(const ezAtomicInteger<T>& value);

  /// Assigns a new integer value to this object
  ezAtomicInteger& operator=(T value);

  /// Assignment operator
  ezAtomicInteger& operator=(const ezAtomicInteger& value);

  /// Increments the internal value and returns the incremented value
  T Increment();

  /// Decrements the internal value and returns the decremented value
  T Decrement();

  void Add(T x);
  void Subtract(T x);

  void And(T x);
  void Or(T x);
  void Xor(T x);

  void Min(T x);
  void Max(T x);

  /// Replaces the internal value with x and returns the original internal value.
  T Swap(T x);

  operator T() const;

private:
  volatile T m_value;
};

// Include inline file
#include <Foundation/Threading/Implementation/AtomicInteger_inl.h>

typedef ezAtomicInteger<ezInt32> ezAtomicInteger32;
typedef ezAtomicInteger<ezInt64> ezAtomicInteger64;
