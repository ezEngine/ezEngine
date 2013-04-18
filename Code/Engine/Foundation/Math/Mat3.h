#pragma once

#include <Foundation/Math/Vec3.h>

/// \brief A 4x4 component matrix class.
class EZ_FOUNDATION_DLL ezMat3
{
public:
  EZ_DECLARE_POD_TYPE();

// *** Data ***
public:
  // The elements are stored in column-major order.
  // That means first is column 0 (with elements of row 0, row 1, row 2),
  // then column 1, then column 2
    
  /// \brief The union that allows different representations of the matrix data.
  union
  {
    /// \brief The matrix as a 9-element float array (column-major)
    float m_fElementsCM[9];

    /// \brief The matrix as a 3x3 float array
    float m_fColumn[3][3];
  };

// *** Constructors ***
public:
  /// \brief Default Constructor DOES NOT INITIALIZE the matrix, at all.
  /// \test Requires a unit-test.
  ezMat3();

  /// \brief Copies 9 values from pData into the matrix. Can handle the data in row-major or column-major order.
  ///
  /// \param pData
  ///   The array of float values from which to set the matrix data.
  /// \param layout
  ///   The layout in which pData stores the matrix. The data will get transposed, if necessary.
  /// \test Requires a unit-test.
  ezMat3(const float* const pData, ezMatrixLayout::Enum layout);

  /// \brief Sets each element manually: Naming is "column-n row-m"
  /// \test Requires a unit-test.
  ezMat3(float c1r1, float c2r1, float c3r1,
         float c1r2, float c2r2, float c3r2,
         float c1r3, float c2r3, float c3r3);

  /// \brief Copies 9 values from pData into the matrix. Can handle the data in row-major or column-major order.
  ///
  /// \param pData
  ///   The array of float values from which to set the matrix data.
  /// \param layout
  ///   The layout in which pData stores the matrix. The data will get transposed, if necessary.
  /// \test Requires a unit-test.
  void SetFromArray(const float* const pData, ezMatrixLayout::Enum layout);

  /// \brief Copies the 9 values of this matrix into the given array. 'layout' defines whether the data should end up in column-major or row-major format.
  /// \test Requires a unit-test.
  void GetAsArray(float* out_pData, ezMatrixLayout::Enum layout) const;

  /// \brief Sets each element manually: Naming is "column-n row-m"
  /// \test Requires a unit-test.
  void SetElements(float c1r1, float c2r1, float c3r1,
                   float c1r2, float c2r2, float c3r2,
                   float c1r3, float c2r3, float c3r3);

// *** Special matrix constructors ***
public:

  /// \brief Sets all elements to zero.
  /// \test Requires a unit-test.
  void SetZero();

  /// \brief Sets all elements to zero, except the diagonal, which is set to one.
  /// \test Requires a unit-test.
  void SetIdentity();

  /// \brief Sets the matrix to all zero, except the diagonal, which is set to x,y,z,1
  /// \test Requires a unit-test.
  void SetScalingMatrix(const ezVec3& vScale);

  /// \brief Sets this matrix to be a rotation matrix around the X-axis.
  /// \test Requires a unit-test.
  void SetRotationMatrixX(float fAngle);

  /// \brief Sets this matrix to be a rotation matrix around the Y-axis.
  /// \test Requires a unit-test.
  void SetRotationMatrixY(float fAngle);

  /// \brief Sets this matrix to be a rotation matrix around the Z-axis.
  /// \test Requires a unit-test.
  void SetRotationMatrixZ(float fAngle);

  /// \brief Sets this matrix to be a rotation matrix around the given axis.
  /// \test Requires a unit-test.
  void SetRotationMatrix(const ezVec3& vAxis, float fAngle);

  /// \brief Sets this matrix to be a look-at matrix (without the translation).
  /// \test Requires a unit-test.
  void SetLookInDirectionMatrix (ezVec3 vLookDir, ezVec3 vUpDir = ezVec3 (0, 1, 0));

// *** Common Matrix Operations ***
public:

  /// \brief Returns an Identity Matrix.
  /// \test Requires a unit-test.
  static const ezMat3 IdentityMatrix();

  /// \brief Returns a Zero Matrix.
  /// \test Requires a unit-test.
  static const ezMat3 ZeroMatrix();

  /// \brief Transposes this matrix.
  /// \test Requires a unit-test.
  void Transpose();

  /// \brief Returns the transpose of this matrix.
  /// \test Requires a unit-test.
  const ezMat3 GetTranspose() const;

  /// \brief Inverts this matrix. Return value indicates whether the matrix could be Inverted.
  /// \test Requires a unit-test.
  ezResult Invert(float fEpsilon = ezMath_SmallEpsilon);

  /// \brief Returns the inverse of this matrix.
  /// \test Requires a unit-test.
  const ezMat3 GetInverse() const;

// *** Checks ***
public:

  /// \brief Checks whether all elements are zero.
  /// \test Requires a unit-test.
  bool IsZero(float fEpsilon = ezMath_DefaultEpsilon) const;

  /// \brief Checks whether this is an identity matrix.
  /// \test Requires a unit-test.
  bool IsIdentity(float fEpsilon = ezMath_DefaultEpsilon) const;

  /// \brief Checks whether all components are finite numbers.
  /// \test Requires a unit-test.
  bool IsValid() const;


// *** Special Accessors ***
public:

  /// \brief Returns all 3 components of the i-th row.
  /// \test Requires a unit-test.
  ezVec3 GetRow(ezUInt32 uiRow) const;

  /// \brief Sets all 3 components of the i-th row.
  /// \test Requires a unit-test.
  void SetRow(ezUInt32 uiRow, const ezVec3& row);

  /// \brief Returns all 3 components of the i-th column.
  /// \test Requires a unit-test.
  ezVec3 GetColumn(ezUInt32 uiColumn) const;

  /// \brief Sets all 3 components of the i-th column.
  /// \test Requires a unit-test.
  void SetColumn(ezUInt32 uiColumn, const ezVec3& column);

  /// \brief Returns all 3 components on the diagonal of the matrix.
  /// \test Requires a unit-test.
  ezVec3 GetDiagonal() const;

  /// \brief Sets all 3 components on the diagonal of the matrix.
  /// \test Requires a unit-test.
  void SetDiagonal(const ezVec3& diag);


// *** Operators ***
public:

  /// \brief Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only. Useful as an optimization.
  /// \test Requires a unit-test.
  const ezVec3 TransformDirection(const ezVec3& v) const;

  /// \brief Component-wise multiplication (commutative)
  /// \test Requires a unit-test.
  void operator*= (float f);

  /// \brief Component-wise division.
  /// \test Requires a unit-test.
  void operator/= (float f);

  /// \brief Equality Check.
  /// \test Requires a unit-test.
  bool IsIdentical(const ezMat3& rhs) const;

  /// \brief Equality Check with epsilon.
  /// \test Requires a unit-test.
  bool IsEqual(const ezMat3& rhs, float fEpsilon) const;
};


// *** free functions ***

/// \brief Matrix-Matrix multiplication
/// \test Requires a unit-test.
const ezMat3 operator* (const ezMat3& m1, const ezMat3& m2);	

/// \brief Matrix-vector multiplication
/// \test Requires a unit-test.
const ezVec3 operator* (const ezMat3& m, const ezVec3& v);	

/// \brief Component-wise multiplication (commutative)
/// \test Requires a unit-test.
const ezMat3 operator* (const ezMat3& m1, float f);

/// \brief Component-wise multiplication (commutative)
/// \test Requires a unit-test.
const ezMat3 operator* (float f, const ezMat3& m1);

/// \brief Component-wise division
/// \test Requires a unit-test.
const ezMat3 operator/ (const ezMat3& m1, float f);

/// \brief Adding two matrices (component-wise)
/// \test Requires a unit-test.
const ezMat3 operator+ (const ezMat3& m1, const ezMat3& m2);

/// \brief Subtracting two matrices (component-wise)
/// \test Requires a unit-test.
const ezMat3 operator- (const ezMat3& m1, const ezMat3& m2);

/// \brief Comparison Operator ==
/// \test Requires a unit-test.
bool operator== (const ezMat3& lhs, const ezMat3& rhs);

/// \brief Comparison Operator !=
/// \test Requires a unit-test.
bool operator!= (const ezMat3& lhs, const ezMat3& rhs);

#include <Foundation/Math/Implementation/Mat3_inl.h>




