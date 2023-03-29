#ifndef AE_FOUNDATION_MATH_MATRIX_H
#define AE_FOUNDATION_MATH_MATRIX_H

#include "Declarations.h"
#include "Vec3.h"

namespace AE_NS_FOUNDATION
{
  //! A 4x4 component matrix class.
  class AE_FOUNDATION_DLL aeMatrix
  {
  // *** Data ***
  public:
    // The elements are stored in column-major order.
    // That means first is colum 0 (with elements of row 0, row 1, row 2, row 3),
    // then column 1, then column 2 and finally column 3
    
    //! The union that allows different representations of the matrix data.
    union
    {
      //! The matrix as a 16-element float array
      float m_fElements[16];
      //! The matrix as a 4x4 float array
      float m_fColumn[4][4];
    };

  public:

    enum Layout
    {
      RowMajor,
      ColumnMajor
    };

  // *** Constructors ***
  public:
    //! Default Constructor DOES NOT INITIALIZE the matrix, at all.
    aeMatrix (void) {}
    //! Copies 16 values from pData into the matrix. Can handle the data in row-major or column-major order.
    explicit aeMatrix (const float* const pData, Layout layout);
    //! Sets each element manually: Naming is "column-n row-m"
    explicit aeMatrix (float c1r1, float c2r1, float c3r1, float c4r1,
                       float c1r2, float c2r2, float c3r2, float c4r2,
                       float c1r3, float c2r3, float c3r3, float c4r3,
                       float c1r4, float c2r4, float c3r4, float c4r4,
                       Layout layout = RowMajor);

    //! Copies 16 values from pData into the matrix. Can handle the data in row-major or column-major order.
    void SetMatrix (const float* const pData, Layout layout);
    //! Sets each element manually: Naming is "column-n row-m"
    void SetMatrix (float c1r1, float c2r1, float c3r1, float c4r1,
                    float c1r2, float c2r2, float c3r2, float c4r2,
                    float c1r3, float c2r3, float c3r3, float c4r3,
                    float c1r4, float c2r4, float c3r4, float c4r4,
                    Layout layout = RowMajor);

  // *** Special matrix constructors ***
  public:

    //! Sets all elements to zero.
    void SetZero (void);
    //! Sets all elements to zero, except the diagonal, which is set to one.
    void SetIdentity (void);

    //! Sets the matrix to all zero, except the last column, which is set to x,y,z,1
    void SetTranslationMatrix (const aeVec3& vTranslation);
    //! Sets the matrix to all zero, except the diagonal, which is set to x,y,z,1
    void SetScalingMatrix (float x, float y, float z);
    //! Sets the matrix to all zero, except the diagonal, which is set to x,y,z,1
    void SetScalingMatrix (float xyz) {SetScalingMatrix (xyz, xyz, xyz);}

    //! Sets this matrix to be a rotation matrix around the X-axis.
    void SetRotationMatrixX (float fAngle);
    //! Sets this matrix to be a rotation matrix around the Y-axis.
    void SetRotationMatrixY (float fAngle);
    //! Sets this matrix to be a rotation matrix around the Z-axis.
    void SetRotationMatrixZ (float fAngle);
    //! Sets this matrix to be a rotation matrix around the given axis.
    void SetRotationMatrix (const aeVec3& vAxis, float fAngle);

    //! Defines a matrix that alignes objects along the given vLookDirY vector (with their Y vector).
    void SetLookAtMatrixY (const aeVec3& vPos, const aeVec3& vLookDirY, const aeVec3& vOrthoDirZ = aeVec3::GetAxisZ ());
    //! Defines a matrix that alignes objects along the given vLookDirZ vector (with their Z vector).
    void SetLookAtMatrixZ (const aeVec3& vPos, const aeVec3& vLookDirZ, const aeVec3& vOrhtoDirY = aeVec3::GetAxisY ());

    //! Defines a matrix that alignes objects along the given vAlignDirY vector (with their Y vector).
    void SetObjectOrientationMatrixX (const aeVec3& vPos, const aeVec3& vAlignDirX, const aeVec3& vOrthoDirY = aeVec3::GetAxisY ());
    //! Defines a matrix that alignes objects along the given vAlignDirY vector (with their Y vector).
    void SetObjectOrientationMatrixY (const aeVec3& vPos, const aeVec3& vAlignDirY, const aeVec3& vOrthoDirZ = aeVec3::GetAxisZ ());
    //! Defines a matrix that alignes objects along the given vAlignDirZ vector (with their Z vector).
    void SetObjectOrientationMatrixZ (const aeVec3& vPos, const aeVec3& vAlignDirZ, const aeVec3& vOrhtoDirY = aeVec3::GetAxisY ());

  // *** Common Matrix Operations ***
  public:

    static const aeMatrix IdentityMatrix ();

    static const aeMatrix ZeroMatrix ();

    //! Transponates this matrix.
    void Transpose (void);
    //! Returns the transpose of this matrix.
    const aeMatrix GetTranspose (void) const;

    //! Inverts this matrix.
    void Invert (void);
    //! Returns the inverse of this matrix.
    const aeMatrix GetInverse (void) const;

  // *** Checks ***
  public:
    //! Checks whether all elements are zero.
    bool IsZeroMatrix (float fEpsilon) const;
    //! Checks whether this is an identity matrix.
    bool IsIdentityMatrix (float fEpsilon) const;
    //! Checks whether all components are finite numbers.
    bool IsValid () const;


  // *** Special Accessors ***
  public:

    //! Returns all 4 components of the i-th row.
    void GetRow (aeUInt32 uiRow, float& v1, float& v2, float& v3, float& v4) const;
    //! Sets all 4 components of the i-th row.
    void SetRow (aeUInt32 uiRow, float v1, float v2, float v3, float v4);

    //! Returns all 4 components of the i-th column.
    void GetColumn (aeUInt32 uiColumn, float& v1, float& v2, float& v3, float& v4) const;
    //! Sets all 4 components of the i-th column.
    void SetColumn (aeUInt32 uiColumn, float v1, float v2, float v3, float v4);

    //! Returns the quaternion that represents the same rotation as this matrix. Does not work with all (rotation)-matrices, so not available until resolved.
    const aeQuaternion GetAsQuaternion (void) const;

    //! Stores the row-major matrix as column-major in the 9-element pDst buffer.
    void GetAsOpenGL3x3Matrix (float* pDst) const;
    //! Stores the row-major matrix as column-major in the 9-element pDst buffer.
    void GetAsOpenGL3x3Matrix (double* pDst) const;

    //! Stores the row-major matrix as column-major in the 16-element pDst buffer.
    void GetAsOpenGL4x4Matrix (float* pDst) const;
    //! Stores the row-major matrix as column-major in the 16-element pDst buffer.
    void GetAsOpenGL4x4Matrix (double* pDst) const;

    //! Returns the first 3 components of the last column.
    const aeVec3 GetTranslationVector (void) const;
    //! Sets the first 3 components of the last column.
    void SetTranslationVector (const aeVec3& v);

  // *** Operators ***
  public:

    //! Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
    const aeVec3 TransformPosition (const aeVec3& v) const;
    //! Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only. Useful as an optimization.
    const aeVec3 TransformDirection (const aeVec3& v) const;
    //! Matrix-vector multiplication with an additional 4th vector component. The result is stored in inout_w.
    const aeVec3 TransformWithWComponent (const aeVec3& v, float* inout_w) const;

    //! Adds v to the 4th column (translation only)
    void operator+= (const aeVec3& v);
    //! Subtracts v from the 4th column (translation only)
    void operator-= (const aeVec3& v);

    //! Component-wise multiplication (commutative)
    void operator*= (float f);
    //! Component-wise division
    void operator/= (float f);

    //! Equality Check
    bool IsIdentical (const aeMatrix& rhs) const;
    //! Equality Check with epsilon
    bool IsEqual (const aeMatrix& rhs, float fEpsilon) const;
  };


  // *** free functions ***

  //! Matrix-Matrix multiplication
  const aeMatrix operator* (const aeMatrix& m1, const aeMatrix& m2);	
  //! Matrix-vector multiplication
  const aeVec3   operator* (const aeMatrix& m, const aeVec3& v);	
  //! Adds v to the 4th column (translation only)
  const aeMatrix operator+ (const aeMatrix& m, const aeVec3& v);
  //! Subtracts v from the 4th column (translation only)
  const aeMatrix operator- (const aeMatrix& m, const aeVec3& v);	

  //! Component-wise multiplication (commutative)
  const aeMatrix operator* (const aeMatrix& m1, float f);
  //! Component-wise multiplication (commutative)
  const aeMatrix operator* (float f, const aeMatrix& m1);

  //! Component-wise division
  const aeMatrix operator/ (const aeMatrix& m1, float f);

  //! Adding two matrices (component-wise)
  const aeMatrix operator+ (const aeMatrix& m1, const aeMatrix& m2);
  //! Subtracting two matrices (component-wise)
  const aeMatrix operator- (const aeMatrix& m1, const aeMatrix& m2);

  //! Comparison Operator ==
  bool operator== (const aeMatrix& lhs, const aeMatrix& rhs);
  //! Comparison Operator !=
  bool operator!= (const aeMatrix& lhs, const aeMatrix& rhs);
}

#include "Inline/Matrix.inl"

#endif



