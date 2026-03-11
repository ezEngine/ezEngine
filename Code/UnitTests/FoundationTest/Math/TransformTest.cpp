#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Transform.h>

template <typename Type>
void TestTransform()
{
  using ezTransformType = ezTransformTemplate<Type>;
  using ezVec3Type = ezVec3Template<Type>;
  using ezMat3Type = ezMat3Template<Type>;
  using ezMat4Type = ezMat4Template<Type>;
  using ezQuatType = ezQuatTemplate<Type>;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructors")
  {
    ezTransformType t0;

    {
      ezTransformType t(ezVec3Type(1, 2, 3));
      EZ_TEST_VEC3(t.m_vPosition, ezVec3Type(1, 2, 3), 0);
    }

    {
      ezQuatType qRot;
      qRot = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(1, 2, 3).GetNormalized(), ezAngleTemplate<Type>::MakeFromDegree(42.0f));

      ezTransformType t(ezVec3Type(4, 5, 6), qRot);
      EZ_TEST_VEC3(t.m_vPosition, ezVec3Type(4, 5, 6), 0);
      EZ_TEST_BOOL(t.m_qRotation == qRot);
    }

    {
      ezMat3Type mRot = ezMat3Type::MakeAxisRotation(ezVec3Type(1, 2, 3).GetNormalized(), ezAngleTemplate<Type>::MakeFromDegree(42.0f));

      ezQuatType q;
      q = ezQuatType::MakeFromMat3(mRot);

      ezTransformType t(ezVec3Type(4, 5, 6), q);
      EZ_TEST_VEC3(t.m_vPosition, ezVec3Type(4, 5, 6), 0);
      EZ_TEST_BOOL(t.m_qRotation.GetAsMat3().IsEqual(mRot, (Type)0.0001));
    }

    {
      ezQuatType qRot;
      qRot.SetIdentity();

      ezTransformType t(ezVec3Type(4, 5, 6), qRot, ezVec3Type(2, 3, 4));
      EZ_TEST_VEC3(t.m_vPosition, ezVec3Type(4, 5, 6), 0);
      EZ_TEST_BOOL(t.m_qRotation.GetAsMat3().IsEqual(ezMat3Type::MakeFromValues(1, 0, 0, 0, 1, 0, 0, 0, 1), (Type)0.001));
      EZ_TEST_VEC3(t.m_vScale, ezVec3Type(2, 3, 4), 0);
    }

    {
      ezMat3Type mRot = ezMat3Type::MakeAxisRotation(ezVec3Type(1, 2, 3).GetNormalized(), ezAngleTemplate<Type>::MakeFromDegree(42.0f));
      ezMat4Type mTrans;
      mTrans.SetTransformationMatrix(mRot, ezVec3Type(1, 2, 3));

      ezTransformType t = ezTransformType::MakeFromMat4(mTrans);
      EZ_TEST_VEC3(t.m_vPosition, ezVec3Type(1, 2, 3), 0);
      EZ_TEST_BOOL(t.m_qRotation.GetAsMat3().IsEqual(mRot, (Type)0.001));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetIdentity")
  {
    ezTransformType t;
    t.SetIdentity();

    EZ_TEST_VEC3(t.m_vPosition, ezVec3Type(0), 0);
    EZ_TEST_BOOL(t.m_qRotation == ezQuatType::MakeIdentity());
    EZ_TEST_BOOL(t.m_vScale == ezVec3Type((Type)1.0));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsMat4")
  {
    ezQuatType qRot;
    qRot.SetIdentity();

    ezTransformType t(ezVec3Type(4, 5, 6), qRot, ezVec3Type(2, 3, 4));
    EZ_TEST_BOOL(t.GetAsMat4() == ezMat4Type::MakeFromValues(2, 0, 0, 4, 0, 3, 0, 5, 0, 0, 4, 6, 0, 0, 0, 1));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator + / -")
  {
    ezTransformType t0, t1;
    t0.SetIdentity();
    t1.SetIdentity();

    t1 = t0 + ezVec3Type(2, 3, 4);
    EZ_TEST_VEC3(t1.m_vPosition, ezVec3Type(2, 3, 4), (Type)0.0001);

    t1 = t1 - ezVec3Type(4, 2, 1);
    EZ_TEST_VEC3(t1.m_vPosition, ezVec3Type(-2, 1, 3), (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator * (quat)")
  {
    ezQuatType qRotX, qRotY;
    qRotX = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(1, 0, 0), ezAngleTemplate<Type>::MakeFromRadian(1.57079637f));
    qRotY = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromRadian(1.57079637f));

    ezTransformType t0, t1;
    t0.SetIdentity();
    t1.SetIdentity();

    t1 = qRotX * t0;
    EZ_TEST_VEC3(t1.m_vPosition, ezVec3Type(0, 0, 0), (Type)0.0001);

    ezQuatType q;
    q = ezQuatType::MakeFromMat3(ezMat3Type::MakeFromValues(1, 0, 0, 0, 0, -1, 0, 1, 0));
    EZ_TEST_BOOL(t1.m_qRotation.IsEqualRotation(q, (Type)0.0001));

    t1 = qRotY * t1;
    EZ_TEST_VEC3(t1.m_vPosition, ezVec3Type(0, 0, 0), (Type)0.0001);
    q = ezQuatType::MakeFromMat3(ezMat3Type::MakeFromValues(0, 1, 0, 0, 0, -1, -1, 0, 0));
    EZ_TEST_BOOL(t1.m_qRotation.IsEqualRotation(q, (Type)0.0001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator * (vec3)")
  {
    ezQuatType qRotX, qRotY;
    qRotX = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(1, 0, 0), ezAngleTemplate<Type>::MakeFromRadian(1.57079637f));
    qRotY = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromRadian(1.57079637f));

    ezTransformType t;
    t.SetIdentity();

    t = qRotX * t;

    EZ_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotX, (Type)0.0001));
    EZ_TEST_VEC3(t.m_vPosition, ezVec3Type(0, 0, 0), (Type)0.0001);
    EZ_TEST_VEC3(t.m_vScale, ezVec3Type(1, 1, 1), (Type)0.0001);

    t = t + ezVec3Type(1, 2, 3);

    EZ_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotX, (Type)0.0001));
    EZ_TEST_VEC3(t.m_vPosition, ezVec3Type(1, 2, 3), (Type)0.0001);
    EZ_TEST_VEC3(t.m_vScale, ezVec3Type(1, 1, 1), (Type)0.0001);

    t = qRotY * t;

    EZ_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotY * qRotX, (Type)0.0001));
    EZ_TEST_VEC3(t.m_vPosition, ezVec3Type(1, 2, 3), (Type)0.0001);
    EZ_TEST_VEC3(t.m_vScale, ezVec3Type(1, 1, 1), (Type)0.0001);

    ezQuatType q;
    q = ezQuatType::MakeFromMat3(ezMat3Type::MakeFromValues(0, 1, 0, 0, 0, -1, -1, 0, 0));
    EZ_TEST_BOOL(t.m_qRotation.IsEqualRotation(q, (Type)0.0001));

    ezVec3Type v;
    v = t * ezVec3Type(4, 5, 6);

    EZ_TEST_VEC3(v, ezVec3Type(5 + 1, -6 + 2, -4 + 3), (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical")
  {
    ezTransformType t(ezVec3Type(1, 2, 3));
    t.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));

    EZ_TEST_BOOL(t.IsIdentical(t));

    ezTransformType t2(ezVec3Type(1, 2, 4));
    t2.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));

    EZ_TEST_BOOL(!t.IsIdentical(t2));

    ezTransformType t3(ezVec3Type(1, 2, 3));
    t3.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(91));

    EZ_TEST_BOOL(!t.IsIdentical(t3));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator == / !=")
  {
    ezTransformType t(ezVec3Type(1, 2, 3));
    t.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));

    EZ_TEST_BOOL(t == t);

    ezTransformType t2(ezVec3Type(1, 2, 4));
    t2.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));

    EZ_TEST_BOOL(t != t2);

    ezTransformType t3(ezVec3Type(1, 2, 3));
    t3.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(91));

    EZ_TEST_BOOL(t != t3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    ezTransformType t(ezVec3Type(1, 2, 3));
    t.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));

    EZ_TEST_BOOL(t.IsEqual(t, ezMath::DefaultEpsilon<Type>()));

    ezTransformType t2(ezVec3Type(1, 2, (Type)3 + 2*ezMath::DefaultEpsilon<Type>()));
    t2.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));

    EZ_TEST_BOOL(t.IsEqual(t2, ezMath::LargeEpsilon<Type>()));
    EZ_TEST_BOOL(!t.IsEqual(t2, ezMath::DefaultEpsilon<Type>()));

    ezTransformType t3(ezVec3Type(1, 2, 3));
    t3.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90 + ezMath::VeryHugeEpsilon<Type>()));

    EZ_TEST_BOOL(t.IsEqual(t3, ezMath::VeryHugeEpsilon<Type>()));
    EZ_TEST_BOOL(!t.IsEqual(t3, ezMath::DefaultEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(ezTransformType, ezTransformType)")
  {
    ezTransformType tParent(ezVec3Type(1, 2, 3));
    tParent.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromRadian(1.57079637f));
    tParent.m_vScale.Set(2);

    ezTransformType tToChild(ezVec3Type(4, 5, 6));
    tToChild.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 0, 1), ezAngleTemplate<Type>::MakeFromRadian(1.57079637f));
    tToChild.m_vScale.Set(4);

    // this is exactly the same as SetGlobalTransform
    ezTransformType tChild;
    tChild = tParent * tToChild;

    EZ_TEST_VEC3(tChild.m_vPosition, ezVec3Type(13, 12, -5), (Type)0.003);
    EZ_TEST_BOOL(tChild.m_qRotation.GetAsMat3().IsEqual(ezMat3Type::MakeFromValues(0, 0, 1, 1, 0, 0, 0, 1, 0), (Type)0.0001));
    EZ_TEST_VEC3(tChild.m_vScale, ezVec3Type(8, 8, 8), (Type)0.0001);

    // verify that it works exactly like a 4x4 matrix
    const ezMat4Type mParent = tParent.GetAsMat4();
    const ezMat4Type mToChild = tToChild.GetAsMat4();
    const ezMat4Type mChild = mParent * mToChild;

    EZ_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), (Type)0.0001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(ezTransformType, ezMat4)")
  {
    ezTransformType tParent(ezVec3Type(1, 2, 3));
    tParent.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale.Set(2);

    ezTransformType tToChild(ezVec3Type(4, 5, 6));
    tToChild.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 0, 1), ezAngleTemplate<Type>::MakeFromDegree(90));
    tToChild.m_vScale.Set(4);

    // this is exactly the same as SetGlobalTransform
    ezTransformType tChild;
    tChild = tParent * tToChild;

    EZ_TEST_VEC3(tChild.m_vPosition, ezVec3Type(13, 12, -5), (Type)0.0001);
    EZ_TEST_BOOL(tChild.m_qRotation.GetAsMat3().IsEqual(ezMat3Type::MakeFromValues(0, 0, 1, 1, 0, 0, 0, 1, 0), (Type)0.0001));
    EZ_TEST_VEC3(tChild.m_vScale, ezVec3Type(8, 8, 8), (Type)0.0001);

    // verify that it works exactly like a 4x4 matrix
    const ezMat4Type mParent = tParent.GetAsMat4();
    const ezMat4Type mToChild = tToChild.GetAsMat4();
    const ezMat4Type mChild = mParent * mToChild;

    EZ_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), (Type)0.0001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(ezMat4, ezTransformType)")
  {
    ezTransformType tParent(ezVec3Type(1, 2, 3));
    tParent.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale.Set(2);

    ezTransformType tToChild(ezVec3Type(4, 5, 6));
    tToChild.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 0, 1), ezAngleTemplate<Type>::MakeFromDegree(90));
    tToChild.m_vScale.Set(4);

    // this is exactly the same as SetGlobalTransform
    ezTransformType tChild;
    tChild = tParent * tToChild;

    EZ_TEST_VEC3(tChild.m_vPosition, ezVec3Type(13, 12, -5), (Type)0.0001);
    EZ_TEST_BOOL(tChild.m_qRotation.GetAsMat3().IsEqual(ezMat3Type::MakeFromValues(0, 0, 1, 1, 0, 0, 0, 1, 0), (Type)0.0001));
    EZ_TEST_VEC3(tChild.m_vScale, ezVec3Type(8, 8, 8), (Type)0.0001);

    // verify that it works exactly like a 4x4 matrix
    const ezMat4Type mParent = tParent.GetAsMat4();
    const ezMat4Type mToChild = tToChild.GetAsMat4();
    const ezMat4Type mChild = mParent * mToChild;

    EZ_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), (Type)0.0001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Invert / GetInverse")
  {
    ezTransformType tParent(ezVec3Type(1, 2, 3));
    tParent.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale.Set(2);

    ezTransformType tToChild(ezVec3Type(4, 5, 6));
    tToChild.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 0, 1), ezAngleTemplate<Type>::MakeFromDegree(90));
    tToChild.m_vScale.Set(4);

    ezTransformType tChild;
    tChild = ezTransformType::MakeGlobalTransform(tParent, tToChild);

    // negate twice -> get back original
    tToChild.Invert();
    tToChild.Invert();

    ezTransformType tInvToChild = tToChild.GetInverse();

    ezTransformType tParentFromChild;
    tParentFromChild = ezTransformType::MakeGlobalTransform(tChild, tInvToChild);

    EZ_TEST_BOOL(tParent.IsEqual(tParentFromChild, (Type)0.0001));
  }

  //////////////////////////////////////////////////////////////////////////
  // Tests copied and ported over from ezSimdTransform
  //////////////////////////////////////////////////////////////////////////

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezTransformType t0;

    {
      ezQuatType qRot;
      qRot = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(1, 2, 3).GetNormalized(), ezAngleTemplate<Type>::MakeFromDegree(42.0f));

      ezVec3Type pos(4, 5, 6);
      ezVec3Type scale(7, 8, 9);

      ezTransformType t(pos);
      EZ_TEST_BOOL((t.m_vPosition == pos));
      EZ_TEST_BOOL(t.m_qRotation == ezQuatType::MakeIdentity());
      EZ_TEST_BOOL((t.m_vScale == ezVec3Type(1)));

      t = ezTransformType(pos, qRot);
      EZ_TEST_BOOL((t.m_vPosition == pos));
      EZ_TEST_BOOL(t.m_qRotation == qRot);
      EZ_TEST_BOOL((t.m_vScale == ezVec3Type(1)));

      t = ezTransformType(pos, qRot, scale);
      EZ_TEST_BOOL((t.m_vPosition == pos));
      EZ_TEST_BOOL(t.m_qRotation == qRot);
      EZ_TEST_BOOL((t.m_vScale == scale));

      t = ezTransformType(ezVec3Type::MakeZero(), qRot);
      EZ_TEST_BOOL(t.m_vPosition.IsZero());
      EZ_TEST_BOOL(t.m_qRotation == qRot);
      EZ_TEST_BOOL((t.m_vScale == ezVec3Type(1)));
    }

    {
      ezTransformType t;
      t.SetIdentity();

      EZ_TEST_BOOL(t.m_vPosition.IsZero());
      EZ_TEST_BOOL(t.m_qRotation == ezQuatType::MakeIdentity());
      EZ_TEST_BOOL((t.m_vScale == ezVec3Type(1)));

      EZ_TEST_BOOL(t == ezTransformType::MakeIdentity());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Inverse")
  {
    ezTransformType tParent(ezVec3Type(1, 2, 3));
    tParent.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale = ezVec3Type(2);

    ezTransformType tToChild(ezVec3Type(4, 5, 6));
    tToChild.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 0, 1), ezAngleTemplate<Type>::MakeFromDegree(90));
    tToChild.m_vScale = ezVec3Type(4);

    ezTransformType tChild;
    tChild = tParent * tToChild;

    // invert twice -> get back original
    ezTransformType t2 = tToChild;
    t2.Invert();
    t2.Invert();
    EZ_TEST_BOOL(t2.IsEqual(tToChild, (Type)0.0001));

    ezTransformType tInvToChild = tToChild.GetInverse();

    ezTransformType tParentFromChild;
    tParentFromChild = tChild * tInvToChild;

    EZ_TEST_BOOL(tParent.IsEqual(tParentFromChild, (Type)0.0001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetLocalTransform")
  {
    ezQuatType q;
    q = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 0, 1), ezAngleTemplate<Type>::MakeFromDegree(90));

    ezTransformType tParent(ezVec3Type(1, 2, 3));
    tParent.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale = ezVec3Type(2);

    ezTransformType tChild;
    tChild.m_vPosition = ezVec3Type(13, 12, -5);
    tChild.m_qRotation = tParent.m_qRotation * q;
    tChild.m_vScale = ezVec3Type(8);

    ezTransformType tToChild;
    tToChild = ezTransformType::MakeLocalTransform(tParent, tChild);

    EZ_TEST_BOOL(tToChild.m_vPosition.IsEqual(ezVec3Type(4, 5, 6), (Type)0.0001));
    EZ_TEST_BOOL(tToChild.m_qRotation.IsEqualRotation(q, (Type)0.0001));
    EZ_TEST_BOOL((tToChild.m_vScale == ezVec3Type(4)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetGlobalTransform")
  {
    ezTransformType tParent(ezVec3Type(1, 2, 3));
    tParent.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));
    tParent.m_vScale = ezVec3Type(2);

    ezTransformType tToChild(ezVec3Type(4, 5, 6));
    tToChild.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 0, 1), ezAngleTemplate<Type>::MakeFromDegree(90));
    tToChild.m_vScale = ezVec3Type(4);

    ezTransformType tChild;
    tChild = ezTransformType::MakeGlobalTransform(tParent, tToChild);

    EZ_TEST_BOOL(tChild.m_vPosition.IsEqual(ezVec3Type(13, 12, -5), (Type)0.0001));
    EZ_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, (Type)0.0001));
    EZ_TEST_BOOL((tChild.m_vScale == ezVec3Type(8)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsMat4")
  {
    ezTransformType t(ezVec3Type(1, 2, 3));
    t.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(34));
    t.m_vScale = ezVec3Type(2, -1, 5);

    ezMat4Type m = t.GetAsMat4();

    ezMat4Type refM;
    refM.SetZero();
    {
      ezQuatType q;
      q = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(34));

      ezTransformType referenceTransform(ezVec3Type(1, 2, 3), q, ezVec3Type(2, -1, 5));
      ezMat4Type tmp = referenceTransform.GetAsMat4();
      refM = ezMat4Type::MakeFromColumnMajorArray(tmp.m_fElementsCM);
    }
    EZ_TEST_BOOL(m.IsEqual(refM, ezMath::DefaultEpsilon<Type>()));

    ezVec3Type p[8] = {
      ezVec3Type(-4, 0, 0), ezVec3Type(5, 0, 0), ezVec3Type(0, -6, 0), ezVec3Type(0, 7, 0), ezVec3Type(0, 0, -8), ezVec3Type(0, 0, 9), ezVec3Type(1, -2, 3), ezVec3Type(-4, 5, 7)};

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(p); ++i)
    {
      ezVec3Type pt = t.TransformPosition(p[i]);
      ezVec3Type pm = m.TransformPosition(p[i]);

      EZ_TEST_BOOL(pt.IsEqual(pm, ezMath::DefaultEpsilon<Type>()));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformPos / Dir / operator*")
  {
    ezQuatType qRotX, qRotY;
    qRotX = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(1, 0, 0), ezAngleTemplate<Type>::MakeFromDegree(90.0f));
    qRotY = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90.0f));

    ezTransformType t(ezVec3Type(1, 2, 3), qRotY * qRotX, ezVec3Type(2, -2, 4));

    ezVec3Type v;
    v = t.TransformPosition(ezVec3Type(4, 5, 6));
    EZ_TEST_BOOL(v.IsEqual(ezVec3Type((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), (Type)0.0001));

    v = t.TransformDirection(ezVec3Type(4, 5, 6));
    EZ_TEST_BOOL(v.IsEqual(ezVec3Type((5 * -2), (-6 * 4), (-4 * 2)), (Type)0.0001));

    v = t * ezVec3Type(4, 5, 6);
    EZ_TEST_BOOL(v.IsEqual(ezVec3Type((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), (Type)0.0001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    {
      ezTransformType tParent(ezVec3Type(1, 2, 3));
      tParent.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));
      tParent.m_vScale = ezVec3Type(2);

      ezTransformType tToChild(ezVec3Type(4, 5, 6));
      tToChild.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 0, 1), ezAngleTemplate<Type>::MakeFromDegree(90));
      tToChild.m_vScale = ezVec3Type(4);

      // this is exactly the same as SetGlobalTransform
      ezTransformType tChild;
      tChild = tParent * tToChild;

      EZ_TEST_BOOL(tChild.m_vPosition.IsEqual(ezVec3Type(13, 12, -5), (Type)0.0001));
      EZ_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, (Type)0.0001));
      EZ_TEST_BOOL((tChild.m_vScale == ezVec3Type(8)));

      tChild = tParent;
      tChild = tChild * tToChild;

      EZ_TEST_BOOL(tChild.m_vPosition.IsEqual(ezVec3Type(13, 12, -5), (Type)0.0001));
      EZ_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, (Type)0.0001));
      EZ_TEST_BOOL((tChild.m_vScale == ezVec3Type(8)));

      ezVec3Type a(7, 8, 9);
      ezVec3Type b;
      b = tToChild.TransformPosition(a);
      b = tParent.TransformPosition(b);

      ezVec3Type c;
      c = tChild.TransformPosition(a);

      EZ_TEST_BOOL(b.IsEqual(c, (Type)0.0001));

      // verify that it works exactly like a 4x4 matrix
      const ezMat4Type mParent = tParent.GetAsMat4();
      const ezMat4Type mToChild = tToChild.GetAsMat4();
      const ezMat4Type mChild = mParent * mToChild;

      EZ_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), (Type)0.0001));
    }

    {
      ezTransformType t(ezVec3Type(1, 2, 3));
      t.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));
      t.m_vScale = ezVec3Type(2);

      ezQuatType q;
      q = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 0, 1), ezAngleTemplate<Type>::MakeFromDegree(90));

      ezTransformType t2 = t * q;
      ezTransformType t4 = q * t;

      ezTransformType t3 = t;
      t3 = t3 * q;
      EZ_TEST_BOOL(t2 == t3);
      EZ_TEST_BOOL(t3 != t4);

      ezVec3Type a(7, 8, 9);
      ezVec3Type b;
      b = t2.TransformPosition(a);

      ezVec3Type c = q * a;
      c = t.TransformPosition(c);

      EZ_TEST_BOOL(b.IsEqual(c, (Type)0.0001));
    }

    {
      ezTransformType t(ezVec3Type(1, 2, 3));
      t.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));
      t.m_vScale = ezVec3Type(2);

      ezVec3Type p(4, 5, 6);

      ezTransformType t2 = t + p;
      ezTransformType t3 = t;
      t3 += p;
      EZ_TEST_BOOL(t2 == t3);

      ezVec3Type a(7, 8, 9);
      ezVec3Type b;
      b = t2.TransformPosition(a);

      ezVec3Type c = t.TransformPosition(a) + p;

      EZ_TEST_BOOL(b.IsEqual(c, (Type)0.0001));
    }

    {
      ezTransformType t(ezVec3Type(1, 2, 3));
      t.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));
      t.m_vScale = ezVec3Type(2);

      ezVec3Type p(4, 5, 6);

      ezTransformType t2 = t - p;
      ezTransformType t3 = t;
      t3 -= p;
      EZ_TEST_BOOL(t2 == t3);

      ezVec3Type a(7, 8, 9);
      ezVec3Type b;
      b = t2.TransformPosition(a);

      ezVec3Type c = t.TransformPosition(a) - p;

      EZ_TEST_BOOL(b.IsEqual(c, (Type)0.0001));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Comparison")
  {
    ezTransformType t(ezVec3Type(1, 2, 3));
    t.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));

    EZ_TEST_BOOL(t == t);

    ezTransformType t2(ezVec3Type(1, 2, 4));
    t2.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(90));

    EZ_TEST_BOOL(t != t2);

    ezTransformType t3(ezVec3Type(1, 2, 3));
    t3.m_qRotation = ezQuatType::MakeFromAxisAndAngle(ezVec3Type(0, 1, 0), ezAngleTemplate<Type>::MakeFromDegree(91));

    EZ_TEST_BOOL(t != t3);
  }
}


EZ_CREATE_SIMPLE_TEST(Math, Transformf)
{
  TestTransform<float>();
}
EZ_CREATE_SIMPLE_TEST(Math, Transformd)
{
  TestTransform<double>();
}
