#include <CoreTest/CoreTestPCH.h>

#include <Core/World/CoordinateSystem.h>

void TestLength(const ezCoordinateSystemConversion& atoB, const ezCoordinateSystemConversion& btoA, float fSourceLength, float fTargetLength)
{
  EZ_TEST_FLOAT(atoB.ConvertSourceLength(fSourceLength), fTargetLength, ezMath::DefaultEpsilon<float>());
  EZ_TEST_FLOAT(atoB.ConvertTargetLength(fTargetLength), fSourceLength, ezMath::DefaultEpsilon<float>());

  EZ_TEST_FLOAT(btoA.ConvertTargetLength(fSourceLength), fTargetLength, ezMath::DefaultEpsilon<float>());
  EZ_TEST_FLOAT(btoA.ConvertSourceLength(fTargetLength), fSourceLength, ezMath::DefaultEpsilon<float>());
}

void TestPosition(
  const ezCoordinateSystemConversion& atoB, const ezCoordinateSystemConversion& btoA, const ezVec3& vSourcePos, const ezVec3& vTargetPos)
{
  TestLength(atoB, btoA, vSourcePos.GetLength(), vTargetPos.GetLength());

  EZ_TEST_VEC3(atoB.ConvertSourcePosition(vSourcePos), vTargetPos, ezMath::DefaultEpsilon<float>());
  EZ_TEST_VEC3(atoB.ConvertTargetPosition(vTargetPos), vSourcePos, ezMath::DefaultEpsilon<float>());
  EZ_TEST_VEC3(btoA.ConvertSourcePosition(vTargetPos), vSourcePos, ezMath::DefaultEpsilon<float>());
  EZ_TEST_VEC3(btoA.ConvertTargetPosition(vSourcePos), vTargetPos, ezMath::DefaultEpsilon<float>());
}

void TestRotation(const ezCoordinateSystemConversion& atoB, const ezCoordinateSystemConversion& btoA, const ezVec3& vSourceStartDir,
  const ezVec3& vSourceEndDir, const ezQuat& qSourceRot, const ezVec3& vTargetStartDir, const ezVec3& vTargetEndDir, const ezQuat& qTargetRot)
{
  TestPosition(atoB, btoA, vSourceStartDir, vTargetStartDir);
  TestPosition(atoB, btoA, vSourceEndDir, vTargetEndDir);

  EZ_TEST_BOOL(atoB.ConvertSourceRotation(qSourceRot).IsEqualRotation(qTargetRot, ezMath::DefaultEpsilon<float>()));
  EZ_TEST_BOOL(atoB.ConvertTargetRotation(qTargetRot).IsEqualRotation(qSourceRot, ezMath::DefaultEpsilon<float>()));
  EZ_TEST_BOOL(btoA.ConvertSourceRotation(qTargetRot).IsEqualRotation(qSourceRot, ezMath::DefaultEpsilon<float>()));
  EZ_TEST_BOOL(btoA.ConvertTargetRotation(qSourceRot).IsEqualRotation(qTargetRot, ezMath::DefaultEpsilon<float>()));

  EZ_TEST_VEC3(qSourceRot * vSourceStartDir, vSourceEndDir, ezMath::DefaultEpsilon<float>());
  EZ_TEST_VEC3(qTargetRot * vTargetStartDir, vTargetEndDir, ezMath::DefaultEpsilon<float>());
}

ezQuat FromAxisAndAngle(const ezVec3& vAxis, ezAngle angle)
{
  ezQuat q;
  q.SetFromAxisAndAngle(vAxis.GetNormalized(), angle);
  return q;
}

bool IsRightHanded(const ezCoordinateSystem& cs)
{
  ezVec3 vF = cs.m_vUpDir.CrossRH(cs.m_vRightDir);

  return vF.Dot(cs.m_vForwardDir) > 0;
}

void TestCoordinateSystemConversion(const ezCoordinateSystem& a, const ezCoordinateSystem& b)
{
  const bool bAisRH = IsRightHanded(a);
  const bool bBisRH = IsRightHanded(b);
  const ezAngle A_CWRot = bAisRH ? ezAngle::Degree(-90.0f) : ezAngle::Degree(90.0f);
  const ezAngle B_CWRot = bBisRH ? ezAngle::Degree(-90.0f) : ezAngle::Degree(90.0f);

  ezCoordinateSystemConversion AtoB;
  AtoB.SetConversion(a, b);

  ezCoordinateSystemConversion BtoA;
  BtoA.SetConversion(b, a);

  TestPosition(AtoB, BtoA, a.m_vForwardDir, b.m_vForwardDir);
  TestPosition(AtoB, BtoA, a.m_vRightDir, b.m_vRightDir);
  TestPosition(AtoB, BtoA, a.m_vUpDir, b.m_vUpDir);

  TestRotation(AtoB, BtoA, a.m_vForwardDir, a.m_vRightDir, FromAxisAndAngle(a.m_vUpDir, A_CWRot), b.m_vForwardDir, b.m_vRightDir,
    FromAxisAndAngle(b.m_vUpDir, B_CWRot));
  TestRotation(AtoB, BtoA, a.m_vUpDir, a.m_vForwardDir, FromAxisAndAngle(a.m_vRightDir, A_CWRot), b.m_vUpDir, b.m_vForwardDir,
    FromAxisAndAngle(b.m_vRightDir, B_CWRot));
  TestRotation(AtoB, BtoA, a.m_vUpDir, a.m_vRightDir, FromAxisAndAngle(a.m_vForwardDir, -A_CWRot), b.m_vUpDir, b.m_vRightDir,
    FromAxisAndAngle(b.m_vForwardDir, -B_CWRot));
}


EZ_CREATE_SIMPLE_TEST(World, CoordinateSystem)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "EZ / OpenXR")
  {
    ezCoordinateSystem ezCoordSysLH;
    ezCoordSysLH.m_vForwardDir = ezVec3(1.0f, 0.0f, 0.0f);
    ezCoordSysLH.m_vRightDir = ezVec3(0.0f, 1.0f, 0.0f);
    ezCoordSysLH.m_vUpDir = ezVec3(0.0f, 0.0f, 1.0f);

    ezCoordinateSystem openXrRH;
    openXrRH.m_vForwardDir = ezVec3(0.0f, 0.0f, -1.0f);
    openXrRH.m_vRightDir = ezVec3(1.0f, 0.0f, 0.0f);
    openXrRH.m_vUpDir = ezVec3(0.0f, 1.0f, 0.0f);

    TestCoordinateSystemConversion(ezCoordSysLH, openXrRH);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Scaled EZ / Scaled OpenXR")
  {
    ezCoordinateSystem ezCoordSysLH;
    ezCoordSysLH.m_vForwardDir = ezVec3(0.1f, 0.0f, 0.0f);
    ezCoordSysLH.m_vRightDir = ezVec3(0.0f, 0.1f, 0.0f);
    ezCoordSysLH.m_vUpDir = ezVec3(0.0f, 0.0f, 0.1f);

    ezCoordinateSystem openXrRH;
    openXrRH.m_vForwardDir = ezVec3(0.0f, 0.0f, -20.0f);
    openXrRH.m_vRightDir = ezVec3(20.0f, 0.0f, 0.0f);
    openXrRH.m_vUpDir = ezVec3(0.0f, 20.0f, 0.0f);

    TestCoordinateSystemConversion(ezCoordSysLH, openXrRH);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "EZ / Flipped EZ")
  {
    ezCoordinateSystem ezCoordSysLH;
    ezCoordSysLH.m_vForwardDir = ezVec3(1.0f, 0.0f, 0.0f);
    ezCoordSysLH.m_vRightDir = ezVec3(0.0f, 1.0f, 0.0f);
    ezCoordSysLH.m_vUpDir = ezVec3(0.0f, 0.0f, 1.0f);

    ezCoordinateSystem ezCoordSysFlippedLH;
    ezCoordSysFlippedLH.m_vForwardDir = ezVec3(-1.0f, 0.0f, 0.0f);
    ezCoordSysFlippedLH.m_vRightDir = ezVec3(0.0f, -1.0f, 0.0f);
    ezCoordSysFlippedLH.m_vUpDir = ezVec3(0.0f, 0.0f, 1.0f);

    TestCoordinateSystemConversion(ezCoordSysLH, ezCoordSysFlippedLH);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "OpenXR / Flipped OpenXR")
  {
    ezCoordinateSystem openXrRH;
    openXrRH.m_vForwardDir = ezVec3(0.0f, 0.0f, -1.0f);
    openXrRH.m_vRightDir = ezVec3(1.0f, 0.0f, 0.0f);
    openXrRH.m_vUpDir = ezVec3(0.0f, 1.0f, 0.0f);

    ezCoordinateSystem openXrFlippedRH;
    openXrFlippedRH.m_vForwardDir = ezVec3(0.0f, 0.0f, 1.0f);
    openXrFlippedRH.m_vRightDir = ezVec3(-1.0f, 0.0f, 0.0f);
    openXrFlippedRH.m_vUpDir = ezVec3(0.0f, 1.0f, 0.0f);

    TestCoordinateSystemConversion(openXrRH, openXrFlippedRH);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Identity")
  {
    ezCoordinateSystem ezCoordSysLH;
    ezCoordSysLH.m_vForwardDir = ezVec3(1.0f, 0.0f, 0.0f);
    ezCoordSysLH.m_vRightDir = ezVec3(0.0f, 1.0f, 0.0f);
    ezCoordSysLH.m_vUpDir = ezVec3(0.0f, 0.0f, 1.0f);

    TestCoordinateSystemConversion(ezCoordSysLH, ezCoordSysLH);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Default Constructed")
  {
    ezCoordinateSystem ezCoordSysLH;
    ezCoordSysLH.m_vForwardDir = ezVec3(1.0f, 0.0f, 0.0f);
    ezCoordSysLH.m_vRightDir = ezVec3(0.0f, 1.0f, 0.0f);
    ezCoordSysLH.m_vUpDir = ezVec3(0.0f, 0.0f, 1.0f);

    const ezAngle rot = ezAngle::Degree(90.0f);

    ezCoordinateSystemConversion defaultConstucted;

    TestPosition(defaultConstucted, defaultConstucted, ezCoordSysLH.m_vForwardDir, ezCoordSysLH.m_vForwardDir);
    TestPosition(defaultConstucted, defaultConstucted, ezCoordSysLH.m_vRightDir, ezCoordSysLH.m_vRightDir);
    TestPosition(defaultConstucted, defaultConstucted, ezCoordSysLH.m_vUpDir, ezCoordSysLH.m_vUpDir);

    TestRotation(defaultConstucted, defaultConstucted, ezCoordSysLH.m_vForwardDir, ezCoordSysLH.m_vRightDir,
      FromAxisAndAngle(ezCoordSysLH.m_vUpDir, rot), ezCoordSysLH.m_vForwardDir, ezCoordSysLH.m_vRightDir,
      FromAxisAndAngle(ezCoordSysLH.m_vUpDir, rot));
    TestRotation(defaultConstucted, defaultConstucted, ezCoordSysLH.m_vUpDir, ezCoordSysLH.m_vForwardDir,
      FromAxisAndAngle(ezCoordSysLH.m_vRightDir, rot), ezCoordSysLH.m_vUpDir, ezCoordSysLH.m_vForwardDir,
      FromAxisAndAngle(ezCoordSysLH.m_vRightDir, rot));
    TestRotation(defaultConstucted, defaultConstucted, ezCoordSysLH.m_vUpDir, ezCoordSysLH.m_vRightDir,
      FromAxisAndAngle(ezCoordSysLH.m_vForwardDir, -rot), ezCoordSysLH.m_vUpDir, ezCoordSysLH.m_vRightDir,
      FromAxisAndAngle(ezCoordSysLH.m_vForwardDir, -rot));
  }
}
