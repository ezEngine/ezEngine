#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdTransformd.h>

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdTransformd)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezSimdTransformd t0;

    {
      ezSimdQuatd qRot = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(1, 2, 3).GetNormalized<3>(), ezAngled::MakeFromDegree(42.0).GetRadian());

      ezSimdVec4d pos(4, 5, 6);
      ezSimdVec4d scale(7, 8, 9);

      ezSimdTransformd t(pos);
      EZ_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      EZ_TEST_BOOL(t.m_Rotation == ezSimdQuatd::MakeIdentity());
      EZ_TEST_BOOL((t.m_Scale == ezSimdVec4d(1.0)).AllSet<3>());

      t = ezSimdTransformd(pos, qRot);
      EZ_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      EZ_TEST_BOOL(t.m_Rotation == qRot);
      EZ_TEST_BOOL((t.m_Scale == ezSimdVec4d(1.0)).AllSet<3>());

      t = ezSimdTransformd(pos, qRot, scale);
      EZ_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      EZ_TEST_BOOL(t.m_Rotation == qRot);
      EZ_TEST_BOOL((t.m_Scale == scale).AllSet<3>());

      t = ezSimdTransformd(qRot);
      EZ_TEST_BOOL(t.m_Position.IsZero<3>());
      EZ_TEST_BOOL(t.m_Rotation == qRot);
      EZ_TEST_BOOL((t.m_Scale == ezSimdVec4d(1.0)).AllSet<3>());
    }

    {
      ezSimdQuatd qRot;
      qRot = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(1, 2, 3).GetNormalized<3>(), ezAngled::MakeFromDegree(42.0).GetRadian());

      ezSimdVec4d pos(4, 5, 6);
      ezSimdVec4d scale(7, 8, 9);

      ezSimdTransformd t = ezSimdTransformd::Make(pos);
      EZ_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      EZ_TEST_BOOL(t.m_Rotation == ezSimdQuatd::MakeIdentity());
      EZ_TEST_BOOL((t.m_Scale == ezSimdVec4d(1.0)).AllSet<3>());

      t = ezSimdTransformd::Make(pos, qRot);
      EZ_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      EZ_TEST_BOOL(t.m_Rotation == qRot);
      EZ_TEST_BOOL((t.m_Scale == ezSimdVec4d(1.0)).AllSet<3>());

      t = ezSimdTransformd::Make(pos, qRot, scale);
      EZ_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      EZ_TEST_BOOL(t.m_Rotation == qRot);
      EZ_TEST_BOOL((t.m_Scale == scale).AllSet<3>());
    }

    {
      ezSimdTransformd t = ezSimdTransformd::MakeIdentity();

      EZ_TEST_BOOL(t.m_Position.IsZero<3>());
      EZ_TEST_BOOL(t.m_Rotation == ezSimdQuatd::MakeIdentity());
      EZ_TEST_BOOL((t.m_Scale == ezSimdVec4d(1.0)).AllSet<3>());

      EZ_TEST_BOOL(t == ezSimdTransformd::MakeIdentity());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Inverse")
  {
    ezSimdTransformd tParent(ezSimdVec4d(1, 2, 3));
    tParent.m_Rotation = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 1, 0), ezAngled::MakeFromDegree(90).GetRadian());
    tParent.m_Scale = ezSimdVec4d(2.0);

    ezSimdTransformd tToChild(ezSimdVec4d(4, 5, 6));
    tToChild.m_Rotation = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngled::MakeFromDegree(90).GetRadian());
    tToChild.m_Scale = ezSimdVec4d(4.0);

    ezSimdTransformd tChild;
    tChild = tParent * tToChild;

    // invert twice -> get back original
    ezSimdTransformd t2 = tToChild;
    t2.Invert();
    t2.Invert();
    EZ_TEST_BOOL(t2.IsEqual(tToChild, 0.0001f));

    ezSimdTransformd tInvToChild = tToChild.GetInverse();

    ezSimdTransformd tParentFromChild;
    tParentFromChild = tChild * tInvToChild;

    EZ_TEST_BOOL(tParent.IsEqual(tParentFromChild, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetLocalTransform")
  {
    ezSimdQuatd q;
    q = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngled::MakeFromDegree(90).GetRadian());

    ezSimdTransformd tParent(ezSimdVec4d(1, 2, 3));
    tParent.m_Rotation = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 1, 0), ezAngled::MakeFromDegree(90).GetRadian());
    tParent.m_Scale = ezSimdVec4d(2.0);

    ezSimdTransformd tChild;
    tChild.m_Position = ezSimdVec4d(13, 12, -5);
    tChild.m_Rotation = tParent.m_Rotation * q;
    tChild.m_Scale = ezSimdVec4d(8.0);

    ezSimdTransformd tToChild = ezSimdTransformd::MakeLocalTransform(tParent, tChild);

    EZ_TEST_BOOL(tToChild.m_Position.IsEqual(ezSimdVec4d(4, 5, 6), 0.0001f).AllSet<3>());
    EZ_TEST_BOOL(tToChild.m_Rotation.IsEqualRotation(q, 0.0001f));
    EZ_TEST_BOOL((tToChild.m_Scale == ezSimdVec4d(4.0)).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetGlobalTransform")
  {
    ezSimdTransformd tParent(ezSimdVec4d(1, 2, 3));
    tParent.m_Rotation = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 1, 0), ezAngled::MakeFromDegree(90).GetRadian());
    tParent.m_Scale = ezSimdVec4d(2.0);

    ezSimdTransformd tToChild(ezSimdVec4d(4, 5, 6));
    tToChild.m_Rotation = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngled::MakeFromDegree(90).GetRadian());
    tToChild.m_Scale = ezSimdVec4d(4.0);

    ezSimdTransformd tChild = ezSimdTransformd::MakeGlobalTransform(tParent, tToChild);

    EZ_TEST_BOOL(tChild.m_Position.IsEqual(ezSimdVec4d(13, 12, -5), 0.0001f).AllSet<3>());
    EZ_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
    EZ_TEST_BOOL((tChild.m_Scale == ezSimdVec4d(8.0)).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsMat4")
  {
    ezSimdTransformd t(ezSimdVec4d(1, 2, 3));
    t.m_Rotation = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 1, 0), ezAngled::MakeFromDegree(34).GetRadian());
    t.m_Scale = ezSimdVec4d(2, -1, 5);

    ezSimdMat4d m = t.GetAsMat4();

    // reference
    ezSimdMat4d refM;
    {
      ezQuatd q = ezQuatd::MakeFromAxisAndAngle(ezVec3d(0, 1, 0), ezAngled::MakeFromDegree(34));

      ezTransformd referenceTransform(ezVec3d(1, 2, 3), q, ezVec3d(2, -1, 5));
      ezMat4d tmp = referenceTransform.GetAsMat4();
      refM = ezSimdMat4d::MakeFromColumnMajorArray(tmp.m_fElementsCM);
    }
    EZ_TEST_BOOL(m.IsEqual(refM, 0.00001f));

    ezSimdVec4d p[8] = {ezSimdVec4d(-4, 0, 0), ezSimdVec4d(5, 0, 0), ezSimdVec4d(0, -6, 0), ezSimdVec4d(0, 7, 0), ezSimdVec4d(0, 0, -8),
      ezSimdVec4d(0, 0, 9), ezSimdVec4d(1, -2, 3), ezSimdVec4d(-4, 5, 7)};

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(p); ++i)
    {
      ezSimdVec4d pt = t.TransformPosition(p[i]);
      ezSimdVec4d pm = m.TransformPosition(p[i]);

      EZ_TEST_BOOL(pt.IsEqual(pm, 0.00001f).AllSet<3>());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformPos / Dir / operator*")
  {
    ezSimdQuatd qRotX, qRotY;
    qRotX = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(1, 0, 0), ezAngled::MakeFromDegree(90.0).GetRadian());
    qRotY = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 1, 0), ezAngled::MakeFromDegree(90.0).GetRadian());

    ezSimdTransformd t(ezSimdVec4d(1, 2, 3, 10), qRotY * qRotX, ezSimdVec4d(2, -2, 4, 11));

    ezSimdVec4d v;
    v = t.TransformPosition(ezSimdVec4d(4, 5, 6, 12));
    EZ_TEST_BOOL(v.IsEqual(ezSimdVec4d((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f).AllSet<3>());

    v = t.TransformDirection(ezSimdVec4d(4, 5, 6, 13));
    EZ_TEST_BOOL(v.IsEqual(ezSimdVec4d((5 * -2), (-6 * 4), (-4 * 2)), 0.0001f).AllSet<3>());

    v = t * ezSimdVec4d(4, 5, 6, 12);
    EZ_TEST_BOOL(v.IsEqual(ezSimdVec4d((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f).AllSet<3>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    {
      ezSimdTransformd tParent(ezSimdVec4d(1, 2, 3));
      tParent.m_Rotation = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 1, 0), ezAngled::MakeFromDegree(90).GetRadian());
      tParent.m_Scale = ezSimdVec4d(2.0);

      ezSimdTransformd tToChild(ezSimdVec4d(4, 5, 6));
      tToChild.m_Rotation = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngled::MakeFromDegree(90).GetRadian());
      tToChild.m_Scale = ezSimdVec4d(4.0);

      // this is exactly the same as SetGlobalTransform
      ezSimdTransformd tChild;
      tChild = tParent * tToChild;

      EZ_TEST_BOOL(tChild.m_Position.IsEqual(ezSimdVec4d(13, 12, -5), 0.0001f).AllSet<3>());
      EZ_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
      EZ_TEST_BOOL((tChild.m_Scale == ezSimdVec4d(8.0)).AllSet<3>());

      tChild = tParent;
      tChild *= tToChild;

      EZ_TEST_BOOL(tChild.m_Position.IsEqual(ezSimdVec4d(13, 12, -5), 0.0001f).AllSet<3>());
      EZ_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
      EZ_TEST_BOOL((tChild.m_Scale == ezSimdVec4d(8.0)).AllSet<3>());

      ezSimdVec4d a(7, 8, 9);
      ezSimdVec4d b;
      b = tToChild.TransformPosition(a);
      b = tParent.TransformPosition(b);

      ezSimdVec4d c;
      c = tChild.TransformPosition(a);

      EZ_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());

      // verify that it works exactly like a 4x4 matrix
      /*const ezMat4 mParent = tParent.GetAsMat4();
      const ezMat4 mToChild = tToChild.GetAsMat4();
      const ezMat4 mChild = mParent * mToChild;

      EZ_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));*/
    }

    {
      ezSimdTransformd t(ezSimdVec4d(1, 2, 3));
      t.m_Rotation = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 1, 0), ezAngled::MakeFromDegree(90).GetRadian());
      t.m_Scale = ezSimdVec4d(2.0);

      ezSimdQuatd q = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngled::MakeFromDegree(90).GetRadian());

      ezSimdTransformd t2 = t * q;
      ezSimdTransformd t4 = q * t;

      ezSimdTransformd t3 = t;
      t3 *= q;
      EZ_TEST_BOOL(t2 == t3);
      EZ_TEST_BOOL(t3 != t4);

      ezSimdVec4d a(7, 8, 9);
      ezSimdVec4d b;
      b = t2.TransformPosition(a);

      ezSimdVec4d c = q * a;
      c = t.TransformPosition(c);

      EZ_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }

    {
      ezSimdTransformd t(ezSimdVec4d(1, 2, 3));
      t.m_Rotation = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 1, 0), ezAngle::MakeFromDegree(90).GetRadian());
      t.m_Scale = ezSimdVec4d(2.0);

      ezSimdVec4d p(4, 5, 6);

      ezSimdTransformd t2 = t + p;
      ezSimdTransformd t3 = t;
      t3 += p;
      EZ_TEST_BOOL(t2 == t3);

      ezSimdVec4d a(7, 8, 9);
      ezSimdVec4d b;
      b = t2.TransformPosition(a);

      ezSimdVec4d c = t.TransformPosition(a) + p;

      EZ_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }

    {
      ezSimdTransformd t(ezSimdVec4d(1, 2, 3));
      t.m_Rotation = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 1, 0), ezAngle::MakeFromDegree(90).GetRadian());
      t.m_Scale = ezSimdVec4d(2.0);

      ezSimdVec4d p(4, 5, 6);

      ezSimdTransformd t2 = t - p;
      ezSimdTransformd t3 = t;
      t3 -= p;
      EZ_TEST_BOOL(t2 == t3);

      ezSimdVec4d a(7, 8, 9);
      ezSimdVec4d b;
      b = t2.TransformPosition(a);

      ezSimdVec4d c = t.TransformPosition(a) - p;

      EZ_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Comparison")
  {
    ezSimdTransformd t(ezSimdVec4d(1, 2, 3));
    t.m_Rotation = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 1, 0), ezAngled::MakeFromDegree(90).GetRadian());

    EZ_TEST_BOOL(t == t);

    ezSimdTransformd t2(ezSimdVec4d(1, 2, 4));
    t2.m_Rotation = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 1, 0), ezAngled::MakeFromDegree(90).GetRadian());

    EZ_TEST_BOOL(t != t2);

    ezSimdTransformd t3(ezSimdVec4d(1, 2, 3));
    t3.m_Rotation = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 1, 0), ezAngled::MakeFromDegree(91).GetRadian());

    EZ_TEST_BOOL(t != t3);
  }
}
