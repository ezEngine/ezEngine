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
  ezVec3Template(); // [tested]

  /// \brief Initializes the vector with x,y,z
  ezVec3Template(Type x, Type y, Type z); // [tested]

  /// \brief Initializes all 3 components with xyz
  explicit ezVec3Template(Type v); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  /// \brief Returns a vector with all components set to Not-a-Number (NaN).
  [[nodiscard]] static ezVec3Template<Type> MakeNaN() { return ezVec3Template<Type>(ezMath::NaN<Type>()); }

  /// \brief Returns a vector with all components set to zero.
  [[nodiscard]] static ezVec3Template<Type> MakeZero() { return ezVec3Template<Type>(0); } // [tested]
  /*[[deprecated("Use ezVec3::MakeZero() instead.")]]*/ static ezVec3Template<Type> ZeroVector() { return ezVec3Template<Type>(0); }

  /// \brief Returns a vector initialized to the X unit vector (1, 0, 0).
  [[nodiscard]] static ezVec3Template<Type> MakeAxisX() { return ezVec3Template<Type>(1, 0, 0); } // [tested]
  /*[[deprecated("Use ezVec3::MakeAxisX() instead.")]]*/ static const ezVec3Template<Type> UnitXAxis() { return ezVec3Template(1, 0, 0); }

  /// \brief Returns a vector initialized to the Y unit vector (0, 1, 0).
  [[nodiscard]] static ezVec3Template<Type> MakeAxisY() { return ezVec3Template<Type>(0, 1, 0); } // [tested]
  /*[[deprecated("Use ezVec3::MakeAxisY() instead.")]]*/ static const ezVec3Template<Type> UnitYAxis() { return ezVec3Template(0, 1, 0); }

  /// \brief Returns a vector initialized to the Z unit vector (0, 0, 1).
  [[nodiscard]] static ezVec3Template<Type> MakeAxisZ() { return ezVec3Template<Type>(0, 0, 1); } // [tested]
  /*[[deprecated("Use ezVec3::MakeAxisZ() instead.")]]*/ static const ezVec3Template<Type> UnitZAxis() { return ezVec3Template(0, 0, 1); }

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
  Type GetLength() const; // [tested]

  /// \brief Tries to rescale the vector to the given length. If the vector is too close to zero, EZ_FAILURE is returned and the vector is
  /// set to zero.
  ezResult SetLength(Type fNewLength, Type fEpsilon = ezMath::DefaultEpsilon<Type>()); // [tested]

  /// \brief Returns the squared length. Faster, since no square-root is taken. Useful, if one only wants to compare the lengths of two
  /// vectors.
  Type GetLengthSquared() const; // [tested]

  /// \brief Normalizes this vector and returns its previous length in one operation. More efficient than calling GetLength and then
  /// Normalize.
  Type GetLengthAndNormalize(); // [tested]

  /// \brief Returns a normalized version of this vector, leaves the vector itself unchanged.
  const ezVec3Template<Type> GetNormalized() const; // [tested]

  /// \brief Normalizes this vector.
  void Normalize(); // [tested]

  /// \brief Tries to normalize this vector. If the vector is too close to zero, EZ_FAILURE is returned and the vector is set to the given
  /// fallback value.
  ezResult NormalizeIfNotZero(const ezVec3Template<Type>& vFallback = ezVec3Template(1, 0, 0), Type fEpsilon = ezMath::SmallEpsilon<Type>()); // [tested]

  /// \brief Returns, whether this vector is (0, 0, 0).
  bool IsZero() const; // [tested]

  /// \brief Returns, whether this vector is (0, 0, 0) within a given epsilon.
  bool IsZero(Type fEpsilon) const; // [tested]

  /// \brief Returns, whether the squared length of this vector is between 0.999f and 1.001f.
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
  /// \brief Returns the positive angle between *this and rhs.
  /// Both this and rhs must be normalized
  ezAngle GetAngleBetween(const ezVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the Dot-product of the two vectors (commutative, order does not matter)
  Type Dot(const ezVec3Template<Type>& rhs) const; // [tested]



  /// \brief Returns the Cross-product of the two vectors (NOT commutative, order DOES matter)
  const ezVec3Template<Type> CrossRH(const ezVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise minimum of *this and rhs
  const ezVec3Template<Type> CompMin(const ezVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise maximum of *this and rhs
  const ezVec3Template<Type> CompMax(const ezVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise clamped value of *this between low and high.
  const ezVec3Template<Type> CompClamp(const ezVec3Template<Type>& vLow, const ezVec3Template<Type>& vHigh) const; // [tested]

  /// \brief Returns the component-wise multiplication of *this and rhs
  const ezVec3Template<Type> CompMul(const ezVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise division of *this and rhs
  const ezVec3Template<Type> CompDiv(const ezVec3Template<Type>& rhs) const; // [tested]

  /// brief Returns the component-wise absolute of *this.
  const ezVec3Template<Type> Abs() const; // [tested]


  // *** Other common operations ***
public:
  /// \brief Calculates the normal of the triangle defined by the three vertices. Vertices are assumed to be ordered counter-clockwise.
  ezResult CalculateNormal(const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2, const ezVec3Template<Type>& v3); // [tested]

  /// \brief Modifies this direction vector to be orthogonal to the given (normalized) direction vector. The result is NOT normalized.
  ///
  /// \note This function may fail, e.g. create a vector that is zero, if the given normal is parallel to the vector itself.
  ///       If you need to handle such cases, you should manually check afterwards, whether the result is zero, or cannot be normalized.
  void MakeOrthogonalTo(const ezVec3Template<Type>& vNormal); // [tested]

  /// \brief Returns some arbitrary vector orthogonal to this one. The vector is NOT normalized.
  const ezVec3Template<Type> GetOrthogonalVector() const; // [tested]

  /// \brief Returns this vector reflected at vNormal.
  const ezVec3Template<Type> GetReflectedVector(const ezVec3Template<Type>& vNormal) const; // [tested]

  /// \brief Returns this vector, refracted at vNormal, using the refraction index of the current medium and the medium it enters.
  const ezVec3Template<Type> GetRefractedVector(const ezVec3Template<Type>& vNormal, Type fRefIndex1, Type fRefIndex2) const;

  /// \brief Returns a random point inside a unit sphere (radius 1).
  [[nodiscard]] static ezVec3Template<Type> MakeRandomPointInSphere(ezRandom& inout_rng);                                                      // [tested]
  [[deprecated("Use ezVec3::MakeRandomPointInSphere() instead.")]] static ezVec3Template<Type> CreateRandomPointInSphere(ezRandom& inout_rng); // [tested]

  /// \brief Creates a random direction vector. The vector is normalized.
  [[nodiscard]] static ezVec3Template<Type> MakeRandomDirection(ezRandom& inout_rng);                                                  // [tested]
  [[deprecated("Use ezVec3::MakeRandomDirection() instead.")]] static ezVec3Template<Type> CreateRandomDirection(ezRandom& inout_rng); // [tested]

  /// \brief Creates a random vector around the x axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  [[nodiscard]] static ezVec3Template<Type> MakeRandomDeviationX(ezRandom& inout_rng, const ezAngle& maxDeviation);                                                   // [tested]
  [[deprecated("Use ezVec3::MakeRandomDeviationX() instead.")]] static ezVec3Template<Type> CreateRandomDeviationX(ezRandom& inout_rng, const ezAngle& maxDeviation); // [tested]

  /// \brief Creates a random vector around the y axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  [[nodiscard]] static ezVec3Template<Type> MakeRandomDeviationY(ezRandom& inout_rng, const ezAngle& maxDeviation);                                                   // [tested]
  [[deprecated("Use ezVec3::MakeRandomDeviationY() instead.")]] static ezVec3Template<Type> CreateRandomDeviationY(ezRandom& inout_rng, const ezAngle& maxDeviation); // [tested]

  /// \brief Creates a random vector around the z axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  [[nodiscard]] static ezVec3Template<Type> MakeRandomDeviationZ(ezRandom& inout_rng, const ezAngle& maxDeviation);                                                   // [tested]
  [[deprecated("Use ezVec3::MakeRandomDeviationZ() instead.")]] static ezVec3Template<Type> CreateRandomDeviationZ(ezRandom& inout_rng, const ezAngle& maxDeviation); // [tested]

  /// \brief Creates a random vector around the given normal with a maximum deviation.
  /// \note If you are going to do this many times with the same axis, rather than calling this function, instead manually
  /// do what this function does (see inline code) and only compute the quaternion once.
  [[nodiscard]] static ezVec3Template<Type> MakeRandomDeviation(ezRandom& inout_rng, const ezAngle& maxDeviation, const ezVec3Template<Type>& vNormal);                                                  // [tested]
  [[deprecated("Use ezVec3::MakeRandomDeviation() instead.")]] static ezVec3Template<Type> CreateRandomDeviation(ezRandom& inout_rng, const ezAngle& maxDeviation, const ezVec3Template<Type>& vNormal); // [tested]
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
