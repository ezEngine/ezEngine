#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Quat.h>

template <typename Type>
void TestQuat()
{
  using ezQuatType = ezQuatTemplate<Type>;
  using ezVec3Type = ezVec3Template<Type>;
  using ezMat3Type = ezMat3Template<Type>;
  using ezMat4Type = ezMat4Template<Type>;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Default Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (ezMath::SupportsNaN<Type>())
    {
      // In debug the default constructor initializes everything with NaN.
      ezQuatType p;
      EZ_TEST_BOOL(ezMath::IsNaN(p.x) && ezMath::IsNaN(p.y) && ezMath::IsNaN(p.z) && ezMath::IsNaN(p.w));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    Type testBlock[4] = {(Type)1, (Type)2, (Type)3, (Type)4};
    ezQuatType* p = ::new ((void*)&testBlock[0]) ezQuatType;
    EZ_TEST_BOOL(p->x == (Type)1 && p->y == (Type)2 && p->z == (Type)3 && p->w == (Type)4);
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(x,y,z,w)")
  {
    ezQuatType q((Type)1, (Type)2, (Type)3, (Type)4);

    EZ_TEST_VEC3(q.GetVectorPart(), ezVec3Type((Type)1, (Type)2, (Type)3), (Type)0.0001);
    EZ_TEST_FLOAT(q.w, (Type)4, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeIdentity")
  {
    ezQuatType q = ezQuatType::MakeIdentity();

    EZ_TEST_VEC3(q.GetVectorPart(), ezVec3Type((Type)0, (Type)0, (Type)0), (Type)0.0001);
    EZ_TEST_FLOAT(q.w, (Type)1, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetIdentity")
  {
    ezQuatType q((Type)1, (Type)2, (Type)3, (Type)4);

    q.SetIdentity();

    EZ_TEST_VEC3(q.GetVectorPart(), ezVec3Type((Type)0, (Type)0, (Type)0), (Type)0.0001);
    EZ_TEST_FLOAT(q.w, (Type)1, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetElements")
  {
    ezQuatType q((Type)5, (Type)6, (Type)7, (Type)8);

    q = ezQuatType((Type)1, (Type)2, (Type)3, (Type)4);

    EZ_TEST_VEC3(q.GetVectorPart(), ezVec3Type((Type)1, (Type)2, (Type)3), (Type)0.0001);
    EZ_TEST_FLOAT(q.w, (Type)4, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromAxisAndAngle / operator* (quat, vec)")
  {
    {
      ezQuatType q;
      q = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)1, (Type)0, (Type)0), ezAngleTemplate<Type>::MakeFromDegree((Type)90));

      EZ_TEST_VEC3(q * ezVec3Type((Type)0, (Type)1, (Type)0), ezVec3Type((Type)0, (Type)0, (Type)1), (Type)0.0001);
    }

    {
      ezQuatType q;
      q = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)1, (Type)0), ezAngleTemplate<Type>::MakeFromDegree((Type)90));

      EZ_TEST_VEC3(q * ezVec3Type((Type)1, (Type)0, (Type)0), ezVec3Type((Type)0, (Type)0, (Type)-1), (Type)0.0001);
    }

    {
      ezQuatType q;
      q = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)90));

      EZ_TEST_VEC3(q * ezVec3Type((Type)0, (Type)1, (Type)0), ezVec3Type((Type)-1, (Type)0, (Type)0), (Type)0.0001);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetShortestRotation / IsEqualRotation")
  {
    ezQuatType q1, q2, q3;
    q1 = ezQuatType::MakeShortestRotation(ezVec3Type((Type)0, (Type)1, (Type)0), ezVec3Type((Type)1, (Type)0, (Type)0));
    q2 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)-1), ezAngleTemplate<Type>::MakeFromDegree((Type)90));
    q3 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)-90));

    EZ_TEST_BOOL(q1.IsEqualRotation(q2, ezMath::LargeEpsilon<Type>()));
    EZ_TEST_BOOL(q1.IsEqualRotation(q3, ezMath::LargeEpsilon<Type>()));

    EZ_TEST_BOOL(ezQuatType::MakeIdentity().IsEqualRotation(ezQuatType::MakeIdentity(), ezMath::LargeEpsilon<Type>()));
    EZ_TEST_BOOL(ezQuatType::MakeIdentity().IsEqualRotation(ezQuatType((Type)0, (Type)0, (Type)0, (Type)-1), ezMath::LargeEpsilon<Type>()));

    ezQuatType q4((Type)0, (Type)0, (Type)0, (Type)1 + ezMath::VerySmallEpsilon<Type>()*1.2);
    ezQuatType q5((Type)0, (Type)0, (Type)0, (Type)1 + ezMath::VerySmallEpsilon<Type>()*2.3);
    EZ_TEST_BOOL(q4.IsEqualRotation(q5, ezMath::LargeEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromMat3")
  {
    ezMat3Type m;
    m = ezMat3Type::MakeRotationZ(ezAngleTemplate<Type>::MakeFromDegree((Type)-90));

    ezQuatType q1, q2, q3;
    q1 = ezQuatType::MakeFromMat3(m);
    q2 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)-1), ezAngleTemplate<Type>::MakeFromDegree((Type)90));
    q3 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)-90));

    EZ_TEST_BOOL(q1.IsEqualRotation(q2, ezMath::LargeEpsilon<Type>()));
    EZ_TEST_BOOL(q1.IsEqualRotation(q3, ezMath::LargeEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetSlerp")
  {
    ezQuatType q1, q2, q3, qr;
    q1 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)45));
    q2 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)0));
    q3 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)90));

    qr = ezQuatType::MakeSlerp(q2, q3, (Type)0.5);

    EZ_TEST_BOOL(q1.IsEqualRotation(qr, (Type)0.0001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRotationAxisAndAngle")
  {
    ezQuatType q1, q2, q3;
    q1 = ezQuatType::MakeShortestRotation(ezVec3Type((Type)0, (Type)1, (Type)0), ezVec3Type((Type)1, (Type)0, (Type)0));
    q2 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)-1), ezAngleTemplate<Type>::MakeFromDegree((Type)90));
    q3 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)-90));

    ezVec3Type axis;
    ezAngleTemplate<Type> angle;

    q1.GetRotationAxisAndAngle(axis, angle);
    EZ_TEST_VEC3(axis, ezVec3Type((Type)0, (Type)0, (Type)-1), (Type)0.001);
    EZ_TEST_FLOAT(angle.GetDegree(), (Type)90, ezMath::LargeEpsilon<Type>());

    q2.GetRotationAxisAndAngle(axis, angle);
    EZ_TEST_VEC3(axis, ezVec3Type((Type)0, (Type)0, (Type)-1), (Type)0.001);
    EZ_TEST_FLOAT(angle.GetDegree(), (Type)90, ezMath::LargeEpsilon<Type>());

    q3.GetRotationAxisAndAngle(axis, angle);
    EZ_TEST_VEC3(axis, ezVec3Type((Type)0, (Type)0, (Type)-1), (Type)0.001);
    EZ_TEST_FLOAT(angle.GetDegree(), (Type)90, ezMath::LargeEpsilon<Type>());

    ezQuatType::MakeIdentity().GetRotationAxisAndAngle(axis, angle);
    EZ_TEST_VEC3(axis, ezVec3Type((Type)1, (Type)0, (Type)0), (Type)0.001);
    EZ_TEST_FLOAT(angle.GetDegree(), (Type)0, ezMath::LargeEpsilon<Type>());

    ezQuatType otherIdentity((Type)0, (Type)0, (Type)0, (Type)-1);
    otherIdentity.GetRotationAxisAndAngle(axis, angle);
    EZ_TEST_VEC3(axis, ezVec3Type((Type)1, (Type)0, (Type)0), (Type)0.001);
    EZ_TEST_FLOAT(angle.GetDegree(), (Type)360, ezMath::LargeEpsilon<Type>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsMat3")
  {
    ezQuatType q;
    q = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)90));

    ezMat3Type mr;
    mr = ezMat3Type::MakeRotationZ(ezAngleTemplate<Type>::MakeFromDegree((Type)90));

    ezMat3Type m = q.GetAsMat3();

    EZ_TEST_BOOL(mr.IsEqual(m, ezMath::DefaultEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsMat4")
  {
    ezQuatType q;
    q = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)90));

    ezMat4Type mr;
    mr = ezMat4Type::MakeRotationZ(ezAngleTemplate<Type>::MakeFromDegree((Type)90));

    ezMat4Type m = q.GetAsMat4();

    EZ_TEST_BOOL(mr.IsEqual(m, ezMath::DefaultEpsilon<Type>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsValid / Normalize")
  {
    ezQuatType q((Type)1, (Type)2, (Type)3, (Type)4);
    EZ_TEST_BOOL(!q.IsValid((Type)0.001));

    q.Normalize();
    EZ_TEST_BOOL(q.IsValid((Type)0.001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetInverse / Invert")
  {
    ezQuatType q, q1;
    q = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)90));
    q1 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)-90));

    ezQuatType q2 = q.GetInverse();
    EZ_TEST_BOOL(q1.IsEqualRotation(q2, (Type)0.0001));

    ezQuatType q3 = q;
    q3.Invert();
    EZ_TEST_BOOL(q1.IsEqualRotation(q3, (Type)0.0001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Dot")
  {
    ezQuatType q, q1, q2;
    q = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)90));
    q1 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)-90));
    q2 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)1, (Type)0), ezAngleTemplate<Type>::MakeFromDegree((Type)45));

    EZ_TEST_FLOAT(q.Dot(q), (Type)1.0, (Type)0.0001);
    EZ_TEST_FLOAT(q.Dot(ezQuatType::MakeIdentity()), ezMath::Cos(ezAngleTemplate<Type>::MakeFromRadian(ezAngleTemplate<Type>::DegToRad((Type)90.0 / (Type)2.0))), (Type)0.0001);
    EZ_TEST_FLOAT(q.Dot(q1), (Type)0.0, (Type)0.0001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(quat, quat)")
  {
    ezQuatType q1, q2, qr, q3;
    q1 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)60));
    q2 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)30));
    q3 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)90));

    qr = q1 * q2;

    EZ_TEST_BOOL(qr.IsEqualRotation(q3, (Type)0.0001));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator==/!=")
  {
    ezQuatType q1, q2;
    q1 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)60));
    q2 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)30));
    EZ_TEST_BOOL(q1 != q2);

    q2 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)1, (Type)0, (Type)0), ezAngleTemplate<Type>::MakeFromDegree((Type)60));
    EZ_TEST_BOOL(q1 != q2);

    q2 = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)60));
    EZ_TEST_BOOL(q1 == q2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::SupportsNaN<Type>())
    {
      ezQuatType q;

      q.SetIdentity();
      EZ_TEST_BOOL(!q.IsNaN());

      q.SetIdentity();
      q.w = ezMath::NaN<Type>();
      EZ_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.x = ezMath::NaN<Type>();
      EZ_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.y = ezMath::NaN<Type>();
      EZ_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.z = ezMath::NaN<Type>();
      EZ_TEST_BOOL(q.IsNaN());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "rotation direction")
  {
    ezMat3Type m;
    m = ezMat3Type::MakeRotationZ(ezAngleTemplate<Type>::MakeFromDegree((Type)90.0));

    ezQuatType q;
    q = ezQuatType::MakeFromAxisAndAngle(ezVec3Type((Type)0, (Type)0, (Type)1), ezAngleTemplate<Type>::MakeFromDegree((Type)90.0));

    ezVec3Type xAxis((Type)1, (Type)0, (Type)0);

    ezVec3Type temp1 = m.TransformDirection(xAxis);
    ezVec3Type temp2 = q.GetAsMat3().TransformDirection(xAxis);

    EZ_TEST_BOOL(temp1.IsEqual(temp2, (Type)0.01));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsEulerAngles / SetFromEulerAngles")
  {
    ezAngleTemplate<Type> ax, ay, az;

    for (ezUInt32 x = 0; x < 360; x += 15)
    {
      ezQuatType q = ezQuatType::MakeFromEulerAngles(ezAngleTemplate<Type>::MakeFromDegree((Type)x), {}, {});

      ezMat3Type m;
      m = ezMat3Type::MakeRotationX(ezAngleTemplate<Type>::MakeFromDegree((Type)x));
      ezQuatType qm;
      qm = ezQuatType::MakeFromMat3(m);
      EZ_TEST_BOOL(q.IsEqualRotation(qm, (Type)0.01));

      ezVec3Type axis;
      ezAngleTemplate<Type> angle;
      q.GetRotationAxisAndAngle(axis, angle, (Type)0.01);

      EZ_TEST_VEC3(axis, ezVec3Type::MakeAxisX(), (Type)0.001);
      EZ_TEST_FLOAT(angle.GetDegree(), (Type)x, (Type)0.1);

      q.GetAsEulerAngles(ax, ay, az);
      EZ_TEST_BOOL(ax.IsEqualNormalized(ezAngleTemplate<Type>::MakeFromDegree((Type)x), ezAngleTemplate<Type>::MakeFromDegree((Type)0.1)));
    }

    for (ezInt32 y = -90; y < 360; y += 15)
    {
      ezQuatType q = ezQuatType::MakeFromEulerAngles({}, ezAngleTemplate<Type>::MakeFromDegree((Type)y), {});

      ezMat3Type m;
      m = ezMat3Type::MakeRotationY(ezAngleTemplate<Type>::MakeFromDegree((Type)y));
      ezQuatType qm;
      qm = ezQuatType::MakeFromMat3(m);
      EZ_TEST_BOOL(q.IsEqualRotation(qm, (Type)0.01));

      ezVec3Type axis;
      ezAngleTemplate<Type> angle;
      q.GetRotationAxisAndAngle(axis, angle, (Type)0.01);

      if (y < 0)
      {
        EZ_TEST_VEC3(axis, -ezVec3Type::MakeAxisY(), (Type)0.001);
        EZ_TEST_FLOAT(angle.GetDegree(), (Type)-y, (Type)0.1);
      }
      else if (y > 0)
      {
        EZ_TEST_VEC3(axis, ezVec3Type::MakeAxisY(), (Type)0.001);
        EZ_TEST_FLOAT(angle.GetDegree(), (Type)y, (Type)0.1);
      }

      // pitch is only defined in -90..90 range
      if (y >= -90 && y <= 90)
      {
        q.GetAsEulerAngles(ax, ay, az);
        EZ_TEST_FLOAT(ay.GetDegree(), (Type)y, (Type)0.1);
      }
    }

    for (ezUInt32 z = 15; z < 360; z += 15)
    {
      ezQuatType q = ezQuatType::MakeFromEulerAngles({}, {}, ezAngleTemplate<Type>::MakeFromDegree((Type)z));

      ezMat3Type m;
      m = ezMat3Type::MakeRotationZ(ezAngleTemplate<Type>::MakeFromDegree((Type)z));
      ezQuatType qm;
      qm = ezQuatType::MakeFromMat3(m);
      EZ_TEST_BOOL(q.IsEqualRotation(qm, (Type)0.01));

      ezVec3Type axis;
      ezAngleTemplate<Type> angle;
      q.GetRotationAxisAndAngle(axis, angle, (Type)0.01);

      EZ_TEST_VEC3(axis, ezVec3Type::MakeAxisZ(), (Type)0.001);
      EZ_TEST_FLOAT(angle.GetDegree(), (Type)z, (Type)0.1);

      q.GetAsEulerAngles(ax, ay, az);
      EZ_TEST_BOOL(az.IsEqualNormalized(ezAngleTemplate<Type>::MakeFromDegree((Type)z), ezAngleTemplate<Type>::MakeFromDegree((Type)0.1)));
    }

    for (ezUInt32 x = 0; x < 360; x += 15)
    {
      for (ezUInt32 y = 0; y < 360; y += 15)
      {
        for (ezUInt32 z = 0; z < 360; z += 30)
        {
          ezQuatType q1 = ezQuatType::MakeFromEulerAngles(ezAngleTemplate<Type>::MakeFromDegree((Type)x), ezAngleTemplate<Type>::MakeFromDegree((Type)y), ezAngleTemplate<Type>::MakeFromDegree((Type)z));

          q1.GetAsEulerAngles(ax, ay, az);

          ezQuatType q2 = ezQuatType::MakeFromEulerAngles(ax, ay, az);

          EZ_TEST_BOOL(q1.IsEqualRotation(q2, (Type)0.1));

          // Check that euler order is ZYX aka 3-2-1
          ezQuatType q3;
          {
            ezQuatType xRot, yRot, zRot;
            xRot = ezQuatType::MakeFromAxisAndAngle(ezVec3Type::MakeAxisX(), ezAngleTemplate<Type>::MakeFromDegree((Type)x));
            yRot = ezQuatType::MakeFromAxisAndAngle(ezVec3Type::MakeAxisY(), ezAngleTemplate<Type>::MakeFromDegree((Type)y));
            zRot = ezQuatType::MakeFromAxisAndAngle(ezVec3Type::MakeAxisZ(), ezAngleTemplate<Type>::MakeFromDegree((Type)z));

            q3 = zRot * yRot * xRot;
          }
          EZ_TEST_BOOL(q1.IsEqualRotation(q3, (Type)0.01));
        }
      }
    }
  }
}

EZ_CREATE_SIMPLE_TEST(Math, Quaternionf)
{
  TestQuat<float>();
}

EZ_CREATE_SIMPLE_TEST(Math, Quaterniond)
{
  TestQuat<double>();
}
