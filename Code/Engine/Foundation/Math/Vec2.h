#pragma once

#include <Foundation/Math/Math.h>

/// \brief A 2-component vector class.
class EZ_FOUNDATION_DLL ezVec2
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();


// *** Data ***
public:

  union
  {
    /// \brief The vector as x/y
    struct { float x, y; };

    /// \brief The vector as a 2-component float-array.
    float m_Data[2];
  };

// *** Constructors ***
public:

  /// \brief default-constructed vector is uninitialized (for speed)
  ezVec2(); // [tested]

  /// \brief Initializes the vector with x,y
  ezVec2(float X, float Y); // [tested]

  /// \brief Initializes all components with xy
  explicit ezVec2(float xy); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  /// \brief Static function that returns a zero-vector.
  static const ezVec2 ZeroVector() { return ezVec2(0); } // [tested]

// *** Conversions ***
public:

  /// \brief Returns an ezVec3 with x,y from this vector and z set by the parameter.
  const ezVec3 GetAsVec3(float z); // [tested]

  /// \brief Returns an ezVec4 with x,y from this vector and z and w set by the parameters.
  const ezVec4 GetAsVec4(float z, float w); // [tested]

// *** Functions to set the vector to specific values ***
public:

  /// \brief Sets all components to this value.
  void Set(float xy); // [tested]

  /// \brief Sets the vector to these values.
  void Set(float x, float y); // [tested]

  /// \brief Sets the vector to all zero.
  void SetZero(); // [tested]

// *** Functions dealing with length ***
public:

  /// \brief Returns the length of the vector.
  float GetLength() const; // [tested]

  /// \brief Returns the squared length. Faster, since no square-root is taken. Useful, if one only wants to compare the lengths of two vectors.
  float GetLengthSquared() const; // [tested]

  /// \brief Normalizes this vector and returns its previous length in one operation. More efficient than calling GetLength and then Normalize.
  float GetLengthAndNormalize(); // [tested]

  /// \brief Returns a normalized version of this vector, leaves the vector itself unchanged.
  const ezVec2 GetNormalized() const; // [tested]

  /// \brief Normalizes this vector.
  void Normalize(); // [tested]

  /// \brief Tries to normalize this vector. If the vector is too close to zero, EZ_FAILURE is returned and the vector is set to the given fallback value.
  ezResult NormalizeIfNotZero(const ezVec2& vFallback = ezVec2(1, 0), float fEpsilon = ezMath_SmallEpsilon); // [tested]
    
  /// \brief Returns, whether this vector is (0, 0).
  bool IsZero() const; // [tested]

  /// \brief Returns, whether this vector is (0, 0) within a certain threshold.
  bool IsZero(float fEpsilon) const; // [tested]

  /// \brief Returns, whether the squared length of this vector is between 0.999f and 1.001f.
  bool IsNormalized(float fEpsilon = ezMath_HugeEpsilon) const; // [tested]

  /// \brief Returns true, if any of x or y is NaN
  bool IsNaN() const; // [tested]

  /// \brief Checks that all components are finite numbers.
  bool IsValid() const; // [tested]


// *** Operators ***
public:

  /// \brief Returns the negation of this vector.
  const ezVec2 operator- () const; // [tested]

  /// \brief Adds cc component-wise to this vector
  void operator+= (const ezVec2& cc); // [tested]

  /// \brief Subtracts cc component-wise from this vector
  void operator-= (const ezVec2& cc); // [tested]

  /// \brief Multiplies all components of this vector with f
  void operator*= (float f); // [tested]

  /// \brief Divides all components of this vector by f
  void operator/= (float f); // [tested]

  /// \brief Equality Check (bitwise)
  bool IsIdentical(const ezVec2& rhs) const; // [tested]

  /// \brief Equality Check with epsilon
  bool IsEqual(const ezVec2& rhs, float fEpsilon) const; // [tested]


// *** Common vector operations ***
public:

  /// \brief Returns the positive angle between *this and rhs (in degree).
  float GetAngleBetween(const ezVec2& rhs) const; // [tested]

  /// \brief Returns the Dot-product of the two vectors (commutative, order does not matter)
  float Dot(const ezVec2& rhs) const; // [tested]

  /// \brief Returns the component-wise minimum of *this and rhs
  const ezVec2 CompMin(const ezVec2& rhs) const; // [tested]

  /// \brief Returns the component-wise maximum of *this and rhs
  const ezVec2 CompMax(const ezVec2& rhs) const; // [tested]

  /// \brief Returns the component-wise multiplication of *this and rhs
  const ezVec2 CompMult(const ezVec2& rhs) const; // [tested]

  /// \brief Returns the component-wise division of *this and rhs
  const ezVec2 CompDiv(const ezVec2& rhs) const; // [tested]


// *** Other common operations ***
public:

  /// \brief Modifies this direction vector to be orthogonal to the given (normalized) direction vector. The result is NOT normalized.
  ///
  /// \note This function may fail, e.g. create a vector that is zero, if the given normal is parallel to the vector itself.
  ///       If you need to handle such cases, you should manually check afterwards, whether the result is zero, or cannot be normalized.
  void MakeOrthogonalTo(const ezVec2& vNormal); // [tested]

  /// \brief Returns some arbitrary vector orthogonal to this one. The vector is NOT normalized.
  const ezVec2 GetOrthogonalVector() const; // [tested]

  /// \brief Returns this vector reflected at vNormal.
  const ezVec2 GetReflectedVector(const ezVec2& vNormal) const; // [tested]
};



// *** Operators ***

/// \brief Component-wise addition.
const ezVec2 operator+ (const ezVec2& v1, const ezVec2& v2); // [tested]

/// \brief Component-wise subtraction.
const ezVec2 operator- (const ezVec2& v1, const ezVec2& v2); // [tested]

/// \brief Returns a scaled vector.
const ezVec2 operator* (float f, const ezVec2& v); // [tested]

/// \brief Returns a scaled vector.
const ezVec2 operator* (const ezVec2& v, float f); // [tested]

/// \brief Returns a scaled vector.
const ezVec2 operator/ (const ezVec2& v, float f); // [tested]

/// \brief Returns true, if both vectors are identical.
bool operator== (const ezVec2& v1, const ezVec2& v2); // [tested]

/// \brief Returns true, if both vectors are not identical.
bool operator!= (const ezVec2& v1, const ezVec2& v2); // [tested]

/// \brief Strict weak ordering. Useful for sorting vertices into a map.
bool operator< (const ezVec2& v1, const ezVec2& v2);

#include <Foundation/Math/Implementation/Vec2_inl.h>




