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

  /// \brief The union that allows different representations of the data.
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
  /// \test Requires a unit-test.
  ezVec2();

  /// \brief Initializes the vector with x,y
  /// \test Requires a unit-test.
  ezVec2(float X, float Y);

  /// \brief Initializes all components with xy
  /// \test Requires a unit-test.
  explicit ezVec2(float xy);

  // no copy-constructor and operator= since the default-generated ones will be faster

  /// \brief Static function that returns a zero-vector.
  /// \test Requires a unit-test.
  static const ezVec2 ZeroVector() { return ezVec2(0); }

// *** Conversions ***
public:

  /// \brief Returns an ezVec3 with x,y from this vector and z set by the parameter.
  /// \test Requires a unit-test.
  const ezVec3 GetAsVec3(float z);

  /// \brief Returns an ezVec4 with x,y from this vector and z and w set by the parameters.
  /// \test Requires a unit-test.
  const ezVec4 GetAsVec4(float z, float w);

// *** Functions to set the vector to specific values ***
public:

  /// \brief Sets all components to this value.
  /// \test Requires a unit-test.
  void Set(float xy);

  /// \brief Sets the vector to these values.
  /// \test Requires a unit-test.
  void Set(float x, float y);

  /// \brief Sets the vector to all zero.
  /// \test Requires a unit-test.
  void SetZero();

// *** Functions dealing with length ***
public:

  /// \brief Returns the length of the vector.
  /// \test Requires a unit-test.
  float GetLength() const;

  /// \brief Returns the squared length. Faster, since no square-root is taken. Useful, if one only wants to compare the lengths of two vectors.
  /// \test Requires a unit-test.
  float GetLengthSquared() const;

  /// \brief Normalizes this vector and returns its previous length in one operation. More efficient than calling GetLength and then Normalize.
  /// \test Requires a unit-test.
  float GetLengthAndNormalize();

  /// \brief Returns a normalized version of this vector, leaves the vector itself unchanged.
  /// \test Requires a unit-test.
  const ezVec2 GetNormalized() const;

  /// \brief Normalizes this vector.
  /// \test Requires a unit-test.
  void Normalize();

  /// \brief Tries to normalize this vector. If the vector is too close to zero, EZ_FAILURE is returned and the vector is set to the given fallback value.
  /// \test Requires a unit-test.
  ezResult NormalizeIfNotZero(const ezVec2& vFallback = ezVec2(1, 0), float fEpsilon = ezMath_SmallEpsilon);
    
  /// \brief Returns, whether this vector is (0, 0).
  /// \test Requires a unit-test.
  bool IsZero() const;

  /// \brief Returns, whether this vector is (0, 0) within a certain threshold.
  /// \test Requires a unit-test.
  bool IsZero(float fEpsilon) const;

  /// \brief Returns, whether the squared length of this vector is between 0.999f and 1.001f.
  /// \test Requires a unit-test.
  bool IsNormalized(float fEpsilon = ezMath_HugeEpsilon) const;

  /// \brief Returns true, if any of x or y is NaN
  /// \test Requires a unit-test.
  bool IsNaN() const;

  /// \brief Checks that all components are finite numbers.
  /// \test Requires a unit-test.
  bool IsValid() const;


// *** Operators ***
public:

  /// \brief Returns the negation of this vector.
  /// \test Requires a unit-test.
  const ezVec2 operator- () const;

  /// \brief Adds cc component-wise to this vector
  /// \test Requires a unit-test.
  void operator+= (const ezVec2& cc);

  /// \brief Subtracts cc component-wise from this vector
  /// \test Requires a unit-test.
  void operator-= (const ezVec2& cc);

  /// \brief Multiplies all components of this vector with f
  /// \test Requires a unit-test.
  void operator*= (float f);

  /// \brief Divides all components of this vector by f
  /// \test Requires a unit-test.
  void operator/= (float f);

  /// \brief Equality Check (bitwise)
  /// \test Requires a unit-test.
  bool IsIdentical(const ezVec2& rhs) const;

  /// \brief Equality Check with epsilon
  /// \test Requires a unit-test.
  bool IsEqual(const ezVec2& rhs, float fEpsilon) const;


// *** Common vector operations ***
public:

  /// \brief Returns the positive angle between *this and rhs (in degree).
  /// \test Requires a unit-test.
  float GetAngleBetween(const ezVec2& rhs) const;

  /// \brief Returns the Dot-product of the two vectors (commutative, order does not matter)
  /// \test Requires a unit-test.
  float Dot(const ezVec2& rhs) const;

  /// \brief Returns the component-wise minimum of *this and rhs
  /// \test Requires a unit-test.
  const ezVec2 CompMin(const ezVec2& rhs) const;

  /// \brief Returns the component-wise maximum of *this and rhs
  /// \test Requires a unit-test.
  const ezVec2 CompMax(const ezVec2& rhs) const;

  /// \brief Returns the component-wise multiplication of *this and rhs
  /// \test Requires a unit-test.
  const ezVec2 CompMult(const ezVec2& rhs) const;

  /// \brief Returns the component-wise division of *this and rhs
  /// \test Requires a unit-test.
  const ezVec2 CompDiv(const ezVec2& rhs) const;


// *** Other common operations ***
public:

  /// \brief Modifies this direction vector to be orthogonal to the given (normalized) direction vector. The result is NOT normalized.
  ///
  /// \note This function may fail, e.g. create a vector that is zero, if the given normal is parallel to the vector itself.
  ///       If you need to handle such cases, you should manually check afterwards, whether the result is zero, or cannot be normalized.
  /// \test Requires a unit-test.
  void MakeOrthogonalTo(const ezVec2& vNormal);

  /// \brief Returns some arbitrary vector orthogonal to this one. The vector is NOT normalized.
  /// \test Requires a unit-test.
  const ezVec2 GetOrthogonalVector() const;

  /// \brief Returns this vector reflected at vNormal.
  /// \test Requires a unit-test.
  const ezVec2 GetReflectedVector(const ezVec2& vNormal) const;
};



// *** Operators ***

/// \test Requires a unit-test.
const ezVec2 operator+ (const ezVec2& v1, const ezVec2& v2);

/// \test Requires a unit-test.
const ezVec2 operator- (const ezVec2& v1, const ezVec2& v2);

/// \test Requires a unit-test.
const ezVec2 operator* (float f, const ezVec2& v);

/// \test Requires a unit-test.
const ezVec2 operator* (const ezVec2& v, float f);

/// \test Requires a unit-test.
const ezVec2 operator/ (const ezVec2& v, float f);

/// \test Requires a unit-test.
bool operator== (const ezVec2& v1, const ezVec2& v2);

/// \test Requires a unit-test.
bool operator!= (const ezVec2& v1, const ezVec2& v2);

/// \brief Strict weak ordering. Useful for sorting vertices into a map.
bool operator< (const ezVec2& v1, const ezVec2& v2);

#include <Foundation/Math/Implementation/Vec2_inl.h>




