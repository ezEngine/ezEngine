#include <PCH.h>
#include <Foundation/SimdMath/SimdQuat.h>

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdQuat)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    ezSimdQuat vDefCtor;
    EZ_TEST_BOOL(vDefCtor.IsNaN());
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    float EZ_ALIGN_16(testBlock[4]) = { 1, 2, 3, 4 };
    ezSimdQuat* pDefCtor = ::new ((void*)&testBlock[0]) ezSimdQuat;
    EZ_TEST_BOOL(pDefCtor->m_v.x() == 1.0f && pDefCtor->m_v.y() == 2.0f &&
      pDefCtor->m_v.z() == 3.0f && pDefCtor->m_v.w() == 4.0f);
#endif

    // Make sure the class didn't accidentally change in size.
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    EZ_CHECK_AT_COMPILETIME(sizeof(ezSimdQuat) == 16);
    EZ_CHECK_AT_COMPILETIME(EZ_ALIGNMENT_OF(ezSimdQuat) == 16);
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IdentityQuaternion")
  {
    ezSimdQuat q = ezSimdQuat::IdentityQuaternion();

    EZ_TEST_BOOL(q.m_v.x() == 0.0f && q.m_v.y() == 0.0f && q.m_v.z() == 0.0f && q.m_v.w() == 1.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetIdentity")
  {
    ezSimdQuat q(ezSimdVec4f(1, 2, 3, 4));

    q.SetIdentity();

    EZ_TEST_BOOL(q.m_v.x() == 0.0f && q.m_v.y() == 0.0f && q.m_v.z() == 0.0f && q.m_v.w() == 1.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromAxisAndAngle / operator* (quat, vec)")
  {
    {
      ezSimdQuat q;
      q.SetFromAxisAndAngle(ezSimdVec4f(1, 0, 0), ezAngle::Degree(90));

      EZ_TEST_BOOL((q * ezSimdVec4f(0, 1, 0)).IsEqual(ezSimdVec4f(0, 0, 1), 0.0001f).AllSet());
    }

    {
      ezSimdQuat q;
      q.SetFromAxisAndAngle(ezSimdVec4f(0, 1, 0), ezAngle::Degree(90));

      EZ_TEST_BOOL((q * ezSimdVec4f(1, 0, 0)).IsEqual(ezSimdVec4f(0, 0, -1), 0.0001f).AllSet());
    }

    {
      ezSimdQuat q;
      q.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(90));

      EZ_TEST_BOOL((q * ezSimdVec4f(0, 1, 0)).IsEqual(ezSimdVec4f(-1, 0, 0), 0.0001f).AllSet());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetShortestRotation / IsEqualRotation")
  {
    ezSimdQuat q1, q2, q3;
    q1.SetShortestRotation(ezSimdVec4f(0, 1, 0), ezSimdVec4f(1, 0, 0));
    q2.SetFromAxisAndAngle(ezSimdVec4f(0, 0, -1), ezAngle::Degree(90));
    q3.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(-90));

    EZ_TEST_BOOL(q1.IsEqualRotation(q2, ezMath::BasicType<float>::LargeEpsilon()));
    EZ_TEST_BOOL(q1.IsEqualRotation(q3, ezMath::BasicType<float>::LargeEpsilon()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetSlerp")
  {
    ezSimdQuat q1, q2, q3, qr;
    q1.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(45));
    q2.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(0));
    q3.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(90));

    qr.SetSlerp(q2, q3, 0.5f);

    EZ_TEST_BOOL(q1.IsEqualRotation(qr, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRotationAxisAndAngle")
  {
    ezSimdQuat q1, q2, q3;
    q1.SetShortestRotation(ezSimdVec4f(0, 1, 0), ezSimdVec4f(1, 0, 0));
    q2.SetFromAxisAndAngle(ezSimdVec4f(0, 0, -1), ezAngle::Degree(90));
    q3.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(-90));

    ezSimdVec4f axis;
    ezSimdFloat angle;

    EZ_TEST_BOOL(q1.GetRotationAxisAndAngle(axis, angle) == EZ_SUCCESS);
    EZ_TEST_BOOL(axis.IsEqual(ezSimdVec4f(0, 0, -1), 0.001f).AllSet<3>());
    EZ_TEST_FLOAT(ezAngle::RadToDeg((float)angle), 90, ezMath::BasicType<float>::LargeEpsilon());

    EZ_TEST_BOOL(q2.GetRotationAxisAndAngle(axis, angle) == EZ_SUCCESS);
    EZ_TEST_BOOL(axis.IsEqual(ezSimdVec4f(0, 0, -1), 0.001f).AllSet<3>());
    EZ_TEST_FLOAT(ezAngle::RadToDeg((float)angle), 90, ezMath::BasicType<float>::LargeEpsilon());

    EZ_TEST_BOOL(q3.GetRotationAxisAndAngle(axis, angle) == EZ_SUCCESS);
    EZ_TEST_BOOL(axis.IsEqual(ezSimdVec4f(0, 0, -1), 0.001f).AllSet<3>());
    EZ_TEST_FLOAT(ezAngle::RadToDeg((float)angle), 90, ezMath::BasicType<float>::LargeEpsilon());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsValid / Normalize")
  {
    ezSimdQuat q(ezSimdVec4f(1, 2, 3, 4));
    EZ_TEST_BOOL(!q.IsValid(0.001f));

    q.Normalize();
    EZ_TEST_BOOL(q.IsValid(0.001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator-")
  {
    ezSimdQuat q, q1;
    q.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(90));
    q1.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(-90));

    ezSimdQuat q2 = -q;
    EZ_TEST_BOOL(q1.IsEqualRotation(q2, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(quat, quat)")
  {
    ezSimdQuat q1, q2, qr, q3;
    q1.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(60));
    q2.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(30));
    q3.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(90));

    qr = q1 * q2;

    EZ_TEST_BOOL(qr.IsEqualRotation(q3, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator==/!=")
  {
    ezSimdQuat q1, q2;
    q1.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(60));
    q2.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(30));
    EZ_TEST_BOOL(q1 != q2);

    q2.SetFromAxisAndAngle(ezSimdVec4f(1, 0, 0), ezAngle::Degree(60));
    EZ_TEST_BOOL(q1 != q2);

    q2.SetFromAxisAndAngle(ezSimdVec4f(0, 0, 1), ezAngle::Degree(60));
    EZ_TEST_BOOL(q1 == q2);
  }
}
