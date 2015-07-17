#pragma once

#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Angle.h>

/// \brief A 3x3 component matrix class.
template<typename Type>
class ezMat3Template
{
public:
  EZ_DECLARE_POD_TYPE();

  typedef Type ComponentType;

// *** Data ***
public:
  // The elements are stored in column-major order.
  // That means first is column 0 (with elements of row 0, row 1, row 2),
  // then column 1, then column 2
    
  /// \brief The matrix as a 9-element Type array (column-major)
  Type m_fElementsCM[9];

  Type& Element(ezInt32 column, ezInt32 row) { return m_fElementsCM[column*3 + row]; }
  Type Element(ezInt32 column, ezInt32 row) const { return m_fElementsCM[column*3 + row]; }

// *** Constructors ***
public:
  /// \brief Default Constructor DOES NOT INITIALIZE the matrix, at all.
  ezMat3Template(); // [tested]

  /// \brief Copies 9 values from pData into the matrix. Can handle the data in row-major or column-major order.
  ///
  /// \param pData
  ///   The array of Type values from which to set the matrix data.
  /// \param layout
  ///   The layout in which pData stores the matrix. The data will get transposed, if necessary.
  ///   The data should be in column-major format, if you want to prevent unnecessary transposes.
  ezMat3Template(const Type* const pData, ezMatrixLayout::Enum layout); // [tested]

  /// \brief Sets each element manually: Naming is "column-n row-m"
  ezMat3Template(Type c1r1, Type c2r1, Type c3r1,
                 Type c1r2, Type c2r2, Type c3r2,
                 Type c1r3, Type c2r3, Type c3r3); // [tested]

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    EZ_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that all code-paths properly initialize this object.");
  }
#endif

  /// \brief Copies 9 values from pData into the matrix. Can handle the data in row-major or column-major order.
  ///
  /// \param pData
  ///   The array of Type values from which to set the matrix data.
  /// \param layout
  ///   The layout in which pData stores the matrix. The data will get transposed, if necessary.
  ///   The data should be in column-major format, if you want to prevent unnecessary transposes.
  void SetFromArray(const Type* const pData, ezMatrixLayout::Enum layout); // [tested]

  /// \brief Copies the 9 values of this matrix into the given array. 'layout' defines whether the data should end up in column-major or row-major format.
  void GetAsArray(Type* out_pData, ezMatrixLayout::Enum layout) const; // [tested]

  /// \brief Sets each element manually: Naming is "column-n row-m"
  void SetElements(Type c1r1, Type c2r1, Type c3r1,
                   Type c1r2, Type c2r2, Type c3r2,
                   Type c1r3, Type c2r3, Type c3r3); // [tested]

// *** Special matrix constructors ***
public:

  /// \brief Sets all elements to zero.
  void SetZero(); // [tested]

  /// \brief Sets all elements to zero, except the diagonal, which is set to one.
  void SetIdentity(); // [tested]

  /// \brief Sets the matrix to all zero, except the diagonal, which is set to x,y,z,1
  void SetScalingMatrix(const ezVec3Template<Type>& vScale); // [tested]

  /// \brief Sets this matrix to be a rotation matrix around the X-axis.
  void SetRotationMatrixX(ezAngle angle); // [tested]

  /// \brief Sets this matrix to be a rotation matrix around the Y-axis.
  void SetRotationMatrixY(ezAngle angle); // [tested]

  /// \brief Sets this matrix to be a rotation matrix around the Z-axis.
  void SetRotationMatrixZ(ezAngle angle); // [tested]

  /// \brief Sets this matrix to be a rotation matrix around the given axis.
  void SetRotationMatrix(const ezVec3Template<Type>& vAxis, ezAngle angle); // [tested]

  /// \brief Sets this matrix to be a look-at matrix (without the translation).
  void SetLookInDirectionMatrix(ezVec3Template<Type> vLookDir, ezVec3Template<Type> vUpDir = ezVec3Template<Type> (0, 0, 1));

// *** Common Matrix Operations ***
public:

  /// \brief Returns an Identity Matrix.
  static const ezMat3Template<Type> IdentityMatrix(); // [tested]

  /// \brief Returns a Zero Matrix.
  static const ezMat3Template<Type> ZeroMatrix(); // [tested]

  /// \brief Transposes this matrix.
  void Transpose(); // [tested]

  /// \brief Returns the transpose of this matrix.
  const ezMat3Template<Type> GetTranspose() const; // [tested]

  /// \brief Inverts this matrix. Return value indicates whether the matrix could be Inverted.
  ezResult Invert(Type fEpsilon = ezMath::BasicType<Type>::SmallEpsilon()); // [tested]

  /// \brief Returns the inverse of this matrix.
  const ezMat3Template<Type> GetInverse() const; // [tested]

// *** Checks ***
public:

  /// \brief Checks whether all elements are zero.
  bool IsZero(Type fEpsilon = ezMath::BasicType<Type>::DefaultEpsilon()) const; // [tested]

  /// \brief Checks whether this is an identity matrix.
  bool IsIdentity(Type fEpsilon = ezMath::BasicType<Type>::DefaultEpsilon()) const; // [tested]

  /// \brief Checks whether all components are finite numbers.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

// *** Special Accessors ***
public:

  /// \brief Returns all 3 components of the i-th row.
  ezVec3Template<Type> GetRow(ezUInt32 uiRow) const; // [tested]

  /// \brief Sets all 3 components of the i-th row.
  void SetRow(ezUInt32 uiRow, const ezVec3Template<Type>& row); // [tested]

  /// \brief Returns all 3 components of the i-th column.
  ezVec3Template<Type> GetColumn(ezUInt32 uiColumn) const; // [tested]

  /// \brief Sets all 3 components of the i-th column.
  void SetColumn(ezUInt32 uiColumn, const ezVec3Template<Type>& column); // [tested]

  /// \brief Returns all 3 components on the diagonal of the matrix.
  ezVec3Template<Type> GetDiagonal() const; // [tested]

  /// \brief Sets all 3 components on the diagonal of the matrix.
  void SetDiagonal(const ezVec3Template<Type>& diag); // [tested]

  /// \brief Returns the 3 scaling factors that are encoded in the matrix.
  const ezVec3Template<Type> GetScalingFactors() const; // [tested]

  /// \brief Tries to set the three scaling factors in the matrix. Returns EZ_FAILURE if the matrix columns cannot be normalized and thus no rescaling is possible.
  ezResult SetScalingFactors(const ezVec3Template<Type>& vXYZ, Type fEpsilon = ezMath::BasicType<Type>::DefaultEpsilon()); // [tested]


// *** Operators ***
public:

  /// \brief Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only. Useful as an optimization.
  const ezVec3Template<Type> TransformDirection(const ezVec3Template<Type>& v) const; // [tested]

  /// \brief Component-wise multiplication (commutative)
  void operator*= (Type f);

  /// \brief Component-wise division.
  void operator/= (Type f); // [tested]

  /// \brief Equality Check.
  bool IsIdentical(const ezMat3Template<Type>& rhs) const; // [tested]

  /// \brief Equality Check with epsilon.
  bool IsEqual(const ezMat3Template<Type>& rhs, Type fEpsilon) const; // [tested]
};


// *** free functions ***

/// \brief Matrix-Matrix multiplication
template<typename Type>
const ezMat3Template<Type> operator* (const ezMat3Template<Type>& m1, const ezMat3Template<Type>& m2); // [tested]

/// \brief Matrix-vector multiplication
template<typename Type>
const ezVec3Template<Type> operator* (const ezMat3Template<Type>& m, const ezVec3Template<Type>& v); // [tested]

/// \brief Component-wise multiplication (commutative)
template<typename Type>
const ezMat3Template<Type> operator* (const ezMat3Template<Type>& m1, Type f); // [tested]

/// \brief Component-wise multiplication (commutative)
template<typename Type>
const ezMat3Template<Type> operator* (Type f, const ezMat3Template<Type>& m1); // [tested]

/// \brief Component-wise division
template<typename Type>
const ezMat3Template<Type> operator/ (const ezMat3Template<Type>& m1, Type f); // [tested]

/// \brief Adding two matrices (component-wise)
template<typename Type>
const ezMat3Template<Type> operator+ (const ezMat3Template<Type>& m1, const ezMat3Template<Type>& m2); // [tested]

/// \brief Subtracting two matrices (component-wise)
template<typename Type>
const ezMat3Template<Type> operator- (const ezMat3Template<Type>& m1, const ezMat3Template<Type>& m2); // [tested]

/// \brief Comparison Operator ==
template<typename Type>
bool operator== (const ezMat3Template<Type>& lhs, const ezMat3Template<Type>& rhs); // [tested]

/// \brief Comparison Operator !=
template<typename Type>
bool operator!= (const ezMat3Template<Type>& lhs, const ezMat3Template<Type>& rhs); // [tested]

#include <Foundation/Math/Implementation/Mat3_inl.h>




