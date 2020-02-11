#include <FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Tracks/Curve1D.h>

ezCurve1D::ControlPoint::ControlPoint()
{
  m_Position.SetZero();
  m_LeftTangent.SetZero();
  m_RightTangent.SetZero();
  m_uiOriginalIndex = 0;
}

ezCurve1D::ezCurve1D()
{
  Clear();
}

void ezCurve1D::Clear()
{
  m_fMinX = 0;
  m_fMaxX = 0;
  m_fMinY = 0;
  m_fMaxY = 0;

  m_ControlPoints.Clear();
}

bool ezCurve1D::IsEmpty() const
{
  return m_ControlPoints.IsEmpty();
}

ezCurve1D::ControlPoint& ezCurve1D::AddControlPoint(double x)
{
  auto& cp = m_ControlPoints.ExpandAndGetRef();
  cp.m_uiOriginalIndex = static_cast<ezUInt16>(m_ControlPoints.GetCount() - 1);
  cp.m_Position.x = x;
  cp.m_Position.y = 0;
  cp.m_LeftTangent.x = -0.1f;
  cp.m_LeftTangent.y = 0.0f;
  cp.m_RightTangent.x = +0.1f;
  cp.m_RightTangent.y = 0.0f;

  return cp;
}

void ezCurve1D::QueryExtents(double& minx, double& maxx) const
{
  minx = m_fMinX;
  maxx = m_fMaxX;
}

void ezCurve1D::QueryExtremeValues(double& minVal, double& maxVal) const
{
  minVal = m_fMinY;
  maxVal = m_fMaxY;
}

ezUInt32 ezCurve1D::GetNumControlPoints() const
{
  return m_ControlPoints.GetCount();
}

void ezCurve1D::SortControlPoints()
{
  m_ControlPoints.Sort();

  RecomputeExtents();
}

ezInt32 ezCurve1D::FindApproxControlPoint(double x) const
{
  ezUInt32 uiLowIdx = 0;
  ezUInt32 uiHighIdx = m_LinearApproximation.GetCount();

  // do a binary search to reduce the search space
  while (uiHighIdx - uiLowIdx > 8)
  {
    const ezUInt32 uiMidIdx = uiLowIdx + ((uiHighIdx - uiLowIdx) >> 1); // lerp

    // doesn't matter whether to use > or >=
    if (m_LinearApproximation[uiMidIdx].x > x)
      uiHighIdx = uiMidIdx;
    else
      uiLowIdx = uiMidIdx;
  }

  // now do a linear search to find the final item
  for (ezUInt32 idx = uiLowIdx; idx < uiHighIdx; ++idx)
  {
    if (m_LinearApproximation[idx].x >= x)
    {
      // when m_LinearApproximation[0].x >= x, we want to return -1
      return ((ezInt32)idx) - 1;
    }
  }

  // return last index
  return (ezInt32)uiHighIdx - 1;
}

double ezCurve1D::Evaluate(double x) const
{
  EZ_ASSERT_DEBUG(!m_LinearApproximation.IsEmpty(), "Cannot evaluate curve without precomputing curve approximation data first. Call "
                                                    "CreateLinearApproximation() on curve before calling Evaluate().");

  if (m_LinearApproximation.GetCount() >= 2)
  {
    const ezUInt32 numCPs = m_LinearApproximation.GetCount();
    const ezInt32 iControlPoint = FindApproxControlPoint(x);

    if (iControlPoint == -1)
    {
      // clamp to left value
      return m_LinearApproximation[0].y;
    }
    else if (iControlPoint == numCPs - 1)
    {
      // clamp to right value
      return m_LinearApproximation[numCPs - 1].y;
    }
    else
    {
      const double v1 = m_LinearApproximation[iControlPoint].y;
      const double v2 = m_LinearApproximation[iControlPoint + 1].y;

      // interpolate
      double lerpX = x - m_LinearApproximation[iControlPoint].x;
      const double len = (m_LinearApproximation[iControlPoint + 1].x - m_LinearApproximation[iControlPoint].x);

      if (len <= 0)
        lerpX = 0;
      else
        lerpX /= len; // TODO remove division ?

      return ezMath::Lerp(v1, v2, lerpX);
    }
  }
  else if (m_LinearApproximation.GetCount() == 1)
  {
    return m_LinearApproximation[0].y;
  }

  return 0;
}

double ezCurve1D::ConvertNormalizedPos(double pos) const
{
  double fMin, fMax;
  QueryExtents(fMin, fMax);

  return ezMath::Lerp(fMin, fMax, pos);
}


double ezCurve1D::NormalizeValue(double value) const
{
  double fMin, fMax;
  QueryExtremeValues(fMin, fMax);

  if (fMin >= fMax)
    return 0;

  return (value - fMin) / (fMax - fMin);
}

ezUInt64 ezCurve1D::GetHeapMemoryUsage() const
{
  return m_ControlPoints.GetHeapMemoryUsage();
}

void ezCurve1D::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 4;

  stream << uiVersion;

  const ezUInt32 numCp = m_ControlPoints.GetCount();

  stream << numCp;

  for (const auto& cp : m_ControlPoints)
  {
    stream << cp.m_Position;
    stream << cp.m_LeftTangent;
    stream << cp.m_RightTangent;
    stream << cp.m_TangentModeRight;
    stream << cp.m_TangentModeLeft;
  }
}

void ezCurve1D::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;

  stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion <= 4, "Incorrect version '{0}' for ezCurve1D", uiVersion);

  ezUInt32 numCp = 0;

  stream >> numCp;

  m_ControlPoints.SetCountUninitialized(numCp);

  if (uiVersion <= 2)
  {
    for (auto& cp : m_ControlPoints)
    {
      ezVec2 pos;
      stream >> pos;
      cp.m_Position.Set(pos.x, pos.y);

      if (uiVersion >= 2)
      {
        stream >> cp.m_LeftTangent;
        stream >> cp.m_RightTangent;
      }
    }
  }
  else
  {
    for (auto& cp : m_ControlPoints)
    {
      stream >> cp.m_Position;
      stream >> cp.m_LeftTangent;
      stream >> cp.m_RightTangent;

      if (uiVersion >= 4)
      {
        stream >> cp.m_TangentModeRight;
        stream >> cp.m_TangentModeLeft;
      }
    }
  }
}

void ezCurve1D::CreateLinearApproximation(double fMaxError /*= 0.01f*/, ezUInt8 uiMaxSubDivs /*= 8*/)
{
  m_LinearApproximation.Clear();

  /// \todo Since we do this, we actually don't need the linear approximation anymore and could just evaluate the full curve
  ApplyTangentModes();

  ClampTangents();

  if (m_ControlPoints.IsEmpty())
  {
    m_LinearApproximation.PushBack(ezVec2d::ZeroVector());
    return;
  }

  for (ezUInt32 i = 1; i < m_ControlPoints.GetCount(); ++i)
  {
    double fMinY, fMaxY;
    ApproximateMinMaxValues(m_ControlPoints[i - 1], m_ControlPoints[i], fMinY, fMaxY);

    const double rangeY = ezMath::Max(0.1, fMaxY - fMinY);
    const double fMaxErrorY = fMaxError * rangeY;
    const double fMaxErrorX = (m_ControlPoints[i].m_Position.x - m_ControlPoints[i - 1].m_Position.x) * fMaxError;


    m_LinearApproximation.PushBack(m_ControlPoints[i - 1].m_Position);

    ApproximateCurve(m_ControlPoints[i - 1].m_Position,
                     m_ControlPoints[i - 1].m_Position +
                         ezVec2d(m_ControlPoints[i - 1].m_RightTangent.x, m_ControlPoints[i - 1].m_RightTangent.y),
                     m_ControlPoints[i].m_Position + ezVec2d(m_ControlPoints[i].m_LeftTangent.x, m_ControlPoints[i].m_LeftTangent.y),
                     m_ControlPoints[i].m_Position, fMaxErrorX, fMaxErrorY, uiMaxSubDivs);
  }

  m_LinearApproximation.PushBack(m_ControlPoints.PeekBack().m_Position);

  RecomputeLinearApproxExtremes();
}

void ezCurve1D::RecomputeExtents()
{
  m_fMinX = ezMath::MaxValue<float>();
  m_fMaxX = -ezMath::MaxValue<float>();

  for (const auto& cp : m_ControlPoints)
  {
    m_fMinX = ezMath::Min(m_fMinX, cp.m_Position.x);
    m_fMaxX = ezMath::Max(m_fMaxX, cp.m_Position.x);

    // ignore X values that could go outside the control point range due to Bezier curve interpolation
    // we just assume the curve is always restricted along X by the CPs

    // m_fMinX = ezMath::Min(m_fMinX, cp.m_Position.x + cp.m_LeftTangent.x);
    // m_fMaxX = ezMath::Max(m_fMaxX, cp.m_Position.x + cp.m_LeftTangent.x);

    // m_fMinX = ezMath::Min(m_fMinX, cp.m_Position.x + cp.m_RightTangent.x);
    // m_fMaxX = ezMath::Max(m_fMaxX, cp.m_Position.x + cp.m_RightTangent.x);
  }
}


void ezCurve1D::RecomputeLinearApproxExtremes()
{
  m_fMinY = ezMath::MaxValue<float>();
  m_fMaxY = -ezMath::MaxValue<float>();

  for (const auto& cp : m_LinearApproximation)
  {
    m_fMinY = ezMath::Min(m_fMinY, cp.y);
    m_fMaxY = ezMath::Max(m_fMaxY, cp.y);
  }
}

void ezCurve1D::ApproximateMinMaxValues(const ControlPoint& lhs, const ControlPoint& rhs, double& fMinY, double& fMaxY)
{
  fMinY = ezMath::Min(lhs.m_Position.y, rhs.m_Position.y);
  fMaxY = ezMath::Max(lhs.m_Position.y, rhs.m_Position.y);

  fMinY = ezMath::Min(fMinY, lhs.m_Position.y + lhs.m_RightTangent.y);
  fMaxY = ezMath::Max(fMaxY, lhs.m_Position.y + lhs.m_RightTangent.y);

  fMinY = ezMath::Min(fMinY, rhs.m_Position.y + rhs.m_LeftTangent.y);
  fMaxY = ezMath::Max(fMaxY, rhs.m_Position.y + rhs.m_LeftTangent.y);
}

void ezCurve1D::ApproximateCurve(const ezVec2d& p0, const ezVec2d& p1, const ezVec2d& p2, const ezVec2d& p3, double fMaxErrorX,
                                 double fMaxErrorY, ezInt32 iSubDivLeft)
{
  const ezVec2d cubicCenter = ezMath::EvaluateBezierCurve(0.5, p0, p1, p2, p3);

  ApproximateCurvePiece(p0, p1, p2, p3, 0.0f, p0, 0.5, cubicCenter, fMaxErrorX, fMaxErrorY, iSubDivLeft);

  // always insert the center point
  // with an S curve the cubicCenter and the linearCenter can be identical even though the rest of the curve is absolutely not linear
  m_LinearApproximation.PushBack(cubicCenter);

  ApproximateCurvePiece(p0, p1, p2, p3, 0.5, cubicCenter, 1.0, p3, fMaxErrorX, fMaxErrorY, iSubDivLeft);
}

void ezCurve1D::ApproximateCurvePiece(const ezVec2d& p0, const ezVec2d& p1, const ezVec2d& p2, const ezVec2d& p3, double tLeft,
                                      const ezVec2d& pLeft, double tRight, const ezVec2d& pRight, double fMaxErrorX, double fMaxErrorY,
                                      ezInt32 iSubDivLeft)
{
  // this is a safe guard
  if (iSubDivLeft <= 0)
    return;

  const double tCenter = ezMath::Lerp(tLeft, tRight, 0.5);

  const ezVec2d cubicCenter = ezMath::EvaluateBezierCurve(tCenter, p0, p1, p2, p3);
  const ezVec2d linearCenter = ezMath::Lerp(pLeft, pRight, 0.5);

  // check whether the linear interpolation between pLeft and pRight would already result in a good enough approximation
  // if not, subdivide the curve further

  const double fThisErrorX = ezMath::Abs(cubicCenter.x - linearCenter.x);
  const double fThisErrorY = ezMath::Abs(cubicCenter.y - linearCenter.y);

  if (fThisErrorX < fMaxErrorX && fThisErrorY < fMaxErrorY)
    return;

  ApproximateCurvePiece(p0, p1, p2, p3, tLeft, pLeft, tCenter, cubicCenter, fMaxErrorX, fMaxErrorY, iSubDivLeft - 1);

  m_LinearApproximation.PushBack(cubicCenter);

  ApproximateCurvePiece(p0, p1, p2, p3, tCenter, cubicCenter, tRight, pRight, fMaxErrorX, fMaxErrorY, iSubDivLeft - 1);
}

void ezCurve1D::ClampTangents()
{
  if (m_ControlPoints.GetCount() < 2)
    return;

  for (ezUInt32 i = 1; i < m_ControlPoints.GetCount() - 1; ++i)
  {
    auto& tCP = m_ControlPoints[i];
    const auto& pCP = m_ControlPoints[i - 1];
    const auto& nCP = m_ControlPoints[i + 1];

    ezVec2d lpt = tCP.m_Position + ezVec2d(tCP.m_LeftTangent.x, tCP.m_LeftTangent.y);
    ezVec2d rpt = tCP.m_Position + ezVec2d(tCP.m_RightTangent.x, tCP.m_RightTangent.y);

    lpt.x = ezMath::Clamp(lpt.x, pCP.m_Position.x, tCP.m_Position.x);
    rpt.x = ezMath::Clamp(rpt.x, tCP.m_Position.x, nCP.m_Position.x);

    const ezVec2d tangentL = lpt - tCP.m_Position;
    const ezVec2d tangentR = rpt - tCP.m_Position;

    tCP.m_LeftTangent.Set((float)tangentL.x, (float)tangentL.y);
    tCP.m_RightTangent.Set((float)tangentR.x, (float)tangentR.y);
  }

  // first CP
  {
    auto& tCP = m_ControlPoints[0];
    const auto& nCP = m_ControlPoints[1];

    ezVec2d rpt = tCP.m_Position + ezVec2d(tCP.m_RightTangent.x, tCP.m_RightTangent.y);
    rpt.x = ezMath::Clamp(rpt.x, tCP.m_Position.x, nCP.m_Position.x);

    const ezVec2d tangentR = rpt - tCP.m_Position;
    tCP.m_RightTangent.Set((float)tangentR.x, (float)tangentR.y);
  }

  // last CP
  {
    auto& tCP = m_ControlPoints[m_ControlPoints.GetCount() - 1];
    const auto& pCP = m_ControlPoints[m_ControlPoints.GetCount() - 2];

    ezVec2d lpt = tCP.m_Position + ezVec2d(tCP.m_LeftTangent.x, tCP.m_LeftTangent.y);
    lpt.x = ezMath::Clamp(lpt.x, pCP.m_Position.x, tCP.m_Position.x);

    const ezVec2d tangentL = lpt - tCP.m_Position;
    tCP.m_LeftTangent.Set((float)tangentL.x, (float)tangentL.y);
  }
}

void ezCurve1D::ApplyTangentModes()
{
  if (m_ControlPoints.GetCount() < 2)
    return;

  for (ezUInt32 i = 1; i < m_ControlPoints.GetCount() - 1; ++i)
  {
    const auto& cp = m_ControlPoints[i];

    if (cp.m_TangentModeLeft == ezCurveTangentMode::FixedLength)
      MakeFixedLengthTangentLeft(i);
    else if (cp.m_TangentModeLeft == ezCurveTangentMode::Linear)
      MakeLinearTangentLeft(i);
    else if (cp.m_TangentModeLeft == ezCurveTangentMode::Auto)
      MakeAutoTangentLeft(i);

    if (cp.m_TangentModeRight == ezCurveTangentMode::FixedLength)
      MakeFixedLengthTangentRight(i);
    else if (cp.m_TangentModeRight == ezCurveTangentMode::Linear)
      MakeLinearTangentRight(i);
    else if (cp.m_TangentModeRight == ezCurveTangentMode::Auto)
      MakeAutoTangentRight(i);
  }

  // first CP
  {
    const ezUInt32 i = 0;
    const auto& cp = m_ControlPoints[i];

    if (cp.m_TangentModeRight == ezCurveTangentMode::FixedLength)
      MakeFixedLengthTangentRight(i);
    else if (cp.m_TangentModeRight == ezCurveTangentMode::Linear)
      MakeLinearTangentRight(i);
    else if (cp.m_TangentModeRight == ezCurveTangentMode::Auto)
      MakeLinearTangentRight(i); // note: first point will always be linear in auto mode
  }

  // last CP
  {
    const ezUInt32 i = m_ControlPoints.GetCount() - 1;
    const auto& cp = m_ControlPoints[i];

    if (cp.m_TangentModeLeft == ezCurveTangentMode::FixedLength)
      MakeFixedLengthTangentLeft(i);
    else if (cp.m_TangentModeLeft == ezCurveTangentMode::Linear)
      MakeLinearTangentLeft(i);
    else if (cp.m_TangentModeLeft == ezCurveTangentMode::Auto)
      MakeLinearTangentLeft(i); // note: last point will always be linear in auto mode
  }
}

void ezCurve1D::MakeFixedLengthTangentLeft(ezUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];

  const double lengthL = (pCP.m_Position.x - tCP.m_Position.x) * 0.3333333333;

  if (lengthL >= -0.0000001)
  {
    tCP.m_LeftTangent.SetZero();
  }
  else
  {
    const double tLen = ezMath::Min((double)tCP.m_LeftTangent.x, -0.001);

    const double fNormL = lengthL / tLen;
    tCP.m_LeftTangent.x = (float)lengthL;
    tCP.m_LeftTangent.y *= (float)fNormL;
  }
}

void ezCurve1D::MakeFixedLengthTangentRight(ezUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const double lengthR = (nCP.m_Position.x - tCP.m_Position.x) * 0.3333333333;

  if (lengthR <= 0.0000001)
  {
    tCP.m_RightTangent.SetZero();
  }
  else
  {
    const double tLen = ezMath::Max((double)tCP.m_RightTangent.x, 0.001);

    const double fNormR = lengthR / tLen;
    tCP.m_RightTangent.x = (float)lengthR;
    tCP.m_RightTangent.y *= (float)fNormR;
  }
}

void ezCurve1D::MakeLinearTangentLeft(ezUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];

  const ezVec2d tangent = (pCP.m_Position - tCP.m_Position) * 0.3333333333;
  tCP.m_LeftTangent.Set((float)tangent.x, (float)tangent.y);
}

void ezCurve1D::MakeLinearTangentRight(ezUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const ezVec2d tangent = (nCP.m_Position - tCP.m_Position) * 0.3333333333;
  tCP.m_RightTangent.Set((float)tangent.x, (float)tangent.y);
}

void ezCurve1D::MakeAutoTangentLeft(ezUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const double len = (nCP.m_Position.x - pCP.m_Position.x);
  if (len <= 0)
    return;

  const double fLerpFactor = (tCP.m_Position.x - pCP.m_Position.x) / len;

  const ezVec2d dirP = (tCP.m_Position - pCP.m_Position) * 0.3333333333;
  const ezVec2d dirN = (nCP.m_Position - tCP.m_Position) * 0.3333333333;

  const ezVec2d tangent = ezMath::Lerp(dirP, dirN, fLerpFactor);

  tCP.m_LeftTangent.Set(-(float)tangent.x, -(float)tangent.y);
}

void ezCurve1D::MakeAutoTangentRight(ezUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const double len = (nCP.m_Position.x - pCP.m_Position.x);
  if (len <= 0)
    return;

  const double fLerpFactor = (tCP.m_Position.x - pCP.m_Position.x) / (nCP.m_Position.x - pCP.m_Position.x);

  const ezVec2d dirP = (tCP.m_Position - pCP.m_Position) * 0.3333333333;
  const ezVec2d dirN = (nCP.m_Position - tCP.m_Position) * 0.3333333333;

  const ezVec2d tangent = ezMath::Lerp(dirP, dirN, fLerpFactor);

  tCP.m_RightTangent.Set((float)tangent.x, (float)tangent.y);
}

EZ_STATICLINK_FILE(Foundation, Foundation_Tracks_Implementation_Curve1D);

