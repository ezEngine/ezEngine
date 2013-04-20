#include <PCH.h>

EZ_CREATE_SIMPLE_TEST(Math, Mat3)
{



  EZ_TEST_BLOCK(true, "Default Constructor")
  {
    #if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
        // In debug the default constructor initializes everything with NaN.
        ezMat3 m;
        EZ_TEST(ezMath::IsNaN(m.m_fElementsCM[0]) && 
                ezMath::IsNaN(m.m_fElementsCM[1]) && 
                ezMath::IsNaN(m.m_fElementsCM[2]) && 
                ezMath::IsNaN(m.m_fElementsCM[3]) && 
                ezMath::IsNaN(m.m_fElementsCM[4]) && 
                ezMath::IsNaN(m.m_fElementsCM[5]) && 
                ezMath::IsNaN(m.m_fElementsCM[6]) && 
                ezMath::IsNaN(m.m_fElementsCM[7]) && 
                ezMath::IsNaN(m.m_fElementsCM[8]));
    #else
        // Placement new of the default constructor should not have any effect on the previous data.
        float testBlock[9] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
        ezMat3* p = ::new ((void*) &testBlock[0]) ezMat3;

        EZ_TEST(m.m_fElementsCM[0] == 1.0f && 
                m.m_fElementsCM[1] == 2.0f && 
                m.m_fElementsCM[2] == 3.0f && 
                m.m_fElementsCM[3] == 4.0f && 
                m.m_fElementsCM[4] == 5.0f && 
                m.m_fElementsCM[5] == 6.0f && 
                m.m_fElementsCM[6] == 7.0f && 
                m.m_fElementsCM[7] == 8.0f && 
                m.m_fElementsCM[8] == 9.0f);
    #endif
  }

  EZ_TEST_BLOCK(true, "Constructor (Array Data)")
  {
    const float data[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    {
      ezMat3 m(data, ezMatrixLayout::ColumnMajor);

      EZ_TEST(m.m_fElementsCM[0] == 1.0f && 
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
      ezMat3 m(data, ezMatrixLayout::RowMajor);

      EZ_TEST(m.m_fElementsCM[0] == 1.0f && 
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

  EZ_TEST_BLOCK(true, "Constructor (Elements)")
  {
    ezMat3 m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 1, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][0], 2, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][0], 3, 0.00001f);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 4, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][1], 5, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][1], 6, 0.00001f);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 7, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][2], 8, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][2], 9, 0.00001f);
  }

  EZ_TEST_BLOCK(true, "SetFromArray")
  {
    const float data[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    {
      ezMat3 m;
      m.SetFromArray(data, ezMatrixLayout::ColumnMajor);

      EZ_TEST(m.m_fElementsCM[0] == 1.0f && 
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
      ezMat3 m;
      m.SetFromArray(data, ezMatrixLayout::RowMajor);

      EZ_TEST(m.m_fElementsCM[0] == 1.0f && 
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

  EZ_TEST_BLOCK(true, "SetElements")
  {
    ezMat3 m;
    m.SetElements (1, 2, 3,
                   4, 5, 6,
                   7, 8, 9);

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 1, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][0], 2, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][0], 3, 0.00001f);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 4, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][1], 5, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][1], 6, 0.00001f);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 7, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][2], 8, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][2], 9, 0.00001f);
  }

  EZ_TEST_BLOCK(true, "GetAsArray")
  {
    ezMat3 m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    float data[9];

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

  EZ_TEST_BLOCK(true, "SetZero")
  {
    ezMat3 m;
    m.SetZero();

    for (ezUInt32 i = 0; i < 9; ++i)
      EZ_TEST_FLOAT(m.m_fElementsCM[i], 0.0f, 0.0f);
  }

  EZ_TEST_BLOCK(true, "SetIdentity")
  {
    ezMat3 m;
    m.SetIdentity();

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[1][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][0], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][1], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[2][1], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][2], 1, 0);
  }

  EZ_TEST_BLOCK(true, "SetScalingMatrix")
  {
    ezMat3 m;
    m.SetScalingMatrix(ezVec3(2, 3, 4));

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 2, 0); EZ_TEST_FLOAT(m.m_fColumn[1][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][0], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][1], 3, 0); EZ_TEST_FLOAT(m.m_fColumn[2][1], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][2], 4, 0);
  }

  EZ_TEST_BLOCK(true, "SetRotationMatrixX")
  {
    ezMat3 m;

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
    ezMat3 m;

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
    ezMat3 m;

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
    ezMat3 m;

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
    ezMat3 m = ezMat3::IdentityMatrix();

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[1][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][0], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][1], 1, 0); EZ_TEST_FLOAT(m.m_fColumn[2][1], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][2], 1, 0);
  }

  EZ_TEST_BLOCK(true, "ZeroMatrix")
  {
    ezMat3 m = ezMat3::ZeroMatrix();

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][0], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][0], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][1], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][1], 0, 0);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[1][2], 0, 0); EZ_TEST_FLOAT(m.m_fColumn[2][2], 0, 0);
  }

  EZ_TEST_BLOCK(true, "Transpose")
  {
    ezMat3 m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    m.Transpose();

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 1, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][0], 4, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][0], 7, 0.00001f);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 2, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][1], 5, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][1], 8, 0.00001f);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 3, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][2], 6, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][2], 9, 0.00001f);
  }

  EZ_TEST_BLOCK(true, "GetTranspose")
  {
    ezMat3 m0(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    ezMat3 m = m0.GetTranspose();

    EZ_TEST_FLOAT(m.m_fColumn[0][0], 1, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][0], 4, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][0], 7, 0.00001f);
    EZ_TEST_FLOAT(m.m_fColumn[0][1], 2, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][1], 5, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][1], 8, 0.00001f);
    EZ_TEST_FLOAT(m.m_fColumn[0][2], 3, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[1][2], 6, 0.00001f); EZ_TEST_FLOAT(m.m_fColumn[2][2], 9, 0.00001f);
  }

  EZ_TEST_BLOCK(true, "Invert")
  {
    for (float x = 1.0f; x < 360.0f; x += 10.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 17.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 23.0f)
        {
          ezMat3 m, inv;
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
          ezMat3 m, inv;
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
    ezMat3 m;

    m.SetIdentity();
    EZ_TEST(!m.IsZero());

    m.SetZero();
    EZ_TEST(m.IsZero());
  }

  EZ_TEST_BLOCK(true, "IsIdentity")
  {
    ezMat3 m;

    m.SetIdentity();
    EZ_TEST(m.IsIdentity());

    m.SetZero();
    EZ_TEST(!m.IsIdentity());
  }

  EZ_TEST_BLOCK(true, "IsValid")
  {
    ezMat3 m;

    m.SetZero();
    EZ_TEST(m.IsValid());

    m.m_fElementsCM[0] = ezMath::NaN();
    EZ_TEST(!m.IsValid());
  }

  EZ_TEST_BLOCK(true, "GetRow")
  {
    ezMat3 m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    EZ_TEST_VEC3(m.GetRow(0), ezVec3(1, 2, 3), 0.0f);
    EZ_TEST_VEC3(m.GetRow(1), ezVec3(4, 5, 6), 0.0f);
    EZ_TEST_VEC3(m.GetRow(2), ezVec3(7, 8, 9), 0.0f);
  }

  EZ_TEST_BLOCK(true, "SetRow")
  {
    ezMat3 m;
    m.SetZero();

    m.SetRow(0, ezVec3(1, 2, 3));
    EZ_TEST_VEC3(m.GetRow(0), ezVec3(1, 2, 3), 0.0f);

    m.SetRow(1, ezVec3(4, 5, 6));
    EZ_TEST_VEC3(m.GetRow(1), ezVec3(4, 5, 6), 0.0f);

    m.SetRow(2, ezVec3(7, 8, 9));
    EZ_TEST_VEC3(m.GetRow(2), ezVec3(7, 8, 9), 0.0f);
  }

  EZ_TEST_BLOCK(true, "GetColumn")
  {
    ezMat3 m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    EZ_TEST_VEC3(m.GetColumn(0), ezVec3(1, 4, 7), 0.0f);
    EZ_TEST_VEC3(m.GetColumn(1), ezVec3(2, 5, 8), 0.0f);
    EZ_TEST_VEC3(m.GetColumn(2), ezVec3(3, 6, 9), 0.0f);
  }

  EZ_TEST_BLOCK(true, "SetColumn")
  {
    ezMat3 m;
    m.SetZero();

    m.SetColumn(0, ezVec3(1, 2, 3));
    EZ_TEST_VEC3(m.GetColumn(0), ezVec3(1, 2, 3), 0.0f);

    m.SetColumn(1, ezVec3(4, 5, 6));
    EZ_TEST_VEC3(m.GetColumn(1), ezVec3(4, 5, 6), 0.0f);

    m.SetColumn(2, ezVec3(7, 8, 9));
    EZ_TEST_VEC3(m.GetColumn(2), ezVec3(7, 8, 9), 0.0f);
  }

  EZ_TEST_BLOCK(true, "GetDiagonal")
  {
    ezMat3 m(1, 2, 3,
             4, 5, 6,
             7, 8, 9);

    EZ_TEST_VEC3(m.GetDiagonal(), ezVec3(1, 5, 9), 0.0f);
  }

  EZ_TEST_BLOCK(true, "SetDiagonal")
  {
    ezMat3 m;
    m.SetZero();

    m.SetDiagonal(ezVec3(1, 2, 3));
    EZ_TEST_VEC3(m.GetColumn(0), ezVec3(1, 0, 0), 0.0f);
    EZ_TEST_VEC3(m.GetColumn(1), ezVec3(0, 2, 0), 0.0f);
    EZ_TEST_VEC3(m.GetColumn(2), ezVec3(0, 0, 3), 0.0f);
  }

}

