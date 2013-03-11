#pragma once

#include <Foundation/Math/Vec3.h>

/// A 4x4 component matrix class.
class EZ_FOUNDATION_DLL ezMat3
{
public:
  EZ_DECLARE_POD_TYPE();

// *** Data ***
public:
  // The elements are stored in column-major order.
  // That means first is column 0 (with elements of row 0, row 1, row 2),
  // then column 1, then column 2
    
  /// The union that allows different representations of the matrix data.
  union
  {
    /// The matrix as a 9-element float array (column-major)
    float m_fElementsCM[9];

    /// The matrix as a 3x3 float array
    float m_fColumn[3][3];
  };

// *** Constructors ***
public:
  /// Default Constructor DOES NOT INITIALIZE the matrix, at all.
  ezMat3();

  /// Copies 9 values from pData into the matrix. Can handle the data in row-major or column-major order.
  ezMat3(const float* const pData, ezMatrixLayout::Enum layout);

  /// Sets each element manually: Naming is "column-n row-m"
  ezMat3(float c1r1, float c2r1, float c3r1,
         float c1r2, float c2r2, float c3r2,
         float c1r3, float c2r3, float c3r3,
         ezMatrixLayout::Enum layout = ezMatrixLayout::RowMajor);

  /// Copies 9 values from pData into the matrix. Can handle the data in row-major or column-major order.
  void SetFromArray(const float* const pData, ezMatrixLayout::Enum layout);

  /// Copies the 9 values of this matrix into the given array. 'layout' defines whether the data should end up in column-major or row-major format.
  void GetAsArray(float* out_pData, ezMatrixLayout::Enum layout) const;

  /// Sets each element manually: Naming is "column-n row-m"
  void SetElements(float c1r1, float c2r1, float c3r1,
                   float c1r2, float c2r2, float c3r2,
                   float c1r3, float c2r3, float c3r3,
                   ezMatrixLayout::Enum layout = ezMatrixLayout::RowMajor);

// *** Special matrix constructors ***
public:

  /// Sets all elements to zero.
  void SetZero();

  /// Sets all elements to zero, except the diagonal, which is set to one.
  void SetIdentity();

  /// Sets the matrix to all zero, except the diagonal, which is set to x,y,z,1
  void SetScalingMatrix(const ezVec3& vScale);

  /// Sets this matrix to be a rotation matrix around the X-axis.
  void SetRotationMatrixX(float fAngle);

  /// Sets this matrix to be a rotation matrix around the Y-axis.
  void SetRotationMatrixY(float fAngle);

  /// Sets this matrix to be a rotation matrix around the Z-axis.
  void SetRotationMatrixZ(float fAngle);

  /// Sets this matrix to be a rotation matrix around the given axis.
  void SetRotationMatrix(const ezVec3& vAxis, float fAngle);

  /// Sets this matrix to be a look-at matrix (without the translation).
  void SetLookInDirectionMatrix (ezVec3 vLookDir, ezVec3 vUpDir = ezVec3 (0, 1, 0));

// *** Common Matrix Operations ***
public:

  /// Returns an Identity Matrix.
  static const ezMat3 IdentityMatrix();

  /// Returns a Zero Matrix.
  static const ezMat3 ZeroMatrix();

  /// Transposes this matrix.
  void Transpose();

  /// Returns the transpose of this matrix.
  const ezMat3 GetTranspose() const;

  /// Inverts this matrix. Return value indicates whether the matrix could be Inverted.
  ezResult Invert(float fEpsilon = ezMath_SmallEpsilon);

  /// Returns the inverse of this matrix.
  const ezMat3 GetInverse() const;

// *** Checks ***
public:

  /// Checks whether all elements are zero.
  bool IsZero(float fEpsilon = ezMath_DefaultEpsilon) const;

  /// Checks whether this is an identity matrix.
  bool IsIdentity(float fEpsilon = ezMath_DefaultEpsilon) const;

  /// Checks whether all components are finite numbers.
  bool IsValid() const;


// *** Special Accessors ***
public:

  /// Returns all 3 components of the i-th row.
  ezVec3 GetRow(ezUInt32 uiRow) const;

  /// Sets all 3 components of the i-th row.
  void SetRow(ezUInt32 uiRow, const ezVec3& row);

  /// Returns all 3 components of the i-th column.
  ezVec3 GetColumn(ezUInt32 uiColumn) const;

  /// Sets all 3 components of the i-th column.
  void SetColumn(ezUInt32 uiColumn, const ezVec3& column);

  /// Returns all 3 components on the diagonal of the matrix.
  ezVec3 GetDiagonal() const;

  /// Sets all 3 components on the diagonal of the matrix.
  void SetDiagonal(const ezVec3& diag);


// *** Operators ***
public:

  /// Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only. Useful as an optimization.
  const ezVec3 TransformDirection(const ezVec3& v) const;

  /// Component-wise multiplication (commutative)
  void operator*= (float f);

  /// Component-wise division
  void operator/= (float f);

  /// Equality Check
  bool IsIdentical(const ezMat3& rhs) const;

  /// Equality Check with epsilon
  bool IsEqual(const ezMat3& rhs, float fEpsilon) const;
};


// *** free functions ***

/// Matrix-Matrix multiplication
const ezMat3 operator* (const ezMat3& m1, const ezMat3& m2);	

/// Matrix-vector multiplication
const ezVec3 operator* (const ezMat3& m, const ezVec3& v);	

/// Component-wise multiplication (commutative)
const ezMat3 operator* (const ezMat3& m1, float f);

/// Component-wise multiplication (commutative)
const ezMat3 operator* (float f, const ezMat3& m1);

/// Component-wise division
const ezMat3 operator/ (const ezMat3& m1, float f);

/// Adding two matrices (component-wise)
const ezMat3 operator+ (const ezMat3& m1, const ezMat3& m2);

/// Subtracting two matrices (component-wise)
const ezMat3 operator- (const ezMat3& m1, const ezMat3& m2);

/// Comparison Operator ==
bool operator== (const ezMat3& lhs, const ezMat3& rhs);

/// Comparison Operator !=
bool operator!= (const ezMat3& lhs, const ezMat3& rhs);

#include <Foundation/Math/Implementation/Mat3_inl.h>




