#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Implementation/AllClasses_inl.h>
#include <Foundation/Math/Mat3.h>

template <typename Type>
void TestMat3()
{
  using ezMat3Type = ezMat3Template<Type>;
  using ezVec3Type = ezVec3Template<Type>;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Default Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (ezMath::SupportsNaN<Type>())
    {
      // In debug the default constructor initializes everything with NaN.
      ezMat3Type m;
      EZ_TEST_BOOL(ezMath::IsNaN(m.m_fElementsCM[0]) && ezMath::IsNaN(m.m_fElementsCM[1]) && ezMath::IsNaN(m.m_fElementsCM[2]) &&
                   ezMath::IsNaN(m.m_fElementsCM[3]) && ezMath::IsNaN(m.m_fElementsCM[4]) && ezMath::IsNaN(m.m_fElementsCM[5]) &&
                   ezMath::IsNaN(m.m_fElementsCM[6]) && ezMath::IsNaN(m.m_fElementsCM[7]) && ezMath::IsNaN(m.m_fElementsCM[8]));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    Type testBlock[9] = {(Type)1, (Type)2, (Type)3, (Type)4, (Type)5, (Type)6, (Type)7, (Type)8, (Type)9};

    ezMat3Type* m = ::new ((void*)&testBlock[0]) ezMat3Type;

    EZ_TEST_BOOL(m->m_fElementsCM[0] == (Type)1 && m->m_fElementsCM[1] == (Type)2 && m->m_fElementsCM[2] == (Type)3 &&
                 m->m_fElementsCM[3] == (Type)4 && m->m_fElementsCM[4] == (Type)5 && m->m_fElementsCM[5] == (Type)6 &&
                 m->m_fElementsCM[6] == (Type)7 && m->m_fElementsCM[7] == (Type)8 && m->m_fElementsCM[8] == (Type)9);
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (Array Data)")
  {
    const Type data[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    {
      ezMat3Type m = ezMat3Type::MakeFromColumnMajorArray(data);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)2 && m.m_fElementsCM[2] == (Type)3 &&
                   m.m_fElementsCM[3] == (Type)4 && m.m_fElementsCM[4] == (Type)5 && m.m_fElementsCM[5] == (Type)6 &&
                   m.m_fElementsCM[6] == (Type)7 && m.m_fElementsCM[7] == (Type)8 && m.m_fElementsCM[8] == (Type)9);
    }

    {
      ezMat3Type m = ezMat3Type::MakeFromRowMajorArray(data);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)4 && m.m_fElementsCM[2] == (Type)7 &&
                   m.m_fElementsCM[3] == (Type)2 && m.m_fElementsCM[4] == (Type)5 && m.m_fElementsCM[5] == (Type)8 &&
                   m.m_fElementsCM[6] == (Type)3 && m.m_fElementsCM[7] == (Type)6 && m.m_fElementsCM[8] == (Type)9);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (Elements)")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    
        EZ_TEST_FLOAT(m.Element(0, 0), 1, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(1, 0), 2, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(2, 0), 3, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(0, 1), 4, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(1, 1), 5, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(2, 1), 6, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(0, 2), 7, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(1, 2), 8, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(2, 2), 9, ezMath::DefaultEpsilon<Type>());
      }
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromArray")
  {
    const Type data[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    {
      ezMat3Type m = ezMat3Type::MakeFromColumnMajorArray(data);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)2 && m.m_fElementsCM[2] == (Type)3 &&
                   m.m_fElementsCM[3] == (Type)4 && m.m_fElementsCM[4] == (Type)5 && m.m_fElementsCM[5] == (Type)6 &&
                   m.m_fElementsCM[6] == (Type)7 && m.m_fElementsCM[7] == (Type)8 && m.m_fElementsCM[8] == (Type)9);
    }

    {
      ezMat3Type m = ezMat3Type::MakeFromRowMajorArray(data);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)4 && m.m_fElementsCM[2] == (Type)7 &&
                   m.m_fElementsCM[3] == (Type)2 && m.m_fElementsCM[4] == (Type)5 && m.m_fElementsCM[5] == (Type)8 &&
                   m.m_fElementsCM[6] == (Type)3 && m.m_fElementsCM[7] == (Type)6 && m.m_fElementsCM[8] == (Type)9);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetElements")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    
        EZ_TEST_FLOAT(m.Element(0, 0), 1, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(1, 0), 2, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(2, 0), 3, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(0, 1), 4, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(1, 1), 5, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(2, 1), 6, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(0, 2), 7, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(1, 2), 8, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(2, 2), 9, ezMath::DefaultEpsilon<Type>());
      }
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsArray")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    Type data[9];

    m.GetAsArray(data, ezMatrixLayout::ColumnMajor);
    EZ_TEST_FLOAT(data[0], 1, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[1], 4, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[2], 7, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[3], 2, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[4], 5, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[5], 8, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[6], 3, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[7], 6, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[8], 9, ezMath::LargeEpsilon<Type>());

    m.GetAsArray(data, ezMatrixLayout::RowMajor);
    EZ_TEST_FLOAT(data[0], 1, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[1], 2, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[2], 3, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[3], 4, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[4], 5, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[5], 6, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[6], 7, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[7], 8, ezMath::LargeEpsilon<Type>());
    EZ_TEST_FLOAT(data[8], 9, ezMath::LargeEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetZero")
  {
    ezMat3Type m;
    m.SetZero();

    for (ezUInt32 i = 0; i < 9; ++i)
      EZ_TEST_FLOAT(m.m_fElementsCM[i], (Type)0, (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetIdentity")
  {
    ezMat3Type m;
    m.SetIdentity();

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0);
    EZ_TEST_FLOAT(m.Element(1, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 1), 1, 0);
    EZ_TEST_FLOAT(m.Element(2, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 2), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetScalingMatrix")
  {
    ezMat3Type m = ezMat3Type::MakeScaling(ezVec3Type(2, 3, 4));

    EZ_TEST_FLOAT(m.Element(0, 0), 2, 0);
    EZ_TEST_FLOAT(m.Element(1, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 1), 3, 0);
    EZ_TEST_FLOAT(m.Element(2, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 2), 4, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRotationMatrixX")
  {
    ezMat3Type m;

    m = ezMat3Type::MakeRotationX(ezAngleTemplate<Type>::MakeFromDegree(90));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, -3, 2), ezMath::LargeEpsilon<Type>()));

    m = ezMat3Type::MakeRotationX(ezAngleTemplate<Type>::MakeFromDegree(180));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, -2, -3), ezMath::LargeEpsilon<Type>()));

    m = ezMat3Type::MakeRotationX(ezAngleTemplate<Type>::MakeFromDegree(270));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, 3, -2), ezMath::LargeEpsilon<Type>()));

    m = ezMat3Type::MakeRotationX(ezAngleTemplate<Type>::MakeFromDegree(360));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, 2, 3), ezMath::LargeEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRotationMatrixY")
  {
    ezMat3Type m;

    m = ezMat3Type::MakeRotationY(ezAngleTemplate<Type>::MakeFromDegree(90));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(3, 2, -1), ezMath::LargeEpsilon<Type>()));

    m = ezMat3Type::MakeRotationY(ezAngleTemplate<Type>::MakeFromDegree(180));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-1, 2, -3), ezMath::LargeEpsilon<Type>()));

    m = ezMat3Type::MakeRotationY(ezAngleTemplate<Type>::MakeFromDegree(270));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-3, 2, 1), ezMath::LargeEpsilon<Type>()));

    m = ezMat3Type::MakeRotationY(ezAngleTemplate<Type>::MakeFromDegree(360));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, 2, 3), ezMath::LargeEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRotationMatrixZ")
  {
    ezMat3Type m;

    m = ezMat3Type::MakeRotationZ(ezAngleTemplate<Type>::MakeFromDegree(90));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-2, 1, 3), ezMath::LargeEpsilon<Type>()));

    m = ezMat3Type::MakeRotationZ(ezAngleTemplate<Type>::MakeFromDegree(180));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-1, -2, 3), ezMath::LargeEpsilon<Type>()));

    m = ezMat3Type::MakeRotationZ(ezAngleTemplate<Type>::MakeFromDegree(270));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(2, -1, 3), ezMath::LargeEpsilon<Type>()));

    m = ezMat3Type::MakeRotationZ(ezAngleTemplate<Type>::MakeFromDegree(360));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, 2, 3), ezMath::LargeEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRotationMatrix")
  {
    ezMat3Type m;

    m = ezMat3Type::MakeAxisRotation(ezVec3Type(1, 0, 0), ezAngleTemplate<Type>::MakeFromDegree(90));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, -3, 2), ezMath::DefaultEpsilon<Type>()));

    m = ezMat3Type::MakeAxisRotation(ezVec3Type(1, 0, 0), ezAngleTemplate<Type>::MakeFromDegree(180));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, -2, -3), ezMath::DefaultEpsilon<Type>()));

    m = ezMat3Type::MakeAxisRotation(ezVec3Type(1, 0, 0), ezAngleTemplate<Type>::MakeFromDegree(270));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, 3, -2), ezMath::DefaultEpsilon<Type>()));

    m = ezMat3Type::MakeAxisRotation(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(3, 2, -1), ezMath::DefaultEpsilon<Type>()));

    m = ezMat3Type::MakeAxisRotation(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(180));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-1, 2, -3), ezMath::DefaultEpsilon<Type>()));

    m = ezMat3Type::MakeAxisRotation(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(270));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-3, 2, 1), ezMath::DefaultEpsilon<Type>()));

    m = ezMat3Type::MakeAxisRotation(ezVec3Type(0, 0, 1), ezAngleTemplate<Type>::MakeFromDegree(90));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-2, 1, 3), ezMath::DefaultEpsilon<Type>()));

    m = ezMat3Type::MakeAxisRotation(ezVec3Type(0, 0, 1), ezAngleTemplate<Type>::MakeFromDegree(180));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-1, -2, 3), ezMath::DefaultEpsilon<Type>()));

    m = ezMat3Type::MakeAxisRotation(ezVec3Type(0, 0, 1), ezAngleTemplate<Type>::MakeFromDegree(270));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(2, -1, 3), ezMath::DefaultEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeIdentity")
  {
    ezMat3Type m = ezMat3Type::MakeIdentity();

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0);
    EZ_TEST_FLOAT(m.Element(1, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 1), 1, 0);
    EZ_TEST_FLOAT(m.Element(2, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 2), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeZero")
  {
    ezMat3Type m = ezMat3Type::MakeZero();

    EZ_TEST_FLOAT(m.Element(0, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 2), 0, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transpose")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m.Transpose();

    
        EZ_TEST_FLOAT(m.Element(0, 0), 1, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(1, 0), 4, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(2, 0), 7, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(0, 1), 2, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(1, 1), 5, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(2, 1), 8, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(0, 2), 3, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(1, 2), 6, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(2, 2), 9, ezMath::DefaultEpsilon<Type>());
      }
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetTranspose")
  {
    ezMat3Type m0 = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    ezMat3Type m = m0.GetTranspose();

    
        EZ_TEST_FLOAT(m.Element(0, 0), 1, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(1, 0), 4, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(2, 0), 7, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(0, 1), 2, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(1, 1), 5, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(2, 1), 8, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(0, 2), 3, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(1, 2), 6, ezMath::DefaultEpsilon<Type>());
        EZ_TEST_FLOAT(m.Element(2, 2), 9, ezMath::DefaultEpsilon<Type>());
      }
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Invert")
  {
    for (Type x = (Type)1.0; x < (Type)360.0; x += (Type)10.0)
    {
      for (Type y = (Type)2.0; y < (Type)360.0; y += (Type)17.0)
      {
        for (Type z = (Type)3.0; z < (Type)360.0; z += (Type)23.0)
        {
          ezMat3Type m, inv;
          m = ezMat3Type::MakeAxisRotation(ezVec3Type(x, y, z).GetNormalized(), ezAngleTemplate<Type>::MakeFromDegree((Type)19.0));
          inv = m;
          EZ_TEST_BOOL(inv.Invert() == EZ_SUCCESS);

          ezVec3Type v = m * ezVec3Type(1, 1, 1);
          ezVec3Type vinv = inv * v;

          EZ_TEST_VEC3(vinv, ezVec3Type(1, 1, 1), ezMath::DefaultEpsilon<Type>());
        }
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetInverse")
  {
    for (Type x = (Type)1.0; x < (Type)360.0; x += (Type)9.0)
    {
      for (Type y = (Type)2.0; y < (Type)360.0; y += (Type)19.0)
      {
        for (Type z = (Type)3.0; z < (Type)360.0; z += (Type)21.0)
        {
          ezMat3Type m, inv;
          m = ezMat3Type::MakeAxisRotation(ezVec3Type(x, y, z).GetNormalized(), ezAngleTemplate<Type>::MakeFromDegree((Type)83.0));
          inv = m.GetInverse();

          ezVec3Type v = m * ezVec3Type(1, 1, 1);
          ezVec3Type vinv = inv * v;

          EZ_TEST_VEC3(vinv, ezVec3Type(1, 1, 1), ezMath::DefaultEpsilon<Type>());
        }
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsZero")
  {
    ezMat3Type m;

    m.SetIdentity();
    EZ_TEST_BOOL(!m.IsZero());

    m.SetZero();
    EZ_TEST_BOOL(m.IsZero());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentity")
  {
    ezMat3Type m;

    m.SetIdentity();
    EZ_TEST_BOOL(m.IsIdentity());

    m.SetZero();
    EZ_TEST_BOOL(!m.IsIdentity());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsValid")
  {
    if (ezMath::SupportsNaN<Type>())
    {
      ezMat3Type m;

      m.SetZero();
      EZ_TEST_BOOL(m.IsValid());

      m.m_fElementsCM[0] = ezMath::NaN<Type>();
      EZ_TEST_BOOL(!m.IsValid());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRow")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    EZ_TEST_VEC3(m.GetRow(0), ezVec3Type(1, 2, 3), (Type)0);
    EZ_TEST_VEC3(m.GetRow(1), ezVec3Type(4, 5, 6), (Type)0);
    EZ_TEST_VEC3(m.GetRow(2), ezVec3Type(7, 8, 9), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRow")
  {
    ezMat3Type m;
    m.SetZero();

    m.SetRow(0, ezVec3Type(1, 2, 3));
    EZ_TEST_VEC3(m.GetRow(0), ezVec3Type(1, 2, 3), (Type)0);

    m.SetRow(1, ezVec3Type(4, 5, 6));
    EZ_TEST_VEC3(m.GetRow(1), ezVec3Type(4, 5, 6), (Type)0);

    m.SetRow(2, ezVec3Type(7, 8, 9));
    EZ_TEST_VEC3(m.GetRow(2), ezVec3Type(7, 8, 9), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetColumn")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    EZ_TEST_VEC3(m.GetColumn(0), ezVec3Type(1, 4, 7), (Type)0);
    EZ_TEST_VEC3(m.GetColumn(1), ezVec3Type(2, 5, 8), (Type)0);
    EZ_TEST_VEC3(m.GetColumn(2), ezVec3Type(3, 6, 9), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetColumn")
  {
    ezMat3Type m;
    m.SetZero();

    m.SetColumn(0, ezVec3Type(1, 2, 3));
    EZ_TEST_VEC3(m.GetColumn(0), ezVec3Type(1, 2, 3), (Type)0);

    m.SetColumn(1, ezVec3Type(4, 5, 6));
    EZ_TEST_VEC3(m.GetColumn(1), ezVec3Type(4, 5, 6), (Type)0);

    m.SetColumn(2, ezVec3Type(7, 8, 9));
    EZ_TEST_VEC3(m.GetColumn(2), ezVec3Type(7, 8, 9), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDiagonal")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    EZ_TEST_VEC3(m.GetDiagonal(), ezVec3Type(1, 5, 9), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetDiagonal")
  {
    ezMat3Type m;
    m.SetZero();

    m.SetDiagonal(ezVec3Type(1, 2, 3));
    EZ_TEST_VEC3(m.GetColumn(0), ezVec3Type(1, 0, 0), (Type)0);
    EZ_TEST_VEC3(m.GetColumn(1), ezVec3Type(0, 2, 0), (Type)0);
    EZ_TEST_VEC3(m.GetColumn(2), ezVec3Type(0, 0, 3), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetScalingFactors")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 5, 6, 7, 9, 10, 11);

    ezVec3Type s = m.GetScalingFactors();
    EZ_TEST_VEC3(s,
      ezVec3Type(ezMath::Sqrt((Type)(1 * 1 + 5 * 5 + 9 * 9)), ezMath::Sqrt((Type)(2 * 2 + 6 * 6 + 10 * 10)),
        ezMath::Sqrt((Type)(3 * 3 + 7 * 7 + 11 * 11))),
      ezMath::LargeEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetScalingFactors")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 5, 6, 7, 9, 10, 11);

    EZ_TEST_BOOL(m.SetScalingFactors(ezVec3Type(1, 2, 3)) == EZ_SUCCESS);

    ezVec3Type s = m.GetScalingFactors();
    EZ_TEST_VEC3(s, ezVec3Type(1, 2, 3), ezMath::LargeEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformDirection")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    const ezVec3Type r = m.TransformDirection(ezVec3Type(1, 2, 3));

    EZ_TEST_VEC3(r, ezVec3Type(1 * 1 + 2 * 2 + 3 * 3, 1 * 4 + 2 * 5 + 3 * 6, 1 * 7 + 2 * 8 + 3 * 9), ezMath::LargeEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*=")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m *= (Type)2;

    EZ_TEST_VEC3(m.GetRow(0), ezVec3Type(2, 4, 6), ezMath::LargeEpsilon<Type>());
    EZ_TEST_VEC3(m.GetRow(1), ezVec3Type(8, 10, 12), ezMath::LargeEpsilon<Type>());
    EZ_TEST_VEC3(m.GetRow(2), ezVec3Type(14, 16, 18), ezMath::LargeEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/=")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m *= (Type)4;
    m /= (Type)2;

    EZ_TEST_VEC3(m.GetRow(0), ezVec3Type(2, 4, 6), ezMath::LargeEpsilon<Type>());
    EZ_TEST_VEC3(m.GetRow(1), ezVec3Type(8, 10, 12), ezMath::LargeEpsilon<Type>());
    EZ_TEST_VEC3(m.GetRow(2), ezVec3Type(14, 16, 18), ezMath::LargeEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    ezMat3Type m2 = m;

    EZ_TEST_BOOL(m.IsIdentical(m2));

    m2.m_fElementsCM[0] += ezMath::DefaultEpsilon<Type>();
    EZ_TEST_BOOL(!m.IsIdentical(m2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    ezMat3Type m2 = m;

    EZ_TEST_BOOL(m.IsEqual(m2, ezMath::LargeEpsilon<Type>()));

    m2.m_fElementsCM[0] += ezMath::DefaultEpsilon<Type>();
    EZ_TEST_BOOL(m.IsEqual(m2, ezMath::LargeEpsilon<Type>()));
    EZ_TEST_BOOL(!m.IsEqual(m2, ezMath::SmallEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(mat, mat)")
  {
    ezMat3Type m1 = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    ezMat3Type m2 = ezMat3Type::MakeFromValues(-1, -2, -3, -4, -5, -6, -7, -8, -9);

    ezMat3Type r = m1 * m2;

    EZ_TEST_VEC3(r.GetColumn(0), ezVec3Type(-1 * 1 + -4 * 2 + -7 * 3, -1 * 4 + -4 * 5 + -7 * 6, -1 * 7 + -4 * 8 + -7 * 9), ezMath::LargeEpsilon<Type>());
    EZ_TEST_VEC3(r.GetColumn(1), ezVec3Type(-2 * 1 + -5 * 2 + -8 * 3, -2 * 4 + -5 * 5 + -8 * 6, -2 * 7 + -5 * 8 + -8 * 9), ezMath::LargeEpsilon<Type>());
    EZ_TEST_VEC3(r.GetColumn(2), ezVec3Type(-3 * 1 + -6 * 2 + -9 * 3, -3 * 4 + -6 * 5 + -9 * 6, -3 * 7 + -6 * 8 + -9 * 9), ezMath::LargeEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(mat, vec)")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    const ezVec3Type r = m * (ezVec3Type(1, 2, 3));

    EZ_TEST_VEC3(r, ezVec3Type(1 * 1 + 2 * 2 + 3 * 3, 1 * 4 + 2 * 5 + 3 * 6, 1 * 7 + 2 * 8 + 3 * 9), ezMath::LargeEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(mat, float) | operator*(float, mat)")
  {
    ezMat3Type m0 = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    ezMat3Type m = m0 * (Type)2;
    ezMat3Type m2 = (Type)2 * m0;

    EZ_TEST_VEC3(m.GetRow(0), ezVec3Type(2, 4, 6), ezMath::LargeEpsilon<Type>());
    EZ_TEST_VEC3(m.GetRow(1), ezVec3Type(8, 10, 12), ezMath::LargeEpsilon<Type>());
    EZ_TEST_VEC3(m.GetRow(2), ezVec3Type(14, 16, 18), ezMath::LargeEpsilon<Type>());

    EZ_TEST_VEC3(m2.GetRow(0), ezVec3Type(2, 4, 6), ezMath::LargeEpsilon<Type>());
    EZ_TEST_VEC3(m2.GetRow(1), ezVec3Type(8, 10, 12), ezMath::LargeEpsilon<Type>());
    EZ_TEST_VEC3(m2.GetRow(2), ezVec3Type(14, 16, 18), ezMath::LargeEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/(mat, float)")
  {
    ezMat3Type m0 = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m0 *= (Type)4;

    ezMat3Type m = m0 / (Type)2;

    EZ_TEST_VEC3(m.GetRow(0), ezVec3Type(2, 4, 6), ezMath::LargeEpsilon<Type>());
    EZ_TEST_VEC3(m.GetRow(1), ezVec3Type(8, 10, 12), ezMath::LargeEpsilon<Type>());
    EZ_TEST_VEC3(m.GetRow(2), ezVec3Type(14, 16, 18), ezMath::LargeEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+(mat, mat) | operator-(mat, mat)")
  {
    ezMat3Type m0 = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    ezMat3Type m1 = ezMat3Type::MakeFromValues(-1, -2, -3, -4, -5, -6, -7, -8, -9);

    EZ_TEST_BOOL((m0 + m1).IsZero());
    EZ_TEST_BOOL((m0 - m1).IsEqual(m0 * (Type)2, ezMath::LargeEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator== (mat, mat) | operator!= (mat, mat)")
  {
    ezMat3Type m = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);

    ezMat3Type m2 = m;

    EZ_TEST_BOOL(m == m2);

    m2.m_fElementsCM[0] += ezMath::DefaultEpsilon<Type>();

    EZ_TEST_BOOL(m != m2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::SupportsNaN<Type>())
    {
      ezMat3Type m;

      m.SetIdentity();
      EZ_TEST_BOOL(!m.IsNaN());

      for (ezUInt32 i = 0; i < 9; ++i)
      {
        m.SetIdentity();
        m.m_fElementsCM[i] = ezMath::NaN<Type>();

        EZ_TEST_BOOL(m.IsNaN());
      }
    }
  }
}


EZ_CREATE_SIMPLE_TEST(Math, Mat3f)
{
  TestMat3<float>();
}
EZ_CREATE_SIMPLE_TEST(Math, Mat3d)
{
  TestMat3<double>();
}
