#include <PCH.h>
#include <Foundation/Math/Frustum.h>

EZ_CREATE_SIMPLE_TEST(Math, Frustum)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezFrustum f;
    EZ_TEST_INT(f.GetNumPlanes(), 0);
    EZ_TEST_BOOL(f.GetPosition().IsZero());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFrustum (planes)")
  {
    ezFrustum f;

    ezPlane p[2];
    p[0].SetFromNormalAndPoint(ezVec3(1, 0, 0), ezVec3(1, 2, 3));
    p[1].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));

    f.SetFrustum(ezVec3(5, 6, 7), 2, p);

    EZ_TEST_INT(f.GetNumPlanes(), 2);

    EZ_TEST_VEC3(f.GetPosition(), ezVec3(5, 6, 7), 0.00001f);
    EZ_TEST_BOOL(f.GetPlane(0) == p[0]);
    EZ_TEST_BOOL(f.GetPlane(1) == p[1]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TransformFrustum")
  {
    ezFrustum f;

    ezPlane p[2];
    p[0].SetFromNormalAndPoint(ezVec3(1, 0, 0), ezVec3(1, 2, 3));
    p[1].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));

    f.SetFrustum(ezVec3(5, 6, 7), 2, p);

    ezMat4 mTransform;
    mTransform.SetRotationMatrixY(ezAngle::Degree(90.0f));
    mTransform.SetTranslationVector(ezVec3(2, 3, 4));

    f.TransformFrustum(mTransform);

    EZ_TEST_INT(f.GetNumPlanes(), 2);

    p[0].Transform(mTransform);
    p[1].Transform(mTransform);

    EZ_TEST_VEC3(f.GetPosition(), mTransform * ezVec3(5, 6, 7), 0.00001f);
    EZ_TEST_BOOL(f.GetPlane(0).IsEqual(p[0], 0.001f));
    EZ_TEST_BOOL(f.GetPlane(1).IsEqual(p[1], 0.001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "InvertFrustum")
  {
    ezFrustum f;

    ezPlane p[2];
    p[0].SetFromNormalAndPoint(ezVec3(1, 0, 0), ezVec3(1, 2, 3));
    p[1].SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(2, 3, 4));

    f.SetFrustum(ezVec3(5, 6, 7), 2, p);

    f.InvertFrustum();

    EZ_TEST_INT(f.GetNumPlanes(), 2);

    p[0].Flip();
    p[1].Flip();

    EZ_TEST_VEC3(f.GetPosition(), ezVec3(5, 6, 7), 0.00001f);
    EZ_TEST_BOOL(f.GetPlane(0) == p[0]);
    EZ_TEST_BOOL(f.GetPlane(1) == p[1]);
  }
}

