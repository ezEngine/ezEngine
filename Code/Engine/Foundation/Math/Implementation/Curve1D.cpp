#include <Foundation/PCH.h>
#include <Foundation/Math/Curve1D.h>
#include <Foundation/IO/Stream.h>

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

ezCurve1D::ControlPoint& ezCurve1D::AddControlPoint(float x)
{
  auto& cp = m_ControlPoints.ExpandAndGetRef();
  cp.m_Position.x = x;
  cp.m_Position.y = 0;
  cp.m_LeftTangent.x = -0.1f;
  cp.m_LeftTangent.y = 0.0f;
  cp.m_RightTangent.x = +0.1f;
  cp.m_RightTangent.y = 0.0f;

  return cp;
}

void ezCurve1D::QueryExtents(float& minx, float& maxx) const
{
  minx = m_fMinX;
  maxx = m_fMaxX;
}

void ezCurve1D::QueryExtremeValues(float& minVal, float& maxVal) const
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


ezInt32 ezCurve1D::FindApproxControlPoint(float x) const
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

float ezCurve1D::Evaluate(float x) const
{
  EZ_ASSERT_DEBUG(!m_LinearApproximation.IsEmpty(), "Cannot evaluate curve without precomputing curve approximation data first. Call CreateLinearApproximation() on curve before calling Evaluate().");

  if (m_LinearApproximation.GetCount() == 1)
  {
    return m_LinearApproximation[0].y;
  }
  else if (m_LinearApproximation.GetCount() >= 2)
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
      const float v1 = m_LinearApproximation[iControlPoint].y;
      const float v2 = m_LinearApproximation[iControlPoint + 1].y;

      // interpolate
      float lerpX = x - m_LinearApproximation[iControlPoint].x;
      lerpX /= (m_LinearApproximation[iControlPoint + 1].x - m_LinearApproximation[iControlPoint].x);

      return ezMath::Lerp(v1, v2, lerpX);
    }
  }

  return 0.0f;
}

float ezCurve1D::ConvertNormalizedPos(float pos) const
{
  float fMin, fMax;
  QueryExtents(fMin, fMax);

  return ezMath::Lerp(fMin, fMax, pos);
}


float ezCurve1D::NormalizeValue(float value) const
{
  float fMin, fMax;
  QueryExtremeValues(fMin, fMax);

  if (fMin >= fMax)
    return 0.0f;

  return (value - fMin) / (fMax - fMin);
}

ezUInt64 ezCurve1D::GetHeapMemoryUsage() const
{
  return m_ControlPoints.GetHeapMemoryUsage();
}

void ezCurve1D::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 2;

  stream << uiVersion;

  const ezUInt32 numCp = m_ControlPoints.GetCount();

  stream << numCp;

  for (const auto& cp : m_ControlPoints)
  {
    stream << cp.m_Position;
    stream << cp.m_LeftTangent;
    stream << cp.m_RightTangent;
  }
}

void ezCurve1D::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;

  stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion <= 2, "Incorrect version '{0}' for ezCurve1D", uiVersion);

  ezUInt32 numCp = 0;

  stream >> numCp;

  m_ControlPoints.SetCountUninitialized(numCp);

  for (auto& cp : m_ControlPoints)
  {
    stream >> cp.m_Position;

    if (uiVersion >= 2)
    {
      stream >> cp.m_LeftTangent;
      stream >> cp.m_RightTangent;
    }
  }
}

void ezCurve1D::CreateLinearApproximation(float fMaxError /*= 0.01f*/)
{
  ClampTangents();

  m_LinearApproximation.Clear();

  if (m_ControlPoints.IsEmpty())
  {
    m_LinearApproximation.PushBack(ezVec2::ZeroVector());
    return;
  }

  for (ezUInt32 i = 1; i < m_ControlPoints.GetCount(); ++i)
  {
    m_LinearApproximation.PushBack(m_ControlPoints[i - 1].m_Position);

    ApproximateCurve(m_ControlPoints[i - 1].m_Position, 
                     m_ControlPoints[i - 1].m_Position + m_ControlPoints[i - 1].m_RightTangent, 
                     m_ControlPoints[i].m_Position + m_ControlPoints[i].m_LeftTangent, 
                     m_ControlPoints[i].m_Position,
                     fMaxError * fMaxError);
  }

  m_LinearApproximation.PushBack(m_ControlPoints.PeekBack().m_Position);

  RecomputeExtremes();
}

void ezCurve1D::RecomputeExtents()
{
  m_fMinX = ezMath::BasicType<float>::MaxValue();
  m_fMaxX = -ezMath::BasicType<float>::MaxValue();

  for (const auto& cp : m_ControlPoints)
  {
    m_fMinX = ezMath::Min(m_fMinX, cp.m_Position.x);
    m_fMaxX = ezMath::Max(m_fMaxX, cp.m_Position.x);

    // ignore X values that could go outside the control point range due to Bezier curve interpolation
    // we just assume the curve is always restricted along X by the CPs

    //m_fMinX = ezMath::Min(m_fMinX, cp.m_Position.x + cp.m_LeftTangent.x);
    //m_fMaxX = ezMath::Max(m_fMaxX, cp.m_Position.x + cp.m_LeftTangent.x);

    //m_fMinX = ezMath::Min(m_fMinX, cp.m_Position.x + cp.m_RightTangent.x);
    //m_fMaxX = ezMath::Max(m_fMaxX, cp.m_Position.x + cp.m_RightTangent.x);
  }
}


void ezCurve1D::RecomputeExtremes()
{
  m_fMinY = ezMath::BasicType<float>::MaxValue();
  m_fMaxY = -ezMath::BasicType<float>::MaxValue();

  for (const auto& cp : m_LinearApproximation)
  {
    m_fMinY = ezMath::Min(m_fMinY, cp.y);
    m_fMaxY = ezMath::Max(m_fMaxY, cp.y);
  }
}

void ezCurve1D::ApproximateCurve(const ezVec2& p0, const ezVec2& p1, const ezVec2& p2, const ezVec2& p3, float fMaxErrorSQR)
{
  const ezVec2 cubicCenter = ezMath::EvaluateBezierCurve(0.5f, p0, p1, p2, p3);
  //const ezVec2 linearCenter = ezMath::Lerp(p0, p3, 0.5f);

  ApproximateCurvePiece(p0, p1, p2, p3, 0.0f, p0, 0.5f, cubicCenter, fMaxErrorSQR);

  // always insert the center point
  // with an S curve the cubicCenter and the linearCenter can be identical even though the rest of the curve is absolutely not linear
  m_LinearApproximation.PushBack(cubicCenter);

  ApproximateCurvePiece(p0, p1, p2, p3, 0.5f, cubicCenter, 1.0f, p3, fMaxErrorSQR);

}

void ezCurve1D::ApproximateCurvePiece(const ezVec2& p0, const ezVec2& p1, const ezVec2& p2, const ezVec2& p3, float tLeft, const ezVec2& pLeft, float tRight, const ezVec2& pRight, float fMaxErrorSQR)
{
  const float tCenter = ezMath::Lerp(tLeft, tRight, 0.5f);

  const ezVec2 cubicCenter = ezMath::EvaluateBezierCurve(tCenter, p0, p1, p2, p3);
  const ezVec2 linearCenter = ezMath::Lerp(pLeft, pRight, 0.5f);

  // check whether the linear interpolation between pLeft and pRight would already result in a good enough approximation
  // if not, subdivide the curve further

  if ((cubicCenter - linearCenter).GetLengthSquared() < fMaxErrorSQR)
    return;

  ApproximateCurvePiece(p0, p1, p2, p3, tLeft, pLeft, tCenter, cubicCenter, fMaxErrorSQR);

  m_LinearApproximation.PushBack(cubicCenter);

  ApproximateCurvePiece(p0, p1, p2, p3, tCenter, cubicCenter, tRight, pRight, fMaxErrorSQR);
}

void ezCurve1D::ClampTangents()
{
  for (ezUInt32 i = 1; i < m_ControlPoints.GetCount() - 1; ++i)
  {
    auto& tCP = m_ControlPoints[i];
    const auto& pCP = m_ControlPoints[i - 1];
    const auto& nCP = m_ControlPoints[i + 1];

    ezVec2 lpt = tCP.m_Position + tCP.m_LeftTangent;
    ezVec2 rpt = tCP.m_Position + tCP.m_RightTangent;

    lpt.x = ezMath::Clamp(lpt.x, pCP.m_Position.x, tCP.m_Position.x);
    rpt.x = ezMath::Clamp(rpt.x, tCP.m_Position.x, nCP.m_Position.x);

    tCP.m_LeftTangent = lpt - tCP.m_Position;
    tCP.m_RightTangent = rpt - tCP.m_Position;
  }

  // first CP
  {
    auto& tCP = m_ControlPoints[0];
    const auto& nCP = m_ControlPoints[1];

    ezVec2 rpt = tCP.m_Position + tCP.m_RightTangent;
    rpt.x = ezMath::Clamp(rpt.x, tCP.m_Position.x, nCP.m_Position.x);

    tCP.m_RightTangent = rpt - tCP.m_Position;
  }

  // last CP
  {
    auto& tCP = m_ControlPoints[m_ControlPoints.GetCount() - 1];
    const auto& pCP = m_ControlPoints[m_ControlPoints.GetCount() - 2];

    ezVec2 lpt = tCP.m_Position + tCP.m_LeftTangent;
    lpt.x = ezMath::Clamp(lpt.x, pCP.m_Position.x, tCP.m_Position.x);

    tCP.m_LeftTangent = lpt - tCP.m_Position;
  }
}



EZ_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Curve1D);

