#include <Foundation/PCH.h>
#include <Foundation/Math/Curve1D.h>
#include <Foundation/IO/Stream.h>

ezCurve1D::ezCurve1D()
{
  Clear();
}

void ezCurve1D::Clear()
{
  m_bRecomputeBBox = true;
  m_ControlPoints.Clear();
}

bool ezCurve1D::IsEmpty() const
{
  return m_ControlPoints.IsEmpty();
}

ezCurve1D::ControlPoint& ezCurve1D::AddControlPoint(float x)
{
  m_bRecomputeBBox = true;

  auto& cp = m_ControlPoints.ExpandAndGetRef();
  cp.m_fPosX = x;
  cp.m_fValue = 0;
  cp.m_LeftTangent.x = -0.1f;
  cp.m_LeftTangent.y = 0.0f;
  cp.m_RightTangent.x = +0.1f;
  cp.m_RightTangent.y = 0.0f;

  return cp;
}

void ezCurve1D::GetExtents(float& minx, float& maxx) const
{
  if (m_bRecomputeBBox)
    RecomputeBBox();

  minx = m_fMinX;
  maxx = m_fMaxX;
}


void ezCurve1D::GetExtremeValues(float& minVal, float& maxVal) const
{
  if (m_bRecomputeBBox)
    RecomputeBBox();

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
  RecomputeBBox();
}

float ezCurve1D::Evaluate(float x) const
{
  if (m_ControlPoints.GetCount() == 1)
  {
    return m_ControlPoints[0].m_fValue;
  }
  else if (m_ControlPoints.GetCount() >= 2)
  {
    ezInt32 iControlPoint = -1;

    const ezUInt32 numCPs = m_ControlPoints.GetCount();
    for (ezUInt32 i = 0; i < numCPs; ++i)
    {
      if (m_ControlPoints[i].m_fPosX >= x)
        break;

      iControlPoint = i;
    }

    if (iControlPoint == -1)
    {
      // clamp to left value
      return m_ControlPoints[0].m_fValue;
    }
    else if (iControlPoint == numCPs - 1)
    {
      // clamp to right value
      return m_ControlPoints[numCPs - 1].m_fValue;
    }
    else
    {
      const float v1 = m_ControlPoints[iControlPoint].m_fValue;
      const float v2 = m_ControlPoints[iControlPoint + 1].m_fValue;

      /// \todo Do spline interpolation using the tangents

      // interpolate (linear for now)
      float lerpX = x - m_ControlPoints[iControlPoint].m_fPosX;
      lerpX /= (m_ControlPoints[iControlPoint + 1].m_fPosX - m_ControlPoints[iControlPoint].m_fPosX);

      return ezMath::Lerp(v1, v2, lerpX);
    }
  }

  return 0.0f;
}

float ezCurve1D::ConvertNormalizedPos(float pos) const
{
  float fMin, fMax;
  GetExtents(fMin, fMax);

  return ezMath::Lerp(fMin, fMax, pos);
}


float ezCurve1D::NormalizeValue(float value) const
{
  float fMin, fMax;
  GetExtremeValues(fMin, fMax);

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
  const ezUInt8 uiVersion = 1;

  stream << uiVersion;

  const ezUInt32 numCp = m_ControlPoints.GetCount();

  stream << numCp;

  for (const auto& cp : m_ControlPoints)
  {
    stream << cp.m_fPosX;
    stream << cp.m_fValue;
  }
}

void ezCurve1D::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;

  stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion == 1, "Incorrect version '%u' for ezCurve1D", uiVersion);

  ezUInt32 numCp = 0;

  stream >> numCp;

  m_ControlPoints.SetCountUninitialized(numCp);

  for (auto& cp : m_ControlPoints)
  {
    stream >> cp.m_fPosX;
    stream >> cp.m_fValue;
  }

  m_bRecomputeBBox = true;
  RecomputeBBox();
}

void ezCurve1D::RecomputeBBox() const
{
  if (!m_bRecomputeBBox)
    return;

  m_bRecomputeBBox = false;

  m_fMinX = ezMath::BasicType<float>::MaxValue();
  m_fMaxX = -ezMath::BasicType<float>::MaxValue();

  for (const auto& cp : m_ControlPoints)
  {
    m_fMinX = ezMath::Min(m_fMinX, cp.m_fPosX);
    m_fMaxX = ezMath::Max(m_fMaxX, cp.m_fPosX);
  }


  /// \todo For splines we would need to compute the actually possible values using the tangents

  m_fMinY = ezMath::BasicType<float>::MaxValue();
  m_fMaxY = -ezMath::BasicType<float>::MaxValue();

  for (const auto& cp : m_ControlPoints)
  {
    m_fMinY = ezMath::Min(m_fMinY, cp.m_fValue);
    m_fMaxY = ezMath::Max(m_fMaxY, cp.m_fValue);
  }

}
