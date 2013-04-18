#pragma once

#include <Foundation/Math/Math.h>

/// \brief A 3-component vector class.
class EZ_FOUNDATION_DLL ezVec3
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();


// *** Data ***
public:

  /// \brief The union that allows different representations of the data.
  union
  {
    /// \brief The vector as x/y/z
    struct { float x, y, z; };

    /// \brief The vector as a 3-component float-array.
    float m_Data[3];
  };

// *** Constructors ***
public:

  /// \brief default-constructed vector is uninitialized (for speed)
  ezVec3(); // [tested]

  /// \brief Initializes the vector with x,y,z
  ezVec3(float X, float Y, float Z); // [tested]

  /// \brief Initializes all 3 components with xyz
  explicit ezVec3(float xyz); // [tested]
  // no copy-constructor and operator= since the default-generated ones will be faster

  /// \brief Static function that returns a zero-vector.
  static const ezVec3 ZeroVector() { return ezVec3(0); } // [tested]

// *** Conversions ***
public:

  /// \brief Returns an ezVec2 with x and y from this vector.
  const ezVec2 GetAsVec2() const; // [tested]

  /// \brief Returns an ezVec4 with x,y,z from this vector and w set to the parameter.
  const ezVec4 GetAsVec4(float w) const; // [tested]

  /// \brief Returns an ezVec4 with x,y,z from this vector and w set 1.
  const ezVec4 GetAsPositionVec4() const; // [tested]

  /// \brief Returns an ezVec4 with x,y,z from this vector and w set 0.
  const ezVec4 GetAsDirectionVec4() const; // [tested]

// *** Functions to set the vector to specific values ***
public:

  /// \brief Sets all 3 components to this value.
  void Set(float xyz); // [tested]

  /// \brief Sets the vector to these values.
  void Set(float x, float y, float z); // [tested]

  /// \brief Sets the vector to all zero.
  void SetZero(); // [tested]

// *** Functions dealing with length ***
public:

  /// \brief Returns the length of the vector.
  float GetLength() const; // [tested]

  /// \brief Tries to rescale the vector to the given length. If the vector is too close to zero, EZ_FAILURE is returned and the vector is set to zero.
  ezResult SetLength(float fNewLength, float fEpsilon = ezMath_DefaultEpsilon); // [tested]

  /// \brief Returns the squared length. Faster, since no square-root is taken. Useful, if one only wants to compare the lengths of two vectors.
  float GetLengthSquared() const; // [tested]

  /// \brief Normalizes this vector and returns its previous length in one operation. More efficient than calling GetLength and then Normalize.
  float GetLengthAndNormalize(); // [tested]

  /// \brief Returns a normalized version of this vector, leaves the vector itself unchanged.
  const ezVec3 GetNormalized() const; // [tested]

  /// \brief Normalizes this vector.
  void Normalize(); // [tested]

  /// \brief Tries to normalize this vector. If the vector is too close to zero, EZ_FAILURE is returned and the vector is set to the given fallback value.
  ezResult NormalizeIfNotZero(const ezVec3& vFallback = ezVec3(1, 0, 0), float fEpsilon = ezMath_SmallEpsilon); // [tested]
    
  /// \brief Returns, whether this vector is (0, 0, 0).
  bool IsZero() const; // [tested]

  /// \brief Returns, whether this vector is (0, 0, 0) within a given epsilon.
  bool IsZero(float fEpsilon) const; // [tested]

  /// \brief Returns, whether the squared length of this vector is between 0.999f and 1.001f.
  bool IsNormalized(float fEpsilon = ezMath_HugeEpsilon) const; // [tested]

  /// \brief Returns true, if any of x, y or z is NaN
  bool IsNaN() const; // [tested]

  /// \brief Checks that all components are finite numbers.
  bool IsValid() const; // [tested]


// *** Operators ***
public:

  /// \brief Returns the negation of this vector.
  const ezVec3 operator- () const; // [tested]

  /// \brief Adds cc component-wise to this vector
  void operator+= (const ezVec3& cc); // [tested]

  /// \brief Subtracts cc component-wise from this vector
  void operator-= (const ezVec3& cc); // [tested]

  /// \brief Multiplies all components of this vector with f
  void operator*= (float f); // [tested]

  /// \brief Divides all components of this vector by f
  void operator/= (float f); // [tested]

  /// \brief Equality Check (bitwise)
  bool IsIdentical(const ezVec3& rhs) const; // [tested]

  /// \brief Equality Check with epsilon
  bool IsEqual(const ezVec3& rhs, float fEpsilon) const; // [tested]


// *** Common vector operations ***
public:

  /// \brief Returns the positive angle between *this and rhs (in degree).
  float GetAngleBetween(const ezVec3& rhs) const; // [tested]

  /// \brief Returns the Dot-product of the two vectors (commutative, order does not matter)
  float Dot(const ezVec3& rhs) const; // [tested]

  /// \brief Returns the Cross-product of the two vectors (NOT commutative, order DOES matter)
  const ezVec3 Cross(const ezVec3& rhs) const; // [tested]

  /// \brief Returns the component-wise minimum of *this and rhs
  const ezVec3 CompMin(const ezVec3& rhs) const; // [tested]

  /// \brief Returns the component-wise maximum of *this and rhs
  const ezVec3 CompMax(const ezVec3& rhs) const; // [tested]

  /// \brief Returns the component-wise multiplication of *this and rhs
  const ezVec3 CompMult(const ezVec3& rhs) const; // [tested]

  /// \brief Returns the component-wise division of *this and rhs
  const ezVec3 CompDiv(const ezVec3& rhs) const; // [tested]


// *** Other common operations ***
public:			

  /// \brief Calculates the normal of the triangle defined by the three vertices. Vertices are assumed to be ordered counter-clockwise.
  /// \test Requires a unit-test.
  ezResult CalculateNormal(const ezVec3& v1, const ezVec3& v2, const ezVec3& v3);

  /// \brief Modifies this direction vector to be orthogonal to the given (normalized) direction vector. The result is NOT normalized.
  ///
  /// \note This function may fail, e.g. create a vector that is zero, if the given normal is parallel to the vector itself.
  ///       If you need to handle such cases, you should manually check afterwards, whether the result is zero, or cannot be normalized.
  /// \test Requires a unit-test.
  void MakeOrthogonalTo(const ezVec3& vNormal);

  /// \brief Returns some arbitrary vector orthogonal to this one. The vector is NOT normalized.
  /// \test Requires a unit-test.
  const ezVec3 GetOrthogonalVector() const;

  /// \brief Returns this vector reflected at vNormal.
  /// \test Requires a unit-test.
  const ezVec3 GetReflectedVector(const ezVec3& vNormal) const;

  /// \brief Returns this vector, refracted at vNormal, using the refraction index of the current medium and the medium it enters.
  /// \test Requires a unit-test.
  const ezVec3 GetRefractedVector(const ezVec3& vNormal, float fRefIndex1, float fRefIndex2) const;

};



// *** Operators ***

const ezVec3 operator+ (const ezVec3& v1, const ezVec3& v2); // [tested]
const ezVec3 operator- (const ezVec3& v1, const ezVec3& v2); // [tested]

const ezVec3 operator* (float f, const ezVec3& v); // [tested]
const ezVec3 operator* (const ezVec3& v, float f); // [tested]

const ezVec3 operator/ (const ezVec3& v, float f); // [tested]

bool operator== (const ezVec3& v1, const ezVec3& v2); // [tested]
bool operator!= (const ezVec3& v1, const ezVec3& v2); // [tested]

/// \brief Strict weak ordering. Useful for sorting vertices into a map.
bool operator< (const ezVec3& v1, const ezVec3& v2); // [tested]

#include <Foundation/Math/Implementation/Vec3_inl.h>




