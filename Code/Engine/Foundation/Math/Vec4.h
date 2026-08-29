#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec3.h>

/// A 4-component vector class.
template <typename Type>
class ezVec4Template
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  using ComponentType = Type;

  // *** Data ***
public:
  Type x, y, z, w;

  // *** Constructors ***
public:
  /// Default-constructed vector is uninitialized (for speed)
  ezVec4Template(); // [tested]

  /// Initializes the vector with x,y,z,w
  ezVec4Template(Type x, Type y, Type z, Type w); // [tested]

  /// Initializes the vector from a vec3 and a float.
  ezVec4Template(ezVec3Template<Type> vXyz, Type w);

  /// Initializes all 4 components with xyzw
  explicit ezVec4Template(Type v); // [tested]
  // no copy-constructor and operator= since the default-generated ones will be faster

  /// Returns a vector with all components set to Not-a-Number (NaN).
  EZ_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static ezVec4Template<Type> MakeNaN() { return ezVec4Template<Type>(ezMath::NaN<Type>()); }

  /// Returns a vector with all components set to zero.
  [[nodiscard]] static ezVec4Template<Type> MakeZero() { return ezVec4Template<Type>(0); } // [tested]

  /// Returns a vector initialized to x,y,z,w
  [[nodiscard]] static ezVec4Template<Type> Make(Type x, Type y, Type z, Type w) { return ezVec4Template<Type>(x, y, z, w); } // [tested]

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    EZ_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please "
                               "check that all code-paths properly initialize this object.");
  }
#endif

  // *** Conversions ***
public:
  /// Returns an ezVec2Template with x and y from this vector.
  const ezVec2Template<Type> GetAsVec2() const; // [tested]

  /// Returns an ezVec3Template with x,y and z from this vector.
  const ezVec3Template<Type> GetAsVec3() const; // [tested]

  /// Returns the data as an array.
  const Type* GetData() const { return &x; }

  /// Returns the data as an array.
  Type* GetData() { return &x; }

  // *** Functions to set the vector to specific values ***
public:
  /// Sets all 4 components to this value.
  void Set(Type xyzw); // [tested]

  /// Sets the vector to these values.
  void Set(Type x, Type y, Type z, Type w); // [tested]

  /// Sets the vector to all zero.
  void SetZero(); // [tested]

  // *** Functions dealing with length ***
public:
  /// Returns the length of the vector.
  EZ_DECLARE_IF_FLOAT_TYPE
  Type GetLength() const; // [tested]

  /// Returns the squared length. Faster, since no square-root is taken. Useful, if one only wants to compare the lengths of two
  /// vectors.
  Type GetLengthSquared() const; // [tested]

  /// Normalizes this vector and returns its previous length in one operation. More efficient than calling GetLength and then
  /// Normalize.
  EZ_DECLARE_IF_FLOAT_TYPE
  Type GetLengthAndNormalize(); // [tested]

  /// Returns a normalized version of this vector, leaves the vector itself unchanged.
  EZ_DECLARE_IF_FLOAT_TYPE
  const ezVec4Template<Type> GetNormalized() const; // [tested]

  /// Normalizes this vector.
  EZ_DECLARE_IF_FLOAT_TYPE
  void Normalize(); // [tested]

  /// Tries to normalize this vector. If the vector is too close to zero, EZ_FAILURE is returned and the vector is set to the given
  /// fallback value.
  EZ_DECLARE_IF_FLOAT_TYPE
  ezResult NormalizeIfNotZero(const ezVec4Template<Type>& vFallback = ezVec4Template<Type>(1, 0, 0, 0), Type fEpsilon = ezMath::SmallEpsilon<Type>()); // [tested]

  /// Returns, whether this vector is (0, 0, 0, 0).
  bool IsZero() const; // [tested]

  /// Returns, whether this vector is (0, 0, 0, 0).
  bool IsZero(Type fEpsilon) const; // [tested]

  /// Returns, whether the squared length of this vector is between 0.999f and 1.001f.
  EZ_DECLARE_IF_FLOAT_TYPE
  bool IsNormalized(Type fEpsilon = ezMath::HugeEpsilon<Type>()) const; // [tested]

  /// Returns true, if any of x, y, z or w is NaN.
  bool IsNaN() const; // [tested]

  /// Checks that all components are finite numbers.
  bool IsValid() const; // [tested]


  // *** Operators ***
public:
  /// Returns the negation of this vector.
  const ezVec4Template<Type> operator-() const; // [tested]

  /// Adds cc component-wise to this vector.
  void operator+=(const ezVec4Template<Type>& vCc); // [tested]

  /// Subtracts cc component-wise from this vector.
  void operator-=(const ezVec4Template<Type>& vCc); // [tested]

  /// Multiplies all components of this vector with f.
  void operator*=(Type f); // [tested]

  /// Divides all components of this vector by f.
  void operator/=(Type f); // [tested]

  /// Equality Check (bitwise).
  bool IsIdentical(const ezVec4Template<Type>& rhs) const; // [tested]

  /// Equality Check with epsilon.
  bool IsEqual(const ezVec4Template<Type>& rhs, Type fEpsilon) const; // [tested]


  // *** Common vector operations ***
public:
  /// Returns the dot-product of the two vectors (commutative, order does not matter).
  Type Dot(const ezVec4Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise minimum of *this and rhs.
  const ezVec4Template<Type> CompMin(const ezVec4Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise maximum of *this and rhs.
  const ezVec4Template<Type> CompMax(const ezVec4Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise clamped value of *this between low and high.
  const ezVec4Template<Type> CompClamp(const ezVec4Template<Type>& vLow, const ezVec4Template<Type>& vHigh) const; // [tested]

  /// Returns the component-wise multiplication of *this and rhs.
  const ezVec4Template<Type> CompMul(const ezVec4Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise division of *this and rhs.
  const ezVec4Template<Type> CompDiv(const ezVec4Template<Type>& rhs) const; // [tested]

  /// brief Returns the component-wise absolute of *this.
  const ezVec4Template<Type> Abs() const; // [tested]
};

// *** Operators ***

template <typename Type>
const ezVec4Template<Type> operator+(const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2); // [tested]

template <typename Type>
const ezVec4Template<Type> operator-(const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2); // [tested]


template <typename Type>
const ezVec4Template<Type> operator*(Type f, const ezVec4Template<Type>& v); // [tested]

template <typename Type>
const ezVec4Template<Type> operator*(const ezVec4Template<Type>& v, Type f); // [tested]


template <typename Type>
const ezVec4Template<Type> operator/(const ezVec4Template<Type>& v, Type f); // [tested]


template <typename Type>
bool operator==(const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2); // [tested]

template <typename Type>
bool operator!=(const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2); // [tested]

/// Strict weak ordering. Useful for sorting vertices into a map.
template <typename Type>
bool operator<(const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2); // [tested]

#include <Foundation/Math/Implementation/Vec4_inl.h>
