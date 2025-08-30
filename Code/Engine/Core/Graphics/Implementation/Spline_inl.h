
EZ_ALWAYS_INLINE void ezSpline::ControlPoint::SetPosition(const ezSimdVec4f& vPos)
{
  m_vPos = vPos;
}

EZ_ALWAYS_INLINE ezSplineTangentMode::Enum ezSpline::ControlPoint::GetTangentModeIn() const
{
  float w = m_vPosTangentIn.w();
  return static_cast<ezSplineTangentMode::Enum>(w);
}

EZ_ALWAYS_INLINE void ezSpline::ControlPoint::SetTangentModeIn(ezSplineTangentMode::Enum mode)
{
  float w = static_cast<float>(mode);
  m_vPosTangentIn.SetW(w);
}

EZ_ALWAYS_INLINE void ezSpline::ControlPoint::SetTangentIn(const ezSimdVec4f& vTangent, ezSplineTangentMode::Enum mode /*= ezSplineTangentMode::Custom*/)
{
  m_vPosTangentIn = vTangent;
  SetTangentModeIn(mode);
}

EZ_ALWAYS_INLINE ezSplineTangentMode::Enum ezSpline::ControlPoint::GetTangentModeOut() const
{
  float w = m_vPosTangentOut.w();
  return static_cast<ezSplineTangentMode::Enum>(w);
}

EZ_ALWAYS_INLINE void ezSpline::ControlPoint::SetTangentModeOut(ezSplineTangentMode::Enum mode)
{
  float w = static_cast<float>(mode);
  m_vPosTangentOut.SetW(w);
}

EZ_ALWAYS_INLINE void ezSpline::ControlPoint::SetTangentOut(const ezSimdVec4f& vTangent, ezSplineTangentMode::Enum mode /*= ezSplineTangentMode::Custom*/)
{
  m_vPosTangentOut = vTangent;
  SetTangentModeOut(mode);
}

EZ_ALWAYS_INLINE ezAngle ezSpline::ControlPoint::GetRoll() const
{
  return ezAngle::MakeFromRadian(m_vUpDirAndRoll.w());
}

EZ_ALWAYS_INLINE void ezSpline::ControlPoint::SetRoll(ezAngle roll)
{
  m_vUpDirAndRoll.SetW(roll);
}

EZ_ALWAYS_INLINE void ezSpline::ControlPoint::SetScale(const ezSimdVec4f& vScale)
{
  m_vScale = vScale;
  m_vScaleTangentIn.SetZero();
  m_vScaleTangentOut.SetZero();
}

//////////////////////////////////////////////////////////////////////////

EZ_FORCE_INLINE ezSimdVec4f ezSpline::EvaluatePosition(float fT) const
{
  ezUInt32 uiCp0;
  fT = ClampAndSplitT(fT, uiCp0);

  return EvaluatePosition(uiCp0, fT);
}

EZ_FORCE_INLINE ezSimdVec4f ezSpline::EvaluatePosition(ezUInt32 uiCp0, const ezSimdFloat& fT) const
{
  if (m_ControlPoints.IsEmpty())
    return ezSimdVec4f::MakeZero();

  const ezUInt32 uiCp1 = GetCp1Index(uiCp0);

  return EvaluatePosition(m_ControlPoints[uiCp0], m_ControlPoints[uiCp1], fT);
}

EZ_FORCE_INLINE ezSimdVec4f ezSpline::EvaluateDerivative(float fT) const
{
  ezUInt32 uiCp0;
  fT = ClampAndSplitT(fT, uiCp0);

  return EvaluateDerivative(uiCp0, fT);
}

EZ_FORCE_INLINE ezSimdVec4f ezSpline::EvaluateDerivative(ezUInt32 uiCp0, const ezSimdFloat& fT) const
{
  if (m_ControlPoints.IsEmpty())
    return ezSimdVec4f::MakeZero();

  const ezUInt32 uiCp1 = GetCp1Index(uiCp0);

  return EvaluateDerivative(m_ControlPoints[uiCp0], m_ControlPoints[uiCp1], fT);
}

EZ_FORCE_INLINE ezSimdVec4f ezSpline::EvaluateUpDirection(float fT) const
{
  if (m_ControlPoints.IsEmpty())
    return ezSimdVec4f::MakeZero();

  ezUInt32 uiCp0;
  fT = ClampAndSplitT(fT, uiCp0);

  const ezUInt32 uiCp1 = GetCp1Index(uiCp0);

  ezSimdVec4f forwardDir, rightDir, upDir;
  EvaluateRotation(m_ControlPoints[uiCp0], m_ControlPoints[uiCp1], fT, forwardDir, rightDir, upDir);

  return upDir;
}

EZ_FORCE_INLINE ezSimdVec4f ezSpline::EvaluateScale(float fT) const
{
  if (m_ControlPoints.IsEmpty())
    return ezSimdVec4f::MakeZero();

  ezUInt32 uiCp0;
  fT = ClampAndSplitT(fT, uiCp0);

  const ezUInt32 uiCp1 = GetCp1Index(uiCp0);

  return EvaluateScale(m_ControlPoints[uiCp0], m_ControlPoints[uiCp1], fT);
}

EZ_ALWAYS_INLINE float ezSpline::ClampAndSplitT(float fT, ezUInt32& out_uiIndex) const
{
  float fNumPoints = static_cast<float>(m_ControlPoints.GetCount());
  if (m_bClosed)
  {
    fT = (fT < 0.0f || fT >= fNumPoints) ? 0.0f : fT;
  }
  else
  {
    fT = ezMath::Clamp(fT, 0.0f, fNumPoints - 1.0f);
  }

  const float fIndex = ezMath::Floor(fT);
  out_uiIndex = static_cast<ezUInt32>(fIndex);
  return fT - fIndex;
}

EZ_ALWAYS_INLINE ezUInt32 ezSpline::GetCp1Index(ezUInt32 uiCp0) const
{
  return (uiCp0 + 1 < m_ControlPoints.GetCount()) ? uiCp0 + 1 : 0;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSpline::EvaluatePosition(const ControlPoint& cp0, const ControlPoint& cp1, const ezSimdFloat& fT) const
{
  return ezMath::EvaluateBezierCurve(fT, cp0.m_vPos, cp0.m_vPos + cp0.m_vPosTangentOut, cp1.m_vPos + cp1.m_vPosTangentIn, cp1.m_vPos);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSpline::EvaluateDerivative(const ControlPoint& cp0, const ControlPoint& cp1, const ezSimdFloat& fT) const
{
  return ezMath::EvaluateBezierCurveDerivative(fT, cp0.m_vPos, cp0.m_vPos + cp0.m_vPosTangentOut, cp1.m_vPos + cp1.m_vPosTangentIn, cp1.m_vPos);
}

EZ_ALWAYS_INLINE void ezSpline::EvaluateRotation(const ControlPoint& cp0, const ControlPoint& cp1, const ezSimdFloat& fT, ezSimdVec4f& out_forwardDir, ezSimdVec4f& out_rightDir, ezSimdVec4f& out_upDir) const
{
  ezSimdVec4f upDir = ezMath::EvaluateBezierCurve(fT, cp0.m_vUpDirAndRoll, cp0.m_vUpDirAndRoll + cp0.m_vUpDirTangentOut, cp1.m_vUpDirAndRoll + cp1.m_vUpDirTangentIn, cp1.m_vUpDirAndRoll);

  out_forwardDir = EvaluateDerivative(cp0, cp1, fT);
  out_forwardDir.NormalizeIfNotZero<3>(ezSimdVec4f(1, 0, 0));

  out_rightDir = upDir.CrossRH(out_forwardDir).GetNormalized<3>();
  out_upDir = out_forwardDir.CrossRH(out_rightDir).GetNormalized<3>();
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSpline::EvaluateScale(const ControlPoint& cp0, const ControlPoint& cp1, const ezSimdFloat& fT) const
{
  return ezMath::EvaluateBezierCurve(fT, cp0.m_vScale, cp0.m_vScale + cp0.m_vScaleTangentOut, cp1.m_vScale + cp1.m_vScaleTangentIn, cp1.m_vScale);
}
