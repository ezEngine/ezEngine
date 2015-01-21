#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec3.h>

/// \brief A 4-component vector class.
template<typename Type>
class ezVec4Template
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  typedef Type ComponentType;

// *** Data ***
public:

  Type x, y, z, w;

// *** Constructors ***
public:

  /// \brief Default-constructed vector is uninitialized (for speed)
  ezVec4Template();  // [tested]

  /// \brief Initializes the vector with x,y,z,w
  ezVec4Template(Type X, Type Y, Type Z, Type W);  // [tested]

  /// \brief Initializes all 4 components with xyzw
  explicit ezVec4Template(Type xyzw);  // [tested]
  // no copy-constructor and operator= since the default-generated ones will be faster

  /// \brief Static function that returns a zero-vector.
  static const ezVec4Template ZeroVector() { return ezVec4Template(0); }  // [tested]

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    EZ_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that all code-paths properly initialize this object.");
  }
#endif

// *** Conversions ***
public:

  /// \brief Returns an ezVec2Template with x and y from this vector.
  const ezVec2Template<Type> GetAsVec2() const;  // [tested]

  /// \brief Returns an ezVec3Template with x,y and z from this vector.
  const ezVec3Template<Type> GetAsVec3() const;  // [tested]

  /// \brief Returns the data as an array.
  const Type* GetData() const { return &x; }

  /// \brief Returns the data as an array.
  Type* GetData() { return &x; }

// *** Functions to set the vector to specific values ***
public:

  /// \brief Sets all 4 components to this value.
  void Set(Type xyzw);  // [tested]

  /// \brief Sets the vector to these values.
  void Set(Type x, Type y, Type z, Type w);  // [tested]

  /// \brief Sets the vector to all zero.
  void SetZero();  // [tested]

// *** Functions dealing with length ***
public:

  /// \brief Returns the length of the vector.
  Type GetLength() const; // [tested]

  /// \brief Returns the squared length. Faster, since no square-root is taken. Useful, if one only wants to compare the lengths of two vectors.
  Type GetLengthSquared() const; // [tested]

  /// \brief Normalizes this vector and returns its previous length in one operation. More efficient than calling GetLength and then Normalize.
  Type GetLengthAndNormalize(); // [tested]

  /// \brief Returns a normalized version of this vector, leaves the vector itself unchanged.
  const ezVec4Template GetNormalized() const; // [tested]

  /// \brief Normalizes this vector.
  void Normalize(); // [tested]

  /// \brief Tries to normalize this vector. If the vector is too close to zero, EZ_FAILURE is returned and the vector is set to the given fallback value.
  ezResult NormalizeIfNotZero(const ezVec4Template& vFallback = ezVec4Template(1, 0, 0, 0), Type fEpsilon = ezMath::BasicType<Type>::SmallEpsilon()); // [tested]
    
  /// \brief Returns, whether this vector is (0, 0, 0, 0).
  bool IsZero() const; // [tested]

  /// \brief Returns, whether this vector is (0, 0, 0, 0).
  bool IsZero(Type fEpsilon) const; // [tested]

  /// \brief Returns, whether the squared length of this vector is between 0.999f and 1.001f.
  bool IsNormalized(Type fEpsilon = ezMath::BasicType<Type>::HugeEpsilon()) const; // [tested]

  /// \brief Returns true, if any of x, y, z or w is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Checks that all components are finite numbers.
  bool IsValid() const; // [tested]


// *** Operators ***
public:

  /// \brief Returns the negation of this vector.
  const ezVec4Template operator- () const; // [tested]

  /// \brief Adds cc component-wise to this vector.
  void operator+= (const ezVec4Template& cc); // [tested]

  /// \brief Subtracts cc component-wise from this vector.
  void operator-= (const ezVec4Template& cc); // [tested]

  /// \brief Multiplies all components of this vector with f.
  void operator*= (Type f); // [tested]

  /// \brief Divides all components of this vector by f.
  void operator/= (Type f); // [tested]

  /// \brief Equality Check (bitwise).
  bool IsIdentical(const ezVec4Template& rhs) const; // [tested]

  /// \brief Equality Check with epsilon.
  bool IsEqual(const ezVec4Template& rhs, Type fEpsilon) const; // [tested]


// *** Common vector operations ***
public:

  /// \brief Returns the Dot-product of the two vectors (commutative, order does not matter).
  Type Dot(const ezVec4Template& rhs) const; // [tested]

  /// \brief Returns the component-wise minimum of *this and rhs.
  const ezVec4Template CompMin(const ezVec4Template& rhs) const; // [tested]

  /// \brief Returns the component-wise maximum of *this and rhs.
  const ezVec4Template CompMax(const ezVec4Template& rhs) const; // [tested]

  /// \brief Returns the component-wise multiplication of *this and rhs.
  const ezVec4Template CompMult(const ezVec4Template& rhs) const; // [tested]

  /// \brief Returns the component-wise division of *this and rhs.
  const ezVec4Template CompDiv(const ezVec4Template& rhs) const; // [tested]
};



// *** Operators ***

template<typename Type>
const ezVec4Template<Type> operator+ (const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2); // [tested]

template<typename Type>
const ezVec4Template<Type> operator- (const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2); // [tested]


template<typename Type>
const ezVec4Template<Type> operator* (Type f, const ezVec4Template<Type>& v); // [tested]

template<typename Type>
const ezVec4Template<Type> operator* (const ezVec4Template<Type>& v, Type f); // [tested]


template<typename Type>
const ezVec4Template<Type> operator/ (const ezVec4Template<Type>& v, Type f); // [tested]


template<typename Type>
bool operator== (const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2); // [tested]

template<typename Type>
bool operator!= (const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2); // [tested]

/// \brief Strict weak ordering. Useful for sorting vertices into a map.
template<typename Type>
bool operator< (const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2); // [tested]

#include <Foundation/Math/Implementation/Vec4_inl.h>




