#include <PCH.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Implementation/AllClasses_inl.h>

EZ_CREATE_SIMPLE_TEST(Math, Mat3)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Default Constructor")
  {
    #if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      if (ezMath::BasicType<ezMathTestType>::SupportsNaN())
      {
        // In debug the default constructor initializes everything with NaN.
        ezMat3T m;
        EZ_TEST_BOOL(ezMath::IsNaN(m.m_fElementsCM[0]) && 
                ezMath::IsNaN(m.m_fElementsCM[1]) && 
                ezMath::IsNaN(m.m_fElementsCM[2]) && 
                ezMath::IsNaN(m.m_fElementsCM[3]) && 
                ezMath::IsNaN(m.m_fElementsCM[4]) && 
                ezMath::IsNaN(m.m_fElementsCM[5]) && 
                ezMath::IsNaN(m.m_fElementsCM[6]) && 
                ezMath::IsNaN(m.m_fElementsCM[7]) && 
                ezMath::IsNaN(m.m_fElementsCM[8]));
      }
    #else
        // Placement new of the default constructor should not have any effect on the previous data.
        ezMat3T::ComponentType testBlock[9] = { (ezMat3T::ComponentType) 1, (ezMat3T::ComponentType) 2, (ezMat3T::ComponentType) 3,
                                                (ezMat3T::ComponentType) 4, (ezMat3T::ComponentType) 5, (ezMat3T::ComponentType) 6,
                                                (ezMat3T::ComponentType) 7, (ezMat3T::ComponentType) 8, (ezMat3T::ComponentType) 9 };

        ezMat3T* m = ::new ((void*) &testBlock[0]) ezMat3T;

        EZ_TEST_BOOL(m->m_fElementsCM[0] == (ezMat3T::ComponentType) 1 && 
                m->m_fElementsCM[1] == (ezMat3T::ComponentType) 2 && 
                m->m_fElementsCM[2] == (ezMat3T::ComponentType) 3 && 
                m->m_fElementsCM[3] == (ezMat3T::ComponentType) 4 && 
                m->m_fElementsCM[4] == (ezMat3T::ComponentType) 5 && 
                m->m_fElementsCM[5] == (ezMat3T::ComponentType) 6 && 
                m->m_fElementsCM[6] == (ezMat3T::ComponentType) 7 && 
                m->m_fElementsCM[7] == (ezMat3T::ComponentType) 8 && 
                m->m_fElementsCM[8] == (ezMat3T::ComponentType) 9);
    #endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (Array Data)")
  {
    const ezMathTestType data[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    {
      ezMat3T m(data, ezMatrixLayout::ColumnMajor);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && 
              m.m_fElementsCM[1] == 2.0f && 
              m.m_fElementsCM[2] == 3.0f && 
              m.m_fElementsCM[3] == 4.0f && 
              m.m_fElementsCM[4] == 5.0f && 
              m.m_fElementsCM[5] == 6.0f && 
              m.m_fElementsCM[6] == 7.0f && 
              m.m_fElementsCM[7] == 8.0f && 
              m.m_fElementsCM[8] == 9.0f);
    }

    {
      ezMat3T m(data, ezMatrixLayout::RowMajor);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && 
              m.m_fElementsCM[1] == 4.0f && 
              m.m_fElementsCM[2] == 7.0f && 
              m.m_fElementsCM[3] == 2.0f && 
              m.m_fElementsCM[4] == 5.0f && 
              m.m_fElementsCM[5] == 8.0f && 
              m.m_fElementsCM[6] == 3.0f && 
              m.m_fElementsCM[7] == 6.0f && 
              m.m_fElementsCM[8] == 9.0f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (Elements)")
  {
    ezMat3T m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 0), 2, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 0), 3, 0.00001f);
    EZ_TEST_FLOAT(m.Element(0, 1), 4, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 1), 5, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 1), 6, 0.00001f);
    EZ_TEST_FLOAT(m.Element(0, 2), 7, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 2), 8, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 2), 9, 0.00001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromArray")
  {
    const ezMathTestType data[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    {
      ezMat3T m;
      m.SetFromArray(data, ezMatrixLayout::ColumnMajor);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && 
              m.m_fElementsCM[1] == 2.0f && 
              m.m_fElementsCM[2] == 3.0f && 
              m.m_fElementsCM[3] == 4.0f && 
              m.m_fElementsCM[4] == 5.0f && 
              m.m_fElementsCM[5] == 6.0f && 
              m.m_fElementsCM[6] == 7.0f && 
              m.m_fElementsCM[7] == 8.0f && 
              m.m_fElementsCM[8] == 9.0f);
    }

    {
      ezMat3T m;
      m.SetFromArray(data, ezMatrixLayout::RowMajor);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && 
              m.m_fElementsCM[1] == 4.0f && 
              m.m_fElementsCM[2] == 7.0f && 
              m.m_fElementsCM[3] == 2.0f && 
              m.m_fElementsCM[4] == 5.0f && 
              m.m_fElementsCM[5] == 8.0f && 
              m.m_fElementsCM[6] == 3.0f && 
              m.m_fElementsCM[7] == 6.0f && 
              m.m_fElementsCM[8] == 9.0f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetElements")
  {
    ezMat3T m;
    m.SetElements (1, 2, 3,
                   4, 5, 6,
                   7, 8, 9);

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 0), 2, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 0), 3, 0.00001f);
    EZ_TEST_FLOAT(m.Element(0, 1), 4, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 1), 5, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 1), 6, 0.00001f);
    EZ_TEST_FLOAT(m.Element(0, 2), 7, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 2), 8, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 2), 9, 0.00001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsArray")
  {
    ezMat3T m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    ezMathTestType data[9];

    m.GetAsArray(data, ezMatrixLayout::ColumnMajor);
    EZ_TEST_FLOAT(data[0], 1, 0.0001f);
    EZ_TEST_FLOAT(data[1], 4, 0.0001f);
    EZ_TEST_FLOAT(data[2], 7, 0.0001f);
    EZ_TEST_FLOAT(data[3], 2, 0.0001f);
    EZ_TEST_FLOAT(data[4], 5, 0.0001f);
    EZ_TEST_FLOAT(data[5], 8, 0.0001f);
    EZ_TEST_FLOAT(data[6], 3, 0.0001f);
    EZ_TEST_FLOAT(data[7], 6, 0.0001f);
    EZ_TEST_FLOAT(data[8], 9, 0.0001f);

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
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetZero")
  {
    ezMat3T m;
    m.SetZero();

    for (ezUInt32 i = 0; i < 9; ++i)
      EZ_TEST_FLOAT(m.m_fElementsCM[i], 0.0f, 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetIdentity")
  {
    ezMat3T m;
    m.SetIdentity();

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0); EZ_TEST_FLOAT(m.Element(1, 0), 0, 0); EZ_TEST_FLOAT(m.Element(2, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0); EZ_TEST_FLOAT(m.Element(1, 1), 1, 0); EZ_TEST_FLOAT(m.Element(2, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0); EZ_TEST_FLOAT(m.Element(1, 2), 0, 0); EZ_TEST_FLOAT(m.Element(2, 2), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetScalingMatrix")
  {
    ezMat3T m;
    m.SetScalingMatrix(ezVec3T(2, 3, 4));

    EZ_TEST_FLOAT(m.Element(0, 0), 2, 0); EZ_TEST_FLOAT(m.Element(1, 0), 0, 0); EZ_TEST_FLOAT(m.Element(2, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0); EZ_TEST_FLOAT(m.Element(1, 1), 3, 0); EZ_TEST_FLOAT(m.Element(2, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0); EZ_TEST_FLOAT(m.Element(1, 2), 0, 0); EZ_TEST_FLOAT(m.Element(2, 2), 4, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRotationMatrixX")
  {
    ezMat3T m;

    m.SetRotationMatrixX(ezAngle::Degree(90));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(1, -3, 2), 0.0001f));

    m.SetRotationMatrixX(ezAngle::Degree(180));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(1, -2, -3), 0.0001f));

    m.SetRotationMatrixX(ezAngle::Degree(270));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(1, 3, -2), 0.0001f));

    m.SetRotationMatrixX(ezAngle::Degree(360));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(1, 2, 3), 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRotationMatrixY")
  {
    ezMat3T m;

    m.SetRotationMatrixY(ezAngle::Degree(90));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(3, 2, -1), 0.0001f));

    m.SetRotationMatrixY(ezAngle::Degree(180));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(-1, 2, -3), 0.0001f));

    m.SetRotationMatrixY(ezAngle::Degree(270));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(-3, 2, 1), 0.0001f));

    m.SetRotationMatrixY(ezAngle::Degree(360));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(1, 2, 3), 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRotationMatrixZ")
  {
    ezMat3T m;

    m.SetRotationMatrixZ(ezAngle::Degree(90));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(-2, 1, 3), 0.0001f));

    m.SetRotationMatrixZ(ezAngle::Degree(180));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(-1, -2, 3), 0.0001f));

    m.SetRotationMatrixZ(ezAngle::Degree(270));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(2, -1, 3), 0.0001f));

    m.SetRotationMatrixZ(ezAngle::Degree(360));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(1, 2, 3), 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRotationMatrix")
  {
    ezMat3T m;

    m.SetRotationMatrix(ezVec3T(1, 0, 0), ezAngle::Degree(90));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(1, -3, 2), ezMath::BasicType<ezMathTestType>::LargeEpsilon()));

    m.SetRotationMatrix(ezVec3T(1, 0, 0), ezAngle::Degree(180));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(1, -2, -3), ezMath::BasicType<ezMathTestType>::LargeEpsilon()));

    m.SetRotationMatrix(ezVec3T(1, 0, 0), ezAngle::Degree(270));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(1, 3, -2), ezMath::BasicType<ezMathTestType>::LargeEpsilon()));

    m.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(90));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(3, 2, -1), ezMath::BasicType<ezMathTestType>::LargeEpsilon()));

    m.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(180));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(-1, 2, -3), ezMath::BasicType<ezMathTestType>::LargeEpsilon()));

    m.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(270));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(-3, 2, 1), ezMath::BasicType<ezMathTestType>::LargeEpsilon()));

    m.SetRotationMatrix(ezVec3T(0, 0, 1), ezAngle::Degree(90));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(-2, 1, 3), ezMath::BasicType<ezMathTestType>::LargeEpsilon()));

    m.SetRotationMatrix(ezVec3T(0, 0, 1), ezAngle::Degree(180));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(-1, -2, 3), ezMath::BasicType<ezMathTestType>::LargeEpsilon()));

    m.SetRotationMatrix(ezVec3T(0, 0, 1), ezAngle::Degree(270));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(2, -1, 3), ezMath::BasicType<ezMathTestType>::LargeEpsilon()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IdentityMatrix")
  {
    ezMat3T m = ezMat3T::IdentityMatrix();

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0); EZ_TEST_FLOAT(m.Element(1, 0), 0, 0); EZ_TEST_FLOAT(m.Element(2, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0); EZ_TEST_FLOAT(m.Element(1, 1), 1, 0); EZ_TEST_FLOAT(m.Element(2, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0); EZ_TEST_FLOAT(m.Element(1, 2), 0, 0); EZ_TEST_FLOAT(m.Element(2, 2), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ZeroMatrix")
  {
    ezMat3T m = ezMat3T::ZeroMatrix();

    EZ_TEST_FLOAT(m.Element(0, 0), 0, 0); EZ_TEST_FLOAT(m.Element(1, 0), 0, 0); EZ_TEST_FLOAT(m.Element(2, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0); EZ_TEST_FLOAT(m.Element(1, 1), 0, 0); EZ_TEST_FLOAT(m.Element(2, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0); EZ_TEST_FLOAT(m.Element(1, 2), 0, 0); EZ_TEST_FLOAT(m.Element(2, 2), 0, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transpose")
  {
    ezMat3T m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    m.Transpose();

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 0), 4, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 0), 7, 0.00001f);
    EZ_TEST_FLOAT(m.Element(0, 1), 2, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 1), 5, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 1), 8, 0.00001f);
    EZ_TEST_FLOAT(m.Element(0, 2), 3, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 2), 6, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 2), 9, 0.00001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetTranspose")
  {
    ezMat3T m0(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    ezMat3T m = m0.GetTranspose();

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 0), 4, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 0), 7, 0.00001f);
    EZ_TEST_FLOAT(m.Element(0, 1), 2, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 1), 5, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 1), 8, 0.00001f);
    EZ_TEST_FLOAT(m.Element(0, 2), 3, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 2), 6, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 2), 9, 0.00001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Invert")
  {
    for (float x = 1.0f; x < 360.0f; x += 10.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 17.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 23.0f)
        {
          ezMat3T m, inv;
          m.SetRotationMatrix(ezVec3T(x, y, z).GetNormalized(), ezAngle::Degree(19.0f));
          inv = m;
          EZ_TEST_BOOL(inv.Invert() == EZ_SUCCESS);

          ezVec3T v = m * ezVec3T(1, 1, 1);
          ezVec3T vinv = inv * v;

          EZ_TEST_VEC3(vinv, ezVec3T(1, 1, 1), ezMath::BasicType<ezMathTestType>::DefaultEpsilon());
        }
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetInverse")
  {
    for (float x = 1.0f; x < 360.0f; x += 9.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 19.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 21.0f)
        {
          ezMat3T m, inv;
          m.SetRotationMatrix(ezVec3T(x, y, z).GetNormalized(), ezAngle::Degree(83.0f));
          inv = m.GetInverse();

          ezVec3T v = m * ezVec3T(1, 1, 1);
          ezVec3T vinv = inv * v;

          EZ_TEST_VEC3(vinv, ezVec3T(1, 1, 1), ezMath::BasicType<ezMathTestType>::DefaultEpsilon());
        }
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsZero")
  {
    ezMat3T m;

    m.SetIdentity();
    EZ_TEST_BOOL(!m.IsZero());

    m.SetZero();
    EZ_TEST_BOOL(m.IsZero());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentity")
  {
    ezMat3T m;

    m.SetIdentity();
    EZ_TEST_BOOL(m.IsIdentity());

    m.SetZero();
    EZ_TEST_BOOL(!m.IsIdentity());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsValid")
  {
    if (ezMath::BasicType<ezMat3T::ComponentType>::SupportsNaN())
    {
      ezMat3T m;

      m.SetZero();
      EZ_TEST_BOOL(m.IsValid());

      m.m_fElementsCM[0] = ezMath::BasicType<ezMat3T::ComponentType>::GetNaN();
      EZ_TEST_BOOL(!m.IsValid());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRow")
  {
    ezMat3T m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    EZ_TEST_VEC3(m.GetRow(0), ezVec3T(1, 2, 3), 0.0f);
    EZ_TEST_VEC3(m.GetRow(1), ezVec3T(4, 5, 6), 0.0f);
    EZ_TEST_VEC3(m.GetRow(2), ezVec3T(7, 8, 9), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRow")
  {
    ezMat3T m;
    m.SetZero();

    m.SetRow(0, ezVec3T(1, 2, 3));
    EZ_TEST_VEC3(m.GetRow(0), ezVec3T(1, 2, 3), 0.0f);

    m.SetRow(1, ezVec3T(4, 5, 6));
    EZ_TEST_VEC3(m.GetRow(1), ezVec3T(4, 5, 6), 0.0f);

    m.SetRow(2, ezVec3T(7, 8, 9));
    EZ_TEST_VEC3(m.GetRow(2), ezVec3T(7, 8, 9), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetColumn")
  {
    ezMat3T m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    EZ_TEST_VEC3(m.GetColumn(0), ezVec3T(1, 4, 7), 0.0f);
    EZ_TEST_VEC3(m.GetColumn(1), ezVec3T(2, 5, 8), 0.0f);
    EZ_TEST_VEC3(m.GetColumn(2), ezVec3T(3, 6, 9), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetColumn")
  {
    ezMat3T m;
    m.SetZero();

    m.SetColumn(0, ezVec3T(1, 2, 3));
    EZ_TEST_VEC3(m.GetColumn(0), ezVec3T(1, 2, 3), 0.0f);

    m.SetColumn(1, ezVec3T(4, 5, 6));
    EZ_TEST_VEC3(m.GetColumn(1), ezVec3T(4, 5, 6), 0.0f);

    m.SetColumn(2, ezVec3T(7, 8, 9));
    EZ_TEST_VEC3(m.GetColumn(2), ezVec3T(7, 8, 9), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDiagonal")
  {
    ezMat3T m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    EZ_TEST_VEC3(m.GetDiagonal(), ezVec3T(1, 5, 9), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetDiagonal")
  {
    ezMat3T m;
    m.SetZero();

    m.SetDiagonal(ezVec3T(1, 2, 3));
    EZ_TEST_VEC3(m.GetColumn(0), ezVec3T(1, 0, 0), 0.0f);
    EZ_TEST_VEC3(m.GetColumn(1), ezVec3T(0, 2, 0), 0.0f);
    EZ_TEST_VEC3(m.GetColumn(2), ezVec3T(0, 0, 3), 0.0f);
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetScalingFactors")
  {
    ezMat3T m(1, 2, 3,
             5, 6, 7,
             9,10,11);

    ezVec3T s = m.GetScalingFactors();
    EZ_TEST_VEC3(s, ezVec3T(ezMath::Sqrt((ezMathTestType) (1*1 + 5*5 + 9*9)), ezMath::Sqrt((ezMathTestType) (2*2 + 6*6 + 10*10)), ezMath::Sqrt((ezMathTestType) (3*3 + 7*7 + 11*11))), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetScalingFactors")
  {
    ezMat3T m(1, 2, 3,
             5, 6, 7,
             9,10,11);

    EZ_TEST_BOOL(m.SetScalingFactors(ezVec3T(1, 2, 3)) == EZ_SUCCESS);

    ezVec3T s = m.GetScalingFactors();
    EZ_TEST_VEC3(s, ezVec3T(1, 2, 3), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformDirection")
  {
    ezMat3T m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    const ezVec3T r = m.TransformDirection(ezVec3T(1, 2, 3));

    EZ_TEST_VEC3(r, ezVec3T(1 * 1 + 2 * 2 + 3 * 3, 1 * 4 + 2 * 5 + 3 * 6, 1 * 7 + 2 * 8 + 3 * 9), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*=")
  {
    ezMat3T m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    m *= 2.0f;

    EZ_TEST_VEC3(m.GetRow(0), ezVec3T(2, 4, 6), 0.0001f);
    EZ_TEST_VEC3(m.GetRow(1), ezVec3T(8, 10, 12), 0.0001f);
    EZ_TEST_VEC3(m.GetRow(2), ezVec3T(14, 16, 18), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/=")
  {
    ezMat3T m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    m *= 4.0f;
    m /= 2.0f;

    EZ_TEST_VEC3(m.GetRow(0), ezVec3T(2, 4, 6), 0.0001f);
    EZ_TEST_VEC3(m.GetRow(1), ezVec3T(8, 10, 12), 0.0001f);
    EZ_TEST_VEC3(m.GetRow(2), ezVec3T(14, 16, 18), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical")
  {
    ezMat3T m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    ezMat3T m2 = m;

    EZ_TEST_BOOL(m.IsIdentical(m2));

    m2.m_fElementsCM[0] += 0.00001f;
    EZ_TEST_BOOL(!m.IsIdentical(m2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    ezMat3T m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    ezMat3T m2 = m;

    EZ_TEST_BOOL(m.IsEqual(m2, 0.0001f));

    m2.m_fElementsCM[0] += 0.00001f;
    EZ_TEST_BOOL(m.IsEqual(m2, 0.0001f));
    EZ_TEST_BOOL(!m.IsEqual(m2, 0.000001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(mat, mat)")
  {
    ezMat3T m1(1, 2, 3,
              4, 5, 6,
              7, 8, 9);

    ezMat3T m2(-1, -2, -3,
              -4, -5, -6,
              -7, -8, -9);

    ezMat3T r = m1 * m2;

    EZ_TEST_VEC3(r.GetColumn(0), ezVec3T(-1 * 1 + -4 * 2 + -7 * 3, -1 * 4 + -4 * 5 + -7 * 6, -1 * 7 + -4 * 8 + -7 * 9), 0.001f);
    EZ_TEST_VEC3(r.GetColumn(1), ezVec3T(-2 * 1 + -5 * 2 + -8 * 3, -2 * 4 + -5 * 5 + -8 * 6, -2 * 7 + -5 * 8 + -8 * 9), 0.001f);
    EZ_TEST_VEC3(r.GetColumn(2), ezVec3T(-3 * 1 + -6 * 2 + -9 * 3, -3 * 4 + -6 * 5 + -9 * 6, -3 * 7 + -6 * 8 + -9 * 9), 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(mat, vec)")
  {
    ezMat3T m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    const ezVec3T r = m * (ezVec3T(1, 2, 3));

    EZ_TEST_VEC3(r, ezVec3T(1 * 1 + 2 * 2 + 3 * 3, 1 * 4 + 2 * 5 + 3 * 6, 1 * 7 + 2 * 8 + 3 * 9), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(mat, float) | operator*(float, mat)")
  {
    ezMat3T m0(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    ezMat3T m = m0 * (ezMathTestType) 2;
    ezMat3T m2 = (ezMathTestType) 2 * m0;

    EZ_TEST_VEC3(m.GetRow(0), ezVec3T(2, 4, 6), 0.0001f);
    EZ_TEST_VEC3(m.GetRow(1), ezVec3T(8, 10, 12), 0.0001f);
    EZ_TEST_VEC3(m.GetRow(2), ezVec3T(14, 16, 18), 0.0001f);

    EZ_TEST_VEC3(m2.GetRow(0), ezVec3T(2, 4, 6), 0.0001f);
    EZ_TEST_VEC3(m2.GetRow(1), ezVec3T(8, 10, 12), 0.0001f);
    EZ_TEST_VEC3(m2.GetRow(2), ezVec3T(14, 16, 18), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/(mat, float)")
  {
    ezMat3T m0(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    m0 *= 4.0f;
    
    ezMat3T m = m0 / (ezMathTestType) 2;

    EZ_TEST_VEC3(m.GetRow(0), ezVec3T(2, 4, 6), 0.0001f);
    EZ_TEST_VEC3(m.GetRow(1), ezVec3T(8, 10, 12), 0.0001f);
    EZ_TEST_VEC3(m.GetRow(2), ezVec3T(14, 16, 18), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+(mat, mat) | operator-(mat, mat)")
  {
    ezMat3T m0(1, 2, 3,
             4, 5, 6,
             7, 8, 9);
  
    ezMat3T m1(-1, -2, -3,
             -4, -5, -6,
             -7, -8, -9);  

    EZ_TEST_BOOL((m0 + m1).IsZero());
    EZ_TEST_BOOL((m0 - m1).IsEqual(m0 * (ezMathTestType) 2, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator== (mat, mat) | operator!= (mat, mat)")
  {
    ezMat3T m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    ezMat3T m2 = m;

    EZ_TEST_BOOL(m == m2);

    m2.m_fElementsCM[0] += 0.00001f;

    EZ_TEST_BOOL(m != m2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::BasicType<ezMathTestType>::SupportsNaN())
    {
      ezMat3T m;

      m.SetIdentity();
      EZ_TEST_BOOL(!m.IsNaN());

      for (ezUInt32 i = 0; i < 9; ++i)
      {
        m.SetIdentity();
        m.m_fElementsCM[i] = ezMath::BasicType<ezMathTestType>::GetNaN();

        EZ_TEST_BOOL(m.IsNaN());
      }
    }
  }
}

