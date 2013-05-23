#include <PCH.h>
#include <Foundation/Math/Quat.h>

EZ_CREATE_SIMPLE_TEST(Math, Quaternion)
{
  EZ_TEST_BLOCK(true, "Default Constructor")
  {
    #if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
        // In debug the default constructor initializes everything with NaN.
        ezQuat p;
        EZ_TEST(ezMath::IsNaN(p.v.x) && ezMath::IsNaN(p.v.y) && ezMath::IsNaN(p.v.z) && ezMath::IsNaN(p.w));
    #else
        // Placement new of the default constructor should not have any effect on the previous data.
        float testBlock[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        ezQuat* p = ::new ((void*) &testBlock[0]) ezQuat;
        EZ_TEST(p->v.x == 1.0f && p->v.y == 2.0f && p->v.z == 3.0f && p->w == 4.0f);
    #endif
  }

  EZ_TEST_BLOCK(true, "Constructor(x,y,z,w)")
  {
    ezQuat q(1, 2, 3, 4);

    EZ_TEST_VEC3(q.v, ezVec3(1, 2, 3), 0.0001f);
    EZ_TEST_FLOAT(q.w, 4, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "SetIdentity")
  {
    ezQuat q(1, 2, 3, 4);

    q.SetIdentity();

    EZ_TEST_VEC3(q.v, ezVec3(0, 0, 0), 0.0001f);
    EZ_TEST_FLOAT(q.w, 1, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "SetElements")
  {
    ezQuat q(5, 6, 7, 8);

    q.SetElements(1, 2, 3, 4);

    EZ_TEST_VEC3(q.v, ezVec3(1, 2, 3), 0.0001f);
    EZ_TEST_FLOAT(q.w, 4, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "SetFromAxisAndAngle / operator* (quat, vec)")
  {
    {
      ezQuat q;
      q.SetFromAxisAndAngle(ezVec3(1, 0, 0), 90);

      EZ_TEST_VEC3(q * ezVec3(0, 1, 0), ezVec3(0, 0, 1), 0.0001f);
    }

    {
      ezQuat q;
      q.SetFromAxisAndAngle(ezVec3(0, 1, 0), 90);

      EZ_TEST_VEC3(q * ezVec3(1, 0, 0), ezVec3(0, 0, -1), 0.0001f);
    }

    {
      ezQuat q;
      q.SetFromAxisAndAngle(ezVec3(0, 0, 1), 90);

      EZ_TEST_VEC3(q * ezVec3(0, 1, 0), ezVec3(-1, 0, 0), 0.0001f);
    }
  }

  EZ_TEST_BLOCK(true, "SetShortestRotation / IsEqualRotation")
  {
    ezQuat q1, q2, q3;
    q1.SetShortestRotation(ezVec3(0, 1, 0), ezVec3(1, 0, 0));
    q2.SetFromAxisAndAngle(ezVec3(0, 0, -1), 90);
    q3.SetFromAxisAndAngle(ezVec3(0, 0, 1), -90);

    EZ_TEST(q1.IsEqualRotation(q2, 0.0001f));
    EZ_TEST(q1.IsEqualRotation(q3, 0.0001f));
  }

  EZ_TEST_BLOCK(true, "SetFromMat3")
  {
    ezMat3 m;
    m.SetRotationMatrixZ(-90);

    ezQuat q1, q2, q3;
    q1.SetFromMat3(m);
    q2.SetFromAxisAndAngle(ezVec3(0, 0, -1), 90);
    q3.SetFromAxisAndAngle(ezVec3(0, 0, 1), -90);

    EZ_TEST(q1.IsEqualRotation(q2, 0.0001f));
    EZ_TEST(q1.IsEqualRotation(q3, 0.0001f));
  }

  EZ_TEST_BLOCK(true, "SetSlerp")
  {
    ezQuat q1, q2, q3, qr;
    q1.SetFromAxisAndAngle(ezVec3(0, 0, 1), 45);
    q2.SetFromAxisAndAngle(ezVec3(0, 0, 1), 0);
    q3.SetFromAxisAndAngle(ezVec3(0, 0, 1), 90);

    qr.SetSlerp(q2, q3, 0.5f);

    EZ_TEST(q1.IsEqualRotation(qr, 0.0001f));
  }

  EZ_TEST_BLOCK(true, "GetRotationAxisAndAngle")
  {
    ezQuat q1, q2, q3;
    q1.SetShortestRotation(ezVec3(0, 1, 0), ezVec3(1, 0, 0));
    q2.SetFromAxisAndAngle(ezVec3(0, 0, -1), 90);
    q3.SetFromAxisAndAngle(ezVec3(0, 0, 1), -90);

    ezVec3 axis;
    float angle;

    EZ_TEST(q1.GetRotationAxisAndAngle(axis, angle) == EZ_SUCCESS);
    EZ_TEST_VEC3(axis, ezVec3(0, 0, -1), 0.001f);
    EZ_TEST_FLOAT(angle, 90, 0.0001f);

    EZ_TEST(q2.GetRotationAxisAndAngle(axis, angle) == EZ_SUCCESS);
    EZ_TEST_VEC3(axis, ezVec3(0, 0, -1), 0.001f);
    EZ_TEST_FLOAT(angle, 90, 0.0001f);

    EZ_TEST(q3.GetRotationAxisAndAngle(axis, angle) == EZ_SUCCESS);
    EZ_TEST_VEC3(axis, ezVec3(0, 0, -1), 0.001f);
    EZ_TEST_FLOAT(angle, 90, 0.0001f);
  }

  EZ_TEST_BLOCK(true, "GetAsMat3")
  {
    ezQuat q;
    q.SetFromAxisAndAngle(ezVec3(0, 0, 1), 90);

    ezMat3 mr;
    mr.SetRotationMatrixZ(90);

    ezMat3 m = q.GetAsMat3();

    EZ_TEST(mr.IsEqual(m, 0.00001f));
  }

  EZ_TEST_BLOCK(true, "GetAsMat4")
  {
    ezQuat q;
    q.SetFromAxisAndAngle(ezVec3(0, 0, 1), 90);

    ezMat4 mr;
    mr.SetRotationMatrixZ(90);

    ezMat4 m = q.GetAsMat4();

    EZ_TEST(mr.IsEqual(m, 0.00001f));
  }

  EZ_TEST_BLOCK(true, "IsValid / Normalize")
  {
    ezQuat q(1, 2, 3, 4);
    EZ_TEST(!q.IsValid(0.001f));

    q.Normalize();
    EZ_TEST(q.IsValid(0.001f));
  }

  EZ_TEST_BLOCK(true, "operator-")
  {
    ezQuat q, q1;
    q.SetFromAxisAndAngle(ezVec3(0, 0, 1), 90);
    q1.SetFromAxisAndAngle(ezVec3(0, 0, 1), -90);

    ezQuat q2 = -q;
    EZ_TEST(q1.IsEqualRotation(q2, 0.0001f));
  }

  EZ_TEST_BLOCK(true, "operator*(quat, quat)")
  {
    ezQuat q1, q2, qr, q3;
    q1.SetFromAxisAndAngle(ezVec3(0, 0, 1), 60);
    q2.SetFromAxisAndAngle(ezVec3(0, 0, 1), 30);
    q3.SetFromAxisAndAngle(ezVec3(0, 0, 1), 90);

    qr = q1 * q2;

    EZ_TEST(qr.IsEqualRotation(q3, 0.0001f));
  }
}

