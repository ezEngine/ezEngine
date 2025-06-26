#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec2.h>

/// \brief A 3-component vector class.
template <typename Type>
class ezVec3Template
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  using ComponentType = Type;

  // *** Data ***
public:
  Type x, y, z;

  // *** Constructors ***
public:
  /// \brief default-constructed vector is uninitialized (for speed)
  ezVec3Template<Type>(); // [tested]

  /// \brief Initializes the vector with x,y,z
  ezVec3Template<Type>(Type x, Type y, Type z); // [tested]

  /// \brief Initializes all 3 components with xyz
  explicit ezVec3Template<Type>(Type v); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  /// \brief Returns a vector with all components set to Not-a-Number (NaN).
  EZ_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static ezVec3Template<Type> MakeNaN() { return ezVec3Template<Type>(ezMath::NaN<Type>()); }

  /// \brief Returns a vector with all components set to zero.
  [[nodiscard]] static ezVec3Template<Type> MakeZero() { return ezVec3Template<Type>(0); } // [tested]

  /// \brief Returns a vector initialized to the X unit vector (1, 0, 0).
  [[nodiscard]] static ezVec3Template<Type> MakeAxisX() { return ezVec3Template<Type>(1, 0, 0); } // [tested]

  /// \brief Returns a vector initialized to the Y unit vector (0, 1, 0).
  [[nodiscard]] static ezVec3Template<Type> MakeAxisY() { return ezVec3Template<Type>(0, 1, 0); } // [tested]

  /// \brief Returns a vector initialized to the Z unit vector (0, 0, 1).
  [[nodiscard]] static ezVec3Template<Type> MakeAxisZ() { return ezVec3Template<Type>(0, 0, 1); } // [tested]

  /// \brief Returns a vector initialized to x,y,z
  [[nodiscard]] static ezVec3Template<Type> Make(Type x, Type y, Type z) { return ezVec3Template<Type>(x, y, z); } // [tested]

  /// \brief Returns a vector that is orthogonal to vDirection.
  ///
  /// Uses the vBasis1 and vBasis2 vectors as candidates to create the orthogonal vector from. The basis that is less similar to the direction
  /// will be used to to compute the orthogonal vector.
  ///
  /// All input vectors must be normalized.
  EZ_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static ezVec3Template<Type> MakeOrthogonalVector(const ezVec3Template<Type>& vDirection, const ezVec3Template<Type>& vBasis1 = MakeAxisX(), const ezVec3Template<Type>& vBasis2 = MakeAxisY()); // [tested]

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    EZ_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please "
                               "check that all code-paths properly initialize this object.");
  }
#endif

  // *** Conversions ***
public:
  /// \brief Returns an ezVec2Template with x and y from this vector.
  const ezVec2Template<Type> GetAsVec2() const; // [tested]

  /// \brief Returns an ezVec4Template with x,y,z from this vector and w set to the parameter.
  const ezVec4Template<Type> GetAsVec4(Type w) const; // [tested]

  /// \brief Returns an ezVec4Template with x,y,z from this vector and w set 1.
  const ezVec4Template<Type> GetAsPositionVec4() const; // [tested]

  /// \brief Returns an ezVec4Template with x,y,z from this vector and w set 0.
  const ezVec4Template<Type> GetAsDirectionVec4() const; // [tested]

  /// \brief Returns the data as an array.
  const Type* GetData() const { return &x; }

  /// \brief Returns the data as an array.
  Type* GetData() { return &x; }

  // *** Functions to set the vector to specific values ***
public:
  /// \brief Sets all 3 components to this value.
  void Set(Type xyz); // [tested]

  /// \brief Sets the vector to these values.
  void Set(Type x, Type y, Type z); // [tested]

  /// \brief Sets the vector to all zero.
  void SetZero(); // [tested]

  // *** Functions dealing with length ***
public:
  /// \brief Returns the length of the vector.
  EZ_DECLARE_IF_FLOAT_TYPE
  Type GetLength() const; // [tested]

  /// \brief Returns the length between this position and rhs.
  EZ_DECLARE_IF_FLOAT_TYPE
  Type GetDistanceTo(const ezVec3Template<Type>& rhs) const;

  /// \brief Returns the squared length between this position and rhs.
  EZ_DECLARE_IF_FLOAT_TYPE
  Type GetSquaredDistanceTo(const ezVec3Template<Type>& rhs) const;

  /// \brief Tries to rescale the vector to the given length. If the vector is too close to zero, EZ_FAILURE is returned and the vector is
  /// set to zero.
  EZ_DECLARE_IF_FLOAT_TYPE
  ezResult SetLength(Type fNewLength, Type fEpsilon = ezMath::DefaultEpsilon<Type>()); // [tested]

  /// \brief Returns the squared length. Faster, since no square-root is taken. Useful, if one only wants to compare the lengths of two
  /// vectors.
  Type GetLengthSquared() const; // [tested]

  /// \brief Normalizes this vector and returns its previous length in one operation. More efficient than calling GetLength and then
  /// Normalize.
  EZ_DECLARE_IF_FLOAT_TYPE
  Type GetLengthAndNormalize(); // [tested]

  /// \brief Returns a normalized version of this vector, leaves the vector itself unchanged.
  EZ_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] const ezVec3Template<Type> GetNormalized() const; // [tested]

  /// \brief Normalizes this vector.
  EZ_DECLARE_IF_FLOAT_TYPE
  void Normalize(); // [tested]

  /// \brief Tries to normalize this vector. If the vector is too close to zero, EZ_FAILURE is returned and the vector is set to the given
  /// fallback value.
  EZ_DECLARE_IF_FLOAT_TYPE
  ezResult NormalizeIfNotZero(const ezVec3Template<Type>& vFallback = ezVec3Template<Type>(1, 0, 0), Type fEpsilon = ezMath::SmallEpsilon<Type>()); // [tested]

  /// \brief Returns, whether this vector is (0, 0, 0).
  bool IsZero() const; // [tested]

  /// \brief Returns, whether this vector is (0, 0, 0) within a given epsilon.
  bool IsZero(Type fEpsilon) const; // [tested]

  /// \brief Returns, whether the squared length of this vector is between 0.999f and 1.001f.
  EZ_DECLARE_IF_FLOAT_TYPE
  bool IsNormalized(Type fEpsilon = ezMath::HugeEpsilon<Type>()) const; // [tested]

  /// \brief Returns true, if any of x, y or z is NaN
  bool IsNaN() const; // [tested]

  /// \brief Checks that all components are finite numbers.
  bool IsValid() const; // [tested]


  // *** Operators ***
public:
  /// \brief Returns the negation of this vector.
  const ezVec3Template<Type> operator-() const; // [tested]

  /// \brief Adds rhs component-wise to this vector
  void operator+=(const ezVec3Template<Type>& rhs); // [tested]

  /// \brief Subtracts rhs component-wise from this vector
  void operator-=(const ezVec3Template<Type>& rhs); // [tested]

  /// \brief Multiplies rhs component-wise to this vector
  void operator*=(const ezVec3Template<Type>& rhs);

  /// \brief Divides this vector component-wise by rhs
  void operator/=(const ezVec3Template<Type>& rhs);

  /// \brief Multiplies all components of this vector with f
  void operator*=(Type f); // [tested]

  /// \brief Divides all components of this vector by f
  void operator/=(Type f); // [tested]

  /// \brief Equality Check (bitwise)
  bool IsIdentical(const ezVec3Template<Type>& rhs) const; // [tested]

  /// \brief Equality Check with epsilon
  bool IsEqual(const ezVec3Template<Type>& rhs, Type fEpsilon) const; // [tested]


  // *** Common vector operations ***
public:
  /// \brief Returns the shortest angle between *this and rhs.
  /// Both this and rhs must be normalized
  ezAngle GetAngleBetween(const ezVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the angle between vForward and *this, going around the vUp direction.
  ///
  /// Clockwise rotations (looking top down) would result in a small angle,
  /// counter-clockwise rotations give a large angle (360 degree minus the shortest angle).
  /// All vectors must be normalized. vUp must not coincide with vForward, but doesn't need to be orthogonal to it.
  ///
  /// NOTE: This function assumes a right-handed coordinate system.
  /// If you put in vectors from a left-handed coordinate system, the angles will simply invert.
  ///
  /// The order of operands is also important, if you swap this and vForward, the result also inverts.
  ezAngle GetAngleBetween(const ezVec3Template<Type>& vForward, const ezVec3Template<Type>& vUp) const; // [tested]


  /// \brief Returns the Dot-product of the two vectors (commutative, order does not matter)
  [[nodiscard]] Type Dot(const ezVec3Template<Type>& rhs) const; // [tested]



  /// \brief Returns the Cross-product of the two vectors (NOT commutative, order DOES matter)
  [[nodiscard]] const ezVec3Template<Type> CrossRH(const ezVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise minimum of *this and rhs
  [[nodiscard]] const ezVec3Template<Type> CompMin(const ezVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise maximum of *this and rhs
  [[nodiscard]] const ezVec3Template<Type> CompMax(const ezVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise clamped value of *this between low and high.
  [[nodiscard]] const ezVec3Template<Type> CompClamp(const ezVec3Template<Type>& vLow, const ezVec3Template<Type>& vHigh) const; // [tested]

  /// \brief Returns the component-wise multiplication of *this and rhs
  [[nodiscard]] const ezVec3Template<Type> CompMul(const ezVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise division of *this and rhs
  [[nodiscard]] const ezVec3Template<Type> CompDiv(const ezVec3Template<Type>& rhs) const; // [tested]

  /// brief Returns the component-wise absolute of *this.
  [[nodiscard]] const ezVec3Template<Type> Abs() const; // [tested]


  // *** Other common operations ***
public:
  /// \brief Calculates the normal of the triangle defined by the three vertices. Vertices are assumed to be ordered counter-clockwise.
  EZ_DECLARE_IF_FLOAT_TYPE
  ezResult CalculateNormal(const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2, const ezVec3Template<Type>& v3); // [tested]

  /// \brief Modifies this direction vector to be orthogonal to the given (normalized) direction vector. The result is NOT normalized.
  ///
  /// \note This function may fail, e.g. create a vector that is zero, if the given normal is parallel to the vector itself.
  ///       If you need to handle such cases, you should manually check afterwards, whether the result is zero, or cannot be normalized.
  EZ_DECLARE_IF_FLOAT_TYPE
  void MakeOrthogonalTo(const ezVec3Template<Type>& vNormal); // [tested]

  /// \brief Returns some arbitrary vector orthogonal to this one. The vector is NOT normalized.
  EZ_DECLARE_IF_FLOAT_TYPE
  const ezVec3Template<Type> GetOrthogonalVector() const; // [tested]

  /// \brief Returns this vector reflected at vNormal.
  EZ_DECLARE_IF_FLOAT_TYPE
  const ezVec3Template<Type> GetReflectedVector(const ezVec3Template<Type>& vNormal) const; // [tested]

  /// \brief Returns this vector, refracted at vNormal, using the refraction index of the current medium and the medium it enters.
  EZ_DECLARE_IF_FLOAT_TYPE
  const ezVec3Template<Type> GetRefractedVector(const ezVec3Template<Type>& vNormal, Type fRefIndex1, Type fRefIndex2) const;

  /// \brief Returns a random point inside a unit sphere (radius 1).
  EZ_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static ezVec3Template<Type>
  MakeRandomPointInSphere(ezRandom& inout_rng); // [tested]

  /// \brief Creates a random direction vector. The vector is normalized.
  EZ_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static ezVec3Template<Type>
  MakeRandomDirection(ezRandom& inout_rng); // [tested]

  /// \brief Creates a random vector around the x axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  EZ_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static ezVec3Template<Type>
  MakeRandomDeviationX(ezRandom& inout_rng, const ezAngle& maxDeviation); // [tested]

  /// \brief Creates a random vector around the y axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  EZ_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static ezVec3Template<Type>
  MakeRandomDeviationY(ezRandom& inout_rng, const ezAngle& maxDeviation); // [tested]

  /// \brief Creates a random vector around the z axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  EZ_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static ezVec3Template<Type>
  MakeRandomDeviationZ(ezRandom& inout_rng, const ezAngle& maxDeviation); // [tested]

  /// \brief Creates a random vector around the given normal with a maximum deviation.
  /// \note If you are going to do this many times with the same axis, rather than calling this function, instead manually
  /// do what this function does (see inline code) and only compute the quaternion once.
  EZ_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static ezVec3Template<Type>
  MakeRandomDeviation(ezRandom& inout_rng, const ezAngle& maxDeviation, const ezVec3Template<Type>& vNormal); // [tested]
};

// *** Operators ***

template <typename Type>
const ezVec3Template<Type> operator+(const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2); // [tested]

template <typename Type>
const ezVec3Template<Type> operator-(const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2); // [tested]


template <typename Type>
const ezVec3Template<Type> operator*(Type f, const ezVec3Template<Type>& v); // [tested]

template <typename Type>
const ezVec3Template<Type> operator*(const ezVec3Template<Type>& v, Type f); // [tested]


template <typename Type>
const ezVec3Template<Type> operator/(const ezVec3Template<Type>& v, Type f); // [tested]


template <typename Type>
bool operator==(const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2); // [tested]

template <typename Type>
bool operator!=(const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2); // [tested]

/// \brief Strict weak ordering. Useful for sorting vertices into a map.
template <typename Type>
bool operator<(const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2); // [tested]

#include <Foundation/Math/Implementation/Vec3_inl.h>
