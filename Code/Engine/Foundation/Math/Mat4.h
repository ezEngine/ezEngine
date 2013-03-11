#pragma once

#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>
#include <Foundation/Math/Mat3.h>

/// A 4x4 component matrix class.
class EZ_FOUNDATION_DLL ezMat4
{
public:
  EZ_DECLARE_POD_TYPE();

// *** Data ***
public:
  // The elements are stored in column-major order.
  // That means first is colum 0 (with elements of row 0, row 1, row 2, row 3),
  // then column 1, then column 2 and finally column 3
    
  /// The union that allows different representations of the matrix data.
  union
  {
    /// The matrix as a 16-element float array (column-major)
    float m_fElementsCM[16];

    /// The matrix as a 4x4 float array
    float m_fColumn[4][4];
  };

// *** Constructors ***
public:
  /// Default Constructor DOES NOT INITIALIZE the matrix, at all.
  ezMat4();

  /// Copies 16 values from pData into the matrix. Can handle the data in row-major or column-major order.
  ezMat4(const float* const pData, ezMatrixLayout::Enum layout);

  /// Sets each element manually: Naming is "column-n row-m"
  ezMat4(float c1r1, float c2r1, float c3r1, float c4r1,
         float c1r2, float c2r2, float c3r2, float c4r2,
         float c1r3, float c2r3, float c3r3, float c4r3,
         float c1r4, float c2r4, float c3r4, float c4r4,
         ezMatrixLayout::Enum layout = ezMatrixLayout::RowMajor);

  /// Creates a transformation matrix from a rotation and a translation.
  ezMat4(const ezMat3& Rotation, const ezVec3& vTranslation);

  /// Copies 16 values from pData into the matrix. Can handle the data in row-major or column-major order.
  void SetFromArray(const float* const pData, ezMatrixLayout::Enum layout);

  /// Copies the 16 values of this matrix into the given array. 'layout' defines whether the data should end up in column-major or row-major format.
  void GetAsArray(float* out_pData, ezMatrixLayout::Enum layout) const;

  /// Sets each element manually: Naming is "column-n row-m"
  void SetElements(float c1r1, float c2r1, float c3r1, float c4r1,
                 float c1r2, float c2r2, float c3r2, float c4r2,
                 float c1r3, float c2r3, float c3r3, float c4r3,
                 float c1r4, float c2r4, float c3r4, float c4r4,
                 ezMatrixLayout::Enum layout = ezMatrixLayout::RowMajor);

  /// Sets a transformation matrix from a rotation and a translation.
  void SetTransformationMatrix(const ezMat3& Rotation, const ezVec3& vTranslation);

// *** Special matrix constructors ***
public:

  /// Sets all elements to zero.
  void SetZero();

  /// Sets all elements to zero, except the diagonal, which is set to one.
  void SetIdentity();

  /// Sets the matrix to all zero, except the last column, which is set to x,y,z,1
  void SetTranslationMatrix(const ezVec3& vTranslation);

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

  /// Creates a perspective projection matrix.
  void SetPerspectiveProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, ezProjectionDepthRange::Enum DepthRange);

  /// Creates a perspective projection matrix.
  void SetPerspectiveProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, ezProjectionDepthRange::Enum DepthRange);

  /// Creates a perspective projection matrix.
  void SetPerspectiveProjectionMatrixFromFovX(float fFieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, ezProjectionDepthRange::Enum DepthRange);

  /// Creates a perspective projection matrix.
  void SetPerspectiveProjectionMatrixFromFovY(float fFieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, ezProjectionDepthRange::Enum DepthRange);

  /// Creates an orthographic projection matrix.
  void SetOrthographicProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, ezProjectionDepthRange::Enum DepthRange);

  /// Creates an orthographic projection matrix.
  void SetOrthographicProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, ezProjectionDepthRange::Enum DepthRange);

  /// Creates a look-at matrix.
  void SetLookAtMatrix(const ezVec3& vStartPos, const ezVec3& vTargetPos, const ezVec3& vUpDir);

// *** Common Matrix Operations ***
public:

  /// Returns an Identity Matrix.
  static const ezMat4 IdentityMatrix();

  /// Returns a Zero Matrix.
  static const ezMat4 ZeroMatrix();

  /// Transposes this matrix.
  void Transpose();

  /// Returns the transpose of this matrix.
  const ezMat4 GetTranspose() const;

  /// Inverts this matrix. Return value indicates whether the matrix could be inverted.
  ezResult Invert(float fEpsilon = ezMath_SmallEpsilon);

  /// Returns the inverse of this matrix.
  const ezMat4 GetInverse() const;

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

  /// Returns all 4 components of the i-th row.
  ezVec4 GetRow(ezUInt32 uiRow) const;

  /// Sets all 4 components of the i-th row.
  void SetRow(ezUInt32 uiRow, const ezVec4& row);

  /// Returns all 4 components of the i-th column.
  ezVec4 GetColumn(ezUInt32 uiColumn) const;

  /// Sets all 4 components of the i-th column.
  void SetColumn(ezUInt32 uiColumn, const ezVec4& column);

  /// Returns all 4 components on the diagonal of the matrix.
  ezVec4 GetDiagonal() const;

  /// Sets all 4 components on the diagonal of the matrix.
  void SetDiagonal(const ezVec4& diag);

  /// Returns the first 3 components of the last column.
  const ezVec3 GetTranslationVector() const;

  /// Sets the first 3 components of the last column.
  void SetTranslationVector(const ezVec3& v);

  /// Sets the 3x3 rotational part of the matrix.
  void SetRotationalPart(const ezMat3& Rotation);

  /// Returns the 3x3 rotational part of the matrix.
  const ezMat3 GetRotationalPart() const;

  /// Returns the 3 scaling factors that are encoded in the matrix.
  const ezVec3 GetScalingFactors() const;

  /// Tries to set the three scaling factors in the matrix. Returns EZ_FAILURE if the matrix columns cannot be normalized and thus no rescaling is possible.
  ezResult SetScalingFactors(const ezVec3& vXYZ, float fEpsilon = ezMath_DefaultEpsilon);

// *** Operators ***
public:

  /// Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
  const ezVec3 TransformPosition(const ezVec3& v) const;
  /// Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
  void TransformPosition(ezVec3* inout_v, ezUInt32 uiNumVectors, ezUInt32 uiStride = sizeof(ezVec3)) const;

  /// Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only. Useful as an optimization.
  const ezVec3 TransformDirection(const ezVec3& v) const;
  /// Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only. Useful as an optimization.
  void TransformDirection(ezVec3* inout_v, ezUInt32 uiNumVectors, ezUInt32 uiStride = sizeof(ezVec3)) const;

  /// Matrix-vector multiplication.
  const ezVec4 Transform(const ezVec4& v) const;
  /// Matrix-vector multiplication.
  void Transform(ezVec4* inout_v, ezUInt32 uiNumVectors, ezUInt32 uiStride = sizeof(ezVec4)) const;

  /// Component-wise multiplication (commutative)
  void operator*= (float f);

  /// Component-wise division
  void operator/= (float f);

  /// Equality Check
  bool IsIdentical(const ezMat4& rhs) const;

  /// Equality Check with epsilon
  bool IsEqual(const ezMat4& rhs, float fEpsilon) const;
};


// *** free functions ***

/// Matrix-Matrix multiplication
const ezMat4 operator* (const ezMat4& m1, const ezMat4& m2);	

/// Matrix-vector multiplication
const ezVec3 operator* (const ezMat4& m, const ezVec3& v);	

/// Matrix-vector multiplication
const ezVec4 operator* (const ezMat4& m, const ezVec4& v);	

/// Component-wise multiplication (commutative)
const ezMat4 operator* (const ezMat4& m1, float f);

/// Component-wise multiplication (commutative)
const ezMat4 operator* (float f, const ezMat4& m1);

/// Component-wise division
const ezMat4 operator/ (const ezMat4& m1, float f);

/// Adding two matrices (component-wise)
const ezMat4 operator+ (const ezMat4& m1, const ezMat4& m2);

/// Subtracting two matrices (component-wise)
const ezMat4 operator- (const ezMat4& m1, const ezMat4& m2);

/// Comparison Operator ==
bool operator== (const ezMat4& lhs, const ezMat4& rhs);

/// Comparison Operator !=
bool operator!= (const ezMat4& lhs, const ezMat4& rhs);

#include <Foundation/Math/Implementation/Mat4_inl.h>




