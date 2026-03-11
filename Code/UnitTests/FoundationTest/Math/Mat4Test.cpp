#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Implementation/AllClasses_inl.h>
#include <Foundation/Math/Mat4.h>

template <typename Type>
void TestMat4()
{
  using ezMat4Type = ezMat4Template<Type>;
  using ezMat3Type = ezMat3Template<Type>;
  using ezVec3Type = ezVec3Template<Type>;
  using ezVec4Type = ezVec4Template<Type>;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Default Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (ezMath::SupportsNaN<Type>())
    {
      // In debug the default constructor initializes everything with NaN.
      ezMat4Type m;
      EZ_TEST_BOOL(ezMath::IsNaN(m.m_fElementsCM[0]) && ezMath::IsNaN(m.m_fElementsCM[1]) && ezMath::IsNaN(m.m_fElementsCM[2]) &&
                   ezMath::IsNaN(m.m_fElementsCM[3]) && ezMath::IsNaN(m.m_fElementsCM[4]) && ezMath::IsNaN(m.m_fElementsCM[5]) &&
                   ezMath::IsNaN(m.m_fElementsCM[6]) && ezMath::IsNaN(m.m_fElementsCM[7]) && ezMath::IsNaN(m.m_fElementsCM[8]) &&
                   ezMath::IsNaN(m.m_fElementsCM[9]) && ezMath::IsNaN(m.m_fElementsCM[10]) && ezMath::IsNaN(m.m_fElementsCM[11]) &&
                   ezMath::IsNaN(m.m_fElementsCM[12]) && ezMath::IsNaN(m.m_fElementsCM[13]) && ezMath::IsNaN(m.m_fElementsCM[14]) &&
                   ezMath::IsNaN(m.m_fElementsCM[15]));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    Type testBlock[16] = {(Type)1, (Type)2, (Type)3, (Type)4, (Type)5, (Type)6, (Type)7, (Type)8,
                          (Type)9, (Type)10, (Type)11, (Type)12, (Type)13, (Type)14, (Type)15, (Type)16};
    ezMat4Type* m = ::new ((void*)&testBlock[0]) ezMat4Type;

    EZ_TEST_BOOL(m->m_fElementsCM[0] == (Type)1 && m->m_fElementsCM[1] == (Type)2 && m->m_fElementsCM[2] == (Type)3 && m->m_fElementsCM[3] == (Type)4 &&
                 m->m_fElementsCM[4] == (Type)5 && m->m_fElementsCM[5] == (Type)6 && m->m_fElementsCM[6] == (Type)7 && m->m_fElementsCM[7] == (Type)8 &&
                 m->m_fElementsCM[8] == (Type)9 && m->m_fElementsCM[9] == (Type)10 && m->m_fElementsCM[10] == (Type)11 && m->m_fElementsCM[11] == (Type)12 &&
                 m->m_fElementsCM[12] == (Type)13 && m->m_fElementsCM[13] == (Type)14 && m->m_fElementsCM[14] == (Type)15 && m->m_fElementsCM[15] == (Type)16);
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (Array Data)")
  {
    const Type data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      ezMat4Type m = ezMat4Type::MakeFromColumnMajorArray(data);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)2 && m.m_fElementsCM[2] == (Type)3 && m.m_fElementsCM[3] == (Type)4 &&
                   m.m_fElementsCM[4] == (Type)5 && m.m_fElementsCM[5] == (Type)6 && m.m_fElementsCM[6] == (Type)7 && m.m_fElementsCM[7] == (Type)8 &&
                   m.m_fElementsCM[8] == (Type)9 && m.m_fElementsCM[9] == (Type)10 && m.m_fElementsCM[10] == (Type)11 && m.m_fElementsCM[11] == (Type)12 &&
                   m.m_fElementsCM[12] == (Type)13 && m.m_fElementsCM[13] == (Type)14 && m.m_fElementsCM[14] == (Type)15 && m.m_fElementsCM[15] == (Type)16);
    }

    {
      ezMat4Type m = ezMat4Type::MakeFromRowMajorArray(data);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)5 && m.m_fElementsCM[2] == (Type)9 && m.m_fElementsCM[3] == (Type)13 &&
                   m.m_fElementsCM[4] == (Type)2 && m.m_fElementsCM[5] == (Type)6 && m.m_fElementsCM[6] == (Type)10 && m.m_fElementsCM[7] == (Type)14 &&
                   m.m_fElementsCM[8] == (Type)3 && m.m_fElementsCM[9] == (Type)7 && m.m_fElementsCM[10] == (Type)11 && m.m_fElementsCM[11] == (Type)15 &&
                   m.m_fElementsCM[12] == (Type)4 && m.m_fElementsCM[13] == (Type)8 && m.m_fElementsCM[14] == (Type)12 && m.m_fElementsCM[15] == (Type)16);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (Elements)")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    EZ_TEST_FLOAT(m.Element(0, 0), 1, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(1, 0), 2, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(2, 0), 3, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(3, 0), 4, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(0, 1), 5, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(1, 1), 6, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(2, 1), 7, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(3, 1), 8, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(0, 2), 9, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(1, 2), 10, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(2, 2), 11, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(3, 2), 12, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(0, 3), 13, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(1, 3), 14, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(2, 3), 15, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(3, 3), 16, ezMath::SmallEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (composite)")
  {
    ezMat3Type mr = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);
    ezVec3Type vt(10, 11, 12);

    ezMat4Type m(mr, vt);

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0);
    EZ_TEST_FLOAT(m.Element(1, 0), 2, 0);
    EZ_TEST_FLOAT(m.Element(2, 0), 3, 0);
    EZ_TEST_FLOAT(m.Element(3, 0), 10, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 4, 0);
    EZ_TEST_FLOAT(m.Element(1, 1), 5, 0);
    EZ_TEST_FLOAT(m.Element(2, 1), 6, 0);
    EZ_TEST_FLOAT(m.Element(3, 1), 11, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 7, 0);
    EZ_TEST_FLOAT(m.Element(1, 2), 8, 0);
    EZ_TEST_FLOAT(m.Element(2, 2), 9, 0);
    EZ_TEST_FLOAT(m.Element(3, 2), 12, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromArray")
  {
    const Type data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      ezMat4Type m = ezMat4Type::MakeFromColumnMajorArray(data);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)2 && m.m_fElementsCM[2] == (Type)3 && m.m_fElementsCM[3] == (Type)4 &&
                   m.m_fElementsCM[4] == (Type)5 && m.m_fElementsCM[5] == (Type)6 && m.m_fElementsCM[6] == (Type)7 && m.m_fElementsCM[7] == (Type)8 &&
                   m.m_fElementsCM[8] == (Type)9 && m.m_fElementsCM[9] == (Type)10 && m.m_fElementsCM[10] == (Type)11 && m.m_fElementsCM[11] == (Type)12 &&
                   m.m_fElementsCM[12] == (Type)13 && m.m_fElementsCM[13] == (Type)14 && m.m_fElementsCM[14] == (Type)15 && m.m_fElementsCM[15] == (Type)16);
    }

    {
      ezMat4Type m = ezMat4Type::MakeFromRowMajorArray(data);

      EZ_TEST_BOOL(m.m_fElementsCM[0] == (Type)1 && m.m_fElementsCM[1] == (Type)5 && m.m_fElementsCM[2] == (Type)9 && m.m_fElementsCM[3] == (Type)13 &&
                   m.m_fElementsCM[4] == (Type)2 && m.m_fElementsCM[5] == (Type)6 && m.m_fElementsCM[6] == (Type)10 && m.m_fElementsCM[7] == (Type)14 &&
                   m.m_fElementsCM[8] == (Type)3 && m.m_fElementsCM[9] == (Type)7 && m.m_fElementsCM[10] == (Type)11 && m.m_fElementsCM[11] == (Type)15 &&
                   m.m_fElementsCM[12] == (Type)4 && m.m_fElementsCM[13] == (Type)8 && m.m_fElementsCM[14] == (Type)12 && m.m_fElementsCM[15] == (Type)16);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetElements")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    EZ_TEST_FLOAT(m.Element(0, 0), 1, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(1, 0), 2, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(2, 0), 3, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(3, 0), 4, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(0, 1), 5, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(1, 1), 6, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(2, 1), 7, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(3, 1), 8, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(0, 2), 9, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(1, 2), 10, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(2, 2), 11, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(3, 2), 12, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(0, 3), 13, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(1, 3), 14, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(2, 3), 15, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(m.Element(3, 3), 16, ezMath::SmallEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeTransformation")
  {
    ezMat3Type mr = ezMat3Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);
    ezVec3Type vt(10, 11, 12);

    ezMat4Type m = ezMat4Type::MakeTransformation(mr, vt);

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0);
    EZ_TEST_FLOAT(m.Element(1, 0), 2, 0);
    EZ_TEST_FLOAT(m.Element(2, 0), 3, 0);
    EZ_TEST_FLOAT(m.Element(3, 0), 10, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 4, 0);
    EZ_TEST_FLOAT(m.Element(1, 1), 5, 0);
    EZ_TEST_FLOAT(m.Element(2, 1), 6, 0);
    EZ_TEST_FLOAT(m.Element(3, 1), 11, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 7, 0);
    EZ_TEST_FLOAT(m.Element(1, 2), 8, 0);
    EZ_TEST_FLOAT(m.Element(2, 2), 9, 0);
    EZ_TEST_FLOAT(m.Element(3, 2), 12, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsArray")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    Type data[16];

    m.GetAsArray(data, ezMatrixLayout::ColumnMajor);
    EZ_TEST_FLOAT(data[0], 1, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[1], 5, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[2], 9, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[3], 13, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[4], 2, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[5], 6, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[6], 10, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[7], 14, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[8], 3, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[9], 7, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[10], 11, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[11], 15, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[12], 4, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[13], 8, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[14], 12, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[15], 16, ezMath::SmallEpsilon<Type>());

    m.GetAsArray(data, ezMatrixLayout::RowMajor);
    EZ_TEST_FLOAT(data[0], 1, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[1], 2, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[2], 3, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[3], 4, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[4], 5, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[5], 6, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[6], 7, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[7], 8, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[8], 9, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[9], 10, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[10], 11, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[11], 12, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[12], 13, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[13], 14, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[14], 15, ezMath::SmallEpsilon<Type>());
    EZ_TEST_FLOAT(data[15], 16, ezMath::SmallEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetZero")
  {
    ezMat4Type m;
    m.SetZero();

    for (ezUInt32 i = 0; i < 16; ++i)
      EZ_TEST_FLOAT(m.m_fElementsCM[i], (Type)0, (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetIdentity")
  {
    ezMat4Type m;
    m.SetIdentity();

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0);
    EZ_TEST_FLOAT(m.Element(1, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 1), 1, 0);
    EZ_TEST_FLOAT(m.Element(2, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 2), 1, 0);
    EZ_TEST_FLOAT(m.Element(3, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetTranslationMatrix")
  {
    ezMat4Type m = ezMat4Type::MakeTranslation(ezVec3Type(2, 3, 4));

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0);
    EZ_TEST_FLOAT(m.Element(1, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 0), 2, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 1), 1, 0);
    EZ_TEST_FLOAT(m.Element(2, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 1), 3, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 2), 1, 0);
    EZ_TEST_FLOAT(m.Element(3, 2), 4, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetScalingMatrix")
  {
    ezMat4Type m = ezMat4Type::MakeScaling(ezVec3Type(2, 3, 4));

    EZ_TEST_FLOAT(m.Element(0, 0), 2, 0);
    EZ_TEST_FLOAT(m.Element(1, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 1), 3, 0);
    EZ_TEST_FLOAT(m.Element(2, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 2), 4, 0);
    EZ_TEST_FLOAT(m.Element(3, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRotationMatrixX")
  {
    ezMat4Type m;

    m = ezMat4Type::MakeRotationX(ezAngleTemplate<Type>::MakeFromDegree(90));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, -3, 2), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeRotationX(ezAngleTemplate<Type>::MakeFromDegree(180));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, -2, -3), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeRotationX(ezAngleTemplate<Type>::MakeFromDegree(270));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, 3, -2), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeRotationX(ezAngleTemplate<Type>::MakeFromDegree(360));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, 2, 3), ezMath::DefaultEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRotationMatrixY")
  {
    ezMat4Type m;

    m = ezMat4Type::MakeRotationY(ezAngleTemplate<Type>::MakeFromDegree(90));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(3, 2, -1), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeRotationY(ezAngleTemplate<Type>::MakeFromDegree(180));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-1, 2, -3), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeRotationY(ezAngleTemplate<Type>::MakeFromDegree(270));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-3, 2, 1), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeRotationY(ezAngleTemplate<Type>::MakeFromDegree(360));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, 2, 3), ezMath::DefaultEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRotationMatrixZ")
  {
    ezMat4Type m;

    m = ezMat4Type::MakeRotationZ(ezAngleTemplate<Type>::MakeFromDegree(90));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-2, 1, 3), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeRotationZ(ezAngleTemplate<Type>::MakeFromDegree(180));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-1, -2, 3), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeRotationZ(ezAngleTemplate<Type>::MakeFromDegree(270));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(2, -1, 3), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeRotationZ(ezAngleTemplate<Type>::MakeFromDegree(360));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, 2, 3), ezMath::DefaultEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRotationMatrix")
  {
    ezMat4Type m;

    m = ezMat4Type::MakeAxisRotation(ezVec3Type(1, 0, 0), ezAngleTemplate<Type>::MakeFromDegree(90));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, -3, 2), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeAxisRotation(ezVec3Type(1, 0, 0), ezAngleTemplate<Type>::MakeFromDegree(180));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, -2, -3), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeAxisRotation(ezVec3Type(1, 0, 0), ezAngleTemplate<Type>::MakeFromDegree(270));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(1, 3, -2), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeAxisRotation(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(3, 2, -1), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeAxisRotation(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(180));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-1, 2, -3), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeAxisRotation(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(270));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-3, 2, 1), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeAxisRotation(ezVec3Type(0, 0, 1), ezAngleTemplate<Type>::MakeFromDegree(90));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-2, 1, 3), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeAxisRotation(ezVec3Type(0, 0, 1), ezAngleTemplate<Type>::MakeFromDegree(180));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(-1, -2, 3), ezMath::DefaultEpsilon<Type>()));

    m = ezMat4Type::MakeAxisRotation(ezVec3Type(0, 0, 1), ezAngleTemplate<Type>::MakeFromDegree(270));
    EZ_TEST_BOOL((m * ezVec3Type(1, 2, 3)).IsEqual(ezVec3Type(2, -1, 3), ezMath::DefaultEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeIdentity")
  {
    ezMat4Type m = ezMat4Type::MakeIdentity();

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0);
    EZ_TEST_FLOAT(m.Element(1, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 1), 1, 0);
    EZ_TEST_FLOAT(m.Element(2, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 2), 1, 0);
    EZ_TEST_FLOAT(m.Element(3, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeZero")
  {
    ezMat4Type m = ezMat4Type::MakeZero();

    EZ_TEST_FLOAT(m.Element(0, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 0), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 1), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 2), 0, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(1, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(2, 3), 0, 0);
    EZ_TEST_FLOAT(m.Element(3, 3), 0, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transpose")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m.Transpose();

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0);
    EZ_TEST_FLOAT(m.Element(1, 0), 5, 0);
    EZ_TEST_FLOAT(m.Element(2, 0), 9, 0);
    EZ_TEST_FLOAT(m.Element(3, 0), 13, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 2, 0);
    EZ_TEST_FLOAT(m.Element(1, 1), 6, 0);
    EZ_TEST_FLOAT(m.Element(2, 1), 10, 0);
    EZ_TEST_FLOAT(m.Element(3, 1), 14, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 3, 0);
    EZ_TEST_FLOAT(m.Element(1, 2), 7, 0);
    EZ_TEST_FLOAT(m.Element(2, 2), 11, 0);
    EZ_TEST_FLOAT(m.Element(3, 2), 15, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 4, 0);
    EZ_TEST_FLOAT(m.Element(1, 3), 8, 0);
    EZ_TEST_FLOAT(m.Element(2, 3), 12, 0);
    EZ_TEST_FLOAT(m.Element(3, 3), 16, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetTranspose")
  {
    ezMat4Type m0 = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezMat4Type m = m0.GetTranspose();

    EZ_TEST_FLOAT(m.Element(0, 0), 1, 0);
    EZ_TEST_FLOAT(m.Element(1, 0), 5, 0);
    EZ_TEST_FLOAT(m.Element(2, 0), 9, 0);
    EZ_TEST_FLOAT(m.Element(3, 0), 13, 0);
    EZ_TEST_FLOAT(m.Element(0, 1), 2, 0);
    EZ_TEST_FLOAT(m.Element(1, 1), 6, 0);
    EZ_TEST_FLOAT(m.Element(2, 1), 10, 0);
    EZ_TEST_FLOAT(m.Element(3, 1), 14, 0);
    EZ_TEST_FLOAT(m.Element(0, 2), 3, 0);
    EZ_TEST_FLOAT(m.Element(1, 2), 7, 0);
    EZ_TEST_FLOAT(m.Element(2, 2), 11, 0);
    EZ_TEST_FLOAT(m.Element(3, 2), 15, 0);
    EZ_TEST_FLOAT(m.Element(0, 3), 4, 0);
    EZ_TEST_FLOAT(m.Element(1, 3), 8, 0);
    EZ_TEST_FLOAT(m.Element(2, 3), 12, 0);
    EZ_TEST_FLOAT(m.Element(3, 3), 16, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Invert")
  {
    for (Type x = (Type)1.0; x < (Type)360.0; x += (Type)10.0)
    {
      for (Type y = (Type)2.0; y < (Type)360.0; y += (Type)17.0)
      {
        for (Type z = (Type)3.0; z < (Type)360.0; z += (Type)23.0)
        {
          ezMat4Type m, inv;
          m = ezMat4Type::MakeAxisRotation(ezVec3Type(x, y, z).GetNormalized(), ezAngleTemplate<Type>::MakeFromDegree((Type)19.0));
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
          ezMat4Type m, inv;
          m = ezMat4Type::MakeAxisRotation(ezVec3Type(x, y, z).GetNormalized(), ezAngleTemplate<Type>::MakeFromDegree((Type)83.0));
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
    ezMat4Type m;

    m.SetIdentity();
    EZ_TEST_BOOL(!m.IsZero());

    m.SetZero();
    EZ_TEST_BOOL(m.IsZero());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentity")
  {
    ezMat4Type m;

    m.SetIdentity();
    EZ_TEST_BOOL(m.IsIdentity());

    m.SetZero();
    EZ_TEST_BOOL(!m.IsIdentity());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsValid")
  {
    if (ezMath::SupportsNaN<Type>())
    {
      ezMat4Type m;

      m.SetZero();
      EZ_TEST_BOOL(m.IsValid());

      m.m_fElementsCM[0] = ezMath::NaN<Type>();
      EZ_TEST_BOOL(!m.IsValid());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRow")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    EZ_TEST_VEC4(m.GetRow(0), ezVec4Type(1, 2, 3, 4), (Type)0);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4Type(5, 6, 7, 8), (Type)0);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4Type(9, 10, 11, 12), (Type)0);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4Type(13, 14, 15, 16), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRow")
  {
    ezMat4Type m;
    m.SetZero();

    m.SetRow(0, ezVec4Type(1, 2, 3, 4));
    EZ_TEST_VEC4(m.GetRow(0), ezVec4Type(1, 2, 3, 4), (Type)0);

    m.SetRow(1, ezVec4Type(5, 6, 7, 8));
    EZ_TEST_VEC4(m.GetRow(1), ezVec4Type(5, 6, 7, 8), (Type)0);

    m.SetRow(2, ezVec4Type(9, 10, 11, 12));
    EZ_TEST_VEC4(m.GetRow(2), ezVec4Type(9, 10, 11, 12), (Type)0);

    m.SetRow(3, ezVec4Type(13, 14, 15, 16));
    EZ_TEST_VEC4(m.GetRow(3), ezVec4Type(13, 14, 15, 16), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetColumn")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    EZ_TEST_VEC4(m.GetColumn(0), ezVec4Type(1, 5, 9, 13), (Type)0);
    EZ_TEST_VEC4(m.GetColumn(1), ezVec4Type(2, 6, 10, 14), (Type)0);
    EZ_TEST_VEC4(m.GetColumn(2), ezVec4Type(3, 7, 11, 15), (Type)0);
    EZ_TEST_VEC4(m.GetColumn(3), ezVec4Type(4, 8, 12, 16), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetColumn")
  {
    ezMat4Type m;
    m.SetZero();

    m.SetColumn(0, ezVec4Type(1, 2, 3, 4));
    EZ_TEST_VEC4(m.GetColumn(0), ezVec4Type(1, 2, 3, 4), (Type)0);

    m.SetColumn(1, ezVec4Type(5, 6, 7, 8));
    EZ_TEST_VEC4(m.GetColumn(1), ezVec4Type(5, 6, 7, 8), (Type)0);

    m.SetColumn(2, ezVec4Type(9, 10, 11, 12));
    EZ_TEST_VEC4(m.GetColumn(2), ezVec4Type(9, 10, 11, 12), (Type)0);

    m.SetColumn(3, ezVec4Type(13, 14, 15, 16));
    EZ_TEST_VEC4(m.GetColumn(3), ezVec4Type(13, 14, 15, 16), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetDiagonal")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    EZ_TEST_VEC4(m.GetDiagonal(), ezVec4Type(1, 6, 11, 16), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetDiagonal")
  {
    ezMat4Type m;
    m.SetZero();

    m.SetDiagonal(ezVec4Type(1, 2, 3, 4));
    EZ_TEST_VEC4(m.GetColumn(0), ezVec4Type(1, 0, 0, 0), (Type)0);
    EZ_TEST_VEC4(m.GetColumn(1), ezVec4Type(0, 2, 0, 0), (Type)0);
    EZ_TEST_VEC4(m.GetColumn(2), ezVec4Type(0, 0, 3, 0), (Type)0);
    EZ_TEST_VEC4(m.GetColumn(3), ezVec4Type(0, 0, 0, 4), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetTranslationVector")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    EZ_TEST_VEC3(m.GetTranslationVector(), ezVec3Type(4, 8, 12), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetTranslationVector")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m.SetTranslationVector(ezVec3Type(17, 18, 19));
    EZ_TEST_VEC4(m.GetRow(0), ezVec4Type(1, 2, 3, 17), (Type)0);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4Type(5, 6, 7, 18), (Type)0);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4Type(9, 10, 11, 19), (Type)0);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4Type(13, 14, 15, 16), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetRotationalPart")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezMat3Type r = ezMat3Type::MakeFromValues(17, 18, 19, 20, 21, 22, 23, 24, 25);

    m.SetRotationalPart(r);
    EZ_TEST_VEC4(m.GetRow(0), ezVec4Type(17, 18, 19, 4), (Type)0);
    EZ_TEST_VEC4(m.GetRow(1), ezVec4Type(20, 21, 22, 8), (Type)0);
    EZ_TEST_VEC4(m.GetRow(2), ezVec4Type(23, 24, 25, 12), (Type)0);
    EZ_TEST_VEC4(m.GetRow(3), ezVec4Type(13, 14, 15, 16), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRotationalPart")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezMat3Type r = m.GetRotationalPart();
    EZ_TEST_VEC3(r.GetRow(0), ezVec3Type(1, 2, 3), (Type)0);
    EZ_TEST_VEC3(r.GetRow(1), ezVec3Type(5, 6, 7), (Type)0);
    EZ_TEST_VEC3(r.GetRow(2), ezVec3Type(9, 10, 11), (Type)0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetScalingFactors")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezVec3Type s = m.GetScalingFactors();
    EZ_TEST_VEC3(s,
      ezVec3Type(ezMath::Sqrt((Type)(1 * 1 + 5 * 5 + 9 * 9)), ezMath::Sqrt((Type)(2 * 2 + 6 * 6 + 10 * 10)),
        ezMath::Sqrt((Type)(3 * 3 + 7 * 7 + 11 * 11))),
      ezMath::SmallEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetScalingFactors")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    EZ_TEST_BOOL(m.SetScalingFactors(ezVec3Type(1, 2, 3)) == EZ_SUCCESS);

    ezVec3Type s = m.GetScalingFactors();
    EZ_TEST_VEC3(s, ezVec3Type(1, 2, 3), ezMath::SmallEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformDirection")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const ezVec3Type r = m.TransformDirection(ezVec3Type(1, 2, 3));

    EZ_TEST_VEC3(r, ezVec3Type(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), ezMath::SmallEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformDirection(array)")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezVec3Type data[3] = {ezVec3Type(1, 2, 3), ezVec3Type(4, 5, 6), ezVec3Type(7, 8, 9)};

    m.TransformDirection(data, 2);

    EZ_TEST_VEC3(data[0], ezVec3Type(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC3(data[1], ezVec3Type(4 * 1 + 5 * 2 + 6 * 3, 4 * 5 + 5 * 6 + 6 * 7, 4 * 9 + 5 * 10 + 6 * 11), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC3(data[2], ezVec3Type(7, 8, 9), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformPosition")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const ezVec3Type r = m.TransformPosition(ezVec3Type(1, 2, 3));

    EZ_TEST_VEC3(r, ezVec3Type(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), ezMath::SmallEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformPosition(array)")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezVec3Type data[3] = {ezVec3Type(1, 2, 3), ezVec3Type(4, 5, 6), ezVec3Type(7, 8, 9)};

    m.TransformPosition(data, 2);

    EZ_TEST_VEC3(data[0], ezVec3Type(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC3(data[1], ezVec3Type(4 * 1 + 5 * 2 + 6 * 3 + 4, 4 * 5 + 5 * 6 + 6 * 7 + 8, 4 * 9 + 5 * 10 + 6 * 11 + 12), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC3(data[2], ezVec3Type(7, 8, 9), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const ezVec4Type r = m.Transform(ezVec4Type(1, 2, 3, 4));

    EZ_TEST_VEC4(r,
      ezVec4Type(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 8 * 4, 1 * 9 + 2 * 10 + 3 * 11 + 12 * 4, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16),
      ezMath::SmallEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform(array)")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezVec4Type data[3] = {ezVec4Type(1, 2, 3, 4), ezVec4Type(5, 6, 7, 8), ezVec4Type(9, 10, 11, 12)};

    m.Transform(data, 2);

    EZ_TEST_VEC4(data[0],
      ezVec4Type(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 8 * 4, 1 * 9 + 2 * 10 + 3 * 11 + 12 * 4, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16),
      ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(data[1],
      ezVec4Type(5 * 1 + 6 * 2 + 7 * 3 + 8 * 4, 5 * 5 + 6 * 6 + 7 * 7 + 8 * 8, 5 * 9 + 6 * 10 + 7 * 11 + 12 * 8, 5 * 13 + 6 * 14 + 7 * 15 + 8 * 16),
      ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(data[2], ezVec4Type(9, 10, 11, 12), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*=")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m *= (Type)2;

    EZ_TEST_VEC4(m.GetRow(0), ezVec4Type(2, 4, 6, 8), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(m.GetRow(1), ezVec4Type(10, 12, 14, 16), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(m.GetRow(2), ezVec4Type(18, 20, 22, 24), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(m.GetRow(3), ezVec4Type(26, 28, 30, 32), ezMath::SmallEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/=")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m *= (Type)4;
    m /= (Type)2;

    EZ_TEST_VEC4(m.GetRow(0), ezVec4Type(2, 4, 6, 8), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(m.GetRow(1), ezVec4Type(10, 12, 14, 16), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(m.GetRow(2), ezVec4Type(18, 20, 22, 24), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(m.GetRow(3), ezVec4Type(26, 28, 30, 32), ezMath::SmallEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezMat4Type m2 = m;

    EZ_TEST_BOOL(m.IsIdentical(m2));

    m2.m_fElementsCM[0] += ezMath::SmallEpsilon<Type>();
    EZ_TEST_BOOL(!m.IsIdentical(m2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezMat4Type m2 = m;

    EZ_TEST_BOOL(m.IsEqual(m2, ezMath::SmallEpsilon<Type>()));

    m2.m_fElementsCM[0] += ezMath::DefaultEpsilon<Type>();
    EZ_TEST_BOOL(m.IsEqual(m2, ezMath::DefaultEpsilon<Type>()));
    EZ_TEST_BOOL(!m.IsEqual(m2, ezMath::SmallEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(mat, mat)")
  {
    ezMat4Type m1 = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezMat4Type m2 = ezMat4Type::MakeFromValues(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16);

    ezMat4Type r = m1 * m2;

    EZ_TEST_VEC4(r.GetColumn(0),
      ezVec4Type(-1 * 1 + -5 * 2 + -9 * 3 + -13 * 4, -1 * 5 + -5 * 6 + -9 * 7 + -13 * 8, -1 * 9 + -5 * 10 + -9 * 11 + -13 * 12,
        -1 * 13 + -5 * 14 + -9 * 15 + -13 * 16),
      ezMath::LargeEpsilon<Type>());
    EZ_TEST_VEC4(r.GetColumn(1),
      ezVec4Type(-2 * 1 + -6 * 2 + -10 * 3 + -14 * 4, -2 * 5 + -6 * 6 + -10 * 7 + -14 * 8, -2 * 9 + -6 * 10 + -10 * 11 + -14 * 12,
        -2 * 13 + -6 * 14 + -10 * 15 + -14 * 16),
      ezMath::LargeEpsilon<Type>());
    EZ_TEST_VEC4(r.GetColumn(2),
      ezVec4Type(-3 * 1 + -7 * 2 + -11 * 3 + -15 * 4, -3 * 5 + -7 * 6 + -11 * 7 + -15 * 8, -3 * 9 + -7 * 10 + -11 * 11 + -15 * 12,
        -3 * 13 + -7 * 14 + -11 * 15 + -15 * 16),
      ezMath::LargeEpsilon<Type>());
    EZ_TEST_VEC4(r.GetColumn(3),
      ezVec4Type(-4 * 1 + -8 * 2 + -12 * 3 + -16 * 4, -4 * 5 + -8 * 6 + -12 * 7 + -16 * 8, -4 * 9 + -8 * 10 + -12 * 11 + -16 * 12,
        -4 * 13 + -8 * 14 + -12 * 15 + -16 * 16),
      ezMath::LargeEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(mat, vec3)")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const ezVec3Type r = m * ezVec3Type(1, 2, 3);

    EZ_TEST_VEC3(r, ezVec3Type(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), ezMath::SmallEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(mat, vec4)")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const ezVec4Type r = m * ezVec4Type(1, 2, 3, 4);

    EZ_TEST_VEC4(r,
      ezVec4Type(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 4 * 8, 1 * 9 + 2 * 10 + 3 * 11 + 4 * 12, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16),
      ezMath::SmallEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(mat, float) | operator*(float, mat)")
  {
    ezMat4Type m0 = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezMat4Type m = m0 * (Type)2;
    ezMat4Type m2 = (Type)2 * m0;

    EZ_TEST_VEC4(m.GetRow(0), ezVec4Type(2, 4, 6, 8), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(m.GetRow(1), ezVec4Type(10, 12, 14, 16), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(m.GetRow(2), ezVec4Type(18, 20, 22, 24), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(m.GetRow(3), ezVec4Type(26, 28, 30, 32), ezMath::SmallEpsilon<Type>());

    EZ_TEST_VEC4(m2.GetRow(0), ezVec4Type(2, 4, 6, 8), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(m2.GetRow(1), ezVec4Type(10, 12, 14, 16), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(m2.GetRow(2), ezVec4Type(18, 20, 22, 24), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(m2.GetRow(3), ezVec4Type(26, 28, 30, 32), ezMath::SmallEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator/(mat, float)")
  {
    ezMat4Type m0 = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m0 *= (Type)4;

    ezMat4Type m = m0 / (Type)2;

    EZ_TEST_VEC4(m.GetRow(0), ezVec4Type(2, 4, 6, 8), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(m.GetRow(1), ezVec4Type(10, 12, 14, 16), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(m.GetRow(2), ezVec4Type(18, 20, 22, 24), ezMath::SmallEpsilon<Type>());
    EZ_TEST_VEC4(m.GetRow(3), ezVec4Type(26, 28, 30, 32), ezMath::SmallEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+(mat, mat) | operator-(mat, mat)")
  {
    ezMat4Type m0 = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezMat4Type m1 = ezMat4Type::MakeFromValues(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16);

    EZ_TEST_BOOL((m0 + m1).IsZero());
    EZ_TEST_BOOL((m0 - m1).IsEqual(m0 * (Type)2, ezMath::SmallEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator== (mat, mat) | operator!= (mat, mat)")
  {
    ezMat4Type m = ezMat4Type::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    ezMat4Type m2 = m;

    EZ_TEST_BOOL(m == m2);

    m2.m_fElementsCM[0] += ezMath::SmallEpsilon<Type>();

    EZ_TEST_BOOL(m != m2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::SupportsNaN<Type>())
    {
      ezMat4Type m;

      m.SetIdentity();
      EZ_TEST_BOOL(!m.IsNaN());

      for (ezUInt32 i = 0; i < 16; ++i)
      {
        m.SetIdentity();
        m.m_fElementsCM[i] = ezMath::NaN<Type>();

        EZ_TEST_BOOL(m.IsNaN());
      }
    }
  }
}


EZ_CREATE_SIMPLE_TEST(Math, Mat4f)
{
  TestMat4<float>();
}
EZ_CREATE_SIMPLE_TEST(Math, Mat4d)
{
  TestMat4<double>();
}