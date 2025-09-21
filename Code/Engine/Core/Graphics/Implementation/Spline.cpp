#include <GameEngine/GameEnginePCH.h>

#include <Core/Graphics/Spline.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/SimdMath/SimdConversion.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSplineTangentMode, 1)
  EZ_ENUM_CONSTANTS(ezSplineTangentMode::Auto, ezSplineTangentMode::Custom, ezSplineTangentMode::Linear)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezResult ezSpline::ControlPoint::Serialize(ezStreamWriter& s) const
{
  s << ezSimdConversion::ToVec3(m_vPos);
  s << ezSimdConversion::ToVec4(m_vPosTangentIn);  // Contains the tangent mode in w
  s << ezSimdConversion::ToVec4(m_vPosTangentOut); // Contains the tangent mode in w

  s << ezSimdConversion::ToVec4(m_vUpDirAndRoll);  // Roll in w
  s << ezSimdConversion::ToVec3(m_vUpDirTangentIn);
  s << ezSimdConversion::ToVec3(m_vUpDirTangentOut);

  s << ezSimdConversion::ToVec3(m_vScale);
  s << ezSimdConversion::ToVec3(m_vScaleTangentIn);
  s << ezSimdConversion::ToVec3(m_vScaleTangentOut);

  return EZ_SUCCESS;
}

ezResult ezSpline::ControlPoint::Deserialize(ezStreamReader& s)
{
  {
    ezVec3 vPos;
    s >> vPos;

    ezVec4 vPosTangentIn, vPosTangentOut; // Contains the tangent mode in w
    s >> vPosTangentIn;
    s >> vPosTangentOut;

    m_vPos = ezSimdConversion::ToVec3(vPos);
    m_vPosTangentIn = ezSimdConversion::ToVec4(vPosTangentIn);
    m_vPosTangentOut = ezSimdConversion::ToVec4(vPosTangentOut);
  }

  {
    ezVec4 vUpDirAndRoll; // Roll in w
    s >> vUpDirAndRoll;

    ezVec3 vUpDirTangentIn, vUpDirTangentOut;
    s >> vUpDirTangentIn;
    s >> vUpDirTangentOut;

    m_vUpDirAndRoll = ezSimdConversion::ToVec4(vUpDirAndRoll);
    m_vUpDirTangentIn = ezSimdConversion::ToVec3(vUpDirTangentIn);
    m_vUpDirTangentOut = ezSimdConversion::ToVec3(vUpDirTangentOut);
  }

  {
    ezVec3 vScale, vScaleTangentIn, vScaleTangentOut;
    s >> vScale;
    s >> vScaleTangentIn;
    s >> vScaleTangentOut;

    m_vScale = ezSimdConversion::ToVec3(vScale);
    m_vScaleTangentIn = ezSimdConversion::ToVec3(vScaleTangentIn);
    m_vScaleTangentOut = ezSimdConversion::ToVec3(vScaleTangentOut);
  }

  return EZ_SUCCESS;
}

void ezSpline::ControlPoint::SetAutoTangents(const ezSimdVec4f& vDirIn, const ezSimdVec4f& vDirOut)
{
  const ezSimdVec4f autoPosTangent = (vDirIn + vDirOut) * 0.5f;
  const ezSimdFloat eps = ezMath::LargeEpsilon<float>();

  {
    auto tangentModeIn = GetTangentModeIn();
    if (tangentModeIn == ezSplineTangentMode::Auto)
    {
      m_vPosTangentIn = -autoPosTangent;
    }
    else if (tangentModeIn == ezSplineTangentMode::Linear)
    {
      m_vPosTangentIn = -vDirIn;
    }
    else
    {
      EZ_ASSERT_DEV(tangentModeIn == ezSplineTangentMode::Custom, "Unknown spline tangent mode");
    }

    // Sanitize tangent
    if (m_vPosTangentIn.GetLengthSquared<3>() < eps)
    {
      m_vPosTangentIn = vDirIn;
      m_vPosTangentIn.NormalizeIfNotZero<3>(ezSimdVec4f(-1, 0, 0));
      m_vPosTangentIn *= eps;
    }

    SetTangentModeIn(ezSplineTangentMode::Custom);
  }

  {
    auto tangentModeOut = GetTangentModeOut();
    if (tangentModeOut == ezSplineTangentMode::Auto)
    {
      m_vPosTangentOut = autoPosTangent;
    }
    else if (tangentModeOut == ezSplineTangentMode::Linear)
    {
      m_vPosTangentOut = vDirOut;
    }
    else
    {
      EZ_ASSERT_DEV(tangentModeOut == ezSplineTangentMode::Custom, "Unknown spline tangent mode");
    }

    // Sanitize tangent
    if (m_vPosTangentOut.GetLengthSquared<3>() < eps)
    {
      m_vPosTangentOut = vDirOut;
      m_vPosTangentOut.NormalizeIfNotZero<3>(ezSimdVec4f(1, 0, 0));
      m_vPosTangentOut *= eps;
    }

    SetTangentModeOut(ezSplineTangentMode::Custom);
  }
}

//////////////////////////////////////////////////////////////////////////

constexpr ezTypeVersion s_SplineVersion = 1;

ezResult ezSpline::Serialize(ezStreamWriter& ref_writer) const
{
  ref_writer.WriteVersion(s_SplineVersion);

  EZ_SUCCEED_OR_RETURN(ref_writer.WriteArray(m_ControlPoints));
  ref_writer << m_bClosed;

  return EZ_SUCCESS;
}

ezResult ezSpline::Deserialize(ezStreamReader& ref_reader)
{
  /*const ezTypeVersion version =*/ref_reader.ReadVersion(s_SplineVersion);

  EZ_SUCCEED_OR_RETURN(ref_reader.ReadArray(m_ControlPoints));
  ref_reader >> m_bClosed;

  return EZ_SUCCESS;
}

void ezSpline::CalculateUpDirAndAutoTangents(const ezSimdVec4f& vGlobalUpDir, const ezSimdVec4f& vGlobalForwardDir)
{
  const ezUInt32 uiNumPoints = m_ControlPoints.GetCount();
  if (uiNumPoints < 2)
    return;

  const ezUInt32 uiLastIdx = uiNumPoints - 1;
  const ezSimdFloat oneThird(1.0f / 3.0f);

  // Position tangents
  {
    ezUInt32 uiNumTangentsToUpdate = uiNumPoints;
    ezUInt32 uiPrevIdx = uiLastIdx - 1;
    ezUInt32 uiCurIdx = uiLastIdx;
    ezUInt32 uiNextIdx = 0;

    if (!m_bClosed)
    {
      const ezSimdVec4f vStartTangent = (m_ControlPoints[1].m_vPos - m_ControlPoints[0].m_vPos) * oneThird;
      const ezSimdVec4f vEndTangent = (m_ControlPoints[uiLastIdx].m_vPos - m_ControlPoints[uiLastIdx - 1].m_vPos) * oneThird;

      m_ControlPoints[0].SetAutoTangents(vStartTangent, vStartTangent);
      m_ControlPoints[uiLastIdx].SetAutoTangents(vEndTangent, vEndTangent);

      uiNumTangentsToUpdate = uiNumPoints - 2;
      uiPrevIdx = 0;
      uiCurIdx = 1;
      uiNextIdx = 2;
    }

    for (ezUInt32 i = 0; i < uiNumTangentsToUpdate; ++i)
    {
      auto& cCp = m_ControlPoints[uiCurIdx];
      const auto& pCP = m_ControlPoints[uiPrevIdx];
      const auto& nCP = m_ControlPoints[uiNextIdx];

      const ezSimdVec4f dirIn = (cCp.m_vPos - pCP.m_vPos) * oneThird;
      const ezSimdVec4f dirOut = (nCP.m_vPos - cCp.m_vPos) * oneThird;

      cCp.SetAutoTangents(dirIn, dirOut);

      uiPrevIdx = uiCurIdx;
      uiCurIdx = uiNextIdx;
      ++uiNextIdx;
    }
  }

  // Up dir
  {
    for (ezUInt32 i = 0; i < uiNumPoints; ++i)
    {
      auto& cp = m_ControlPoints[i];

      ezSimdVec4f forwardDir = EvaluateDerivative(i, 0.0f);
      forwardDir.NormalizeIfNotZero<3>(vGlobalForwardDir);

      const ezSimdVec4f upDir = [&]()
      {
        if (!forwardDir.IsEqual(vGlobalUpDir, ezMath::HugeEpsilon<float>()).AllSet<3>())
          return vGlobalUpDir;

        if (i > 0)
        {
          auto& prevCp = m_ControlPoints[i - 1];
          if (!forwardDir.IsEqual(prevCp.m_vUpDirAndRoll, ezMath::HugeEpsilon<float>()).AllSet<3>())
          {
            return prevCp.m_vUpDirAndRoll;
          }
        }

        return vGlobalForwardDir;
      }();

      const ezSimdVec4f rightDir = upDir.CrossRH(forwardDir).GetNormalized<3>();
      const ezSimdVec4f upDir2 = forwardDir.CrossRH(rightDir).GetNormalized<3>();
      const ezSimdFloat roll = cp.GetRoll();
      const ezSimdQuat rotation = ezSimdQuat::MakeFromAxisAndAngle(forwardDir, roll);

      cp.m_vUpDirAndRoll = rotation * upDir2;
      cp.m_vUpDirAndRoll.SetW(roll);
      cp.m_vUpDirTangentIn.SetZero();
      cp.m_vUpDirTangentOut.SetZero();
    }
  }

  // up dir and scale tangents
  {
    ezUInt32 uiNumTangentsToUpdate = uiNumPoints;
    ezUInt32 uiPrevIdx = uiLastIdx - 1;
    ezUInt32 uiCurIdx = uiLastIdx;
    ezUInt32 uiNextIdx = 0;


    if (!m_bClosed)
    {
      {
        auto& cp0 = m_ControlPoints[0];
        auto& cp1 = m_ControlPoints[1];

        const ezSimdVec4f vUpDirTangent = (cp1.m_vUpDirAndRoll - cp0.m_vUpDirAndRoll) * oneThird;
        const ezSimdVec4f vScaleTangent = (cp1.m_vScale - cp0.m_vScale) * oneThird;

        cp0.m_vUpDirTangentIn = -vUpDirTangent;
        cp0.m_vUpDirTangentOut = vUpDirTangent;
        cp0.m_vScaleTangentIn = -vScaleTangent;
        cp0.m_vScaleTangentOut = vScaleTangent;
      }

      {
        auto& cpLast = m_ControlPoints[uiLastIdx];
        auto& cpPrev = m_ControlPoints[uiLastIdx - 1];

        const ezSimdVec4f vUpDirTangent = (cpLast.m_vUpDirAndRoll - cpPrev.m_vUpDirAndRoll) * oneThird;
        const ezSimdVec4f vScaleTangent = (cpLast.m_vScale - cpPrev.m_vScale) * oneThird;

        cpLast.m_vUpDirTangentIn = -vUpDirTangent;
        cpLast.m_vUpDirTangentOut = vUpDirTangent;
        cpLast.m_vScaleTangentIn = -vScaleTangent;
        cpLast.m_vScaleTangentOut = vScaleTangent;
      }

      uiNumTangentsToUpdate = uiNumPoints - 2;
      uiPrevIdx = 0;
      uiCurIdx = 1;
      uiNextIdx = 2;
    }

    for (ezUInt32 i = 0; i < uiNumTangentsToUpdate; ++i)
    {
      auto& cCp = m_ControlPoints[uiCurIdx];
      const auto& pCP = m_ControlPoints[uiPrevIdx];
      const auto& nCP = m_ControlPoints[uiNextIdx];

      // Do not use classic auto tangents here, since we don't want overshooting for the up direction and scale.
      const ezSimdVec4f vUpDirTangent = (cCp.m_vUpDirAndRoll - pCP.m_vUpDirAndRoll).CompMin(nCP.m_vUpDirAndRoll - cCp.m_vUpDirAndRoll) * oneThird;
      const ezSimdVec4f vScaleTangent = (cCp.m_vScale - pCP.m_vScale).CompMin(nCP.m_vScale - cCp.m_vScale) * oneThird;

      cCp.m_vUpDirTangentIn = -vUpDirTangent;
      cCp.m_vUpDirTangentOut = vUpDirTangent;
      cCp.m_vScaleTangentIn = -vScaleTangent;
      cCp.m_vScaleTangentOut = vScaleTangent;

      uiPrevIdx = uiCurIdx;
      uiCurIdx = uiNextIdx;
      ++uiNextIdx;
    }
  }
}

ezSimdTransform ezSpline::EvaluateTransform(float fT) const
{
  if (m_ControlPoints.IsEmpty())
    return ezSimdTransform::MakeIdentity();

  ezUInt32 uiCp0;
  fT = ClampAndSplitT(fT, uiCp0);

  const ezUInt32 uiCp1 = GetCp1Index(uiCp0);
  const auto& cp0 = m_ControlPoints[uiCp0];
  const auto& cp1 = m_ControlPoints[uiCp1];

  ezSimdTransform transform;
  transform.m_Position = EvaluatePosition(cp0, cp1, fT);

  ezSimdVec4f forwardDir, rightDir, upDir;
  EvaluateRotation(cp0, cp1, fT, forwardDir, rightDir, upDir);

  ezMat3 mRot;
  mRot.SetColumn(0, ezSimdConversion::ToVec3(forwardDir));
  mRot.SetColumn(1, ezSimdConversion::ToVec3(rightDir));
  mRot.SetColumn(2, ezSimdConversion::ToVec3(upDir));
  transform.m_Rotation = ezSimdConversion::ToQuat(ezQuat::MakeFromMat3(mRot));

  transform.m_Scale = EvaluateScale(cp0, cp1, fT);

  return transform;
}


EZ_STATICLINK_FILE(Core, Core_Graphics_Implementation_Spline);
