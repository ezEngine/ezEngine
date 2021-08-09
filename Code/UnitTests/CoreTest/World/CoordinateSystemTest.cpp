#include <CoreTest/CoreTestPCH.h>

#include <Core/World/CoordinateSystem.h>

void TestLength(const ezCoordinateSystemConversion& AtoB, const ezCoordinateSystemConversion& BtoA, float fSourceLength, float fTargetLength)
{
  EZ_TEST_FLOAT(AtoB.ConvertSourceLength(fSourceLength), fTargetLength, ezMath::DefaultEpsilon<float>());
  EZ_TEST_FLOAT(AtoB.ConvertTargetLength(fTargetLength), fSourceLength, ezMath::DefaultEpsilon<float>());

  EZ_TEST_FLOAT(BtoA.ConvertTargetLength(fSourceLength), fTargetLength, ezMath::DefaultEpsilon<float>());
  EZ_TEST_FLOAT(BtoA.ConvertSourceLength(fTargetLength), fSourceLength, ezMath::DefaultEpsilon<float>());
}

void TestPosition(
  const ezCoordinateSystemConversion& AtoB, const ezCoordinateSystemConversion& BtoA, const ezVec3& vSourcePos, const ezVec3& vTargetPos)
{
  TestLength(AtoB, BtoA, vSourcePos.GetLength(), vTargetPos.GetLength());

  EZ_TEST_VEC3(AtoB.ConvertSourcePosition(vSourcePos), vTargetPos, ezMath::DefaultEpsilon<float>());
  EZ_TEST_VEC3(AtoB.ConvertTargetPosition(vTargetPos), vSourcePos, ezMath::DefaultEpsilon<float>());
  EZ_TEST_VEC3(BtoA.ConvertSourcePosition(vTargetPos), vSourcePos, ezMath::DefaultEpsilon<float>());
  EZ_TEST_VEC3(BtoA.ConvertTargetPosition(vSourcePos), vTargetPos, ezMath::DefaultEpsilon<float>());
}

void TestRotation(const ezCoordinateSystemConversion& AtoB, const ezCoordinateSystemConversion& BtoA, const ezVec3& vSourceStartDir,
  const ezVec3& vSourceEndDir, const ezQuat& qSourceRot, const ezVec3& vTargetStartDir, const ezVec3& vTargetEndDir, const ezQuat& qTargetRot)
{
  TestPosition(AtoB, BtoA, vSourceStartDir, vTargetStartDir);
  TestPosition(AtoB, BtoA, vSourceEndDir, vTargetEndDir);

  EZ_TEST_BOOL(AtoB.ConvertSourceRotation(qSourceRot).IsEqualRotation(qTargetRot, ezMath::DefaultEpsilon<float>()));
  EZ_TEST_BOOL(AtoB.ConvertTargetRotation(qTargetRot).IsEqualRotation(qSourceRot, ezMath::DefaultEpsilon<float>()));
  EZ_TEST_BOOL(BtoA.ConvertSourceRotation(qTargetRot).IsEqualRotation(qSourceRot, ezMath::DefaultEpsilon<float>()));
  EZ_TEST_BOOL(BtoA.ConvertTargetRotation(qSourceRot).IsEqualRotation(qTargetRot, ezMath::DefaultEpsilon<float>()));

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

void TestCoordinateSystemConversion(const ezCoordinateSystem& A, const ezCoordinateSystem& B)
{
  const bool bAisRH = IsRightHanded(A);
  const bool bBisRH = IsRightHanded(B);
  const ezAngle A_CWRot = bAisRH ? ezAngle::Degree(-90.0f) : ezAngle::Degree(90.0f);
  const ezAngle B_CWRot = bBisRH ? ezAngle::Degree(-90.0f) : ezAngle::Degree(90.0f);

  ezCoordinateSystemConversion AtoB;
  AtoB.SetConversion(A, B);

  ezCoordinateSystemConversion BtoA;
  BtoA.SetConversion(B, A);

  TestPosition(AtoB, BtoA, A.m_vForwardDir, B.m_vForwardDir);
  TestPosition(AtoB, BtoA, A.m_vRightDir, B.m_vRightDir);
  TestPosition(AtoB, BtoA, A.m_vUpDir, B.m_vUpDir);

  TestRotation(AtoB, BtoA, A.m_vForwardDir, A.m_vRightDir, FromAxisAndAngle(A.m_vUpDir, A_CWRot), B.m_vForwardDir, B.m_vRightDir,
    FromAxisAndAngle(B.m_vUpDir, B_CWRot));
  TestRotation(AtoB, BtoA, A.m_vUpDir, A.m_vForwardDir, FromAxisAndAngle(A.m_vRightDir, A_CWRot), B.m_vUpDir, B.m_vForwardDir,
    FromAxisAndAngle(B.m_vRightDir, B_CWRot));
  TestRotation(AtoB, BtoA, A.m_vUpDir, A.m_vRightDir, FromAxisAndAngle(A.m_vForwardDir, -A_CWRot), B.m_vUpDir, B.m_vRightDir,
    FromAxisAndAngle(B.m_vForwardDir, -B_CWRot));
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
