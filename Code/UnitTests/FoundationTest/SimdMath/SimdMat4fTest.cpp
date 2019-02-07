#include <FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdTransform.h>

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdMat4f)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (Array Data)")
  {
    const float data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      ezSimdMat4f m(data, ezMatrixLayout::ColumnMajor);

      EZ_TEST_BOOL((m.m_col0 == ezSimdVec4f(1, 2, 3, 4)).AllSet());
      EZ_TEST_BOOL((m.m_col1 == ezSimdVec4f(5, 6, 7, 8)).AllSet());
      EZ_TEST_BOOL((m.m_col2 == ezSimdVec4f(9, 10, 11, 12)).AllSet());
      EZ_TEST_BOOL((m.m_col3 == ezSimdVec4f(13, 14, 15, 16)).AllSet());
    }

    {
      ezSimdMat4f m(data, ezMatrixLayout::RowMajor);

      EZ_TEST_BOOL((m.m_col0 == ezSimdVec4f(1, 5, 9, 13)).AllSet());
      EZ_TEST_BOOL((m.m_col1 == ezSimdVec4f(2, 6, 10, 14)).AllSet());
      EZ_TEST_BOOL((m.m_col2 == ezSimdVec4f(3, 7, 11, 15)).AllSet());
      EZ_TEST_BOOL((m.m_col3 == ezSimdVec4f(4, 8, 12, 16)).AllSet());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (Columns)")
  {
    ezSimdVec4f c0(1, 2, 3, 4);
    ezSimdVec4f c1(5, 6, 7, 8);
    ezSimdVec4f c2(9, 10, 11, 12);
    ezSimdVec4f c3(13, 14, 15, 16);

    ezSimdMat4f m(c0, c1, c2, c3);

    EZ_TEST_BOOL((m.m_col0 == ezSimdVec4f(1, 2, 3, 4)).AllSet());
    EZ_TEST_BOOL((m.m_col1 == ezSimdVec4f(5, 6, 7, 8)).AllSet());
    EZ_TEST_BOOL((m.m_col2 == ezSimdVec4f(9, 10, 11, 12)).AllSet());
    EZ_TEST_BOOL((m.m_col3 == ezSimdVec4f(13, 14, 15, 16)).AllSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromArray")
  {
    const float data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      ezSimdMat4f m;
      m.SetFromArray(data, ezMatrixLayout::ColumnMajor);

      EZ_TEST_BOOL((m.m_col0 == ezSimdVec4f(1, 2, 3, 4)).AllSet());
      EZ_TEST_BOOL((m.m_col1 == ezSimdVec4f(5, 6, 7, 8)).AllSet());
      EZ_TEST_BOOL((m.m_col2 == ezSimdVec4f(9, 10, 11, 12)).AllSet());
      EZ_TEST_BOOL((m.m_col3 == ezSimdVec4f(13, 14, 15, 16)).AllSet());
    }

    {
      ezSimdMat4f m;
      m.SetFromArray(data, ezMatrixLayout::RowMajor);

      EZ_TEST_BOOL((m.m_col0 == ezSimdVec4f(1, 5, 9, 13)).AllSet());
      EZ_TEST_BOOL((m.m_col1 == ezSimdVec4f(2, 6, 10, 14)).AllSet());
      EZ_TEST_BOOL((m.m_col2 == ezSimdVec4f(3, 7, 11, 15)).AllSet());
      EZ_TEST_BOOL((m.m_col3 == ezSimdVec4f(4, 8, 12, 16)).AllSet());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsArray")
  {
    ezSimdMat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    float data[16];

    m.GetAsArray(data, ezMatrixLayout::ColumnMajor);
    EZ_TEST_FLOAT(data[0], 1, 0.0001f);
    EZ_TEST_FLOAT(data[1], 5, 0.0001f);
    EZ_TEST_FLOAT(data[2], 9, 0.0001f);
    EZ_TEST_FLOAT(data[3], 13, 0.0001f);
    EZ_TEST_FLOAT(data[4], 2, 0.0001f);
    EZ_TEST_FLOAT(data[5], 6, 0.0001f);
    EZ_TEST_FLOAT(data[6], 10, 0.0001f);
    EZ_TEST_FLOAT(data[7], 14, 0.0001f);
    EZ_TEST_FLOAT(data[8], 3, 0.0001f);
    EZ_TEST_FLOAT(data[9], 7, 0.0001f);
    EZ_TEST_FLOAT(data[10], 11, 0.0001f);
    EZ_TEST_FLOAT(data[11], 15, 0.0001f);
    EZ_TEST_FLOAT(data[12], 4, 0.0001f);
    EZ_TEST_FLOAT(data[13], 8, 0.0001f);
    EZ_TEST_FLOAT(data[14], 12, 0.0001f);
    EZ_TEST_FLOAT(data[15], 16, 0.0001f);

    m.GetAsArray(data, ezMatrixLayout::RowMajor);
    EZ_TEST_FLOAT(data[0], 1, 0.0001f);
    EZ_TEST_FLOAT(data[1], 2, 0.0001f);
    EZ_TEST_FLOAT(data[2], 3, 0.0001f);
    EZ_TEST_FLOAT(data[3], 4, 0.0001f);
    EZ_TEST_FLOAT(data[4], 5, 0.0001f);
    EZ_TEST_FLOAT(data[5], 6, 0.0001f);
    EZ_TEST_FLOAT(data[6], 7, 0.0001f);
    EZ_TEST_FLOAT(data[7], 8, 0.0001f);
    EZ_TEST_FLOAT(data[8], 9, 0.0001f);
    EZ_TEST_FLOAT(data[9], 10, 0.0001f);
    EZ_TEST_FLOAT(data[10], 11, 0.0001f);
    EZ_TEST_FLOAT(data[11], 12, 0.0001f);
    EZ_TEST_FLOAT(data[12], 13, 0.0001f);
    EZ_TEST_FLOAT(data[13], 14, 0.0001f);
    EZ_TEST_FLOAT(data[14], 15, 0.0001f);
    EZ_TEST_FLOAT(data[15], 16, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetIdentity")
  {
    ezSimdMat4f m;
    m.SetIdentity();

    EZ_TEST_BOOL((m.m_col0 == ezSimdVec4f(1, 0, 0, 0)).AllSet());
    EZ_TEST_BOOL((m.m_col1 == ezSimdVec4f(0, 1, 0, 0)).AllSet());
    EZ_TEST_BOOL((m.m_col2 == ezSimdVec4f(0, 0, 1, 0)).AllSet());
    EZ_TEST_BOOL((m.m_col3 == ezSimdVec4f(0, 0, 0, 1)).AllSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IdentityMatrix")
  {
    ezSimdMat4f m = ezSimdMat4f::IdentityMatrix();

    EZ_TEST_BOOL((m.m_col0 == ezSimdVec4f(1, 0, 0, 0)).AllSet());
    EZ_TEST_BOOL((m.m_col1 == ezSimdVec4f(0, 1, 0, 0)).AllSet());
    EZ_TEST_BOOL((m.m_col2 == ezSimdVec4f(0, 0, 1, 0)).AllSet());
    EZ_TEST_BOOL((m.m_col3 == ezSimdVec4f(0, 0, 0, 1)).AllSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transpose")
  {
    ezSimdMat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m.Transpose();

    EZ_TEST_BOOL((m.m_col0 == ezSimdVec4f(1, 2, 3, 4)).AllSet());
    EZ_TEST_BOOL((m.m_col1 == ezSimdVec4f(5, 6, 7, 8)).AllSet());
    EZ_TEST_BOOL((m.m_col2 == ezSimdVec4f(9, 10, 11, 12)).AllSet());
    EZ_TEST_BOOL((m.m_col3 == ezSimdVec4f(13, 14, 15, 16)).AllSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetTranspose")
  {
    ezSimdMat4f m0(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezSimdMat4f m = m0.GetTranspose();

    EZ_TEST_BOOL((m.m_col0 == ezSimdVec4f(1, 2, 3, 4)).AllSet());
    EZ_TEST_BOOL((m.m_col1 == ezSimdVec4f(5, 6, 7, 8)).AllSet());
    EZ_TEST_BOOL((m.m_col2 == ezSimdVec4f(9, 10, 11, 12)).AllSet());
    EZ_TEST_BOOL((m.m_col3 == ezSimdVec4f(13, 14, 15, 16)).AllSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Invert")
  {
    for (float x = 1.0f; x < 360.0f; x += 20.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 27.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 33.0f)
        {
          ezSimdQuat q;
          q.SetFromAxisAndAngle(ezSimdVec4f(x, y, z).GetNormalized<3>(), ezAngle::Degree(19.0f));

          ezSimdTransform t(q);

          ezSimdMat4f m, inv;
          m = t.GetAsMat4();
          inv = m;
          EZ_TEST_BOOL(inv.Invert() == EZ_SUCCESS);

          ezSimdVec4f v = m.TransformDirection(ezSimdVec4f(1, 3, -10));
          ezSimdVec4f vinv = inv.TransformDirection(v);

          EZ_TEST_BOOL(vinv.IsEqual(ezSimdVec4f(1, 3, -10), ezMath::BasicType<float>::DefaultEpsilon()).AllSet<3>());
        }
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetInverse")
  {
    for (float x = 1.0f; x < 360.0f; x += 19.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 29.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 31.0f)
        {
          ezSimdQuat q;
          q.SetFromAxisAndAngle(ezSimdVec4f(x, y, z).GetNormalized<3>(), ezAngle::Degree(83.0f));

          ezSimdTransform t(q);

          ezSimdMat4f m, inv;
          m = t.GetAsMat4();
          inv = m.GetInverse();

          ezSimdVec4f v = m.TransformDirection(ezSimdVec4f(1, 3, -10));
          ezSimdVec4f vinv = inv.TransformDirection(v);

          EZ_TEST_BOOL(vinv.IsEqual(ezSimdVec4f(1, 3, -10), ezMath::BasicType<float>::DefaultEpsilon()).AllSet<3>());
        }
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    ezSimdMat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezSimdMat4f m2 = m;

    EZ_TEST_BOOL(m.IsEqual(m2, 0.0001f));

    m2.m_col0 += ezSimdVec4f(0.00001f);
    EZ_TEST_BOOL(m.IsEqual(m2, 0.0001f));
    EZ_TEST_BOOL(!m.IsEqual(m2, 0.000001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentity")
  {
    ezSimdMat4f m;

    m.SetIdentity();
    EZ_TEST_BOOL(m.IsIdentity());

    m.m_col0.SetZero();
    EZ_TEST_BOOL(!m.IsIdentity());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsValid")
  {
    ezSimdMat4f m;

    m.SetIdentity();
    EZ_TEST_BOOL(m.IsValid());

    m.m_col0.SetX(ezMath::BasicType<float>::GetNaN());
    EZ_TEST_BOOL(!m.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    ezSimdMat4f m;

    m.SetIdentity();
    EZ_TEST_BOOL(!m.IsNaN());

    float data[16];

    for (ezUInt32 i = 0; i < 16; ++i)
    {
      m.SetIdentity();
      m.GetAsArray(data, ezMatrixLayout::ColumnMajor);
      data[i] = ezMath::BasicType<float>::GetNaN();
      m.SetFromArray(data, ezMatrixLayout::ColumnMajor);

      EZ_TEST_BOOL(m.IsNaN());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRows")
  {
    ezSimdVec4f r0(1, 2, 3, 4);
    ezSimdVec4f r1(5, 6, 7, 8);
    ezSimdVec4f r2(9, 10, 11, 12);
    ezSimdVec4f r3(13, 14, 15, 16);

    ezSimdMat4f m;
    m.SetRows(r0, r1, r2, r3);

    EZ_TEST_BOOL((m.m_col0 == ezSimdVec4f(1, 5, 9, 13)).AllSet());
    EZ_TEST_BOOL((m.m_col1 == ezSimdVec4f(2, 6, 10, 14)).AllSet());
    EZ_TEST_BOOL((m.m_col2 == ezSimdVec4f(3, 7, 11, 15)).AllSet());
    EZ_TEST_BOOL((m.m_col3 == ezSimdVec4f(4, 8, 12, 16)).AllSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRows")
  {
    ezSimdMat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezSimdVec4f r0, r1, r2, r3;
    m.GetRows(r0, r1, r2, r3);

    EZ_TEST_BOOL((r0 == ezSimdVec4f(1, 2, 3, 4)).AllSet());
    EZ_TEST_BOOL((r1 == ezSimdVec4f(5, 6, 7, 8)).AllSet());
    EZ_TEST_BOOL((r2 == ezSimdVec4f(9, 10, 11, 12)).AllSet());
    EZ_TEST_BOOL((r3 == ezSimdVec4f(13, 14, 15, 16)).AllSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformPosition")
  {
    ezSimdMat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const ezSimdVec4f r = m.TransformPosition(ezSimdVec4f(1, 2, 3));

    EZ_TEST_BOOL(
        r.IsEqual(ezSimdVec4f(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001f).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformDirection")
  {
    ezSimdMat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const ezSimdVec4f r = m.TransformDirection(ezSimdVec4f(1, 2, 3));

    EZ_TEST_BOOL(r.IsEqual(ezSimdVec4f(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), 0.0001f).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(mat, mat)")
  {
    ezSimdMat4f m1(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezSimdMat4f m2(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16);

    ezSimdMat4f r = m1 * m2;

    EZ_TEST_BOOL((r.m_col0 == ezSimdVec4f(-1 * 1 + -5 * 2 + -9 * 3 + -13 * 4, -1 * 5 + -5 * 6 + -9 * 7 + -13 * 8,
                                          -1 * 9 + -5 * 10 + -9 * 11 + -13 * 12, -1 * 13 + -5 * 14 + -9 * 15 + -13 * 16))
                     .AllSet());
    EZ_TEST_BOOL((r.m_col1 == ezSimdVec4f(-2 * 1 + -6 * 2 + -10 * 3 + -14 * 4, -2 * 5 + -6 * 6 + -10 * 7 + -14 * 8,
                                          -2 * 9 + -6 * 10 + -10 * 11 + -14 * 12, -2 * 13 + -6 * 14 + -10 * 15 + -14 * 16))
                     .AllSet());
    EZ_TEST_BOOL((r.m_col2 == ezSimdVec4f(-3 * 1 + -7 * 2 + -11 * 3 + -15 * 4, -3 * 5 + -7 * 6 + -11 * 7 + -15 * 8,
                                          -3 * 9 + -7 * 10 + -11 * 11 + -15 * 12, -3 * 13 + -7 * 14 + -11 * 15 + -15 * 16))
                     .AllSet());
    EZ_TEST_BOOL((r.m_col3 == ezSimdVec4f(-4 * 1 + -8 * 2 + -12 * 3 + -16 * 4, -4 * 5 + -8 * 6 + -12 * 7 + -16 * 8,
                                          -4 * 9 + -8 * 10 + -12 * 11 + -16 * 12, -4 * 13 + -8 * 14 + -12 * 15 + -16 * 16))
                     .AllSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator== (mat, mat) | operator!= (mat, mat)")
  {
    ezSimdMat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezSimdMat4f m2 = m;

    EZ_TEST_BOOL(m == m2);

    m2.m_col0 += ezSimdVec4f(0.00001f);

    EZ_TEST_BOOL(m != m2);
  }
}
