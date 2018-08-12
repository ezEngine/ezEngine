#include <PCH.h>

#include <Foundation/Math/Transform.h>

EZ_CREATE_SIMPLE_TEST(Math, Transform)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructors")
  {
    ezTransformT t0;

    {
      ezTransformT t(ezVec3T(1, 2, 3));
      EZ_TEST_VEC3(t.m_vPosition, ezVec3T(1, 2, 3), 0);
    }

    /// \test Enable ezTransform tests that could be converted

    //{
    //  ezQuat qRot;
    //  qRot.SetFromAxisAndAngle(ezVec3T(1, 2, 3).GetNormalized(), ezAngle::Degree(42.0f));

    //  ezTransformT t(ezVec3T(4, 5, 6), qRot);
    //  EZ_TEST_VEC3(t.m_vPosition, ezVec3T(4, 5, 6), 0);
    //  EZ_TEST_BOOL(t.m_qRotation == qRot.GetAsMat3());
    //}

    //{
    //  ezMat3 mRot;
    //  mRot.SetRotationMatrix(ezVec3T(1, 2, 3).GetNormalized(), ezAngle::Degree(42.0f));

    //  ezTransformT t(ezVec3T(4, 5, 6), mRot);
    //  EZ_TEST_VEC3(t.m_vPosition, ezVec3T(4, 5, 6), 0);
    //  EZ_TEST_BOOL(t.m_qRotation == mRot);
    //}

    //{
    //  ezQuat qRot;
    //  qRot.SetIdentity();

    //  ezTransformT t(ezVec3T(4, 5, 6), qRot, ezVec3T(2, 3, 4));
    //  EZ_TEST_VEC3(t.m_vPosition, ezVec3T(4, 5, 6), 0);
    //  EZ_TEST_BOOL(t.m_qRotation == ezMat3(2, 0, 0, 0, 3, 0, 0, 0, 4));
    //}

    //{
    //  ezMat3T mRot;
    //  mRot.SetRotationMatrix(ezVec3T(1, 2, 3).GetNormalized(), ezAngle::Degree(42.0f));
    //  ezMat4T mTrans;
    //  mTrans.SetTransformationMatrix(mRot, ezVec3T(1, 2, 3));

    //  ezTransformT t(mTrans);
    //  EZ_TEST_VEC3(t.m_vPosition, ezVec3T(1, 2, 3), 0);
    //  EZ_TEST_BOOL(t.m_qRotation == mRot);
    //}
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetIdentity")
  {
    ezTransformT t;
    t.SetIdentity();

    EZ_TEST_VEC3(t.m_vPosition, ezVec3T(0), 0);
    EZ_TEST_BOOL(t.m_qRotation == ezQuat::IdentityQuaternion());
    EZ_TEST_BOOL(t.m_vScale == ezVec3T(1.0f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsMat4")
  {
    ezQuat qRot;
    qRot.SetIdentity();

    ezTransformT t(ezVec3T(4, 5, 6), qRot, ezVec3T(2, 3, 4));
    EZ_TEST_BOOL(t.GetAsMat4() == ezMat4(2, 0, 0, 4, 0, 3, 0, 5, 0, 0, 4, 6, 0, 0, 0, 1));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator + / -")
  {
    ezTransformT t0, t1;
    t0.SetIdentity();
    t1.SetIdentity();

    t1 = t0 + ezVec3T(2, 3, 4);
    EZ_TEST_VEC3(t1.m_vPosition, ezVec3T(2, 3, 4), 0.0001f);

    t1 = t1 - ezVec3T(4, 2, 1);
    EZ_TEST_VEC3(t1.m_vPosition, ezVec3T(-2, 1, 3), 0.0001f);
  }

  // EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator *=")
  //{
  //  ezQuat qRotX, qRotY;
  //  qRotX.SetFromAxisAndAngle(ezVec3T(1, 0, 0), ezAngle::Degree(90.0f));
  //  qRotY.SetFromAxisAndAngle(ezVec3T(0, 1, 0), ezAngle::Degree(90.0f));

  //  ezTransformT t;
  //  t.SetIdentity();

  //  t *= qRotX;
  //  EZ_TEST_VEC3(t.m_vPosition, ezVec3T(0, 0, 0), 0.0001f);
  //  //EZ_TEST_BOOL(t.m_qRotation.IsEqual(ezMat3(1, 0, 0, 0, 0, -1, 0, 1, 0), 0.0001f));
  //  EZ_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotX, 0.0001f));
  //  EZ_TEST_VEC3(t.m_vScale, ezVec3T(1), 0.0001f);

  //  t *= qRotY;
  //  EZ_TEST_VEC3(t.m_vPosition, ezVec3T(0, 0, 0), 0.0001f);
  //  //EZ_TEST_BOOL(t.m_qRotation.IsEqual(ezMat3(0, 1, 0, 0, 0, -1, -1, 0, 0), 0.0001f));
  //}

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator * (quat)")
  {
    ezQuat qRotX, qRotY;
    qRotX.SetFromAxisAndAngle(ezVec3T(1, 0, 0), ezAngle::Degree(90.0f));
    qRotY.SetFromAxisAndAngle(ezVec3T(0, 1, 0), ezAngle::Degree(90.0f));

    ezTransformT t0, t1;
    t0.SetIdentity();
    t1.SetIdentity();

    t1 = qRotX * t0;
    EZ_TEST_VEC3(t1.m_vPosition, ezVec3T(0, 0, 0), 0.0001f);
    // EZ_TEST_BOOL(t1.m_qRotation.IsEqual(ezMat3(1, 0, 0, 0, 0, -1, 0, 1, 0), 0.0001f));

    t1 = qRotY * t1;
    EZ_TEST_VEC3(t1.m_vPosition, ezVec3T(0, 0, 0), 0.0001f);
    // EZ_TEST_BOOL(t1.m_qRotation.IsEqual(ezMat3(0, 1, 0, 0, 0, -1, -1, 0, 0), 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator * (vec3)")
  {
    ezQuat qRotX, qRotY;
    qRotX.SetFromAxisAndAngle(ezVec3T(1, 0, 0), ezAngle::Degree(90.0f));
    qRotY.SetFromAxisAndAngle(ezVec3T(0, 1, 0), ezAngle::Degree(90.0f));

    ezTransformT t;
    t.SetIdentity();

    t = qRotX * t;

    EZ_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotX, 0.0001f));
    EZ_TEST_VEC3(t.m_vPosition, ezVec3T(0, 0, 0), 0.0001f);
    EZ_TEST_VEC3(t.m_vScale, ezVec3T(1, 1, 1), 0.0001f);

    t = t + ezVec3T(1, 2, 3);

    EZ_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotX, 0.0001f));
    EZ_TEST_VEC3(t.m_vPosition, ezVec3T(1, 2, 3), 0.0001f);
    EZ_TEST_VEC3(t.m_vScale, ezVec3T(1, 1, 1), 0.0001f);

    t = qRotY * t;

    EZ_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotY * qRotX, 0.0001f));
    EZ_TEST_VEC3(t.m_vPosition, ezVec3T(1, 2, 3), 0.0001f);
    EZ_TEST_VEC3(t.m_vScale, ezVec3T(1, 1, 1), 0.0001f);

    // EZ_TEST_BOOL(t.m_qRotation.IsEqual(ezMat3(0, 1, 0, 0, 0, -1, -1, 0, 0), 0.0001f));

    ezVec3T v;
    v = t * ezVec3T(4, 5, 6);

    EZ_TEST_VEC3(v, ezVec3T(5 + 1, -6 + 2, -4 + 3), 0.0001f);
  }

  // EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetGlobalTransform")
  //{
  //  ezTransformT tParent(ezVec3T(1, 2, 3));
  //  tParent.m_qRotation.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(90));
  //  tParent.m_qRotation.SetScalingFactors(ezVec3T(2));

  //  ezTransformT tToChild(ezVec3T(4, 5, 6));
  //  tToChild.m_qRotation.SetRotationMatrix(ezVec3T(0, 0, 1), ezAngle::Degree(90));
  //  tToChild.m_qRotation.SetScalingFactors(ezVec3T(4));

  //  ezTransformT tChild;
  //  tChild.SetGlobalTransform(tParent, tToChild);

  //  EZ_TEST_VEC3(tChild.m_vPosition, ezVec3T(13, 12, -5), 0.0001f);
  //  EZ_TEST_BOOL(tChild.m_qRotation.IsEqual(ezMat3(0, 0, 8, 8, 0, 0, 0, 8, 0), 0.0001f));
  //}

  // EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetLocalTransform")
  //{
  //  // the inverse of the SetGlobalTransform test

  //  ezTransformT tParent(ezVec3T(1, 2, 3));
  //  tParent.m_qRotation.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(90));
  //  tParent.m_qRotation.SetScalingFactors(ezVec3T(2));

  //  ezTransformT tChild;
  //  tChild.m_vPosition = ezVec3T(13, 12, -5);
  //  tChild.m_qRotation = ezMat3(0, 0, 8, 8, 0, 0, 0, 8, 0);

  //  ezTransformT tToChild;
  //  tToChild.SetLocalTransform(tParent, tChild);

  //  ezMat3 mRot;
  //  mRot.SetRotationMatrix(ezVec3T(0, 0, 1), ezAngle::Degree(90));
  //  mRot.SetScalingFactors(ezVec3T(4));

  //  EZ_TEST_VEC3(tToChild.m_vPosition, ezVec3T(4, 5, 6), 0.0001f);
  //  EZ_TEST_BOOL(tToChild.m_qRotation.IsEqual(mRot, 0.0001f));
  //}

  // EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical")
  //{
  //  ezTransformT t(ezVec3T(1, 2, 3));
  //  t.m_qRotation.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(90));

  //  EZ_TEST_BOOL(t.IsIdentical(t));

  //  ezTransformT t2(ezVec3T(1, 2, 4));
  //  t2.m_qRotation.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(90));

  //  EZ_TEST_BOOL(!t.IsIdentical(t2));

  //  ezTransformT t3(ezVec3T(1, 2, 3));
  //  t3.m_qRotation.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(91));

  //  EZ_TEST_BOOL(!t.IsIdentical(t3));
  //}

  // EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator == / !=")
  //{
  //  ezTransformT t(ezVec3T(1, 2, 3));
  //  t.m_qRotation.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(90));

  //  EZ_TEST_BOOL(t == t);

  //  ezTransformT t2(ezVec3T(1, 2, 4));
  //  t2.m_qRotation.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(90));

  //  EZ_TEST_BOOL(t != t2);

  //  ezTransformT t3(ezVec3T(1, 2, 3));
  //  t3.m_qRotation.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(91));

  //  EZ_TEST_BOOL(t != t3);
  //}

  // EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  //{
  //  ezTransformT t(ezVec3T(1, 2, 3));
  //  t.m_qRotation.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(90));

  //  EZ_TEST_BOOL(t.IsEqual(t, 0.0001f));

  //  ezTransformT t2(ezVec3T(1, 2, 3.0002f));
  //  t2.m_qRotation.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(90));

  //  EZ_TEST_BOOL(t.IsEqual(t2, 0.001f));
  //  EZ_TEST_BOOL(!t.IsEqual(t2, 0.0001f));

  //  ezTransformT t3(ezVec3T(1, 2, 3));
  //  t3.m_qRotation.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(90.01f));

  //  EZ_TEST_BOOL(t.IsEqual(t3, 0.01f));
  //  EZ_TEST_BOOL(!t.IsEqual(t3, 0.0001f));
  //}

  // EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(ezTransformT, ezTransformT)")
  //{
  //  ezTransformT tParent(ezVec3T(1, 2, 3));
  //  tParent.m_qRotation.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(90));
  //  tParent.m_qRotation.SetScalingFactors(ezVec3T(2));

  //  ezTransformT tToChild(ezVec3T(4, 5, 6));
  //  tToChild.m_qRotation.SetRotationMatrix(ezVec3T(0, 0, 1), ezAngle::Degree(90));
  //  tToChild.m_qRotation.SetScalingFactors(ezVec3T(4));

  //  // this is exactly the same as SetGlobalTransform
  //  ezTransformT tChild;
  //  tChild  = tParent * tToChild;

  //  EZ_TEST_VEC3(tChild.m_vPosition, ezVec3T(13, 12, -5), 0.0001f);
  //  EZ_TEST_BOOL(tChild.m_qRotation.IsEqual(ezMat3(0, 0, 8, 8, 0, 0, 0, 8, 0), 0.0001f));

  //  // verify that it works exactly like a 4x4 matrix
  //  const ezMat4 mParent = tParent.GetAsMat4();
  //  const ezMat4 mToChild = tToChild.GetAsMat4();
  //  const ezMat4 mChild = mParent * mToChild;

  //  EZ_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));
  //}

  // EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(ezTransformT, ezMat4)")
  //{
  //  ezTransformT tParent(ezVec3T(1, 2, 3));
  //  tParent.m_qRotation.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(90));
  //  tParent.m_qRotation.SetScalingFactors(ezVec3T(2));

  //  ezTransformT tToChild(ezVec3T(4, 5, 6));
  //  tToChild.m_qRotation.SetRotationMatrix(ezVec3T(0, 0, 1), ezAngle::Degree(90));
  //  tToChild.m_qRotation.SetScalingFactors(ezVec3T(4));

  //  // this is exactly the same as SetGlobalTransform
  //  ezTransformT tChild;
  //  tChild  = tParent * tToChild.GetAsMat4();

  //  EZ_TEST_VEC3(tChild.m_vPosition, ezVec3T(13, 12, -5), 0.0001f);
  //  EZ_TEST_BOOL(tChild.m_qRotation.IsEqual(ezMat3(0, 0, 8, 8, 0, 0, 0, 8, 0), 0.0001f));

  //  // verify that it works exactly like a 4x4 matrix
  //  const ezMat4 mParent = tParent.GetAsMat4();
  //  const ezMat4 mToChild = tToChild.GetAsMat4();
  //  const ezMat4 mChild = mParent * mToChild;

  //  EZ_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));
  //}

  // EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(ezMat4, ezTransformT)")
  //{
  //  ezTransformT tParent(ezVec3T(1, 2, 3));
  //  tParent.m_qRotation.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(90));
  //  tParent.m_qRotation.SetScalingFactors(ezVec3T(2));

  //  ezTransformT tToChild(ezVec3T(4, 5, 6));
  //  tToChild.m_qRotation.SetRotationMatrix(ezVec3T(0, 0, 1), ezAngle::Degree(90));
  //  tToChild.m_qRotation.SetScalingFactors(ezVec3T(4));

  //  // this is exactly the same as SetGlobalTransform
  //  ezTransformT tChild;
  //  tChild  = tParent.GetAsMat4() * tToChild;

  //  EZ_TEST_VEC3(tChild.m_vPosition, ezVec3T(13, 12, -5), 0.0001f);
  //  EZ_TEST_BOOL(tChild.m_qRotation.IsEqual(ezMat3(0, 0, 8, 8, 0, 0, 0, 8, 0), 0.0001f));

  //  // verify that it works exactly like a 4x4 matrix
  //  const ezMat4 mParent = tParent.GetAsMat4();
  //  const ezMat4 mToChild = tToChild.GetAsMat4();
  //  const ezMat4 mChild = mParent * mToChild;

  //  EZ_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));
  //}

  // EZ_TEST_BLOCK(ezTestBlock::Enabled, "Invert / GetInverse")
  //{
  //  ezTransformT tParent(ezVec3T(1, 2, 3));
  //  tParent.m_qRotation.SetRotationMatrix(ezVec3T(0, 1, 0), ezAngle::Degree(90));
  //  tParent.m_qRotation.SetScalingFactors(ezVec3T(2));

  //  ezTransformT tToChild(ezVec3T(4, 5, 6));
  //  tToChild.m_qRotation.SetRotationMatrix(ezVec3T(0, 0, 1), ezAngle::Degree(90));
  //  tToChild.m_qRotation.SetScalingFactors(ezVec3T(4));

  //  ezTransformT tChild;
  //  tChild.SetGlobalTransform(tParent, tToChild);

  //  // negate twice -> get back original
  //  EZ_TEST_BOOL(tToChild.Invert().Succeeded());
  //  EZ_TEST_BOOL(tToChild.Invert().Succeeded());

  //  ezTransformT tInvToChild = tToChild.GetInverse();

  //  ezTransformT tParentFromChild;
  //  tParentFromChild.SetGlobalTransform(tChild, tInvToChild);

  //  EZ_TEST_BOOL(tParent.IsEqual(tParentFromChild, 0.0001f));
  //}

  // EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsArray")
  //{
  //  ezTransformT t;
  //  t.m_qRotation.SetElements(1, 2, 3,
  //                           4, 5, 6,
  //                           7, 8, 9);
  //  t.m_vPosition.Set(10, 11, 12);

  //  {
  //    float f[13] = { 0 };

  //    t.GetAsArray(f, ezMatrixLayout::ColumnMajor);

  //    EZ_TEST_FLOAT(f[0], 1, 0);
  //    EZ_TEST_FLOAT(f[1], 4, 0);
  //    EZ_TEST_FLOAT(f[2], 7, 0);

  //    EZ_TEST_FLOAT(f[3], 2, 0);
  //    EZ_TEST_FLOAT(f[4], 5, 0);
  //    EZ_TEST_FLOAT(f[5], 8, 0);

  //    EZ_TEST_FLOAT(f[6], 3, 0);
  //    EZ_TEST_FLOAT(f[7], 6, 0);
  //    EZ_TEST_FLOAT(f[8], 9, 0);

  //    EZ_TEST_FLOAT(f[9], 10, 0);
  //    EZ_TEST_FLOAT(f[10],11, 0);
  //    EZ_TEST_FLOAT(f[11],12, 0);

  //    EZ_TEST_FLOAT(f[12], 0, 0);
  //  }

  //  {
  //    float f[13] = { 0 };

  //    t.GetAsArray(f, ezMatrixLayout::RowMajor);

  //    EZ_TEST_FLOAT(f[0], 1, 0);
  //    EZ_TEST_FLOAT(f[1], 2, 0);
  //    EZ_TEST_FLOAT(f[2], 3, 0);
  //    EZ_TEST_FLOAT(f[3],10, 0);

  //    EZ_TEST_FLOAT(f[4], 4, 0);
  //    EZ_TEST_FLOAT(f[5], 5, 0);
  //    EZ_TEST_FLOAT(f[6], 6, 0);
  //    EZ_TEST_FLOAT(f[7],11, 0);

  //    EZ_TEST_FLOAT(f[8], 7, 0);
  //    EZ_TEST_FLOAT(f[9], 8, 0);
  //    EZ_TEST_FLOAT(f[10],9, 0);
  //    EZ_TEST_FLOAT(f[11],12, 0);

  //    EZ_TEST_FLOAT(f[12], 0, 0);
  //  }
  //}

  //////////////////////////////////////////////////////////////////////////
  // Tests copied and ported over from ezSimdTransform
  //////////////////////////////////////////////////////////////////////////

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezTransform t0;

    {
      ezQuat qRot;
      qRot.SetFromAxisAndAngle(ezVec3(1, 2, 3).GetNormalized(), ezAngle::Degree(42.0f));

      ezVec3 pos(4, 5, 6);
      ezVec3 scale(7, 8, 9);

      ezTransform t(pos);
      EZ_TEST_BOOL((t.m_vPosition == pos));
      EZ_TEST_BOOL(t.m_qRotation == ezQuat::IdentityQuaternion());
      EZ_TEST_BOOL((t.m_vScale == ezVec3(1)));

      t = ezTransform(pos, qRot);
      EZ_TEST_BOOL((t.m_vPosition == pos));
      EZ_TEST_BOOL(t.m_qRotation == qRot);
      EZ_TEST_BOOL((t.m_vScale == ezVec3(1)));

      t = ezTransform(pos, qRot, scale);
      EZ_TEST_BOOL((t.m_vPosition == pos));
      EZ_TEST_BOOL(t.m_qRotation == qRot);
      EZ_TEST_BOOL((t.m_vScale == scale));

      t = ezTransform(ezVec3::ZeroVector(), qRot);
      EZ_TEST_BOOL(t.m_vPosition.IsZero());
      EZ_TEST_BOOL(t.m_qRotation == qRot);
      EZ_TEST_BOOL((t.m_vScale == ezVec3(1)));
    }

    {
      ezTransform t;
      t.SetIdentity();

      EZ_TEST_BOOL(t.m_vPosition.IsZero());
      EZ_TEST_BOOL(t.m_qRotation == ezQuat::IdentityQuaternion());
      EZ_TEST_BOOL((t.m_vScale == ezVec3(1)));

      EZ_TEST_BOOL(t == ezTransform::Identity());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Inverse")
  {
    ezTransform tParent(ezVec3(1, 2, 3));
    tParent.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
    tParent.m_vScale = ezVec3(2);

    ezTransform tToChild(ezVec3(4, 5, 6));
    tToChild.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
    tToChild.m_vScale = ezVec3(4);

    ezTransform tChild;
    tChild = tParent * tToChild;

    // invert twice -> get back original
    ezTransform t2 = tToChild;
    t2.Invert();
    t2.Invert();
    EZ_TEST_BOOL(t2.IsEqual(tToChild, 0.0001f));

    ezTransform tInvToChild = tToChild.GetInverse();

    ezTransform tParentFromChild;
    tParentFromChild = tChild * tInvToChild;

    EZ_TEST_BOOL(tParent.IsEqual(tParentFromChild, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetLocalTransform")
  {
    ezQuat q;
    q.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));

    ezTransform tParent(ezVec3(1, 2, 3));
    tParent.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
    tParent.m_vScale = ezVec3(2);

    ezTransform tChild;
    tChild.m_vPosition = ezVec3(13, 12, -5);
    tChild.m_qRotation = tParent.m_qRotation * q;
    tChild.m_vScale = ezVec3(8);

    ezTransform tToChild;
    tToChild.SetLocalTransform(tParent, tChild);

    EZ_TEST_BOOL(tToChild.m_vPosition.IsEqual(ezVec3(4, 5, 6), 0.0001f));
    EZ_TEST_BOOL(tToChild.m_qRotation.IsEqualRotation(q, 0.0001f));
    EZ_TEST_BOOL((tToChild.m_vScale == ezVec3(4)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetGlobalTransform")
  {
    ezTransform tParent(ezVec3(1, 2, 3));
    tParent.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
    tParent.m_vScale = ezVec3(2);

    ezTransform tToChild(ezVec3(4, 5, 6));
    tToChild.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
    tToChild.m_vScale = ezVec3(4);

    ezTransform tChild;
    tChild.SetGlobalTransform(tParent, tToChild);

    EZ_TEST_BOOL(tChild.m_vPosition.IsEqual(ezVec3(13, 12, -5), 0.0001f));
    EZ_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, 0.0001f));
    EZ_TEST_BOOL((tChild.m_vScale == ezVec3(8)));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsMat4")
  {
    ezTransform t(ezVec3(1, 2, 3));
    t.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(34));
    t.m_vScale = ezVec3(2, -1, 5);

    ezMat4 m = t.GetAsMat4();

    // reference
    ezMat4 refM;
    {
      ezQuat q;
      q.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(34));

      ezTransform referenceTransform(ezVec3(1, 2, 3), q, ezVec3(2, -1, 5));
      ezMat4 tmp = referenceTransform.GetAsMat4();
      refM.SetFromArray(tmp.m_fElementsCM, ezMatrixLayout::ColumnMajor);
    }
    EZ_TEST_BOOL(m.IsEqual(refM, 0.00001f));

    ezVec3 p[8] = {ezVec3(-4, 0, 0), ezVec3(5, 0, 0), ezVec3(0, -6, 0), ezVec3(0, 7, 0),
                   ezVec3(0, 0, -8), ezVec3(0, 0, 9), ezVec3(1, -2, 3), ezVec3(-4, 5, 7)};

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(p); ++i)
    {
      ezVec3 pt = t.TransformPosition(p[i]);
      ezVec3 pm = m.TransformPosition(p[i]);

      EZ_TEST_BOOL(pt.IsEqual(pm, 0.00001f));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformPos / Dir / operator*")
  {
    ezQuat qRotX, qRotY;
    qRotX.SetFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::Degree(90.0f));
    qRotY.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90.0f));

    ezTransform t(ezVec3(1, 2, 3), qRotY * qRotX, ezVec3(2, -2, 4));

    ezVec3 v;
    v = t.TransformPosition(ezVec3(4, 5, 6));
    EZ_TEST_BOOL(v.IsEqual(ezVec3((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f));

    v = t.TransformDirection(ezVec3(4, 5, 6));
    EZ_TEST_BOOL(v.IsEqual(ezVec3((5 * -2), (-6 * 4), (-4 * 2)), 0.0001f));

    v = t * ezVec3(4, 5, 6);
    EZ_TEST_BOOL(v.IsEqual(ezVec3((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    {
      ezTransform tParent(ezVec3(1, 2, 3));
      tParent.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
      tParent.m_vScale = ezVec3(2);

      ezTransform tToChild(ezVec3(4, 5, 6));
      tToChild.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
      tToChild.m_vScale = ezVec3(4);

      // this is exactly the same as SetGlobalTransform
      ezTransform tChild;
      tChild = tParent * tToChild;

      EZ_TEST_BOOL(tChild.m_vPosition.IsEqual(ezVec3(13, 12, -5), 0.0001f));
      EZ_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, 0.0001f));
      EZ_TEST_BOOL((tChild.m_vScale == ezVec3(8)));

      tChild = tParent;
      tChild = tChild * tToChild;

      EZ_TEST_BOOL(tChild.m_vPosition.IsEqual(ezVec3(13, 12, -5), 0.0001f));
      EZ_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, 0.0001f));
      EZ_TEST_BOOL((tChild.m_vScale == ezVec3(8)));

      ezVec3 a(7, 8, 9);
      ezVec3 b;
      b = tToChild.TransformPosition(a);
      b = tParent.TransformPosition(b);

      ezVec3 c;
      c = tChild.TransformPosition(a);

      EZ_TEST_BOOL(b.IsEqual(c, 0.0001f));

      // verify that it works exactly like a 4x4 matrix
      /*const ezMat4 mParent = tParent.GetAsMat4();
      const ezMat4 mToChild = tToChild.GetAsMat4();
      const ezMat4 mChild = mParent * mToChild;

      EZ_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));*/
    }

    {
      ezTransform t(ezVec3(1, 2, 3));
      t.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
      t.m_vScale = ezVec3(2);

      ezQuat q;
      q.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));

      ezTransform t2 = t * q;
      ezTransform t4 = q * t;

      ezTransform t3 = t;
      t3 = t3 * q;
      EZ_TEST_BOOL(t2 == t3);
      EZ_TEST_BOOL(t3 != t4);

      ezVec3 a(7, 8, 9);
      ezVec3 b;
      b = t2.TransformPosition(a);

      ezVec3 c = q * a;
      c = t.TransformPosition(c);

      EZ_TEST_BOOL(b.IsEqual(c, 0.0001f));
    }

    {
      ezTransform t(ezVec3(1, 2, 3));
      t.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
      t.m_vScale = ezVec3(2);

      ezVec3 p(4, 5, 6);

      ezTransform t2 = t + p;
      ezTransform t3 = t;
      t3 += p;
      EZ_TEST_BOOL(t2 == t3);

      ezVec3 a(7, 8, 9);
      ezVec3 b;
      b = t2.TransformPosition(a);

      ezVec3 c = t.TransformPosition(a) + p;

      EZ_TEST_BOOL(b.IsEqual(c, 0.0001f));
    }

    {
      ezTransform t(ezVec3(1, 2, 3));
      t.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
      t.m_vScale = ezVec3(2);

      ezVec3 p(4, 5, 6);

      ezTransform t2 = t - p;
      ezTransform t3 = t;
      t3 -= p;
      EZ_TEST_BOOL(t2 == t3);

      ezVec3 a(7, 8, 9);
      ezVec3 b;
      b = t2.TransformPosition(a);

      ezVec3 c = t.TransformPosition(a) - p;

      EZ_TEST_BOOL(b.IsEqual(c, 0.0001f));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Comparison")
  {
    ezTransform t(ezVec3(1, 2, 3));
    t.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));

    EZ_TEST_BOOL(t == t);

    ezTransform t2(ezVec3(1, 2, 4));
    t2.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));

    EZ_TEST_BOOL(t != t2);

    ezTransform t3(ezVec3(1, 2, 3));
    t3.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(91));

    EZ_TEST_BOOL(t != t3);
  }
}
