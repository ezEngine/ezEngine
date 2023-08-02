#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Quat.h>

EZ_CREATE_SIMPLE_TEST(Math, Quaternion)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Default Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (ezMath::SupportsNaN<ezMat3T::ComponentType>())
    {
      // In debug the default constructor initializes everything with NaN.
      ezQuatT p;
      EZ_TEST_BOOL(ezMath::IsNaN(p.x) && ezMath::IsNaN(p.y) && ezMath::IsNaN(p.z) && ezMath::IsNaN(p.w));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    ezQuatT::ComponentType testBlock[4] = {
      (ezQuatT::ComponentType)1, (ezQuatT::ComponentType)2, (ezQuatT::ComponentType)3, (ezQuatT::ComponentType)4};
    ezQuatT* p = ::new ((void*)&testBlock[0]) ezQuatT;
    EZ_TEST_BOOL(p->x == (ezMat3T::ComponentType)1 && p->y == (ezMat3T::ComponentType)2 && p->z == (ezMat3T::ComponentType)3 &&
                 p->w == (ezMat3T::ComponentType)4);
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(x,y,z,w)")
  {
    ezQuatT q(1, 2, 3, 4);

    EZ_TEST_VEC3(q.GetVectorPart(), ezVec3T(1, 2, 3), 0.0001f);
    EZ_TEST_FLOAT(q.w, 4, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeIdentity")
  {
    ezQuatT q = ezQuatT::MakeIdentity();

    EZ_TEST_VEC3(q.GetVectorPart(), ezVec3T(0, 0, 0), 0.0001f);
    EZ_TEST_FLOAT(q.w, 1, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetIdentity")
  {
    ezQuatT q(1, 2, 3, 4);

    q.SetIdentity();

    EZ_TEST_VEC3(q.GetVectorPart(), ezVec3T(0, 0, 0), 0.0001f);
    EZ_TEST_FLOAT(q.w, 1, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetElements")
  {
    ezQuatT q(5, 6, 7, 8);

    q = ezQuat(1, 2, 3, 4);

    EZ_TEST_VEC3(q.GetVectorPart(), ezVec3T(1, 2, 3), 0.0001f);
    EZ_TEST_FLOAT(q.w, 4, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromAxisAndAngle / operator* (quat, vec)")
  {
    {
      ezQuatT q;
      q = ezQuat::MakeFromAxisAndAngle(ezVec3T(1, 0, 0), ezAngle::MakeFromDegree(90));

      EZ_TEST_VEC3(q * ezVec3T(0, 1, 0), ezVec3T(0, 0, 1), 0.0001f);
    }

    {
      ezQuatT q;
      q = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 1, 0), ezAngle::MakeFromDegree(90));

      EZ_TEST_VEC3(q * ezVec3T(1, 0, 0), ezVec3T(0, 0, -1), 0.0001f);
    }

    {
      ezQuatT q;
      q = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(90));

      EZ_TEST_VEC3(q * ezVec3T(0, 1, 0), ezVec3T(-1, 0, 0), 0.0001f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetShortestRotation / IsEqualRotation")
  {
    ezQuatT q1, q2, q3;
    q1 = ezQuat::MakeShortestRotation(ezVec3T(0, 1, 0), ezVec3T(1, 0, 0));
    q2 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, -1), ezAngle::MakeFromDegree(90));
    q3 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(-90));

    EZ_TEST_BOOL(q1.IsEqualRotation(q2, ezMath::LargeEpsilon<float>()));
    EZ_TEST_BOOL(q1.IsEqualRotation(q3, ezMath::LargeEpsilon<float>()));

    EZ_TEST_BOOL(ezQuatT::MakeIdentity().IsEqualRotation(ezQuatT::MakeIdentity(), ezMath::LargeEpsilon<float>()));
    EZ_TEST_BOOL(ezQuatT::MakeIdentity().IsEqualRotation(ezQuatT(0, 0, 0, -1), ezMath::LargeEpsilon<float>()));

    ezQuatT q4{0, 0, 0, 1.00000012f};
    ezQuatT q5{0, 0, 0, 1.00000023f};
    EZ_TEST_BOOL(q4.IsEqualRotation(q5, ezMath::LargeEpsilon<float>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromMat3")
  {
    ezMat3T m;
    m.SetRotationMatrixZ(ezAngle::MakeFromDegree(-90));

    ezQuatT q1, q2, q3;
    q1 = ezQuat::MakeFromMat3(m);
    q2 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, -1), ezAngle::MakeFromDegree(90));
    q3 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(-90));

    EZ_TEST_BOOL(q1.IsEqualRotation(q2, ezMath::LargeEpsilon<float>()));
    EZ_TEST_BOOL(q1.IsEqualRotation(q3, ezMath::LargeEpsilon<float>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetSlerp")
  {
    ezQuatT q1, q2, q3, qr;
    q1 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(45));
    q2 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(0));
    q3 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(90));

    qr = ezQuat::MakeSlerp(q2, q3, 0.5f);

    EZ_TEST_BOOL(q1.IsEqualRotation(qr, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRotationAxisAndAngle")
  {
    ezQuatT q1, q2, q3;
    q1 = ezQuat::MakeShortestRotation(ezVec3T(0, 1, 0), ezVec3T(1, 0, 0));
    q2 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, -1), ezAngle::MakeFromDegree(90));
    q3 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(-90));

    ezVec3T axis;
    ezAngle angle;

    q1.GetRotationAxisAndAngle(axis, angle);
    EZ_TEST_VEC3(axis, ezVec3T(0, 0, -1), 0.001f);
    EZ_TEST_FLOAT(angle.GetDegree(), 90, ezMath::LargeEpsilon<ezMat3T::ComponentType>());

    q2.GetRotationAxisAndAngle(axis, angle);
    EZ_TEST_VEC3(axis, ezVec3T(0, 0, -1), 0.001f);
    EZ_TEST_FLOAT(angle.GetDegree(), 90, ezMath::LargeEpsilon<ezMat3T::ComponentType>());

    q3.GetRotationAxisAndAngle(axis, angle);
    EZ_TEST_VEC3(axis, ezVec3T(0, 0, -1), 0.001f);
    EZ_TEST_FLOAT(angle.GetDegree(), 90, ezMath::LargeEpsilon<ezMat3T::ComponentType>());

    ezQuatT::MakeIdentity().GetRotationAxisAndAngle(axis, angle);
    EZ_TEST_VEC3(axis, ezVec3T(1, 0, 0), 0.001f);
    EZ_TEST_FLOAT(angle.GetDegree(), 0, ezMath::LargeEpsilon<ezMat3T::ComponentType>());

    ezQuatT otherIdentity(0, 0, 0, -1);
    otherIdentity.GetRotationAxisAndAngle(axis, angle);
    EZ_TEST_VEC3(axis, ezVec3T(1, 0, 0), 0.001f);
    EZ_TEST_FLOAT(angle.GetDegree(), 360, ezMath::LargeEpsilon<ezMat3T::ComponentType>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsMat3")
  {
    ezQuatT q;
    q = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(90));

    ezMat3T mr;
    mr.SetRotationMatrixZ(ezAngle::MakeFromDegree(90));

    ezMat3T m = q.GetAsMat3();

    EZ_TEST_BOOL(mr.IsEqual(m, ezMath::DefaultEpsilon<ezMat3T::ComponentType>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsMat4")
  {
    ezQuatT q;
    q = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(90));

    ezMat4T mr;
    mr.SetRotationMatrixZ(ezAngle::MakeFromDegree(90));

    ezMat4T m = q.GetAsMat4();

    EZ_TEST_BOOL(mr.IsEqual(m, ezMath::DefaultEpsilon<ezMat3T::ComponentType>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsValid / Normalize")
  {
    ezQuatT q(1, 2, 3, 4);
    EZ_TEST_BOOL(!q.IsValid(0.001f));

    q.Normalize();
    EZ_TEST_BOOL(q.IsValid(0.001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator-/Invert")
  {
    ezQuatT q, q1;
    q = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(90));
    q1 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(-90));

    ezQuatT q2 = -q;
    EZ_TEST_BOOL(q1.IsEqualRotation(q2, 0.0001f));

    ezQuatT q3 = q;
    q3.Invert();
    EZ_TEST_BOOL(q1.IsEqualRotation(q3, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Dot")
  {
    ezQuatT q, q1, q2;
    q = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(90));
    q1 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(-90));
    q2 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 1, 0), ezAngle::MakeFromDegree(45));

    EZ_TEST_FLOAT(q.Dot(q), 1.0f, 0.0001f);
    EZ_TEST_FLOAT(q.Dot(ezQuat::MakeIdentity()), cos(ezAngle::DegToRad(90.0f / 2)), 0.0001f);
    EZ_TEST_FLOAT(q.Dot(q1), 0.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(quat, quat)")
  {
    ezQuatT q1, q2, qr, q3;
    q1 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(60));
    q2 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(30));
    q3 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(90));

    qr = q1 * q2;

    EZ_TEST_BOOL(qr.IsEqualRotation(q3, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator==/!=")
  {
    ezQuatT q1, q2;
    q1 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(60));
    q2 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(30));
    EZ_TEST_BOOL(q1 != q2);

    q2 = ezQuat::MakeFromAxisAndAngle(ezVec3T(1, 0, 0), ezAngle::MakeFromDegree(60));
    EZ_TEST_BOOL(q1 != q2);

    q2 = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(60));
    EZ_TEST_BOOL(q1 == q2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::SupportsNaN<ezMathTestType>())
    {
      ezQuatT q;

      q.SetIdentity();
      EZ_TEST_BOOL(!q.IsNaN());

      q.SetIdentity();
      q.w = ezMath::NaN<ezMathTestType>();
      EZ_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.x = ezMath::NaN<ezMathTestType>();
      EZ_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.y = ezMath::NaN<ezMathTestType>();
      EZ_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.z = ezMath::NaN<ezMathTestType>();
      EZ_TEST_BOOL(q.IsNaN());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "rotation direction")
  {
    ezMat3T m;
    m.SetRotationMatrixZ(ezAngle::MakeFromDegree(90.0f));

    ezQuatT q;
    q = ezQuat::MakeFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::MakeFromDegree(90.0f));

    ezVec3T xAxis(1, 0, 0);

    ezVec3T temp1 = m.TransformDirection(xAxis);
    ezVec3T temp2 = q.GetAsMat3().TransformDirection(xAxis);

    EZ_TEST_BOOL(temp1.IsEqual(temp2, 0.01f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsEulerAngles / SetFromEulerAngles")
  {
    ezAngle ax, ay, az;

    for (ezUInt32 x = 0; x < 360; x += 15)
    {
      ezQuat q = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(x), {}, {});

      ezMat3 m;
      m.SetRotationMatrixX(ezAngle::MakeFromDegree(x));
      ezQuat qm;
      qm = ezQuat::MakeFromMat3(m);
      EZ_TEST_BOOL(q.IsEqualRotation(qm, 0.01f));

      ezVec3 axis;
      ezAngle angle;
      q.GetRotationAxisAndAngle(axis, angle, 0.01f);

      EZ_TEST_VEC3(axis, ezVec3::UnitXAxis(), 0.001f);
      EZ_TEST_FLOAT(angle.GetDegree(), (float)x, 0.1f);

      q.GetAsEulerAngles(ax, ay, az);
      EZ_TEST_BOOL(ax.IsEqualNormalized(ezAngle::MakeFromDegree(x), ezAngle::MakeFromDegree(0.1f)));
    }

    for (ezInt32 y = -90; y < 360; y += 15)
    {
      ezQuat q = ezQuat::MakeFromEulerAngles({}, ezAngle::MakeFromDegree(y), {});

      ezMat3 m;
      m.SetRotationMatrixY(ezAngle::MakeFromDegree(y));
      ezQuat qm;
      qm = ezQuat::MakeFromMat3(m);
      EZ_TEST_BOOL(q.IsEqualRotation(qm, 0.01f));

      ezVec3 axis;
      ezAngle angle;
      q.GetRotationAxisAndAngle(axis, angle, 0.01f);

      if (y < 0)
      {
        EZ_TEST_VEC3(axis, -ezVec3::UnitYAxis(), 0.001f);
        EZ_TEST_FLOAT(angle.GetDegree(), (float)-y, 0.1f);
      }
      else if (y > 0)
      {
        EZ_TEST_VEC3(axis, ezVec3::UnitYAxis(), 0.001f);
        EZ_TEST_FLOAT(angle.GetDegree(), (float)y, 0.1f);
      }

      // pitch is only defined in -90..90 range
      if (y >= -90 && y <= 90)
      {
        q.GetAsEulerAngles(ax, ay, az);
        EZ_TEST_FLOAT(ay.GetDegree(), (float)y, 0.1f);
      }
    }

    for (ezUInt32 z = 15; z < 360; z += 15)
    {
      ezQuat q = ezQuat::MakeFromEulerAngles({}, {}, ezAngle::MakeFromDegree(z));

      ezMat3 m;
      m.SetRotationMatrixZ(ezAngle::MakeFromDegree(z));
      ezQuat qm;
      qm = ezQuat::MakeFromMat3(m);
      EZ_TEST_BOOL(q.IsEqualRotation(qm, 0.01f));

      ezVec3 axis;
      ezAngle angle;
      q.GetRotationAxisAndAngle(axis, angle, 0.01f);

      EZ_TEST_VEC3(axis, ezVec3::UnitZAxis(), 0.001f);
      EZ_TEST_FLOAT(angle.GetDegree(), (float)z, 0.1f);

      q.GetAsEulerAngles(ax, ay, az);
      EZ_TEST_BOOL(az.IsEqualNormalized(ezAngle::MakeFromDegree(z), ezAngle::MakeFromDegree(0.1f)));
    }

    for (ezUInt32 x = 0; x < 360; x += 15)
    {
      for (ezUInt32 y = 0; y < 360; y += 15)
      {
        for (ezUInt32 z = 0; z < 360; z += 30)
        {
          ezQuat q1 = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(x), ezAngle::MakeFromDegree(y), ezAngle::MakeFromDegree(z));

          q1.GetAsEulerAngles(ax, ay, az);

          ezQuat q2 = ezQuat::MakeFromEulerAngles(ax, ay, az);

          EZ_TEST_BOOL(q1.IsEqualRotation(q2, 0.1f));

          // Check that euler order is ZYX aka 3-2-1
          ezQuat q3;
          {
            ezQuat xRot, yRot, zRot;
            xRot = ezQuat::MakeFromAxisAndAngle(ezVec3::UnitXAxis(), ezAngle::MakeFromDegree(x));
            yRot = ezQuat::MakeFromAxisAndAngle(ezVec3::UnitYAxis(), ezAngle::MakeFromDegree(y));
            zRot = ezQuat::MakeFromAxisAndAngle(ezVec3::UnitZAxis(), ezAngle::MakeFromDegree(z));

            q3 = zRot * yRot * xRot;
          }
          EZ_TEST_BOOL(q1.IsEqualRotation(q3, 0.01f));
        }
      }
    }
  }
}
