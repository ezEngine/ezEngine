#include <PCH.h>
#include <Foundation/Math/Quat.h>

EZ_CREATE_SIMPLE_TEST(Math, Quaternion)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Default Constructor")
  {
    #if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      if (ezMath::BasicType<ezMat3T::ComponentType>::SupportsNaN())
      {
        // In debug the default constructor initializes everything with NaN.
        ezQuatT p;
        EZ_TEST_BOOL(ezMath::IsNaN(p.v.x) && ezMath::IsNaN(p.v.y) && ezMath::IsNaN(p.v.z) && ezMath::IsNaN(p.w));
      }
    #else
        // Placement new of the default constructor should not have any effect on the previous data.
        ezQuatT::ComponentType testBlock[4] = { (ezQuatT::ComponentType) 1, (ezQuatT::ComponentType) 2, (ezQuatT::ComponentType) 3, (ezQuatT::ComponentType) 4 };
        ezQuatT* p = ::new ((void*) &testBlock[0]) ezQuatT;
        EZ_TEST_BOOL(p->v.x == (ezMat3T::ComponentType) 1 && 
                p->v.y == (ezMat3T::ComponentType) 2 && 
                p->v.z == (ezMat3T::ComponentType) 3 && 
                p->w   == (ezMat3T::ComponentType) 4);
    #endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(x,y,z,w)")
  {
    ezQuatT q(1, 2, 3, 4);

    EZ_TEST_VEC3(q.v, ezVec3T(1, 2, 3), 0.0001f);
    EZ_TEST_FLOAT(q.w, 4, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IdentityQuaternion")
  {
    ezQuatT q = ezQuatT::IdentityQuaternion();

    EZ_TEST_VEC3(q.v, ezVec3T(0, 0, 0), 0.0001f);
    EZ_TEST_FLOAT(q.w, 1, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetIdentity")
  {
    ezQuatT q(1, 2, 3, 4);

    q.SetIdentity();

    EZ_TEST_VEC3(q.v, ezVec3T(0, 0, 0), 0.0001f);
    EZ_TEST_FLOAT(q.w, 1, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetElements")
  {
    ezQuatT q(5, 6, 7, 8);

    q.SetElements(1, 2, 3, 4);

    EZ_TEST_VEC3(q.v, ezVec3T(1, 2, 3), 0.0001f);
    EZ_TEST_FLOAT(q.w, 4, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromAxisAndAngle / operator* (quat, vec)")
  {
    {
      ezQuatT q;
      q.SetFromAxisAndAngle(ezVec3T(1, 0, 0), ezAngle::Degree(90));

      EZ_TEST_VEC3(q * ezVec3T(0, 1, 0), ezVec3T(0, 0, 1), 0.0001f);
    }

    {
      ezQuatT q;
      q.SetFromAxisAndAngle(ezVec3T(0, 1, 0), ezAngle::Degree(90));

      EZ_TEST_VEC3(q * ezVec3T(1, 0, 0), ezVec3T(0, 0, -1), 0.0001f);
    }

    {
      ezQuatT q;
      q.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(90));

      EZ_TEST_VEC3(q * ezVec3T(0, 1, 0), ezVec3T(-1, 0, 0), 0.0001f);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetShortestRotation / IsEqualRotation")
  {
    ezQuatT q1, q2, q3;
    q1.SetShortestRotation(ezVec3T(0, 1, 0), ezVec3T(1, 0, 0));
    q2.SetFromAxisAndAngle(ezVec3T(0, 0, -1), ezAngle::Degree(90));
    q3.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(-90));

    EZ_TEST_BOOL(q1.IsEqualRotation(q2, ezMath::BasicType<float>::LargeEpsilon()));
    EZ_TEST_BOOL(q1.IsEqualRotation(q3, ezMath::BasicType<float>::LargeEpsilon()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromMat3")
  {
    ezMat3T m;
    m.SetRotationMatrixZ(ezAngle::Degree(-90));

    ezQuatT q1, q2, q3;
    q1.SetFromMat3(m);
    q2.SetFromAxisAndAngle(ezVec3T(0, 0, -1), ezAngle::Degree(90));
    q3.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(-90));

    EZ_TEST_BOOL(q1.IsEqualRotation(q2, ezMath::BasicType<float>::LargeEpsilon()));
    EZ_TEST_BOOL(q1.IsEqualRotation(q3, ezMath::BasicType<float>::LargeEpsilon()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetSlerp")
  {
    ezQuatT q1, q2, q3, qr;
    q1.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(45));
    q2.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(0));
    q3.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(90));

    qr.SetSlerp(q2, q3, 0.5f);

    EZ_TEST_BOOL(q1.IsEqualRotation(qr, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRotationAxisAndAngle")
  {
    ezQuatT q1, q2, q3;
    q1.SetShortestRotation(ezVec3T(0, 1, 0), ezVec3T(1, 0, 0));
    q2.SetFromAxisAndAngle(ezVec3T(0, 0, -1), ezAngle::Degree(90));
    q3.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(-90));

    ezVec3T axis;
    ezAngle angle;

    EZ_TEST_BOOL(q1.GetRotationAxisAndAngle(axis, angle) == EZ_SUCCESS);
    EZ_TEST_VEC3(axis, ezVec3T(0, 0, -1), 0.001f);
    EZ_TEST_FLOAT(angle.GetDegree(), 90, ezMath::BasicType<ezMat3T::ComponentType>::LargeEpsilon());

    EZ_TEST_BOOL(q2.GetRotationAxisAndAngle(axis, angle) == EZ_SUCCESS);
    EZ_TEST_VEC3(axis, ezVec3T(0, 0, -1), 0.001f);
    EZ_TEST_FLOAT(angle.GetDegree(), 90, ezMath::BasicType<ezMat3T::ComponentType>::LargeEpsilon());

    EZ_TEST_BOOL(q3.GetRotationAxisAndAngle(axis, angle) == EZ_SUCCESS);
    EZ_TEST_VEC3(axis, ezVec3T(0, 0, -1), 0.001f);
    EZ_TEST_FLOAT(angle.GetDegree(), 90, ezMath::BasicType<ezMat3T::ComponentType>::LargeEpsilon());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsMat3")
  {
    ezQuatT q;
    q.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(90));

    ezMat3T mr;
    mr.SetRotationMatrixZ(ezAngle::Degree(90));

    ezMat3T m = q.GetAsMat3();

    EZ_TEST_BOOL(mr.IsEqual(m, ezMath::BasicType<ezMat3T::ComponentType>::DefaultEpsilon()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAsMat4")
  {
    ezQuatT q;
    q.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(90));

    ezMat4T mr;
    mr.SetRotationMatrixZ(ezAngle::Degree(90));

    ezMat4T m = q.GetAsMat4();

    EZ_TEST_BOOL(mr.IsEqual(m, ezMath::BasicType<ezMat3T::ComponentType>::DefaultEpsilon()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsValid / Normalize")
  {
    ezQuatT q(1, 2, 3, 4);
    EZ_TEST_BOOL(!q.IsValid(0.001f));

    q.Normalize();
    EZ_TEST_BOOL(q.IsValid(0.001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator-")
  {
    ezQuatT q, q1;
    q.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(90));
    q1.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(-90));

    ezQuatT q2 = -q;
    EZ_TEST_BOOL(q1.IsEqualRotation(q2, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(quat, quat)")
  {
    ezQuatT q1, q2, qr, q3;
    q1.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(60));
    q2.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(30));
    q3.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(90));

    qr = q1 * q2;

    EZ_TEST_BOOL(qr.IsEqualRotation(q3, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator==/!=")
  {
    ezQuatT q1, q2;
    q1.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(60));
    q2.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(30));
    EZ_TEST_BOOL(q1 != q2);

    q2.SetFromAxisAndAngle(ezVec3T(1, 0, 0), ezAngle::Degree(60));
    EZ_TEST_BOOL(q1 != q2);

    q2.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(60));
    EZ_TEST_BOOL(q1 == q2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNaN")
  {
    if (ezMath::BasicType<ezMathTestType>::SupportsNaN())
    {
      ezQuatT q;

      q.SetIdentity();
      EZ_TEST_BOOL(!q.IsNaN());

      q.SetIdentity();
      q.w = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.v.x = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.v.y = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.v.z = ezMath::BasicType<ezMathTestType>::GetNaN();
      EZ_TEST_BOOL(q.IsNaN());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "rotation direction")
  {
    ezMat3T m;
    m.SetRotationMatrixZ(ezAngle::Degree(90.0f));

    ezQuatT q;
    q.SetFromAxisAndAngle(ezVec3T(0, 0, 1), ezAngle::Degree(90.0f));

    ezVec3T xAxis(1, 0, 0);

    ezVec3T temp1 = m.TransformDirection(xAxis);
    ezVec3T temp2 = q.GetAsMat3().TransformDirection(xAxis);

    EZ_TEST_BOOL(temp1.IsEqual(temp2, 0.01f));
  }

}

