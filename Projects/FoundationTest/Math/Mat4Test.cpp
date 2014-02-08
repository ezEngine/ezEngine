#include <PCH.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Implementation/AllClasses_inl.h>

EZ_CREATE_SIMPLE_TEST(Math, Mat4)
{



  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Default Constructor")
  {
    #if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      if (ezMath::BasicType<ezMat3T::ComponentType>::SupportsNaN())
      {
        // In debug the default constructor initializes everything with NaN.
        ezMat4T m;
        EZ_TEST_BOOL(ezMath::IsNaN(m.m_fElementsCM[0]) && 
                ezMath::IsNaN(m.m_fElementsCM[1]) && 
                ezMath::IsNaN(m.m_fElementsCM[2]) && 
                ezMath::IsNaN(m.m_fElementsCM[3]) && 
                ezMath::IsNaN(m.m_fElementsCM[4]) && 
                ezMath::IsNaN(m.m_fElementsCM[5]) && 
                ezMath::IsNaN(m.m_fElementsCM[6]) && 
                ezMath::IsNaN(m.m_fElementsCM[7]) && 
                ezMath::IsNaN(m.m_fElementsCM[8]) && 
                ezMath::IsNaN(m.m_fElementsCM[9]) && 
                ezMath::IsNaN(m.m_fElementsCM[10]) && 
                ezMath::IsNaN(m.m_fElementsCM[11]) && 
                ezMath::IsNaN(m.m_fElementsCM[12]) && 
                ezMath::IsNaN(m.m_fElementsCM[13]) && 
                ezMath::IsNaN(m.m_fElementsCM[14]) && 
                ezMath::IsNaN(m.m_fElementsCM[15]));
      }
    #else
        // Placement new of the default constructor should not have any effect on the previous data.
        ezMat4T::ComponentType testBlock[16] = { (ezMat4T::ComponentType) 1, (ezMat4T::ComponentType) 2, (ezMat4T::ComponentType) 3, (ezMat4T::ComponentType) 4, 
                                                 (ezMat4T::ComponentType) 5, (ezMat4T::ComponentType) 6, (ezMat4T::ComponentType) 7, (ezMat4T::ComponentType) 8, 
                                                 (ezMat4T::ComponentType) 9, (ezMat4T::ComponentType) 10, (ezMat4T::ComponentType) 11, (ezMat4T::ComponentType) 12,
                                                 (ezMat4T::ComponentType) 13, (ezMat4T::ComponentType) 14, (ezMat4T::ComponentType) 15, (ezMat4T::ComponentType) 16 };
        ezMat4T* m = ::new ((void*) &testBlock[0]) ezMat4T;

        EZ_TEST_BOOL(m->m_fElementsCM[0] == 1.0f && 
                m->m_fElementsCM[1] == 2.0f && 
                m->m_fElementsCM[2] == 3.0f && 
                m->m_fElementsCM[3] == 4.0f && 
                m->m_fElementsCM[4] == 5.0f && 
                m->m_fElementsCM[5] == 6.0f && 
                m->m_fElementsCM[6] == 7.0f && 
                m->m_fElementsCM[7] == 8.0f && 
                m->m_fElementsCM[8] == 9.0f && 
                m->m_fElementsCM[9] ==10.0f && 
                m->m_fElementsCM[10]==11.0f && 
                m->m_fElementsCM[11]==12.0f && 
                m->m_fElementsCM[12]==13.0f && 
                m->m_fElementsCM[13]==14.0f && 
                m->m_fElementsCM[14]==15.0f && 
                m->m_fElementsCM[15]==16.0f);
    #endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (Array Data)")
  {
    const ezMathTestType data[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

    {
      ezMat4T m(data, ezMatrixLayout::ColumnMajor);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && 
              m.m_fElementsCM[1] == 2.0f && 
              m.m_fElementsCM[2] == 3.0f && 
              m.m_fElementsCM[3] == 4.0f && 
              m.m_fElementsCM[4] == 5.0f && 
              m.m_fElementsCM[5] == 6.0f && 
              m.m_fElementsCM[6] == 7.0f && 
              m.m_fElementsCM[7] == 8.0f && 
              m.m_fElementsCM[8] == 9.0f && 
              m.m_fElementsCM[9] == 10.0f && 
              m.m_fElementsCM[10] == 11.0f && 
              m.m_fElementsCM[11] == 12.0f && 
              m.m_fElementsCM[12] == 13.0f && 
              m.m_fElementsCM[13] == 14.0f && 
              m.m_fElementsCM[14] == 15.0f && 
              m.m_fElementsCM[15] == 16.0f);
    }

    {
      ezMat4T m(data, ezMatrixLayout::RowMajor);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && 
              m.m_fElementsCM[1] == 5.0f && 
              m.m_fElementsCM[2] == 9.0f && 
              m.m_fElementsCM[3] == 13.0f && 
              m.m_fElementsCM[4] == 2.0f && 
              m.m_fElementsCM[5] == 6.0f && 
              m.m_fElementsCM[6] == 10.0f && 
              m.m_fElementsCM[7] == 14.0f && 
              m.m_fElementsCM[8] == 3.0f && 
              m.m_fElementsCM[9] == 7.0f && 
              m.m_fElementsCM[10] == 11.0f && 
              m.m_fElementsCM[11] == 15.0f && 
              m.m_fElementsCM[12] == 4.0f && 
              m.m_fElementsCM[13] == 8.0f && 
              m.m_fElementsCM[14] == 12.0f && 
              m.m_fElementsCM[15] == 16.0f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (Elements)")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9, 10, 11, 12,
             13, 14, 15, 16);

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 0), 2, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 0), 3, 0.00001f); EZ_TEST_FLOAT(m.Element(3, 0), 4, 0.00001f);
    EZ_TEST_FLOAT(m.Element(0, 1), 5, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 1), 6, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 1), 7, 0.00001f); EZ_TEST_FLOAT(m.Element(3, 1), 8, 0.00001f);
    EZ_TEST_FLOAT(m.Element(0, 2), 9, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 2),10, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 2),11, 0.00001f); EZ_TEST_FLOAT(m.Element(3, 2),12, 0.00001f);
    EZ_TEST_FLOAT(m.Element(0, 3),13, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 3),14, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 3),15, 0.00001f); EZ_TEST_FLOAT(m.Element(3, 3),16, 0.00001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (composite)")
  {
    ezMat3T mr(1, 2, 3,
              4, 5, 6,
              7, 8, 9);
    ezVec3T vt(10, 11, 12);

    ezMat4T m(mr, vt);

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0); EZ_TEST_FLOAT(m.Element(1, 0), 2, 0); EZ_TEST_FLOAT(m.Element(2, 0), 3, 0); EZ_TEST_FLOAT(m.Element(3, 0), 10, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 4, 0); EZ_TEST_FLOAT(m.Element(1, 1), 5, 0); EZ_TEST_FLOAT(m.Element(2, 1), 6, 0); EZ_TEST_FLOAT(m.Element(3, 1), 11, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 7, 0); EZ_TEST_FLOAT(m.Element(1, 2), 8, 0); EZ_TEST_FLOAT(m.Element(2, 2), 9, 0); EZ_TEST_FLOAT(m.Element(3, 2), 12, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 0, 0); EZ_TEST_FLOAT(m.Element(1, 3), 0, 0); EZ_TEST_FLOAT(m.Element(2, 3), 0, 0); EZ_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromArray")
  {
    const ezMathTestType data[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

    {
      ezMat4T m;
      m.SetFromArray(data, ezMatrixLayout::ColumnMajor);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && 
              m.m_fElementsCM[1] == 2.0f && 
              m.m_fElementsCM[2] == 3.0f && 
              m.m_fElementsCM[3] == 4.0f && 
              m.m_fElementsCM[4] == 5.0f && 
              m.m_fElementsCM[5] == 6.0f && 
              m.m_fElementsCM[6] == 7.0f && 
              m.m_fElementsCM[7] == 8.0f && 
              m.m_fElementsCM[8] == 9.0f && 
              m.m_fElementsCM[9] == 10.0f && 
              m.m_fElementsCM[10] == 11.0f && 
              m.m_fElementsCM[11] == 12.0f && 
              m.m_fElementsCM[12] == 13.0f && 
              m.m_fElementsCM[13] == 14.0f && 
              m.m_fElementsCM[14] == 15.0f && 
              m.m_fElementsCM[15] == 16.0f);
    }

    {
      ezMat4T m;
      m.SetFromArray(data, ezMatrixLayout::RowMajor);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && 
              m.m_fElementsCM[1] == 5.0f && 
              m.m_fElementsCM[2] == 9.0f && 
              m.m_fElementsCM[3] == 13.0f && 
              m.m_fElementsCM[4] == 2.0f && 
              m.m_fElementsCM[5] == 6.0f && 
              m.m_fElementsCM[6] == 10.0f && 
              m.m_fElementsCM[7] == 14.0f && 
              m.m_fElementsCM[8] == 3.0f && 
              m.m_fElementsCM[9] == 7.0f && 
              m.m_fElementsCM[10] == 11.0f && 
              m.m_fElementsCM[11] == 15.0f && 
              m.m_fElementsCM[12] == 4.0f && 
              m.m_fElementsCM[13] == 8.0f && 
              m.m_fElementsCM[14] == 12.0f && 
              m.m_fElementsCM[15] == 16.0f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetElements")
  {
    ezMat4T m;
    m.SetElements (1, 2, 3, 4,
                   5, 6, 7, 8,
                   9, 10, 11, 12,
                   13, 14, 15, 16);

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 0), 2, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 0), 3, 0.00001f); EZ_TEST_FLOAT(m.Element(3, 0), 4, 0.00001f);
    EZ_TEST_FLOAT(m.Element(0, 1), 5, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 1), 6, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 1), 7, 0.00001f); EZ_TEST_FLOAT(m.Element(3, 1), 8, 0.00001f);
    EZ_TEST_FLOAT(m.Element(0, 2), 9, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 2),10, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 2),11, 0.00001f); EZ_TEST_FLOAT(m.Element(3, 2),12, 0.00001f);
    EZ_TEST_FLOAT(m.Element(0, 3),13, 0.00001f); EZ_TEST_FLOAT(m.Element(1, 3),14, 0.00001f); EZ_TEST_FLOAT(m.Element(2, 3),15, 0.00001f); EZ_TEST_FLOAT(m.Element(3, 3),16, 0.00001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetTransformationMatrix")
  {
    ezMat3T mr(1, 2, 3,
              4, 5, 6,
              7, 8, 9);
    ezVec3T vt(10, 11, 12);

    ezMat4T m;
    m.SetTransformationMatrix(mr, vt);

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0); EZ_TEST_FLOAT(m.Element(1, 0), 2, 0); EZ_TEST_FLOAT(m.Element(2, 0), 3, 0); EZ_TEST_FLOAT(m.Element(3, 0), 10, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 4, 0); EZ_TEST_FLOAT(m.Element(1, 1), 5, 0); EZ_TEST_FLOAT(m.Element(2, 1), 6, 0); EZ_TEST_FLOAT(m.Element(3, 1), 11, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 7, 0); EZ_TEST_FLOAT(m.Element(1, 2), 8, 0); EZ_TEST_FLOAT(m.Element(2, 2), 9, 0); EZ_TEST_FLOAT(m.Element(3, 2), 12, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 0, 0); EZ_TEST_FLOAT(m.Element(1, 3), 0, 0); EZ_TEST_FLOAT(m.Element(2, 3), 0, 0); EZ_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsArray")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMathTestType data[16];

    m.GetAsArray(data, ezMatrixLayout::ColumnMajor);
    EZ_TEST_FLOAT(data[0], 1, 0.0001f);
    EZ_TEST_FLOAT(data[1], 5, 0.0001f);
    EZ_TEST_FLOAT(data[2], 9, 0.0001f);
    EZ_TEST_FLOAT(data[3],13, 0.0001f);
    EZ_TEST_FLOAT(data[4], 2, 0.0001f);
    EZ_TEST_FLOAT(data[5], 6, 0.0001f);
    EZ_TEST_FLOAT(data[6],10, 0.0001f);
    EZ_TEST_FLOAT(data[7],14, 0.0001f);
    EZ_TEST_FLOAT(data[8], 3, 0.0001f);
    EZ_TEST_FLOAT(data[9], 7, 0.0001f);
    EZ_TEST_FLOAT(data[10],11, 0.0001f);
    EZ_TEST_FLOAT(data[11],15, 0.0001f);
    EZ_TEST_FLOAT(data[12], 4, 0.0001f);
    EZ_TEST_FLOAT(data[13], 8, 0.0001f);
    EZ_TEST_FLOAT(data[14],12, 0.0001f);
    EZ_TEST_FLOAT(data[15],16, 0.0001f);

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
    EZ_TEST_FLOAT(data[9],10, 0.0001f);
    EZ_TEST_FLOAT(data[10],11, 0.0001f);
    EZ_TEST_FLOAT(data[11],12, 0.0001f);
    EZ_TEST_FLOAT(data[12],13, 0.0001f);
    EZ_TEST_FLOAT(data[13],14, 0.0001f);
    EZ_TEST_FLOAT(data[14],15, 0.0001f);
    EZ_TEST_FLOAT(data[15],16, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetZero")
  {
    ezMat4T m;
    m.SetZero();

    for (ezUInt32 i = 0; i < 16; ++i)
      EZ_TEST_FLOAT(m.m_fElementsCM[i], 0.0f, 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetIdentity")
  {
    ezMat4T m;
    m.SetIdentity();

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0); EZ_TEST_FLOAT(m.Element(1, 0), 0, 0); EZ_TEST_FLOAT(m.Element(2, 0), 0, 0); EZ_TEST_FLOAT(m.Element(3, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0); EZ_TEST_FLOAT(m.Element(1, 1), 1, 0); EZ_TEST_FLOAT(m.Element(2, 1), 0, 0); EZ_TEST_FLOAT(m.Element(3, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0); EZ_TEST_FLOAT(m.Element(1, 2), 0, 0); EZ_TEST_FLOAT(m.Element(2, 2), 1, 0); EZ_TEST_FLOAT(m.Element(3, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 0, 0); EZ_TEST_FLOAT(m.Element(1, 3), 0, 0); EZ_TEST_FLOAT(m.Element(2, 3), 0, 0); EZ_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetTranslationMatrix")
  {
    ezMat4T m;
    m.SetTranslationMatrix(ezVec3T(2, 3, 4));

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0); EZ_TEST_FLOAT(m.Element(1, 0), 0, 0); EZ_TEST_FLOAT(m.Element(2, 0), 0, 0); EZ_TEST_FLOAT(m.Element(3, 0), 2, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0); EZ_TEST_FLOAT(m.Element(1, 1), 1, 0); EZ_TEST_FLOAT(m.Element(2, 1), 0, 0); EZ_TEST_FLOAT(m.Element(3, 1), 3, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0); EZ_TEST_FLOAT(m.Element(1, 2), 0, 0); EZ_TEST_FLOAT(m.Element(2, 2), 1, 0); EZ_TEST_FLOAT(m.Element(3, 2), 4, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 0, 0); EZ_TEST_FLOAT(m.Element(1, 3), 0, 0); EZ_TEST_FLOAT(m.Element(2, 3), 0, 0); EZ_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetScalingMatrix")
  {
    ezMat4T m;
    m.SetScalingMatrix(ezVec3T(2, 3, 4));

    EZ_TEST_FLOAT(m.Element(0, 0), 2, 0); EZ_TEST_FLOAT(m.Element(1, 0), 0, 0); EZ_TEST_FLOAT(m.Element(2, 0), 0, 0); EZ_TEST_FLOAT(m.Element(3, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0); EZ_TEST_FLOAT(m.Element(1, 1), 3, 0); EZ_TEST_FLOAT(m.Element(2, 1), 0, 0); EZ_TEST_FLOAT(m.Element(3, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0); EZ_TEST_FLOAT(m.Element(1, 2), 0, 0); EZ_TEST_FLOAT(m.Element(2, 2), 4, 0); EZ_TEST_FLOAT(m.Element(3, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 0, 0); EZ_TEST_FLOAT(m.Element(1, 3), 0, 0); EZ_TEST_FLOAT(m.Element(2, 3), 0, 0); EZ_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRotationMatrixX")
  {
    ezMat4T m;

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
    ezMat4T m;

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
    ezMat4T m;

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
    ezMat4T m;

    m.SetRotationMatrix(ezVec3T(1, 0, 0), ezAngle::Degree(90));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(1, -3, 2), ezMath::BasicType<ezMat3T::ComponentType>::DefaultEpsilon()));

    m.SetRotationMatrix(ezVec3T(1, 0, 0), ezAngle::Degree(180));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(1, -2, -3), ezMath::BasicType<ezMat3T::ComponentType>::DefaultEpsilon()));

    m.SetRotationMatrix(ezVec3T(1, 0, 0), ezAngle::Degree(270));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(1, 3, -2), ezMath::BasicType<ezMat3T::ComponentType>::DefaultEpsilon()));

    m.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(90));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(3, 2, -1), ezMath::BasicType<ezMat3T::ComponentType>::DefaultEpsilon()));

    m.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(180));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(-1, 2, -3), ezMath::BasicType<ezMat3T::ComponentType>::DefaultEpsilon()));

    m.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(270));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(-3, 2, 1), ezMath::BasicType<ezMat3T::ComponentType>::DefaultEpsilon()));

    m.SetRotationMatrix(ezVec3T(0, 0, 1), ezAngle::Degree(90));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(-2, 1, 3), ezMath::BasicType<ezMat3T::ComponentType>::DefaultEpsilon()));

    m.SetRotationMatrix(ezVec3T(0, 0, 1), ezAngle::Degree(180));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(-1, -2, 3), ezMath::BasicType<ezMat3T::ComponentType>::DefaultEpsilon()));

    m.SetRotationMatrix(ezVec3T(0, 0, 1), ezAngle::Degree(270));
    EZ_TEST_BOOL((m * ezVec3T(1, 2, 3)).IsEqual(ezVec3T(2, -1, 3), ezMath::BasicType<ezMat3T::ComponentType>::DefaultEpsilon()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IdentityMatrix")
  {
    ezMat4T m = ezMat4T::IdentityMatrix();

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0); EZ_TEST_FLOAT(m.Element(1, 0), 0, 0); EZ_TEST_FLOAT(m.Element(2, 0), 0, 0); EZ_TEST_FLOAT(m.Element(3, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0); EZ_TEST_FLOAT(m.Element(1, 1), 1, 0); EZ_TEST_FLOAT(m.Element(2, 1), 0, 0); EZ_TEST_FLOAT(m.Element(3, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0); EZ_TEST_FLOAT(m.Element(1, 2), 0, 0); EZ_TEST_FLOAT(m.Element(2, 2), 1, 0); EZ_TEST_FLOAT(m.Element(3, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 0, 0); EZ_TEST_FLOAT(m.Element(1, 3), 0, 0); EZ_TEST_FLOAT(m.Element(2, 3), 0, 0); EZ_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ZeroMatrix")
  {
    ezMat4T m = ezMat4T::ZeroMatrix();

    EZ_TEST_FLOAT(m.Element(0, 0), 0, 0); EZ_TEST_FLOAT(m.Element(1, 0), 0, 0); EZ_TEST_FLOAT(m.Element(2, 0), 0, 0); EZ_TEST_FLOAT(m.Element(3, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0); EZ_TEST_FLOAT(m.Element(1, 1), 0, 0); EZ_TEST_FLOAT(m.Element(2, 1), 0, 0); EZ_TEST_FLOAT(m.Element(3, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0); EZ_TEST_FLOAT(m.Element(1, 2), 0, 0); EZ_TEST_FLOAT(m.Element(2, 2), 0, 0); EZ_TEST_FLOAT(m.Element(3, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 0, 0); EZ_TEST_FLOAT(m.Element(1, 3), 0, 0); EZ_TEST_FLOAT(m.Element(2, 3), 0, 0); EZ_TEST_FLOAT(m.Element(3, 3), 0, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transpose")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    m.Transpose();

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0); EZ_TEST_FLOAT(m.Element(1, 0), 5, 0); EZ_TEST_FLOAT(m.Element(2, 0), 9, 0); EZ_TEST_FLOAT(m.Element(3, 0),13, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 2, 0); EZ_TEST_FLOAT(m.Element(1, 1), 6, 0); EZ_TEST_FLOAT(m.Element(2, 1),10, 0); EZ_TEST_FLOAT(m.Element(3, 1),14, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 3, 0); EZ_TEST_FLOAT(m.Element(1, 2), 7, 0); EZ_TEST_FLOAT(m.Element(2, 2),11, 0); EZ_TEST_FLOAT(m.Element(3, 2),15, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 4, 0); EZ_TEST_FLOAT(m.Element(1, 3), 8, 0); EZ_TEST_FLOAT(m.Element(2, 3),12, 0); EZ_TEST_FLOAT(m.Element(3, 3),16, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetTranspose")
  {
    ezMat4T m0(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat4T m = m0.GetTranspose();

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0); EZ_TEST_FLOAT(m.Element(1, 0), 5, 0); EZ_TEST_FLOAT(m.Element(2, 0), 9, 0); EZ_TEST_FLOAT(m.Element(3, 0),13, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 2, 0); EZ_TEST_FLOAT(m.Element(1, 1), 6, 0); EZ_TEST_FLOAT(m.Element(2, 1),10, 0); EZ_TEST_FLOAT(m.Element(3, 1),14, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 3, 0); EZ_TEST_FLOAT(m.Element(1, 2), 7, 0); EZ_TEST_FLOAT(m.Element(2, 2),11, 0); EZ_TEST_FLOAT(m.Element(3, 2),15, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 4, 0); EZ_TEST_FLOAT(m.Element(1, 3), 8, 0); EZ_TEST_FLOAT(m.Element(2, 3),12, 0); EZ_TEST_FLOAT(m.Element(3, 3),16, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Invert")
  {
    for (float x = 1.0f; x < 360.0f; x += 10.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 17.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 23.0f)
        {
          ezMat4T m, inv;
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
          ezMat4T m, inv;
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
    ezMat4T m;

    m.SetIdentity();
    EZ_TEST_BOOL(!m.IsZero());

    m.SetZero();
    EZ_TEST_BOOL(m.IsZero());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentity")
  {
    ezMat4T m;

    m.SetIdentity();
    EZ_TEST_BOOL(m.IsIdentity());

    m.SetZero();
    EZ_TEST_BOOL(!m.IsIdentity());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsValid")
  {
    if (ezMath::BasicType<ezMat3T::ComponentType>::SupportsNaN())
    {
      ezMat4T m;

      m.SetZero();
      EZ_TEST_BOOL(m.IsValid());

      m.m_fElementsCM[0] = ezMath::BasicType<ezMat4T::ComponentType>::GetNaN();
      EZ_TEST_BOOL(!m.IsValid());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRow")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    EZ_TEST_VEC4(m.GetRow(0), ezVec4T(1, 2, 3, 4), 0.0f);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4T(5, 6, 7, 8), 0.0f);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4T(9,10,11,12), 0.0f);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4T(13,14,15,16), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRow")
  {
    ezMat4T m;
    m.SetZero();

    m.SetRow(0, ezVec4T(1, 2, 3, 4));
    EZ_TEST_VEC4(m.GetRow(0), ezVec4T(1, 2, 3, 4), 0.0f);

    m.SetRow(1, ezVec4T(5, 6, 7, 8));
    EZ_TEST_VEC4(m.GetRow(1), ezVec4T(5, 6, 7, 8), 0.0f);

    m.SetRow(2, ezVec4T(9, 10, 11, 12));
    EZ_TEST_VEC4(m.GetRow(2), ezVec4T(9,10,11,12), 0.0f);

    m.SetRow(3, ezVec4T(13, 14, 15, 16));
    EZ_TEST_VEC4(m.GetRow(3), ezVec4T(13, 14, 15, 16), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetColumn")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    EZ_TEST_VEC4(m.GetColumn(0), ezVec4T(1, 5, 9, 13), 0.0f);
    EZ_TEST_VEC4(m.GetColumn(1), ezVec4T(2, 6, 10, 14), 0.0f);
    EZ_TEST_VEC4(m.GetColumn(2), ezVec4T(3, 7, 11, 15), 0.0f);
    EZ_TEST_VEC4(m.GetColumn(3), ezVec4T(4, 8, 12, 16), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetColumn")
  {
    ezMat4T m;
    m.SetZero();

    m.SetColumn(0, ezVec4T(1, 2, 3, 4));
    EZ_TEST_VEC4(m.GetColumn(0), ezVec4T(1, 2, 3, 4), 0.0f);

    m.SetColumn(1, ezVec4T(5, 6, 7, 8));
    EZ_TEST_VEC4(m.GetColumn(1), ezVec4T(5, 6, 7, 8), 0.0f);

    m.SetColumn(2, ezVec4T(9, 10, 11, 12));
    EZ_TEST_VEC4(m.GetColumn(2), ezVec4T(9,10,11,12), 0.0f);

    m.SetColumn(3, ezVec4T(13, 14, 15, 16));
    EZ_TEST_VEC4(m.GetColumn(3), ezVec4T(13, 14, 15, 16), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDiagonal")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    EZ_TEST_VEC4(m.GetDiagonal(), ezVec4T(1, 6, 11, 16), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetDiagonal")
  {
    ezMat4T m;
    m.SetZero();

    m.SetDiagonal(ezVec4T(1, 2, 3, 4));
    EZ_TEST_VEC4(m.GetColumn(0), ezVec4T(1, 0, 0, 0), 0.0f);
    EZ_TEST_VEC4(m.GetColumn(1), ezVec4T(0, 2, 0, 0), 0.0f);
    EZ_TEST_VEC4(m.GetColumn(2), ezVec4T(0, 0, 3, 0), 0.0f);
    EZ_TEST_VEC4(m.GetColumn(3), ezVec4T(0, 0, 0, 4), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetTranslationVector")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    EZ_TEST_VEC3(m.GetTranslationVector(), ezVec3T(4, 8, 12), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetTranslationVector")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    m.SetTranslationVector(ezVec3T(17, 18, 19));
    EZ_TEST_VEC4(m.GetRow(0), ezVec4T(1, 2, 3, 17), 0.0f);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4T(5, 6, 7, 18), 0.0f);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4T(9,10,11, 19), 0.0f);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4T(13,14,15,16), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRotationalPart")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat3T r(17, 18, 19,
             20, 21, 22,
             23, 24, 25);

    m.SetRotationalPart(r);
    EZ_TEST_VEC4(m.GetRow(0), ezVec4T(17, 18, 19, 4), 0.0f);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4T(20, 21, 22, 8), 0.0f);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4T(23, 24, 25, 12), 0.0f);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4T(13, 14, 15, 16), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRotationalPart")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat3T r = m.GetRotationalPart();
    EZ_TEST_VEC3(r.GetRow(0), ezVec3T(1, 2, 3), 0.0f);
    EZ_TEST_VEC3(r.GetRow(1), ezVec3T(5, 6, 7), 0.0f);
    EZ_TEST_VEC3(r.GetRow(2), ezVec3T(9, 10, 11), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetScalingFactors")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezVec3T s = m.GetScalingFactors();
    EZ_TEST_VEC3(s, ezVec3T(ezMath::Sqrt((ezMathTestType) (1*1 + 5*5 + 9*9)), ezMath::Sqrt((ezMathTestType) (2*2 + 6*6 + 10*10)), ezMath::Sqrt((ezMathTestType) (3*3 + 7*7 + 11*11))), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetScalingFactors")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    EZ_TEST_BOOL(m.SetScalingFactors(ezVec3T(1, 2, 3)) == EZ_SUCCESS);

    ezVec3T s = m.GetScalingFactors();
    EZ_TEST_VEC3(s, ezVec3T(1, 2, 3), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformDirection")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    const ezVec3T r = m.TransformDirection(ezVec3T(1, 2, 3));

    EZ_TEST_VEC3(r, ezVec3T(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformDirection(array)")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezVec3T data[3] = { ezVec3T(1, 2, 3), ezVec3T(4, 5, 6), ezVec3T(7, 8, 9) };

    m.TransformDirection(data, 2);

    EZ_TEST_VEC3(data[0], ezVec3T(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), 0.0001f);
    EZ_TEST_VEC3(data[1], ezVec3T(4 * 1 + 5 * 2 + 6 * 3, 4 * 5 + 5 * 6 + 6 * 7, 4 * 9 + 5 * 10 + 6 * 11), 0.0001f);
    EZ_TEST_VEC3(data[2], ezVec3T(7, 8, 9), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformPosition")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    const ezVec3T r = m.TransformPosition(ezVec3T(1, 2, 3));

    EZ_TEST_VEC3(r, ezVec3T(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformPosition(array)")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezVec3T data[3] = { ezVec3T(1, 2, 3), ezVec3T(4, 5, 6), ezVec3T(7, 8, 9) };

    m.TransformPosition(data, 2);

    EZ_TEST_VEC3(data[0], ezVec3T(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001f);
    EZ_TEST_VEC3(data[1], ezVec3T(4 * 1 + 5 * 2 + 6 * 3 + 4, 4 * 5 + 5 * 6 + 6 * 7 + 8, 4 * 9 + 5 * 10 + 6 * 11 + 12), 0.0001f);
    EZ_TEST_VEC3(data[2], ezVec3T(7, 8, 9), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    const ezVec4T r = m.Transform(ezVec4T(1, 2, 3, 4));

    EZ_TEST_VEC4(r, ezVec4T(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 8 * 4, 1 * 9 + 2 * 10 + 3 * 11 + 12 * 4, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform(array)")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezVec4T data[3] = { ezVec4T(1, 2, 3, 4), ezVec4T(5, 6, 7, 8), ezVec4T(9, 10, 11, 12) };

    m.Transform(data, 2);

    EZ_TEST_VEC4(data[0], ezVec4T(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 8 * 4, 1 * 9 + 2 * 10 + 3 * 11 + 12 * 4, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16), 0.0001f);
    EZ_TEST_VEC4(data[1], ezVec4T(5 * 1 + 6 * 2 + 7 * 3 + 8 * 4, 5 * 5 + 6 * 6 + 7 * 7 + 8 * 8, 5 * 9 + 6 * 10 + 7 * 11 + 12 * 8, 5 * 13 + 6 * 14 + 7 * 15 + 8 * 16), 0.0001f);
    EZ_TEST_VEC4(data[2], ezVec4T(9, 10, 11, 12), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*=")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    m *= 2.0f;

    EZ_TEST_VEC4(m.GetRow(0), ezVec4T(2, 4, 6, 8), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4T(10, 12, 14, 16), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4T(18, 20, 22, 24), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4T(26, 28, 30, 32), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/=")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    m *= 4.0f;
    m /= 2.0f;

    EZ_TEST_VEC4(m.GetRow(0), ezVec4T(2, 4, 6, 8), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4T(10, 12, 14, 16), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4T(18, 20, 22, 24), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4T(26, 28, 30, 32), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat4T m2 = m;

    EZ_TEST_BOOL(m.IsIdentical(m2));

    m2.m_fElementsCM[0] += 0.00001f;
    EZ_TEST_BOOL(!m.IsIdentical(m2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat4T m2 = m;

    EZ_TEST_BOOL(m.IsEqual(m2, 0.0001f));

    m2.m_fElementsCM[0] += 0.00001f;
    EZ_TEST_BOOL(m.IsEqual(m2, 0.0001f));
    EZ_TEST_BOOL(!m.IsEqual(m2, 0.000001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(mat, mat)")
  {
    ezMat4T m1(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat4T m2(-1, -2, -3, -4,
              -5, -6, -7, -8,
              -9,-10,-11,-12,
              -13,-14,-15,-16);

    ezMat4T r = m1 * m2;

    EZ_TEST_VEC4(r.GetColumn(0), ezVec4T(-1 * 1 + -5 * 2 + -9 * 3 + -13 * 4,  -1 * 5 + -5 * 6 + -9 * 7 + -13 * 8, -1 * 9 + -5 * 10 + -9 * 11 + -13 * 12, -1 * 13 + -5 * 14 + -9 * 15 + -13 * 16), 0.001f);
    EZ_TEST_VEC4(r.GetColumn(1), ezVec4T(-2 * 1 + -6 * 2 + -10 * 3 + -14 * 4, -2 * 5 + -6 * 6 + -10 * 7 + -14 * 8, -2 * 9 + -6 * 10 + -10 * 11 + -14 * 12, -2 * 13 + -6 * 14 + -10 * 15 + -14 * 16), 0.001f);
    EZ_TEST_VEC4(r.GetColumn(2), ezVec4T(-3 * 1 + -7 * 2 + -11 * 3 + -15 * 4, -3 * 5 + -7 * 6 + -11 * 7 + -15 * 8, -3 * 9 + -7 * 10 + -11 * 11 + -15 * 12, -3 * 13 + -7 * 14 + -11 * 15 + -15 * 16), 0.001f);
    EZ_TEST_VEC4(r.GetColumn(3), ezVec4T(-4 * 1 + -8 * 2 + -12 * 3 + -16 * 4, -4 * 5 + -8 * 6 + -12 * 7 + -16 * 8, -4 * 9 + -8 * 10 + -12 * 11 + -16 * 12, -4 * 13 + -8 * 14 + -12 * 15 + -16 * 16), 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(mat, vec3)")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    const ezVec3T r = m * ezVec3T(1, 2, 3);

    EZ_TEST_VEC3(r, ezVec3T(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(mat, vec4)")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    const ezVec4T r = m * ezVec4T(1, 2, 3, 4);

    EZ_TEST_VEC4(r, ezVec4T(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 4 * 8, 1 * 9 + 2 * 10 + 3 * 11 + 4 * 12, 1* 13 + 2 * 14 + 3 * 15 + 4 * 16), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(mat, float) | operator*(float, mat)")
  {
    ezMat4T m0(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat4T m = m0 * (ezMathTestType) 2;
    ezMat4T m2 = (ezMathTestType) 2 * m0;

    EZ_TEST_VEC4(m.GetRow(0), ezVec4T(2, 4, 6, 8), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4T(10, 12, 14, 16), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4T(18, 20, 22, 24), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4T(26, 28, 30, 32), 0.0001f);

    EZ_TEST_VEC4(m2.GetRow(0), ezVec4T(2, 4, 6, 8), 0.0001f);
    EZ_TEST_VEC4(m2.GetRow(1), ezVec4T(10, 12, 14, 16), 0.0001f);
    EZ_TEST_VEC4(m2.GetRow(2), ezVec4T(18, 20, 22, 24), 0.0001f);
    EZ_TEST_VEC4(m2.GetRow(3), ezVec4T(26, 28, 30, 32), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/(mat, float)")
  {
    ezMat4T m0(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    m0 *= (ezMathTestType) 4;
    
    ezMat4T m = m0 / (ezMathTestType) 2;

    EZ_TEST_VEC4(m.GetRow(0), ezVec4T(2, 4, 6, 8), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4T(10, 12, 14, 16), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4T(18, 20, 22, 24), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4T(26, 28, 30, 32), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+(mat, mat) | operator-(mat, mat)")
  {
    ezMat4T m0(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);
  
    ezMat4T m1(-1, -2, -3, -4,
             -5, -6, -7, -8,
             -9,-10,-11,-12,
             -13,-14,-15,-16);

    EZ_TEST_BOOL((m0 + m1).IsZero());
    EZ_TEST_BOOL((m0 - m1).IsEqual(m0 * (ezMathTestType) 2, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator== (mat, mat) | operator!= (mat, mat)")
  {
    ezMat4T m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat4T m2 = m;

    EZ_TEST_BOOL(m == m2);

    m2.m_fElementsCM[0] += 0.00001f;

    EZ_TEST_BOOL(m != m2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::BasicType<ezMathTestType>::SupportsNaN())
    {
      ezMat4T m;

      m.SetIdentity();
      EZ_TEST_BOOL(!m.IsNaN());

      for (ezUInt32 i = 0; i < 16; ++i)
      {
        m.SetIdentity();
        m.m_fElementsCM[i] = ezMath::BasicType<ezMathTestType>::GetNaN();

        EZ_TEST_BOOL(m.IsNaN());
      }
    }
  }
}