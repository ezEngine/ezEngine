#ifndef AE_FOUNDATION_MATH_MATRIX_INL
#define AE_FOUNDATION_MATH_MATRIX_INL

namespace AE_NS_FOUNDATION
{
  inline aeMatrix::aeMatrix (const float* const pData, Layout layout)
  {
    SetMatrix (pData, layout);
  }

  inline aeMatrix::aeMatrix (float c1r1, float c2r1, float c3r1, float c4r1,
                                float c1r2, float c2r2, float c3r2, float c4r2,
                                float c1r3, float c2r3, float c3r3, float c4r3,
                                float c1r4, float c2r4, float c3r4, float c4r4,
                                Layout layout)
  {
    SetMatrix (c1r1, c2r1, c3r1, c4r1,
               c1r2, c2r2, c3r2, c4r2,
               c1r3, c2r3, c3r3, c4r3,
               c1r4, c2r4, c3r4, c4r4,
               layout);
  }

  inline const aeMatrix aeMatrix::IdentityMatrix ()
  {
    return aeMatrix (1, 0, 0, 0,
                     0, 1, 0, 0,
                     0, 0, 1, 0,
                     0, 0, 0, 1);
  }

  inline const aeMatrix aeMatrix::ZeroMatrix ()
  {
    return aeMatrix (0, 0, 0, 0,
                     0, 0, 0, 0,
                     0, 0, 0, 0,
                     0, 0, 0, 0);
  }

  inline void aeMatrix::SetMatrix (const float* const pData, Layout layout)
  {
    if (layout == ColumnMajor)
    {
      for (int i = 0; i < 4; ++i)
      {
        m_fColumn[i][0] = pData[i * 4 + 0];
        m_fColumn[i][1] = pData[i * 4 + 1];
        m_fColumn[i][2] = pData[i * 4 + 2];
        m_fColumn[i][3] = pData[i * 4 + 3];
      }
    }
    else
    {
      for (int i = 0; i < 4; ++i)
      {
        m_fColumn[0][i] = pData[i * 4 + 0];
        m_fColumn[1][i] = pData[i * 4 + 1];
        m_fColumn[2][i] = pData[i * 4 + 2];
        m_fColumn[3][i] = pData[i * 4 + 3];
      }
    }
  }

  inline void aeMatrix::SetMatrix (float c1r1, float c2r1, float c3r1, float c4r1,
                                      float c1r2, float c2r2, float c3r2, float c4r2,
                                      float c1r3, float c2r3, float c3r3, float c4r3,
                                      float c1r4, float c2r4, float c3r4, float c4r4,
                                      Layout layout)
  {
    if (layout == ColumnMajor)
    {
      m_fColumn[0][0] = c1r1;	m_fColumn[0][1] = c2r1;	m_fColumn[0][2] = c3r1;	m_fColumn[0][3] = c4r1;
      m_fColumn[1][0] = c1r2;	m_fColumn[1][1] = c2r2;	m_fColumn[1][2] = c3r2;	m_fColumn[1][3] = c4r2;
      m_fColumn[2][0] = c1r3;	m_fColumn[2][1] = c2r3;	m_fColumn[2][2] = c3r3;	m_fColumn[2][3] = c4r3;
      m_fColumn[3][0] = c1r4;	m_fColumn[3][1] = c2r4;	m_fColumn[3][2] = c3r4;	m_fColumn[3][3] = c4r4;
    }
    else
    {
      m_fColumn[0][0] = c1r1;	m_fColumn[1][0] = c2r1;	m_fColumn[2][0] = c3r1;	m_fColumn[3][0] = c4r1;
      m_fColumn[0][1] = c1r2;	m_fColumn[1][1] = c2r2;	m_fColumn[2][1] = c3r2;	m_fColumn[3][1] = c4r2;
      m_fColumn[0][2] = c1r3;	m_fColumn[1][2] = c2r3;	m_fColumn[2][2] = c3r3;	m_fColumn[3][2] = c4r3;
      m_fColumn[0][3] = c1r4;	m_fColumn[1][3] = c2r4;	m_fColumn[2][3] = c3r4;	m_fColumn[3][3] = c4r4;
    }
  }

  inline void aeMatrix::SetZero (void)
  {
    SetMatrix (0, 0, 0, 0,
               0, 0, 0, 0,
               0, 0, 0, 0,
               0, 0, 0, 0);
  }

  inline void aeMatrix::SetIdentity (void)
  {
    SetMatrix (1, 0, 0, 0,
               0, 1, 0, 0,
               0, 0, 1, 0,
               0, 0, 0, 1);
  }

  inline void aeMatrix::SetTranslationMatrix (const aeVec3& vTranslation)
  {
    SetMatrix (1, 0, 0, vTranslation.x,
               0, 1, 0, vTranslation.y,
               0, 0, 1, vTranslation.z,
               0, 0, 0, 1);
  }

  inline void aeMatrix::SetScalingMatrix (float x, float y, float z)
  {
    SetMatrix (x, 0, 0, 0,
               0, y, 0, 0,
               0, 0, z, 0,
               0, 0, 0, 1);
  }

  inline void aeMatrix::SetRotationMatrixX (float fAngle)
  {
    const float fSin = aeMath::SinDeg (fAngle);
    const float fCos = aeMath::CosDeg (fAngle);

    SetMatrix (1.0f, 0.0f, 0.0f, 0.0f,
               0.0f, fCos,-fSin, 0.0f,
               0.0f, fSin, fCos, 0.0f,
               0.0f, 0.0f, 0.0f, 1.0f);
  }

  inline void aeMatrix::SetRotationMatrixY (float fAngle)
  {
    const float fSin = aeMath::SinDeg (fAngle);
    const float fCos = aeMath::CosDeg (fAngle);


    SetMatrix (fCos, 0.0f, fSin, 0.0f,
               0.0f, 1.0f, 0.0f, 0.0f,
              -fSin, 0.0f, fCos, 0.0f,
               0.0f, 0.0f, 0.0f, 1.0f);
  }

  inline void aeMatrix::SetRotationMatrixZ (float fAngle)
  {
    const float fSin = aeMath::SinDeg (fAngle);
    const float fCos = aeMath::CosDeg (fAngle);

    SetMatrix (fCos,-fSin, 0.0f, 0.0f,
               fSin, fCos, 0.0f, 0.0f,
               0.0f, 0.0f, 1.0f, 0.0f,
               0.0f, 0.0f, 0.0f, 1.0f);
  }

  inline void aeMatrix::Transpose (void)
  {
    aeMath::Swap (m_fColumn[0][1], m_fColumn[1][0]);
    aeMath::Swap (m_fColumn[0][2], m_fColumn[2][0]);
    aeMath::Swap (m_fColumn[0][3], m_fColumn[3][0]);
    aeMath::Swap (m_fColumn[1][2], m_fColumn[2][1]);
    aeMath::Swap (m_fColumn[1][3], m_fColumn[3][1]);
    aeMath::Swap (m_fColumn[2][3], m_fColumn[3][2]);
  }

  inline const aeMatrix aeMatrix::GetTranspose (void) const
  {
    return aeMatrix (m_fElements, RowMajor);
  }

  inline void aeMatrix::Invert (void)
  {
    *this = GetInverse (); 
  }

  inline void aeMatrix::GetRow (aeUInt32 iRow, float& v1, float& v2, float& v3, float& v4) const
  {
    AE_CHECK_DEV (iRow <= 3, "Invalid Row Index %d", iRow);

    v1 = m_fColumn[0][iRow];
    v2 = m_fColumn[1][iRow];
    v3 = m_fColumn[2][iRow];
    v4 = m_fColumn[3][iRow];
  }

  inline void aeMatrix::SetRow (aeUInt32 iRow, float v1, float v2, float v3, float v4)
  {
    AE_CHECK_DEV (iRow <= 3, "Invalid Row Index %d", iRow);

    m_fColumn[0][iRow] = v1;
    m_fColumn[1][iRow] = v2;
    m_fColumn[2][iRow] = v3;
    m_fColumn[3][iRow] = v4;
  }

  inline void aeMatrix::GetColumn (aeUInt32 iColumn, float& v1, float& v2, float& v3, float& v4) const
  {
    AE_CHECK_DEV (iColumn <= 3, "Invalid Column Index %d", iColumn);

    v1 = m_fColumn[iColumn][0];
    v2 = m_fColumn[iColumn][1];
    v3 = m_fColumn[iColumn][2];
    v4 = m_fColumn[iColumn][3];
  }

  inline void aeMatrix::SetColumn (aeUInt32 iColumn, float v1, float v2, float v3, float v4)
  {
    AE_CHECK_DEV (iColumn <= 3, "Invalid Column Index %d", iColumn);

    m_fColumn[iColumn][0] = v1;
    m_fColumn[iColumn][1] = v2;
    m_fColumn[iColumn][2] = v3;
    m_fColumn[iColumn][3] = v4;
  }

  inline void aeMatrix::GetAsOpenGL3x3Matrix (float* pDst) const
  {
    pDst[0] = m_fColumn[0][0];		pDst[1] = m_fColumn[0][1];		pDst[2] = m_fColumn[0][2];
    pDst[3] = m_fColumn[1][0];		pDst[4] = m_fColumn[1][1];		pDst[5] = m_fColumn[1][2];
    pDst[6] = m_fColumn[2][0];		pDst[7] = m_fColumn[2][1];		pDst[8] = m_fColumn[2][2];
  }

  inline void aeMatrix::GetAsOpenGL3x3Matrix (double* pDst) const
  {
    pDst[0] = m_fColumn[0][0];		pDst[1] = m_fColumn[0][1];		pDst[2] = m_fColumn[0][2];
    pDst[3] = m_fColumn[1][0];		pDst[4] = m_fColumn[1][1];		pDst[5] = m_fColumn[1][2];
    pDst[6] = m_fColumn[2][0];		pDst[7] = m_fColumn[2][1];		pDst[8] = m_fColumn[2][2];
  }

  inline void aeMatrix::GetAsOpenGL4x4Matrix (float* pDst) const
  {
    pDst[0] = m_fColumn[0][0];		pDst[1] = m_fColumn[0][1];		pDst[2] = m_fColumn[0][2];		pDst[3] = m_fColumn[0][3];
    pDst[4] = m_fColumn[1][0];		pDst[5] = m_fColumn[1][1];		pDst[6] = m_fColumn[1][2];		pDst[7] = m_fColumn[1][3];
    pDst[8] = m_fColumn[2][0];		pDst[9] = m_fColumn[2][1];		pDst[10]= m_fColumn[2][2];		pDst[11]= m_fColumn[2][3];
    pDst[12]= m_fColumn[3][0];		pDst[13]= m_fColumn[3][1];		pDst[14]= m_fColumn[3][2];		pDst[15]= m_fColumn[3][3];
  }

  inline void aeMatrix::GetAsOpenGL4x4Matrix (double* pDst) const
  {
    pDst[0] = m_fColumn[0][0];		pDst[1] = m_fColumn[0][1];		pDst[2] = m_fColumn[0][2];		pDst[3] = m_fColumn[0][3];
    pDst[4] = m_fColumn[1][0];		pDst[5] = m_fColumn[1][1];		pDst[6] = m_fColumn[1][2];		pDst[7] = m_fColumn[1][3];
    pDst[8] = m_fColumn[2][0];		pDst[9] = m_fColumn[2][1];		pDst[10]= m_fColumn[2][2];		pDst[11]= m_fColumn[2][3];
    pDst[12]= m_fColumn[3][0];		pDst[13]= m_fColumn[3][1];		pDst[14]= m_fColumn[3][2];		pDst[15]= m_fColumn[3][3];
  }

  inline const aeVec3 aeMatrix::TransformPosition (const aeVec3& v) const
  {
    aeVec3 r;
    r.x = m_fColumn[0][0] * v.x + m_fColumn[1][0] * v.y + m_fColumn[2][0] * v.z + m_fColumn[3][0];
    r.y = m_fColumn[0][1] * v.x + m_fColumn[1][1] * v.y + m_fColumn[2][1] * v.z + m_fColumn[3][1];
    r.z = m_fColumn[0][2] * v.x + m_fColumn[1][2] * v.y + m_fColumn[2][2] * v.z + m_fColumn[3][2];
    return (r);
  }

  inline const aeVec3 aeMatrix::TransformDirection (const aeVec3& v) const
  {
    aeVec3 r;
    r.x = m_fColumn[0][0] * v.x + m_fColumn[1][0] * v.y + m_fColumn[2][0] * v.z;
    r.y = m_fColumn[0][1] * v.x + m_fColumn[1][1] * v.y + m_fColumn[2][1] * v.z;
    r.z = m_fColumn[0][2] * v.x + m_fColumn[1][2] * v.y + m_fColumn[2][2] * v.z;
    return (r);
  }

  inline const aeVec3 aeMatrix::TransformWithWComponent (const aeVec3& v, float* inout_w) const
  {
    const float w = *inout_w;

    aeVec3 r;
    r.x     = m_fColumn[0][0] * v.x + m_fColumn[1][0] * v.y + m_fColumn[2][0] * v.z + m_fColumn[3][0] * w;
    r.y     = m_fColumn[0][1] * v.x + m_fColumn[1][1] * v.y + m_fColumn[2][1] * v.z + m_fColumn[3][1] * w;
    r.z     = m_fColumn[0][2] * v.x + m_fColumn[1][2] * v.y + m_fColumn[2][2] * v.z + m_fColumn[3][2] * w;
    *inout_w= m_fColumn[0][3] * v.x + m_fColumn[1][3] * v.y + m_fColumn[2][3] * v.z + m_fColumn[3][3] * w;

    return (r);
  }

  inline const aeVec3 aeMatrix::GetTranslationVector (void) const
  {
    return aeVec3 (m_fColumn[3][0], m_fColumn[3][1], m_fColumn[3][2]);
  }

  inline void aeMatrix::SetTranslationVector (const aeVec3& v)
  {
    m_fColumn[3][0] = v.x;
    m_fColumn[3][1] = v.y;
    m_fColumn[3][2] = v.z;
  }

  inline void aeMatrix::operator+= (const aeVec3& v)
  {
    m_fColumn[3][0] += v.x;
    m_fColumn[3][1] += v.y;
    m_fColumn[3][2] += v.z;
  }

  inline void aeMatrix::operator-= (const aeVec3& v)
  {
    m_fColumn[3][0] -= v.x;
    m_fColumn[3][1] -= v.y;
    m_fColumn[3][2] -= v.z;
  }

  inline void aeMatrix::operator*= (float f)
  {
    for (aeInt32 i = 0; i < 16; ++i)
      m_fElements[i] *= f;
  }

  inline void aeMatrix::operator/= (float f)
  {
    const float fInv = aeMath::Invert (f);

    operator*= (fInv);
  }

  inline const aeMatrix operator* (const aeMatrix& m1, const aeMatrix& m2)
  {
    aeMatrix r;
    for (aeInt32 i = 0; i < 4; ++i)
    {
      r.m_fColumn[0][i] = m1.m_fColumn[0][i] * m2.m_fColumn[0][0] + m1.m_fColumn[1][i] * m2.m_fColumn[0][1] + m1.m_fColumn[2][i] * m2.m_fColumn[0][2] + m1.m_fColumn[3][i] * m2.m_fColumn[0][3];
      r.m_fColumn[1][i] = m1.m_fColumn[0][i] * m2.m_fColumn[1][0] + m1.m_fColumn[1][i] * m2.m_fColumn[1][1] + m1.m_fColumn[2][i] * m2.m_fColumn[1][2] + m1.m_fColumn[3][i] * m2.m_fColumn[1][3];
      r.m_fColumn[2][i] = m1.m_fColumn[0][i] * m2.m_fColumn[2][0] + m1.m_fColumn[1][i] * m2.m_fColumn[2][1] + m1.m_fColumn[2][i] * m2.m_fColumn[2][2] + m1.m_fColumn[3][i] * m2.m_fColumn[2][3];
      r.m_fColumn[3][i] = m1.m_fColumn[0][i] * m2.m_fColumn[3][0] + m1.m_fColumn[1][i] * m2.m_fColumn[3][1] + m1.m_fColumn[2][i] * m2.m_fColumn[3][2] + m1.m_fColumn[3][i] * m2.m_fColumn[3][3];
    }
    return (r);
  }

  inline const aeVec3 operator* (const aeMatrix& m, const aeVec3& v)
  {
    return m.TransformPosition (v);
  }

  inline const aeMatrix operator+ (const aeMatrix& m, const aeVec3& v)
  {
    aeMatrix r = m;

    r.m_fColumn[3][0] += v.x;
    r.m_fColumn[3][1] += v.y;
    r.m_fColumn[3][2] += v.z;

    return (m);
  }

  inline const aeMatrix operator- (const aeMatrix& m, const aeVec3& v)
  {
    aeMatrix r = m;

    r.m_fColumn[3][0] -= v.x;
    r.m_fColumn[3][1] -= v.y;
    r.m_fColumn[3][2] -= v.z;

    return (m);
  }



  // *** Stuff needed for matrix inversion ***

  inline float GetDeterminantOf3x3SubMatrix (const aeMatrix& m, aeInt32 i, aeInt32 j)
  {
    const aeInt32 si0 = 0 + ((i <= 0) ? 1 : 0);
    const aeInt32 si1 = 1 + ((i <= 1) ? 1 : 0);
    const aeInt32 si2 = 2 + ((i <= 2) ? 1 : 0);

    const aeInt32 sj0 = 0 + ((j <= 0) ? 1 : 0);
    const aeInt32 sj1 = 1 + ((j <= 1) ? 1 : 0);
    const aeInt32 sj2 = 2 + ((j <= 2) ? 1 : 0);

    float fDet2 = ((m.m_fColumn[sj0][si0] * m.m_fColumn[sj1][si1] * m.m_fColumn[sj2][si2] +
                    m.m_fColumn[sj1][si0] * m.m_fColumn[sj2][si1] * m.m_fColumn[sj0][si2] +
                    m.m_fColumn[sj2][si0] * m.m_fColumn[sj0][si1] * m.m_fColumn[sj1][si2])-
                   (m.m_fColumn[sj0][si2] * m.m_fColumn[sj1][si1] * m.m_fColumn[sj2][si0] +
                    m.m_fColumn[sj1][si2] * m.m_fColumn[sj2][si1] * m.m_fColumn[sj0][si0] +
                    m.m_fColumn[sj2][si2] * m.m_fColumn[sj0][si1] * m.m_fColumn[sj1][si0]));

    return (fDet2);
  }

  inline float GetDeterminantOf4x4Matrix (const aeMatrix& m)
  {
    float det = 0.0;

    det +=  m.m_fColumn[0][0] * GetDeterminantOf3x3SubMatrix (m, 0, 0);
    det += -m.m_fColumn[1][0] * GetDeterminantOf3x3SubMatrix (m, 0, 1);
    det +=  m.m_fColumn[2][0] * GetDeterminantOf3x3SubMatrix (m, 0, 2);
    det += -m.m_fColumn[3][0] * GetDeterminantOf3x3SubMatrix (m, 0, 3);

    return (det);
  }


  // *** free functions ***

  inline const aeMatrix operator* (float f, const aeMatrix& m1)
  {
    return operator* (m1, f);
  }

  inline const aeMatrix operator* (const aeMatrix& m1, float f)
  {
    aeMatrix r;

    for (aeUInt32 i = 0; i < 16; ++i)
      r.m_fElements[i] = m1.m_fElements[i] * f;

    return (r);
  }

  inline const aeMatrix operator/ (const aeMatrix& m1, float f)
  {
    return operator* (m1, aeMath::Invert (f));
  }

  inline const aeMatrix operator+ (const aeMatrix& m1, const aeMatrix& m2)
  {
    aeMatrix r;

    for (aeUInt32 i = 0; i < 16; ++i)
      r.m_fElements[i] = m1.m_fElements[i] + m2.m_fElements[i];

    return (r);
  }

  inline const aeMatrix operator- (const aeMatrix& m1, const aeMatrix& m2)
  {
    aeMatrix r;

    for (aeUInt32 i = 0; i < 16; ++i)
      r.m_fElements[i] = m1.m_fElements[i] - m2.m_fElements[i];

    return (r);
  }

  inline bool aeMatrix::IsIdentical (const aeMatrix& rhs) const
  {
    for (aeUInt32 i = 0; i < 16; ++i)
    {
      if (m_fElements[i] != rhs.m_fElements[i])
        return false;
    }

    return true;
  }

  inline bool aeMatrix::IsEqual (const aeMatrix& rhs, float fEpsilon) const
  {
    AE_CHECK_DEV (fEpsilon >= 0.0f, "aeMatrix::IsEqual: Epsilon may not be negativ.");

    for (aeUInt32 i = 0; i < 16; ++i)
    {
      if (aeMath::IsFloatEqual (m_fElements[i], rhs.m_fElements[i], fEpsilon))
        return false;
    }

    return true;
  }

  inline bool operator== (const aeMatrix& lhs, const aeMatrix& rhs)
  {
    return lhs.IsIdentical (rhs);
  }

  inline bool operator!= (const aeMatrix& lhs, const aeMatrix& rhs)
  {
    return !lhs.IsIdentical (rhs);
  }

  inline bool aeMatrix::IsZeroMatrix (float fEpsilon) const
  {
    for (aeUInt32 i = 0; i < 16; ++i)
    {
      if (!aeMath::IsFloatEqual (m_fElements[i], 0.0f, fEpsilon))
        return false;
    }

    return true;
  }

  inline bool aeMatrix::IsIdentityMatrix (float fEpsilon) const
  {
    if (!aeMath::IsFloatEqual (m_fColumn[0][0], 1.0f, fEpsilon)) return false;
    if (!aeMath::IsFloatEqual (m_fColumn[0][1], 0.0f, fEpsilon)) return false;
    if (!aeMath::IsFloatEqual (m_fColumn[0][2], 0.0f, fEpsilon)) return false;
    if (!aeMath::IsFloatEqual (m_fColumn[0][3], 0.0f, fEpsilon)) return false;

    if (!aeMath::IsFloatEqual (m_fColumn[1][0], 0.0f, fEpsilon)) return false;
    if (!aeMath::IsFloatEqual (m_fColumn[1][1], 1.0f, fEpsilon)) return false;
    if (!aeMath::IsFloatEqual (m_fColumn[1][2], 0.0f, fEpsilon)) return false;
    if (!aeMath::IsFloatEqual (m_fColumn[1][3], 0.0f, fEpsilon)) return false;

    if (!aeMath::IsFloatEqual (m_fColumn[2][0], 0.0f, fEpsilon)) return false;
    if (!aeMath::IsFloatEqual (m_fColumn[2][1], 0.0f, fEpsilon)) return false;
    if (!aeMath::IsFloatEqual (m_fColumn[2][2], 1.0f, fEpsilon)) return false;
    if (!aeMath::IsFloatEqual (m_fColumn[2][3], 0.0f, fEpsilon)) return false;

    if (!aeMath::IsFloatEqual (m_fColumn[3][0], 0.0f, fEpsilon)) return false;
    if (!aeMath::IsFloatEqual (m_fColumn[3][1], 0.0f, fEpsilon)) return false;
    if (!aeMath::IsFloatEqual (m_fColumn[3][2], 0.0f, fEpsilon)) return false;
    if (!aeMath::IsFloatEqual (m_fColumn[3][3], 1.0f, fEpsilon)) return false;

    return true;
  }

  inline bool aeMatrix::IsValid () const
  {
    for (aeUInt32 i = 0; i < 16; ++i)
    {
      if (!aeMath::IsFinite (m_fElements[i]))
        return false;
    }

    return true;

  }
}

#endif



