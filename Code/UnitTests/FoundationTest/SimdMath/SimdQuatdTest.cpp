#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdQuatd.h>

EZ_CREATE_SIMPLE_TEST(SimdMath, ezSimdQuatd)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    ezSimdQuatd vDefCtor;
    EZ_TEST_BOOL(vDefCtor.IsNaN());
#else

#  if EZ_DISABLED(EZ_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) double testBlock[4] = {1, 2, 3, 4};
    ezSimdQuatd* pDefCtor = ::new ((void*)&testBlock[0]) ezSimdQuatd;
    EZ_TEST_BOOL(pDefCtor->m_v.x() == 1.0f && pDefCtor->m_v.y() == 2.0f && pDefCtor->m_v.z() == 3.0f && pDefCtor->m_v.w() == 4.0f);
#  endif

#endif

    // Make sure the class didn't accidentally change in size.
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
    static_assert(sizeof(ezSimdQuatd) == 32);
    static_assert(alignof(ezSimdQuatd) == 32);
#else

    static_assert(sizeof(ezSimdQuatd) == 32);
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE    
    static_assert(alignof(ezSimdQuatd) == 16);
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_FPU
    static_assert(alignof(ezSimdQuatd) == 8);
#endif

#endif

  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IdentityQuaternion")
  {
    ezSimdQuatd q = ezSimdQuatd::MakeIdentity();

    EZ_TEST_BOOL(q.m_v.x() == 0.0f && q.m_v.y() == 0.0f && q.m_v.z() == 0.0f && q.m_v.w() == 1.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetIdentity")
  {
    ezSimdQuatd q(ezSimdVec4d(1, 2, 3, 4));

    q = ezSimdQuatd::MakeIdentity();

    EZ_TEST_BOOL(q.m_v.x() == 0.0f && q.m_v.y() == 0.0f && q.m_v.z() == 0.0f && q.m_v.w() == 1.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFromAxisAndAngle / operator* (quat, vec)")
  {
    {
      ezSimdQuatd q = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(1, 0, 0), ezAngle::MakeFromDegree(90));

      EZ_TEST_BOOL((q * ezSimdVec4d(0, 1, 0)).IsEqual(ezSimdVec4d(0, 0, 1), 0.0001f).AllSet());
    }

    {
      ezSimdQuatd q = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 1, 0), ezAngle::MakeFromDegree(90));

      EZ_TEST_BOOL((q * ezSimdVec4d(1, 0, 0)).IsEqual(ezSimdVec4d(0, 0, -1), 0.0001f).AllSet());
    }

    {
      ezSimdQuatd q = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngle::MakeFromDegree(90));

      EZ_TEST_BOOL((q * ezSimdVec4d(0, 1, 0)).IsEqual(ezSimdVec4d(-1, 0, 0), 0.0001f).AllSet());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetShortestRotation / IsEqualRotation")
  {
    ezSimdQuatd q1, q2, q3;
    q1 = ezSimdQuatd::MakeShortestRotation(ezSimdVec4d(0, 1, 0), ezSimdVec4d(1, 0, 0));
    q2 = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, -1), ezAngle::MakeFromDegree(90));
    q3 = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngle::MakeFromDegree(-90));

    EZ_TEST_BOOL(q1.IsEqualRotation(q2, ezMath::LargeEpsilon<float>()));
    EZ_TEST_BOOL(q1.IsEqualRotation(q3, ezMath::LargeEpsilon<float>()));

    EZ_TEST_BOOL(ezSimdQuatd::MakeIdentity().IsEqualRotation(ezSimdQuatd::MakeIdentity(), ezMath::LargeEpsilon<float>()));
    EZ_TEST_BOOL(ezSimdQuatd::MakeIdentity().IsEqualRotation(ezSimdQuatd(ezSimdVec4d(0, 0, 0, -1)), ezMath::LargeEpsilon<float>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetSlerp")
  {
    ezSimdQuatd q1, q2, q3, qr;
    q1 = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngle::MakeFromDegree(45));
    q2 = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngle::MakeFromDegree(0));
    q3 = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngle::MakeFromDegree(90));

    qr = ezSimdQuatd::MakeSlerp(q2, q3, 0.5f);

    EZ_TEST_BOOL(q1.IsEqualRotation(qr, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRotationAxisAndAngle")
  {
    ezSimdQuatd q1, q2, q3;
    q1 = ezSimdQuatd::MakeShortestRotation(ezSimdVec4d(0, 1, 0), ezSimdVec4d(1, 0, 0));
    q2 = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, -1), ezAngle::MakeFromDegree(90));
    q3 = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngle::MakeFromDegree(-90));

    ezSimdVec4d axis;
    ezSimdDouble angle;

    EZ_TEST_BOOL(q1.GetRotationAxisAndAngle(axis, angle) == EZ_SUCCESS);
    EZ_TEST_BOOL(axis.IsEqual(ezSimdVec4d(0, 0, -1), 0.001f).AllSet<3>());
    EZ_TEST_FLOAT(ezAngle::RadToDeg((float)angle), 90, ezMath::LargeEpsilon<float>());

    EZ_TEST_BOOL(q2.GetRotationAxisAndAngle(axis, angle) == EZ_SUCCESS);
    EZ_TEST_BOOL(axis.IsEqual(ezSimdVec4d(0, 0, -1), 0.001f).AllSet<3>());
    EZ_TEST_FLOAT(ezAngle::RadToDeg((float)angle), 90, ezMath::LargeEpsilon<float>());

    EZ_TEST_BOOL(q3.GetRotationAxisAndAngle(axis, angle) == EZ_SUCCESS);
    EZ_TEST_BOOL(axis.IsEqual(ezSimdVec4d(0, 0, -1), 0.001f).AllSet<3>());
    EZ_TEST_FLOAT(ezAngle::RadToDeg((float)angle), 90, ezMath::LargeEpsilon<float>());

    EZ_TEST_BOOL(ezSimdQuatd::MakeIdentity().GetRotationAxisAndAngle(axis, angle) == EZ_SUCCESS);
    EZ_TEST_BOOL(axis.IsEqual(ezSimdVec4d(1, 0, 0), 0.001f).AllSet<3>());
    EZ_TEST_FLOAT(ezAngle::RadToDeg((float)angle), 0, ezMath::LargeEpsilon<float>());

    ezSimdQuatd otherIdentity(ezSimdVec4d(0, 0, 0, -1));
    EZ_TEST_BOOL(otherIdentity.GetRotationAxisAndAngle(axis, angle) == EZ_SUCCESS);
    EZ_TEST_BOOL(axis.IsEqual(ezSimdVec4d(1, 0, 0), 0.001f).AllSet<3>());
    EZ_TEST_FLOAT(ezAngle::RadToDeg((float)angle), 360, ezMath::LargeEpsilon<float>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsValid / Normalize")
  {
    ezSimdQuatd q(ezSimdVec4d(1, 2, 3, 4));
    EZ_TEST_BOOL(!q.IsValid(0.001f));

    q.Normalize();
    EZ_TEST_BOOL(q.IsValid(0.001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator-")
  {
    ezSimdQuatd q, q1;
    q = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngle::MakeFromDegree(90));
    q1 = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngle::MakeFromDegree(-90));

    ezSimdQuatd q2 = -q;
    EZ_TEST_BOOL(q1.IsEqualRotation(q2, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator*(quat, quat)")
  {
    ezSimdQuatd q1, q2, qr, q3;
    q1 = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngle::MakeFromDegree(60));
    q2 = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngle::MakeFromDegree(30));
    q3 = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngle::MakeFromDegree(90));

    qr = q1 * q2;

    EZ_TEST_BOOL(qr.IsEqualRotation(q3, 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator==/!=")
  {
    ezSimdQuatd q1, q2;
    q1 = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngle::MakeFromDegree(60));
    q2 = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngle::MakeFromDegree(30));
    EZ_TEST_BOOL(q1 != q2);

    q2 = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(1, 0, 0), ezAngle::MakeFromDegree(60));
    EZ_TEST_BOOL(q1 != q2);

    q2 = ezSimdQuatd::MakeFromAxisAndAngle(ezSimdVec4d(0, 0, 1), ezAngle::MakeFromDegree(60));
    EZ_TEST_BOOL(q1 == q2);
  }
}
