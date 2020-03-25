#pragma once

#include <Foundation/Types/TypeTraits.h>

#include <Foundation/Threading/AtomicUtils.h>

/// \brief Integer class that can be manipulated in an atomic (i.e. thread-safe) fashion.
template <typename T>
class ezAtomicInteger
{
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
  T PostDecrement(); // [tested]

  void Add(T x);      // [tested]
  void Subtract(T x); // [tested]

  void And(T x); // [tested]
  void Or(T x);  // [tested]
  void Xor(T x); // [tested]

  void Min(T x); // [tested]
  void Max(T x); // [tested]

  /// \brief Sets the internal value to x and returns the original internal value.
  T Set(T x); // [tested]

  /// \brief Sets the internal value to x if the internal value is equal to expected and returns true, otherwise does nothing and returns false.
  bool TestAndSet(T expected, T x); // [tested]

  T CompareAndSwap(T expected, T x);

  operator T() const; // [tested]

private:
  volatile T m_value;
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

  /// \brief Sets the internal value to \a newValue if the internal value is equal to \a expected and returns true, otherwise does nothing and returns false.
  bool TestAndSet(bool expected, bool newValue);

private:
  ezAtomicInteger<ezInt32> m_AtomicInt;
};

// Include inline file
#include <Foundation/Threading/Implementation/AtomicInteger_inl.h>

typedef ezAtomicInteger<ezInt32> ezAtomicInteger32; // [tested]
typedef ezAtomicInteger<ezInt64> ezAtomicInteger64; // [tested]
