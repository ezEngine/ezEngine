#include <PCH.h>
#include <Foundation/Math/Transform.h>

EZ_CREATE_SIMPLE_TEST(Math, Transform)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructors")
  {
    ezTransform t0;

    {
      ezTransform t(ezVec3(1, 2, 3));
      EZ_TEST_VEC3(t.m_vPosition, ezVec3(1, 2, 3), 0);
    }

    {
      ezQuat qRot;
      qRot.SetFromAxisAndAngle(ezVec3(1, 2, 3).GetNormalized(), ezAngle::Degree(42.0f));

      ezTransform t(ezVec3(4, 5, 6), qRot);
      EZ_TEST_VEC3(t.m_vPosition, ezVec3(4, 5, 6), 0);
      EZ_TEST_BOOL(t.m_Rotation == qRot.GetAsMat3());
    }

    {
      ezMat3 mRot;
      mRot.SetRotationMatrix(ezVec3(1, 2, 3).GetNormalized(), ezAngle::Degree(42.0f));

      ezTransform t(ezVec3(4, 5, 6), mRot);
      EZ_TEST_VEC3(t.m_vPosition, ezVec3(4, 5, 6), 0);
      EZ_TEST_BOOL(t.m_Rotation == mRot);
    }

    {
      ezQuat qRot;
      qRot.SetIdentity();

      ezTransform t(ezVec3(4, 5, 6), qRot, ezVec3(2, 3, 4));
      EZ_TEST_VEC3(t.m_vPosition, ezVec3(4, 5, 6), 0);
      EZ_TEST_BOOL(t.m_Rotation == ezMat3(2, 0, 0, 0, 3, 0, 0, 0, 4));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetIdentity")
  {
    ezTransform t;
    t.SetIdentity();

    EZ_TEST_VEC3(t.m_vPosition, ezVec3(0), 0);
    EZ_TEST_BOOL(t.m_Rotation == ezMat3(1, 0, 0, 0, 1, 0, 0, 0, 1));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsMat4")
  {
    ezQuat qRot;
    qRot.SetIdentity();

    ezTransform t(ezVec3(4, 5, 6), qRot, ezVec3(2, 3, 4));
    EZ_TEST_BOOL(t.GetAsMat4() == ezMat4(2, 0, 0, 4, 0, 3, 0, 5, 0, 0, 4, 6, 0, 0, 0, 1));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator + / -")
  {
    ezTransform t0, t1;
    t0.SetIdentity();
    t1.SetIdentity();

    t1 = t0 + ezVec3(2, 3, 4);
    EZ_TEST_VEC3(t1.m_vPosition, ezVec3(2, 3, 4), 0.0001f);

    t1 = t1 - ezVec3(4, 2, 1);
    EZ_TEST_VEC3(t1.m_vPosition, ezVec3(-2, 1, 3), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator *=")
  {
    ezQuat qRotX, qRotY;
    qRotX.SetFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::Degree(90.0f));
    qRotY.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90.0f));

    ezTransform t;
    t.SetIdentity();

    t *= qRotX;
    EZ_TEST_VEC3(t.m_vPosition, ezVec3(0, 0, 0), 0.0001f);
    EZ_TEST_BOOL(t.m_Rotation.IsEqual(ezMat3(1, 0, 0, 0, 0, -1, 0, 1, 0), 0.0001f));

    t *= qRotY;
    EZ_TEST_VEC3(t.m_vPosition, ezVec3(0, 0, 0), 0.0001f);
    EZ_TEST_BOOL(t.m_Rotation.IsEqual(ezMat3(0, 1, 0, 0, 0, -1, -1, 0, 0), 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator * (quat)")
  {
    ezQuat qRotX, qRotY;
    qRotX.SetFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::Degree(90.0f));
    qRotY.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90.0f));

    ezTransform t0, t1;
    t0.SetIdentity();
    t1.SetIdentity();

    t1 = qRotX * t0;
    EZ_TEST_VEC3(t1.m_vPosition, ezVec3(0, 0, 0), 0.0001f);
    EZ_TEST_BOOL(t1.m_Rotation.IsEqual(ezMat3(1, 0, 0, 0, 0, -1, 0, 1, 0), 0.0001f));

    t1 = qRotY * t1;
    EZ_TEST_VEC3(t1.m_vPosition, ezVec3(0, 0, 0), 0.0001f);
    EZ_TEST_BOOL(t1.m_Rotation.IsEqual(ezMat3(0, 1, 0, 0, 0, -1, -1, 0, 0), 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator * (vec3)")
  {
    ezQuat qRotX, qRotY;
    qRotX.SetFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::Degree(90.0f));
    qRotY.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90.0f));

    ezTransform t;
    t.SetIdentity();

    t = qRotX * t;
    t = t + ezVec3(1, 2, 3);
    t = qRotY * t;
    EZ_TEST_VEC3(t.m_vPosition, ezVec3(1, 2, 3), 0.0001f);
    EZ_TEST_BOOL(t.m_Rotation.IsEqual(ezMat3(0, 1, 0, 0, 0, -1, -1, 0, 0), 0.0001f));

    ezVec3 v;
    v = t * ezVec3(4, 5, 6);

    EZ_TEST_VEC3(v, ezVec3(5 + 1, -6 + 2, -4 + 3), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetGlobalTransform")
  {
    ezTransform tParent(ezVec3(1, 2, 3));
    tParent.m_Rotation.SetRotationMatrix(ezVec3(0, 1, 0), ezAngle::Degree(90));
    tParent.m_Rotation.SetScalingFactors(ezVec3(2));

    ezTransform tToChild(ezVec3(4, 5, 6));
    tToChild.m_Rotation.SetRotationMatrix(ezVec3(0, 0, 1), ezAngle::Degree(90));
    tToChild.m_Rotation.SetScalingFactors(ezVec3(4));

    ezTransform tChild;
    tChild.SetGlobalTransform(tParent, tToChild);

    EZ_TEST_VEC3(tChild.m_vPosition, ezVec3(13, 12, -5), 0.0001f);
    EZ_TEST_BOOL(tChild.m_Rotation.IsEqual(ezMat3(0, 0, 8, 8, 0, 0, 0, 8, 0), 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetLocalTransform")
  {
    // the inverse of the SetGlobalTransform test

    ezTransform tParent(ezVec3(1, 2, 3));
    tParent.m_Rotation.SetRotationMatrix(ezVec3(0, 1, 0), ezAngle::Degree(90));
    tParent.m_Rotation.SetScalingFactors(ezVec3(2));

    ezTransform tChild;
    tChild.m_vPosition = ezVec3(13, 12, -5);
    tChild.m_Rotation = ezMat3(0, 0, 8, 8, 0, 0, 0, 8, 0);

    ezTransform tToChild;
    tToChild.SetLocalTransform(tParent, tChild);

    ezMat3 mRot;
    mRot.SetRotationMatrix(ezVec3(0, 0, 1), ezAngle::Degree(90));
    mRot.SetScalingFactors(ezVec3(4));

    EZ_TEST_VEC3(tToChild.m_vPosition, ezVec3(4, 5, 6), 0.0001f);
    EZ_TEST_BOOL(tToChild.m_Rotation.IsEqual(mRot, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsIdentical")
  {
    ezTransform t(ezVec3(1, 2, 3));
    t.m_Rotation.SetRotationMatrix(ezVec3(0, 1, 0), ezAngle::Degree(90));

    EZ_TEST_BOOL(t.IsIdentical(t));

    ezTransform t2(ezVec3(1, 2, 4));
    t2.m_Rotation.SetRotationMatrix(ezVec3(0, 1, 0), ezAngle::Degree(90));

    EZ_TEST_BOOL(!t.IsIdentical(t2));

    ezTransform t3(ezVec3(1, 2, 3));
    t3.m_Rotation.SetRotationMatrix(ezVec3(0, 1, 0), ezAngle::Degree(91));

    EZ_TEST_BOOL(!t.IsIdentical(t3));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    ezTransform t(ezVec3(1, 2, 3));
    t.m_Rotation.SetRotationMatrix(ezVec3(0, 1, 0), ezAngle::Degree(90));

    EZ_TEST_BOOL(t.IsEqual(t, 0.0001f));

    ezTransform t2(ezVec3(1, 2, 3.0002f));
    t2.m_Rotation.SetRotationMatrix(ezVec3(0, 1, 0), ezAngle::Degree(90));

    EZ_TEST_BOOL(t.IsEqual(t2, 0.001f));
    EZ_TEST_BOOL(!t.IsEqual(t2, 0.0001f));

    ezTransform t3(ezVec3(1, 2, 3));
    t3.m_Rotation.SetRotationMatrix(ezVec3(0, 1, 0), ezAngle::Degree(90.01f));

    EZ_TEST_BOOL(t.IsEqual(t3, 0.01f));
    EZ_TEST_BOOL(!t.IsEqual(t3, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(ezTransform, ezTransform)")
  {
    ezTransform tParent(ezVec3(1, 2, 3));
    tParent.m_Rotation.SetRotationMatrix(ezVec3(0, 1, 0), ezAngle::Degree(90));
    tParent.m_Rotation.SetScalingFactors(ezVec3(2));

    ezTransform tToChild(ezVec3(4, 5, 6));
    tToChild.m_Rotation.SetRotationMatrix(ezVec3(0, 0, 1), ezAngle::Degree(90));
    tToChild.m_Rotation.SetScalingFactors(ezVec3(4));

    // this is exactly the same as SetGlobalTransform
    ezTransform tChild;
    tChild  = tParent * tToChild;

    EZ_TEST_VEC3(tChild.m_vPosition, ezVec3(13, 12, -5), 0.0001f);
    EZ_TEST_BOOL(tChild.m_Rotation.IsEqual(ezMat3(0, 0, 8, 8, 0, 0, 0, 8, 0), 0.0001f));

    // verify that it works exactly like a 4x4 matrix
    const ezMat4 mParent = tParent.GetAsMat4();
    const ezMat4 mToChild = tToChild.GetAsMat4();
    const ezMat4 mChild = mParent * mToChild;

    EZ_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Invert / GetInverse")
  {
    ezTransform tParent(ezVec3(1, 2, 3));
    tParent.m_Rotation.SetRotationMatrix(ezVec3(0, 1, 0), ezAngle::Degree(90));
    tParent.m_Rotation.SetScalingFactors(ezVec3(2));

    ezTransform tToChild(ezVec3(4, 5, 6));
    tToChild.m_Rotation.SetRotationMatrix(ezVec3(0, 0, 1), ezAngle::Degree(90));
    tToChild.m_Rotation.SetScalingFactors(ezVec3(4));

    ezTransform tChild;
    tChild.SetGlobalTransform(tParent, tToChild);

    // negate twice -> get back original
    EZ_TEST_BOOL(tToChild.Invert().Succeeded());
    EZ_TEST_BOOL(tToChild.Invert().Succeeded());

    ezTransform tInvToChild = tToChild.GetInverse();

    ezTransform tParentFromChild;
    tParentFromChild.SetGlobalTransform(tChild, tInvToChild);

    EZ_TEST_BOOL(tParent.IsEqual(tParentFromChild, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsArray")
  {
    ezTransform t;
    t.m_Rotation.SetElements(1, 2, 3,
                             4, 5, 6,
                             7, 8, 9);
    t.m_vPosition.Set(10, 11, 12);

    {
      float f[13] = { 0 };

      t.GetAsArray(f, ezMatrixLayout::ColumnMajor);

      EZ_TEST_FLOAT(f[0], 1, 0);
      EZ_TEST_FLOAT(f[1], 4, 0);
      EZ_TEST_FLOAT(f[2], 7, 0);

      EZ_TEST_FLOAT(f[3], 2, 0);
      EZ_TEST_FLOAT(f[4], 5, 0);
      EZ_TEST_FLOAT(f[5], 8, 0);

      EZ_TEST_FLOAT(f[6], 3, 0);
      EZ_TEST_FLOAT(f[7], 6, 0);
      EZ_TEST_FLOAT(f[8], 9, 0);

      EZ_TEST_FLOAT(f[9], 10, 0);
      EZ_TEST_FLOAT(f[10],11, 0);
      EZ_TEST_FLOAT(f[11],12, 0);

      EZ_TEST_FLOAT(f[12], 0, 0);
    }

    {
      float f[13] = { 0 };

      t.GetAsArray(f, ezMatrixLayout::RowMajor);

      EZ_TEST_FLOAT(f[0], 1, 0);
      EZ_TEST_FLOAT(f[1], 2, 0);
      EZ_TEST_FLOAT(f[2], 3, 0);
      EZ_TEST_FLOAT(f[3],10, 0);

      EZ_TEST_FLOAT(f[4], 4, 0);
      EZ_TEST_FLOAT(f[5], 5, 0);
      EZ_TEST_FLOAT(f[6], 6, 0);
      EZ_TEST_FLOAT(f[7],11, 0);

      EZ_TEST_FLOAT(f[8], 7, 0);
      EZ_TEST_FLOAT(f[9], 8, 0);
      EZ_TEST_FLOAT(f[10],9, 0);
      EZ_TEST_FLOAT(f[11],12, 0);

      EZ_TEST_FLOAT(f[12], 0, 0);
    }
  }
}


