#ifndef AE_FOUNDATION_MATH_VEC3_H
#define AE_FOUNDATION_MATH_VEC3_H

#include "Declarations.h"

namespace AE_NS_FOUNDATION
{
  //! A 3-component vector class.
  class AE_FOUNDATION_DLL aeVec3
  {
  // *** Data ***
  public:
    float x;
    float y;
    float z;

  // *** Constructors ***
  public:
    //! default-constructed vector is uninitialized (for speed)
    aeVec3 (void) {}
    //! Initializes the vector with x,y,z
    aeVec3 (float X, float Y, float Z = 0.0f);
    //! Initializes all 3 components with xyz
    explicit aeVec3 (float xyz);
    // no copy-constructor and operator= since the default-generated ones will be faster

    //! Static function that returns a zero-vector.
    static const aeVec3 ZeroVector (void) { return aeVec3 (0, 0, 0); }

  // *** Functions to set the vector to specific values ***
  public:
    //! Sets all 3 components to this value.
    void SetVector (float xyz);
    //! Sets the vector to these values.
    void SetVector (float x, float y, float z = 0.0f);
    //! Sets the vector to all zero.
    void SetZero (void);

  // *** Functions dealing with length ***
  public:

    //! Returns the length of the vector.
    float GetLength (void) const;
    //! Returns the squared length. Faster, since no square-root is taken. Useful, if one only wants to compare the lengths of two vectors.
    float GetLengthSquared (void) const;

    //! Returns a normalized version of this vector, leaves the vector itself unchanged.
    const aeVec3 GetNormalized (void) const;
    //! Normalizes this vector.
    void Normalize (void);
    
    //! Returns a normalized version of this vector, leaves the vector itself unchanged. If length is zero or the vector is degenerate (NaN), the vector will be zeroed.
    const aeVec3 GetNormalizedSafe (void) const;
    //! Normalizes this vector. If length is zero or the vector is degenerate (NaN), the vector will be zeroed.
    void NormalizeSafe (void);

    //! Returns, whether this vector is (0, 0, 0).
    bool IsZeroVector (void) const;
    //! Returns, whether the squared length of this vector is between 0.999f and 1.001f.
    bool IsNormalized (void) const;

    //! Returns, whether this vector is (0, 0, 0).
    bool IsZeroVector (float fEpsilon) const;
    //! Returns, whether the squared length of this vector is between 0.999f and 1.001f.
    bool IsNormalized (float fEpsilon) const;

    //! Returns true, if any of x, y or z is NaN
    bool IsNaN (void) const;

    //! Checks that all components are finite numbers.
    bool IsValid () const;


  // *** Operators ***
  public:

    //! Returns the negation of this vector.
    const aeVec3 operator- (void) const;

    //! Adds cc component-wise to this vector
    const aeVec3& operator+= (const aeVec3& cc);
    //! Subtracts cc component-wise from this vector
    const aeVec3& operator-= (const aeVec3& cc);
    //! Adds f component-wise to this vector
    const aeVec3& operator+= (float f);
    //! Subtracts f component-wise from this vector
    const aeVec3& operator-= (float f);
    //! Multiplies all components of this vector with f
    const aeVec3& operator*= (float f);
    //! Divides all components of this vector by f
    const aeVec3& operator/= (float f);

    //! Equality Check (bitwise)
    bool IsIdentical (const aeVec3& rhs) const;
    //! Equality Check with epsilon
    bool IsEqual (const aeVec3& rhs, float fEpsilon) const;

  // *** static vector-functions ***
  public:

    //! Returns the vector (1, 0, 0)
    static const aeVec3 GetAxisX (void);
    //! Returns the vector (0, 1, 0)
    static const aeVec3 GetAxisY (void);
    //! Returns the vector (0, 0, 1)
    static const aeVec3 GetAxisZ (void);


  // *** Other common operations ***
  public:			

    //! Calculates the normal of the triangle defined by the three vertices. Vertices are assumed to be ordered counter-clockwise.
    void CalculateNormal (const aeVec3& v1, const aeVec3& v2, const aeVec3& v3);

    void CalculateNormalSafe (const aeVec3& v1, const aeVec3& v2, const aeVec3& v3);

    //! Modifies this direction vector to be orthogonal to the given (normalized) direction vector.
    void MakeOrthogonalTo (const aeVec3& vNormal);

    //! Returns some arbitrary vector orthogonal to this one.
    const aeVec3 GetOrthogonalVector () const;

    //! Returns this vector reflected at vNormal
    const aeVec3 GetReflectedVector (const aeVec3& vNormal) const;
    //! Returns this vector, refracted at vNormal, using the refraction index of the current medium and the medium it enters.
    const aeVec3 GetRefractedVector (const aeVec3& vNormal, float fRefIndex1, float fRefIndex2) const;

  public:

  // *** Common vector operations ***

    //! Returns the positive angle between *this and rhs.
    float GetAngleBetween (const aeVec3& rhs) const;
    //! Returns the angle between *this and rhs.
    //float GetAngleBetweenCCW (const aeVec3& rhs, const aeVec3& vPlaneNormal) const;
    //! Returns the Dot-product of the two vectors (commutative, order does not matter)
    float Dot (const aeVec3& rhs) const;
    //! Returns the Cross-product of the two vectors (NOT commutative, order DOES matter)
    const aeVec3 Cross (const aeVec3& rhs) const;
    //! returns the component-wise minimum of *this and rhs
    const aeVec3 CompMin (const aeVec3& rhs) const;
    //! returns the component-wise maximum of *this and rhs
    const aeVec3 CompMax (const aeVec3& rhs) const;
    //! returns the component-wise multiplication of *this and rhs
    const aeVec3 CompMult (const aeVec3& rhs) const;
    //! returns the component-wise division of *this and rhs
    const aeVec3 CompDiv (const aeVec3& rhs) const;
  };



  // *** Operators ***

  const aeVec3 operator+ (const aeVec3& v1, const aeVec3& v2);
  const aeVec3 operator- (const aeVec3& v1, const aeVec3& v2);

  const aeVec3 operator+ (const aeVec3& v1, float f2);
  const aeVec3 operator- (const aeVec3& v1, float f2);

  const aeVec3 operator* (float f, const aeVec3& v);
  const aeVec3 operator* (const aeVec3& v, float f);

  const aeVec3 operator/ (const aeVec3& v, float f);

  //! Dot-product of two vectors, NOT component-wise multiplication
  //float operator* (const aeVec3& v1, const aeVec3& v2);

  bool operator== (const aeVec3& v1, const aeVec3& v2);
  bool operator!= (const aeVec3& v1, const aeVec3& v2);

  //! Strict weak order. Useful for sorting vertices into a map.
  bool operator< (const aeVec3& v1, const aeVec3& v2);
}

#include "Inline/Vec3.inl"

#endif



