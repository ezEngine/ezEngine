#include <PCH.h>
#include <Foundation/Math/Mat4.h>

EZ_CREATE_SIMPLE_TEST(Math, Mat4)
{



  EZ_TEST_BLOCK(true, "Default Constructor")
  {
    #if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
        // In debug the default constructor initializes everything with NaN.
        ezMat4 m;
        EZ_TEST(ezMath::IsNaN(m.m_fElementsCM[0]) && 
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
    #else
        // Placement new of the default constructor should not have any effect on the previous data.
        float testBlock[16] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };
        ezMat4* m = ::new ((void*) &testBlock[0]) ezMat4;

        EZ_TEST(m->m_fElementsCM[0] == 1.0f && 
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

  EZ_TEST_BLOCK(true, "Constructor (Array Data)")
  {
    const float data[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

    {
      ezMat4 m(data, ezMatrixLayout::ColumnMajor);

      EZ_TEST(m.m_fElementsCM[0] == 1.0f && 
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
      ezMat4 m(data, ezMatrixLayout::RowMajor);

      EZ_TEST(m.m_fElementsCM[0] == 1.0f && 
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

  EZ_TEST_BLOCK(true, "Constructor (Elements)")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9, 10, 11, 12,
             13, 14, 15, 16);

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 1, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][0], 2, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][0], 3, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[3][0], 4, 0.00001f);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 5, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][1], 6, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][1], 7, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[3][1], 8, 0.00001f);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 9, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][2],10, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][2],11, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[3][2],12, 0.00001f);
    EZ_TEST_FLOAT(m.m_fColumn[0][3],13, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][3],14, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][3],15, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[3][3],16, 0.00001f);
  }

  EZ_TEST_BLOCK(true, "Constructor (composite)")
  {
    ezMat3 mr(1, 2, 3,
              4, 5, 6,
              7, 8, 9);
    ezVec3 vt(10, 11, 12);

    ezMat4 m(mr, vt);

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[1][0], 2, 0); EZ_TEST_FLOAT(m.m_fColumn[2][0], 3, 0); EZ_TEST_FLOAT(m.m_fColumn[3][0], 10, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 4, 0); EZ_TEST_FLOAT(m.m_fColumn[1][1], 5, 0); EZ_TEST_FLOAT(m.m_fColumn[2][1], 6, 0); EZ_TEST_FLOAT(m.m_fColumn[3][1], 11, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 7, 0); EZ_TEST_FLOAT(m.m_fColumn[1][2], 8, 0); EZ_TEST_FLOAT(m.m_fColumn[2][2], 9, 0); EZ_TEST_FLOAT(m.m_fColumn[3][2], 12, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][3], 1, 0);
  }

  EZ_TEST_BLOCK(true, "SetFromArray")
  {
    const float data[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

    {
      ezMat4 m;
      m.SetFromArray(data, ezMatrixLayout::ColumnMajor);

      EZ_TEST(m.m_fElementsCM[0] == 1.0f && 
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
      ezMat4 m;
      m.SetFromArray(data, ezMatrixLayout::RowMajor);

      EZ_TEST(m.m_fElementsCM[0] == 1.0f && 
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

  EZ_TEST_BLOCK(true, "SetElements")
  {
    ezMat4 m;
    m.SetElements (1, 2, 3, 4,
                   5, 6, 7, 8,
                   9, 10, 11, 12,
                   13, 14, 15, 16);

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 1, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][0], 2, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][0], 3, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[3][0], 4, 0.00001f);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 5, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][1], 6, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][1], 7, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[3][1], 8, 0.00001f);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 9, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][2],10, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][2],11, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[3][2],12, 0.00001f);
    EZ_TEST_FLOAT(m.m_fColumn[0][3],13, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][3],14, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][3],15, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[3][3],16, 0.00001f);
  }

  EZ_TEST_BLOCK(true, "SetTransformationMatrix")
  {
    ezMat3 mr(1, 2, 3,
              4, 5, 6,
              7, 8, 9);
    ezVec3 vt(10, 11, 12);

    ezMat4 m;
    m.SetTransformationMatrix(mr, vt);

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[1][0], 2, 0); EZ_TEST_FLOAT(m.m_fColumn[2][0], 3, 0); EZ_TEST_FLOAT(m.m_fColumn[3][0], 10, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 4, 0); EZ_TEST_FLOAT(m.m_fColumn[1][1], 5, 0); EZ_TEST_FLOAT(m.m_fColumn[2][1], 6, 0); EZ_TEST_FLOAT(m.m_fColumn[3][1], 11, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 7, 0); EZ_TEST_FLOAT(m.m_fColumn[1][2], 8, 0); EZ_TEST_FLOAT(m.m_fColumn[2][2], 9, 0); EZ_TEST_FLOAT(m.m_fColumn[3][2], 12, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][3], 1, 0);
  }

  EZ_TEST_BLOCK(true, "GetAsArray")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    float data[16];

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

  EZ_TEST_BLOCK(true, "SetZero")
  {
    ezMat4 m;
    m.SetZero();

    for (ezUInt32 i = 0; i < 16; ++i)
      EZ_TEST_FLOAT(m.m_fElementsCM[i], 0.0f, 0.0f);
  }

  EZ_TEST_BLOCK(true, "SetIdentity")
  {
    ezMat4 m;
    m.SetIdentity();

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[1][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][0], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][1], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[2][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][1], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][2], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[3][2], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][3], 1, 0);
  }

  EZ_TEST_BLOCK(true, "SetTranslationMatrix")
  {
    ezMat4 m;
    m.SetTranslationMatrix(ezVec3(2, 3, 4));

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[1][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][0], 2, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][1], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[2][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][1], 3, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][2], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[3][2], 4, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][3], 1, 0);
  }

  EZ_TEST_BLOCK(true, "SetScalingMatrix")
  {
    ezMat4 m;
    m.SetScalingMatrix(ezVec3(2, 3, 4));

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 2, 0); EZ_TEST_FLOAT(m.m_fColumn[1][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][0], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][1], 3, 0); EZ_TEST_FLOAT(m.m_fColumn[2][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][1], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][2], 4, 0); EZ_TEST_FLOAT(m.m_fColumn[3][2], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][3], 1, 0);
  }

  EZ_TEST_BLOCK(true, "SetRotationMatrixX")
  {
    ezMat4 m;

    m.SetRotationMatrixX(90);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(1, -3, 2), 0.0001f));

    m.SetRotationMatrixX(180);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(1, -2, -3), 0.0001f));

    m.SetRotationMatrixX(270);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(1, 3, -2), 0.0001f));

    m.SetRotationMatrixX(360);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(1, 2, 3), 0.0001f));
  }

  EZ_TEST_BLOCK(true, "SetRotationMatrixY")
  {
    ezMat4 m;

    m.SetRotationMatrixY(90);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(3, 2, -1), 0.0001f));

    m.SetRotationMatrixY(180);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(-1, 2, -3), 0.0001f));

    m.SetRotationMatrixY(270);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(-3, 2, 1), 0.0001f));

    m.SetRotationMatrixY(360);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(1, 2, 3), 0.0001f));
  }

  EZ_TEST_BLOCK(true, "SetRotationMatrixZ")
  {
    ezMat4 m;

    m.SetRotationMatrixZ(90);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(-2, 1, 3), 0.0001f));

    m.SetRotationMatrixZ(180);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(-1, -2, 3), 0.0001f));

    m.SetRotationMatrixZ(270);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(2, -1, 3), 0.0001f));

    m.SetRotationMatrixZ(360);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(1, 2, 3), 0.0001f));
  }

  EZ_TEST_BLOCK(true, "SetRotationMatrix")
  {
    ezMat4 m;

    m.SetRotationMatrix(ezVec3(1, 0, 0), 90);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(1, -3, 2), 0.0001f));

    m.SetRotationMatrix(ezVec3(1, 0, 0), 180);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(1, -2, -3), 0.0001f));

    m.SetRotationMatrix(ezVec3(1, 0, 0), 270);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(1, 3, -2), 0.0001f));

    m.SetRotationMatrix(ezVec3(0, 1, 0), 90);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(3, 2, -1), 0.0001f));

    m.SetRotationMatrix(ezVec3(0, 1, 0), 180);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(-1, 2, -3), 0.0001f));

    m.SetRotationMatrix(ezVec3(0, 1, 0), 270);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(-3, 2, 1), 0.0001f));

    m.SetRotationMatrix(ezVec3(0, 0, 1), 90);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(-2, 1, 3), 0.0001f));

    m.SetRotationMatrix(ezVec3(0, 0, 1), 180);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(-1, -2, 3), 0.0001f));

    m.SetRotationMatrix(ezVec3(0, 0, 1), 270);
    EZ_TEST((m * ezVec3(1, 2, 3)).IsEqual(ezVec3(2, -1, 3), 0.0001f));
  }

  EZ_TEST_BLOCK(true, "IdentityMatrix")
  {
    ezMat4 m = ezMat4::IdentityMatrix();

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[1][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][0], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][1], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[2][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][1], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][2], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[3][2], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][3], 1, 0);
  }

  EZ_TEST_BLOCK(true, "ZeroMatrix")
  {
    ezMat4 m = ezMat4::ZeroMatrix();

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][0], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][1], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][2], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][3], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[3][3], 0, 0);
  }

  EZ_TEST_BLOCK(true, "Transpose")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    m.Transpose();

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[1][0], 5, 0); EZ_TEST_FLOAT(m.m_fColumn[2][0], 9, 0); EZ_TEST_FLOAT(m.m_fColumn[3][0],13, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 2, 0); EZ_TEST_FLOAT(m.m_fColumn[1][1], 6, 0); EZ_TEST_FLOAT(m.m_fColumn[2][1],10, 0); EZ_TEST_FLOAT(m.m_fColumn[3][1],14, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 3, 0); EZ_TEST_FLOAT(m.m_fColumn[1][2], 7, 0); EZ_TEST_FLOAT(m.m_fColumn[2][2],11, 0); EZ_TEST_FLOAT(m.m_fColumn[3][2],15, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][3], 4, 0); EZ_TEST_FLOAT(m.m_fColumn[1][3], 8, 0); EZ_TEST_FLOAT(m.m_fColumn[2][3],12, 0); EZ_TEST_FLOAT(m.m_fColumn[3][3],16, 0);
  }

  EZ_TEST_BLOCK(true, "GetTranspose")
  {
    ezMat4 m0(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat4 m = m0.GetTranspose();

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[1][0], 5, 0); EZ_TEST_FLOAT(m.m_fColumn[2][0], 9, 0); EZ_TEST_FLOAT(m.m_fColumn[3][0],13, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 2, 0); EZ_TEST_FLOAT(m.m_fColumn[1][1], 6, 0); EZ_TEST_FLOAT(m.m_fColumn[2][1],10, 0); EZ_TEST_FLOAT(m.m_fColumn[3][1],14, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 3, 0); EZ_TEST_FLOAT(m.m_fColumn[1][2], 7, 0); EZ_TEST_FLOAT(m.m_fColumn[2][2],11, 0); EZ_TEST_FLOAT(m.m_fColumn[3][2],15, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][3], 4, 0); EZ_TEST_FLOAT(m.m_fColumn[1][3], 8, 0); EZ_TEST_FLOAT(m.m_fColumn[2][3],12, 0); EZ_TEST_FLOAT(m.m_fColumn[3][3],16, 0);
  }

  EZ_TEST_BLOCK(true, "Invert")
  {
    for (float x = 1.0f; x < 360.0f; x += 10.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 17.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 23.0f)
        {
          ezMat4 m, inv;
          m.SetRotationMatrix(ezVec3(x, y, z).GetNormalized(), 19.0f);
          inv = m;
          EZ_TEST(inv.Invert() == EZ_SUCCESS);

          ezVec3 v = m * ezVec3(1, 1, 1);
          ezVec3 vinv = inv * v;

          EZ_TEST_VEC3(vinv, ezVec3(1, 1, 1), 0.0001f);
        }
      }
    }
  }

  EZ_TEST_BLOCK(true, "GetInverse")
  {
    for (float x = 1.0f; x < 360.0f; x += 9.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 19.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 21.0f)
        {
          ezMat4 m, inv;
          m.SetRotationMatrix(ezVec3(x, y, z).GetNormalized(), 83.0f);
          inv = m.GetInverse();

          ezVec3 v = m * ezVec3(1, 1, 1);
          ezVec3 vinv = inv * v;

          EZ_TEST_VEC3(vinv, ezVec3(1, 1, 1), 0.0001f);
        }
      }
    }
  }

  EZ_TEST_BLOCK(true, "IsZero")
  {
    ezMat4 m;

    m.SetIdentity();
    EZ_TEST(!m.IsZero());

    m.SetZero();
    EZ_TEST(m.IsZero());
  }

  EZ_TEST_BLOCK(true, "IsIdentity")
  {
    ezMat4 m;

    m.SetIdentity();
    EZ_TEST(m.IsIdentity());

    m.SetZero();
    EZ_TEST(!m.IsIdentity());
  }

  EZ_TEST_BLOCK(true, "IsValid")
  {
    ezMat4 m;

    m.SetZero();
    EZ_TEST(m.IsValid());

    m.m_fElementsCM[0] = ezMath::NaN();
    EZ_TEST(!m.IsValid());
  }

  EZ_TEST_BLOCK(true, "GetRow")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    EZ_TEST_VEC4(m.GetRow(0), ezVec4(1, 2, 3, 4), 0.0f);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4(5, 6, 7, 8), 0.0f);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4(9,10,11,12), 0.0f);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4(13,14,15,16), 0.0f);
  }

  EZ_TEST_BLOCK(true, "SetRow")
  {
    ezMat4 m;
    m.SetZero();

    m.SetRow(0, ezVec4(1, 2, 3, 4));
    EZ_TEST_VEC4(m.GetRow(0), ezVec4(1, 2, 3, 4), 0.0f);

    m.SetRow(1, ezVec4(5, 6, 7, 8));
    EZ_TEST_VEC4(m.GetRow(1), ezVec4(5, 6, 7, 8), 0.0f);

    m.SetRow(2, ezVec4(9, 10, 11, 12));
    EZ_TEST_VEC4(m.GetRow(2), ezVec4(9,10,11,12), 0.0f);

    m.SetRow(3, ezVec4(13, 14, 15, 16));
    EZ_TEST_VEC4(m.GetRow(3), ezVec4(13, 14, 15, 16), 0.0f);
  }

  EZ_TEST_BLOCK(true, "GetColumn")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    EZ_TEST_VEC4(m.GetColumn(0), ezVec4(1, 5, 9, 13), 0.0f);
    EZ_TEST_VEC4(m.GetColumn(1), ezVec4(2, 6, 10, 14), 0.0f);
    EZ_TEST_VEC4(m.GetColumn(2), ezVec4(3, 7, 11, 15), 0.0f);
    EZ_TEST_VEC4(m.GetColumn(3), ezVec4(4, 8, 12, 16), 0.0f);
  }

  EZ_TEST_BLOCK(true, "SetColumn")
  {
    ezMat4 m;
    m.SetZero();

    m.SetColumn(0, ezVec4(1, 2, 3, 4));
    EZ_TEST_VEC4(m.GetColumn(0), ezVec4(1, 2, 3, 4), 0.0f);

    m.SetColumn(1, ezVec4(5, 6, 7, 8));
    EZ_TEST_VEC4(m.GetColumn(1), ezVec4(5, 6, 7, 8), 0.0f);

    m.SetColumn(2, ezVec4(9, 10, 11, 12));
    EZ_TEST_VEC4(m.GetColumn(2), ezVec4(9,10,11,12), 0.0f);

    m.SetColumn(3, ezVec4(13, 14, 15, 16));
    EZ_TEST_VEC4(m.GetColumn(3), ezVec4(13, 14, 15, 16), 0.0f);
  }

  EZ_TEST_BLOCK(true, "GetDiagonal")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    EZ_TEST_VEC4(m.GetDiagonal(), ezVec4(1, 6, 11, 16), 0.0f);
  }

  EZ_TEST_BLOCK(true, "SetDiagonal")
  {
    ezMat4 m;
    m.SetZero();

    m.SetDiagonal(ezVec4(1, 2, 3, 4));
    EZ_TEST_VEC4(m.GetColumn(0), ezVec4(1, 0, 0, 0), 0.0f);
    EZ_TEST_VEC4(m.GetColumn(1), ezVec4(0, 2, 0, 0), 0.0f);
    EZ_TEST_VEC4(m.GetColumn(2), ezVec4(0, 0, 3, 0), 0.0f);
    EZ_TEST_VEC4(m.GetColumn(3), ezVec4(0, 0, 0, 4), 0.0f);
  }

  EZ_TEST_BLOCK(true, "GetTranslationVector")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    EZ_TEST_VEC3(m.GetTranslationVector(), ezVec3(4, 8, 12), 0.0f);
  }

  EZ_TEST_BLOCK(true, "SetTranslationVector")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    m.SetTranslationVector(ezVec3(17, 18, 19));
    EZ_TEST_VEC4(m.GetRow(0), ezVec4(1, 2, 3, 17), 0.0f);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4(5, 6, 7, 18), 0.0f);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4(9,10,11, 19), 0.0f);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4(13,14,15,16), 0.0f);
  }

  EZ_TEST_BLOCK(true, "SetRotationalPart")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat3 r(17, 18, 19,
             20, 21, 22,
             23, 24, 25);

    m.SetRotationalPart(r);
    EZ_TEST_VEC4(m.GetRow(0), ezVec4(17, 18, 19, 4), 0.0f);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4(20, 21, 22, 8), 0.0f);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4(23, 24, 25, 12), 0.0f);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4(13, 14, 15, 16), 0.0f);
  }

  EZ_TEST_BLOCK(true, "GetRotationalPart")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat3 r = m.GetRotationalPart();
    EZ_TEST_VEC3(r.GetRow(0), ezVec3(1, 2, 3), 0.0f);
    EZ_TEST_VEC3(r.GetRow(1), ezVec3(5, 6, 7), 0.0f);
    EZ_TEST_VEC3(r.GetRow(2), ezVec3(9, 10, 11), 0.0f);
  }

  EZ_TEST_BLOCK(true, "GetScalingFactors")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezVec3 s = m.GetScalingFactors();
    EZ_TEST_VEC3(s, ezVec3(ezMath::Sqrt(1*1 + 5*5 + 9*9), ezMath::Sqrt(2*2 + 6*6 + 10*10), ezMath::Sqrt(3*3 + 7*7 + 11*11)), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "SetScalingFactors")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    EZ_TEST(m.SetScalingFactors(ezVec3(1, 2, 3)) == EZ_SUCCESS);

    ezVec3 s = m.GetScalingFactors();
    EZ_TEST_VEC3(s, ezVec3(1, 2, 3), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "TransformDirection")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    const ezVec3 r = m.TransformDirection(ezVec3(1, 2, 3));

    EZ_TEST_VEC3(r, ezVec3(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "TransformDirection(array)")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezVec3 data[3] = { ezVec3(1, 2, 3), ezVec3(4, 5, 6), ezVec3(7, 8, 9) };

    m.TransformDirection(data, 2);

    EZ_TEST_VEC3(data[0], ezVec3(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), 0.0001f);
    EZ_TEST_VEC3(data[1], ezVec3(4 * 1 + 5 * 2 + 6 * 3, 4 * 5 + 5 * 6 + 6 * 7, 4 * 9 + 5 * 10 + 6 * 11), 0.0001f);
    EZ_TEST_VEC3(data[2], ezVec3(7, 8, 9), 0);
  }

  EZ_TEST_BLOCK(true, "TransformPosition")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    const ezVec3 r = m.TransformPosition(ezVec3(1, 2, 3));

    EZ_TEST_VEC3(r, ezVec3(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "TransformPosition(array)")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezVec3 data[3] = { ezVec3(1, 2, 3), ezVec3(4, 5, 6), ezVec3(7, 8, 9) };

    m.TransformPosition(data, 2);

    EZ_TEST_VEC3(data[0], ezVec3(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001f);
    EZ_TEST_VEC3(data[1], ezVec3(4 * 1 + 5 * 2 + 6 * 3 + 4, 4 * 5 + 5 * 6 + 6 * 7 + 8, 4 * 9 + 5 * 10 + 6 * 11 + 12), 0.0001f);
    EZ_TEST_VEC3(data[2], ezVec3(7, 8, 9), 0);
  }

  EZ_TEST_BLOCK(true, "Transform")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    const ezVec4 r = m.Transform(ezVec4(1, 2, 3, 4));

    EZ_TEST_VEC4(r, ezVec4(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 8 * 4, 1 * 9 + 2 * 10 + 3 * 11 + 12 * 4, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "Transform(array)")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezVec4 data[3] = { ezVec4(1, 2, 3, 4), ezVec4(5, 6, 7, 8), ezVec4(9, 10, 11, 12) };

    m.Transform(data, 2);

    EZ_TEST_VEC4(data[0], ezVec4(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 8 * 4, 1 * 9 + 2 * 10 + 3 * 11 + 12 * 4, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16), 0.0001f);
    EZ_TEST_VEC4(data[1], ezVec4(5 * 1 + 6 * 2 + 7 * 3 + 8 * 4, 5 * 5 + 6 * 6 + 7 * 7 + 8 * 8, 5 * 9 + 6 * 10 + 7 * 11 + 12 * 8, 5 * 13 + 6 * 14 + 7 * 15 + 8 * 16), 0.0001f);
    EZ_TEST_VEC4(data[2], ezVec4(9, 10, 11, 12), 0);
  }

  EZ_TEST_BLOCK(true, "operator*=")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    m *= 2.0f;

    EZ_TEST_VEC4(m.GetRow(0), ezVec4(2, 4, 6, 8), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4(10, 12, 14, 16), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4(18, 20, 22, 24), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4(26, 28, 30, 32), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "operator/=")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    m *= 4.0f;
    m /= 2.0f;

    EZ_TEST_VEC4(m.GetRow(0), ezVec4(2, 4, 6, 8), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4(10, 12, 14, 16), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4(18, 20, 22, 24), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4(26, 28, 30, 32), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "IsIdentical")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat4 m2 = m;

    EZ_TEST(m.IsIdentical(m2));

    m2.m_fElementsCM[0] += 0.00001f;
    EZ_TEST(!m.IsIdentical(m2));
  }

  EZ_TEST_BLOCK(true, "IsEqual")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat4 m2 = m;

    EZ_TEST(m.IsEqual(m2, 0.0001f));

    m2.m_fElementsCM[0] += 0.00001f;
    EZ_TEST(m.IsEqual(m2, 0.0001f));
    EZ_TEST(!m.IsEqual(m2, 0.000001f));
  }

  EZ_TEST_BLOCK(true, "operator*(mat, mat)")
  {
    ezMat4 m1(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat4 m2(-1, -2, -3, -4,
              -5, -6, -7, -8,
              -9,-10,-11,-12,
              -13,-14,-15,-16);

    ezMat4 r = m1 * m2;

    EZ_TEST_VEC4(r.GetColumn(0), ezVec4(-1 * 1 + -5 * 2 + -9 * 3 + -13 * 4,  -1 * 5 + -5 * 6 + -9 * 7 + -13 * 8, -1 * 9 + -5 * 10 + -9 * 11 + -13 * 12, -1 * 13 + -5 * 14 + -9 * 15 + -13 * 16), 0.001f);
    EZ_TEST_VEC4(r.GetColumn(1), ezVec4(-2 * 1 + -6 * 2 + -10 * 3 + -14 * 4, -2 * 5 + -6 * 6 + -10 * 7 + -14 * 8, -2 * 9 + -6 * 10 + -10 * 11 + -14 * 12, -2 * 13 + -6 * 14 + -10 * 15 + -14 * 16), 0.001f);
    EZ_TEST_VEC4(r.GetColumn(2), ezVec4(-3 * 1 + -7 * 2 + -11 * 3 + -15 * 4, -3 * 5 + -7 * 6 + -11 * 7 + -15 * 8, -3 * 9 + -7 * 10 + -11 * 11 + -15 * 12, -3 * 13 + -7 * 14 + -11 * 15 + -15 * 16), 0.001f);
    EZ_TEST_VEC4(r.GetColumn(3), ezVec4(-4 * 1 + -8 * 2 + -12 * 3 + -16 * 4, -4 * 5 + -8 * 6 + -12 * 7 + -16 * 8, -4 * 9 + -8 * 10 + -12 * 11 + -16 * 12, -4 * 13 + -8 * 14 + -12 * 15 + -16 * 16), 0.001f);
  }

  EZ_TEST_BLOCK(true, "operator*(mat, vec3)")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    const ezVec3 r = m * ezVec3(1, 2, 3);

    EZ_TEST_VEC3(r, ezVec3(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "operator*(mat, vec4)")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    const ezVec4 r = m * ezVec4(1, 2, 3, 4);

    EZ_TEST_VEC4(r, ezVec4(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 4 * 8, 1 * 9 + 2 * 10 + 3 * 11 + 4 * 12, 1* 13 + 2 * 14 + 3 * 15 + 4 * 16), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "operator*(mat, float) | operator*(float, mat)")
  {
    ezMat4 m0(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat4 m = m0 * 2.0f;
    ezMat4 m2 = 2.0f * m0;

    EZ_TEST_VEC4(m.GetRow(0), ezVec4(2, 4, 6, 8), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4(10, 12, 14, 16), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4(18, 20, 22, 24), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4(26, 28, 30, 32), 0.0001f);

    EZ_TEST_VEC4(m2.GetRow(0), ezVec4(2, 4, 6, 8), 0.0001f);
    EZ_TEST_VEC4(m2.GetRow(1), ezVec4(10, 12, 14, 16), 0.0001f);
    EZ_TEST_VEC4(m2.GetRow(2), ezVec4(18, 20, 22, 24), 0.0001f);
    EZ_TEST_VEC4(m2.GetRow(3), ezVec4(26, 28, 30, 32), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "operator/(mat, float)")
  {
    ezMat4 m0(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    m0 *= 4.0f;
    
    ezMat4 m = m0 / 2.0f;

    EZ_TEST_VEC4(m.GetRow(0), ezVec4(2, 4, 6, 8), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4(10, 12, 14, 16), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4(18, 20, 22, 24), 0.0001f);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4(26, 28, 30, 32), 0.0001f);
  }

  EZ_TEST_BLOCK(true, "operator+(mat, mat) | operator-(mat, mat)")
  {
    ezMat4 m0(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);
  
    ezMat4 m1(-1, -2, -3, -4,
             -5, -6, -7, -8,
             -9,-10,-11,-12,
             -13,-14,-15,-16);

    EZ_TEST((m0 + m1).IsZero());
    EZ_TEST((m0 - m1).IsEqual(m0 * 2.0f, 0.0001f));
  }

  EZ_TEST_BLOCK(true, "operator== (mat, mat) | operator!= (mat, mat)")
  {
    ezMat4 m(1, 2, 3, 4,
             5, 6, 7, 8,
             9,10,11,12,
             13,14,15,16);

    ezMat4 m2 = m;

    EZ_TEST(m == m2);

    m2.m_fElementsCM[0] += 0.00001f;

    EZ_TEST(m != m2);
  }

}