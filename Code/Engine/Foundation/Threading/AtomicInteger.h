#pragma once

#include <Foundation/Types/TypeTraits.h>
#include <Foundation/Threading/AtomicUtils.h>

/// \brief Integer class that can be manipulated in an atomic (i.e. thread-safe) fashion.
template <typename T>
class ezAtomicInteger
{
public:
  EZ_DECLARE_POD_TYPE();

  /// \brief Default constructor
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

  void Add(T x); // [tested]
  void Subtract(T x); // [tested]

  void And(T x); // [tested]
  void Or(T x); // [tested]
  void Xor(T x); // [tested]

  void Min(T x); // [tested]
  void Max(T x); // [tested]

  /// \brief Sets the internal value to x and returns the original internal value.
  T Set(T x); // [tested]

  /// \brief Sets the internal value to x if the internal value is equal to expected and returns true, otherwise does nothing and returns false.
  bool TestAndSet(T expected, T x); // [tested]

  operator T() const; // [tested]

private:
  volatile T m_value;
};

// Include inline file
#include <Foundation/Threading/Implementation/AtomicInteger_inl.h>

typedef ezAtomicInteger<ezInt32> ezAtomicInteger32; // [tested]
typedef ezAtomicInteger<ezInt64> ezAtomicInteger64; // [tested]

