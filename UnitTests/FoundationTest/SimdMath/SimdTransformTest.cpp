#include <PCH.h>

#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdTransform.h>

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdTransform)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezSimdTransform t0;

    {
      ezSimdQuat qRot;
      qRot.SetFromAxisAndAngle(ezSimdVec4f(1, 2, 3).GetNormalized<3>(), ezAngle::Degree(42.0f));

      ezSimdVec4f pos(4, 5, 6);
      ezSimdVec4f scale(7, 8, 9);

      ezSimdTransform t(pos);
      EZ_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      EZ_TEST_BOOL(t.m_Rotation == ezSimdQuat::IdentityQuaternion());
      EZ_TEST_BOOL((t.m_Scale == ezSimdVec4f(1)).AllSet<3>());

      t = ezSimdTransform(pos, qRot);
      EZ_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      EZ_TEST_BOOL(t.m_Rotation == qRot);
      EZ_TEST_BOOL((t.m_Scale == ezSimdVec4f(1)).AllSet<3>());

      t = ezSimdTransform(pos, qRot, scale);
      EZ_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      EZ_TEST_BOOL(t.m_Rotation == qRot);
      EZ_TEST_BOOL((t.m_Scale == scale).AllSet<3>());

      t = ezSimdTransform(qRot);
      EZ_TEST_BOOL(t.m_Position.IsZero<3>());
      EZ_TEST_BOOL(t.m_Rotation == qRot);
      EZ_TEST_BOOL((t.m_Scale == ezSimdVec4f(1)).AllSet<3>());
    }

    {
      ezSimdTransform t;
      t.SetIdentity();

      EZ_TEST_BOOL(t.m_Position.IsZero<3>());
      EZ_TEST_BOOL(t.m_Rotation == ezSimdQuat::IdentityQuaternion());
      EZ_TEST_BOOL((t.m_Scale == ezSimdVec4f(1)).AllSet<3>());

      EZ_TEST_BOOL(t == ezSimdTransform::IdentityTransform());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Inverse")
  {
    ezSimdTransform tParent(ezSimdVec4f(1, 2, 3));
    tParent.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 1, 0), ezAngle::Degree(90));
    tParent.m_Scale = ezSimdVec4f(2);

    ezSimdTransform tToChild(ezSimdVec4f(4, 5, 6));
    tToChild.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(90));
    tToChild.m_Scale = ezSimdVec4f(4);

    ezSimdTransform tChild;
    tChild = tParent * tToChild;

    // invert twice -> get back original
    ezSimdTransform t2 = tToChild;
    t2.Invert();
    t2.Invert();
    EZ_TEST_BOOL(t2.IsEqual(tToChild, 0.0001f));

    ezSimdTransform tInvToChild = tToChild.GetInverse();

    ezSimdTransform tParentFromChild;
    tParentFromChild = tChild * tInvToChild;

    EZ_TEST_BOOL(tParent.IsEqual(tParentFromChild, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetLocalTransform")
  {
    ezSimdQuat q;
    q.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(90));

    ezSimdTransform tParent(ezSimdVec4f(1, 2, 3));
    tParent.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 1, 0), ezAngle::Degree(90));
    tParent.m_Scale = ezSimdVec4f(2);

    ezSimdTransform tChild;
    tChild.m_Position = ezSimdVec4f(13, 12, -5);
    tChild.m_Rotation = tParent.m_Rotation * q;
    tChild.m_Scale = ezSimdVec4f(8);

    ezSimdTransform tToChild;
    tToChild.SetLocalTransform(tParent, tChild);

    EZ_TEST_BOOL(tToChild.m_Position.IsEqual(ezSimdVec4f(4, 5, 6), 0.0001f).AllSet<3>());
    EZ_TEST_BOOL(tToChild.m_Rotation.IsEqualRotation(q, 0.0001f));
    EZ_TEST_BOOL((tToChild.m_Scale == ezSimdVec4f(4)).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetGlobalTransform")
  {
    ezSimdTransform tParent(ezSimdVec4f(1, 2, 3));
    tParent.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 1, 0), ezAngle::Degree(90));
    tParent.m_Scale = ezSimdVec4f(2);

    ezSimdTransform tToChild(ezSimdVec4f(4, 5, 6));
    tToChild.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(90));
    tToChild.m_Scale = ezSimdVec4f(4);

    ezSimdTransform tChild;
    tChild.SetGlobalTransform(tParent, tToChild);

    EZ_TEST_BOOL(tChild.m_Position.IsEqual(ezSimdVec4f(13, 12, -5), 0.0001f).AllSet<3>());
    EZ_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
    EZ_TEST_BOOL((tChild.m_Scale == ezSimdVec4f(8)).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsMat4")
  {
    ezSimdTransform t(ezSimdVec4f(1, 2, 3));
    t.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 1, 0), ezAngle::Degree(34));
    t.m_Scale = ezSimdVec4f(2, -1, 5);

    ezSimdMat4f m = t.GetAsMat4();

    // reference
    ezSimdMat4f refM;
    {
      ezQuat q;
      q.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(34));

      ezTransform referenceTransform(ezVec3(1, 2, 3), q, ezVec3(2, -1, 5));
      ezMat4 tmp = referenceTransform.GetAsMat4();
      refM.SetFromArray(tmp.m_fElementsCM, ezMatrixLayout::ColumnMajor);
    }
    EZ_TEST_BOOL(m.IsEqual(refM, 0.00001f));

    ezSimdVec4f p[8] = {ezSimdVec4f(-4, 0, 0), ezSimdVec4f(5, 0, 0), ezSimdVec4f(0, -6, 0), ezSimdVec4f(0, 7, 0),
                        ezSimdVec4f(0, 0, -8), ezSimdVec4f(0, 0, 9), ezSimdVec4f(1, -2, 3), ezSimdVec4f(-4, 5, 7)};

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(p); ++i)
    {
      ezSimdVec4f pt = t.TransformPosition(p[i]);
      ezSimdVec4f pm = m.TransformPosition(p[i]);

      EZ_TEST_BOOL(pt.IsEqual(pm, 0.00001f).AllSet<3>());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformPos / Dir / operator*")
  {
    ezSimdQuat qRotX, qRotY;
    qRotX.SetFromAxisAndAngle(ezSimdVec4f(1, 0, 0), ezAngle::Degree(90.0f));
    qRotY.SetFromAxisAndAngle(ezSimdVec4f(0, 1, 0), ezAngle::Degree(90.0f));

    ezSimdTransform t(ezSimdVec4f(1, 2, 3, 10), qRotY * qRotX, ezSimdVec4f(2, -2, 4, 11));

    ezSimdVec4f v;
    v = t.TransformPosition(ezSimdVec4f(4, 5, 6, 12));
    EZ_TEST_BOOL(v.IsEqual(ezSimdVec4f((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f).AllSet<3>());

    v = t.TransformDirection(ezSimdVec4f(4, 5, 6, 13));
    EZ_TEST_BOOL(v.IsEqual(ezSimdVec4f((5 * -2), (-6 * 4), (-4 * 2)), 0.0001f).AllSet<3>());

    v = t * ezSimdVec4f(4, 5, 6, 12);
    EZ_TEST_BOOL(v.IsEqual(ezSimdVec4f((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    {
      ezSimdTransform tParent(ezSimdVec4f(1, 2, 3));
      tParent.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 1, 0), ezAngle::Degree(90));
      tParent.m_Scale = ezSimdVec4f(2);

      ezSimdTransform tToChild(ezSimdVec4f(4, 5, 6));
      tToChild.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(90));
      tToChild.m_Scale = ezSimdVec4f(4);

      // this is exactly the same as SetGlobalTransform
      ezSimdTransform tChild;
      tChild = tParent * tToChild;

      EZ_TEST_BOOL(tChild.m_Position.IsEqual(ezSimdVec4f(13, 12, -5), 0.0001f).AllSet<3>());
      EZ_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
      EZ_TEST_BOOL((tChild.m_Scale == ezSimdVec4f(8)).AllSet<3>());

      tChild = tParent;
      tChild *= tToChild;

      EZ_TEST_BOOL(tChild.m_Position.IsEqual(ezSimdVec4f(13, 12, -5), 0.0001f).AllSet<3>());
      EZ_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
      EZ_TEST_BOOL((tChild.m_Scale == ezSimdVec4f(8)).AllSet<3>());

      ezSimdVec4f a(7, 8, 9);
      ezSimdVec4f b;
      b = tToChild.TransformPosition(a);
      b = tParent.TransformPosition(b);

      ezSimdVec4f c;
      c = tChild.TransformPosition(a);

      EZ_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());

      // verify that it works exactly like a 4x4 matrix
      /*const ezMat4 mParent = tParent.GetAsMat4();
      const ezMat4 mToChild = tToChild.GetAsMat4();
      const ezMat4 mChild = mParent * mToChild;

      EZ_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));*/
    }

    {
      ezSimdTransform t(ezSimdVec4f(1, 2, 3));
      t.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 1, 0), ezAngle::Degree(90));
      t.m_Scale = ezSimdVec4f(2);

      ezSimdQuat q;
      q.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(90));

      ezSimdTransform t2 = t * q;
      ezSimdTransform t4 = q * t;

      ezSimdTransform t3 = t;
      t3 *= q;
      EZ_TEST_BOOL(t2 == t3);
      EZ_TEST_BOOL(t3 != t4);

      ezSimdVec4f a(7, 8, 9);
      ezSimdVec4f b;
      b = t2.TransformPosition(a);

      ezSimdVec4f c = q * a;
      c = t.TransformPosition(c);

      EZ_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }

    {
      ezSimdTransform t(ezSimdVec4f(1, 2, 3));
      t.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 1, 0), ezAngle::Degree(90));
      t.m_Scale = ezSimdVec4f(2);

      ezSimdVec4f p(4, 5, 6);

      ezSimdTransform t2 = t + p;
      ezSimdTransform t3 = t;
      t3 += p;
      EZ_TEST_BOOL(t2 == t3);

      ezSimdVec4f a(7, 8, 9);
      ezSimdVec4f b;
      b = t2.TransformPosition(a);

      ezSimdVec4f c = t.TransformPosition(a) + p;

      EZ_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }

    {
      ezSimdTransform t(ezSimdVec4f(1, 2, 3));
      t.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 1, 0), ezAngle::Degree(90));
      t.m_Scale = ezSimdVec4f(2);

      ezSimdVec4f p(4, 5, 6);

      ezSimdTransform t2 = t - p;
      ezSimdTransform t3 = t;
      t3 -= p;
      EZ_TEST_BOOL(t2 == t3);

      ezSimdVec4f a(7, 8, 9);
      ezSimdVec4f b;
      b = t2.TransformPosition(a);

      ezSimdVec4f c = t.TransformPosition(a) - p;

      EZ_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Comparison")
  {
    ezSimdTransform t(ezSimdVec4f(1, 2, 3));
    t.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 1, 0), ezAngle::Degree(90));

    EZ_TEST_BOOL(t == t);

    ezSimdTransform t2(ezSimdVec4f(1, 2, 4));
    t2.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 1, 0), ezAngle::Degree(90));

    EZ_TEST_BOOL(t != t2);

    ezSimdTransform t3(ezSimdVec4f(1, 2, 3));
    t3.m_Rotation.SetFromAxisAndAngle(ezSimdVec4f(0, 1, 0), ezAngle::Degree(91));

    EZ_TEST_BOOL(t != t3);
  }
}
