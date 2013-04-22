#pragma once

#include <Foundation/Math/Math.h>

/// \brief A 4-component vector class.
class EZ_FOUNDATION_DLL ezVec4
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();


// *** Data ***
public:

  union
  {
    /// \brief The vector as x/y/z/w
    struct { float x, y, z, w; };

    /// \brief The vector as a 4-component float-array.
    float m_Data[4];
  };

// *** Constructors ***
public:

  /// \brief Default-constructed vector is uninitialized (for speed)
  ezVec4();  // [tested]

  /// \brief Initializes the vector with x,y,z,w
  ezVec4(float X, float Y, float Z, float W);  // [tested]

  /// \brief Initializes all 4 components with xyzw
  explicit ezVec4(float xyzw);  // [tested]
  // no copy-constructor and operator= since the default-generated ones will be faster

  /// \brief Static function that returns a zero-vector.
  static const ezVec4 ZeroVector() { return ezVec4(0); }  // [tested]

// *** Conversions ***
public:

  /// \brief Returns an ezVec2 with x and y from this vector.
  const ezVec2 GetAsVec2() const;  // [tested]

  /// \brief Returns an ezVec3 with x,y and z from this vector.
  const ezVec3 GetAsVec3() const;  // [tested]

// *** Functions to set the vector to specific values ***
public:

  /// \brief Sets all 4 components to this value.
  void Set(float xyzw);  // [tested]

  /// \brief Sets the vector to these values.
  void Set(float x, float y, float z, float w);  // [tested]

  /// \brief Sets the vector to all zero.
  void SetZero();  // [tested]

// *** Functions dealing with length ***
public:

  /// \brief Returns the length of the vector.
  float GetLength() const; // [tested]

  /// \brief Returns the squared length. Faster, since no square-root is taken. Useful, if one only wants to compare the lengths of two vectors.
  float GetLengthSquared() const; // [tested]

  /// \brief Normalizes this vector and returns its previous length in one operation. More efficient than calling GetLength and then Normalize.
  float GetLengthAndNormalize(); // [tested]

  /// \brief Returns a normalized version of this vector, leaves the vector itself unchanged.
  const ezVec4 GetNormalized() const; // [tested]

  /// \brief Normalizes this vector.
  void Normalize(); // [tested]

  /// \brief Tries to normalize this vector. If the vector is too close to zero, EZ_FAILURE is returned and the vector is set to the given fallback value.
  ezResult NormalizeIfNotZero(const ezVec4& vFallback = ezVec4(1, 0, 0, 0), float fEpsilon = ezMath_SmallEpsilon); // [tested]
    
  /// \brief Returns, whether this vector is (0, 0, 0, 0).
  bool IsZero() const; // [tested]

  /// \brief Returns, whether this vector is (0, 0, 0, 0).
  bool IsZero(float fEpsilon) const; // [tested]

  /// \brief Returns, whether the squared length of this vector is between 0.999f and 1.001f.
  bool IsNormalized(float fEpsilon = ezMath_HugeEpsilon) const; // [tested]

  /// \brief Returns true, if any of x, y, z or w is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Checks that all components are finite numbers.
  bool IsValid() const; // [tested]


// *** Operators ***
public:

  /// \brief Returns the negation of this vector.
  const ezVec4 operator- () const; // [tested]

  /// \brief Adds cc component-wise to this vector.
  void operator+= (const ezVec4& cc); // [tested]

  /// \brief Subtracts cc component-wise from this vector.
  void operator-= (const ezVec4& cc); // [tested]

  /// \brief Multiplies all components of this vector with f.
  void operator*= (float f); // [tested]

  /// \brief Divides all components of this vector by f.
  void operator/= (float f); // [tested]

  /// \brief Equality Check (bitwise).
  bool IsIdentical(const ezVec4& rhs) const; // [tested]

  /// \brief Equality Check with epsilon.
  bool IsEqual(const ezVec4& rhs, float fEpsilon) const; // [tested]


// *** Common vector operations ***
public:

  /// \brief Returns the Dot-product of the two vectors (commutative, order does not matter).
  float Dot(const ezVec4& rhs) const; // [tested]

  /// \brief Returns the component-wise minimum of *this and rhs.
  const ezVec4 CompMin(const ezVec4& rhs) const; // [tested]

  /// \brief Returns the component-wise maximum of *this and rhs.
  const ezVec4 CompMax(const ezVec4& rhs) const; // [tested]

  /// \brief Returns the component-wise multiplication of *this and rhs.
  const ezVec4 CompMult(const ezVec4& rhs) const; // [tested]

  /// \brief Returns the component-wise division of *this and rhs.
  const ezVec4 CompDiv(const ezVec4& rhs) const; // [tested]
};



// *** Operators ***

const ezVec4 operator+ (const ezVec4& v1, const ezVec4& v2); // [tested]
const ezVec4 operator- (const ezVec4& v1, const ezVec4& v2); // [tested]

const ezVec4 operator* (float f, const ezVec4& v); // [tested]
const ezVec4 operator* (const ezVec4& v, float f); // [tested]

const ezVec4 operator/ (const ezVec4& v, float f); // [tested]

bool operator== (const ezVec4& v1, const ezVec4& v2); // [tested]
bool operator!= (const ezVec4& v1, const ezVec4& v2); // [tested]

/// \brief Strict weak ordering. Useful for sorting vertices into a map.
bool operator< (const ezVec4& v1, const ezVec4& v2); // [tested]

#include <Foundation/Math/Implementation/Vec4_inl.h>




