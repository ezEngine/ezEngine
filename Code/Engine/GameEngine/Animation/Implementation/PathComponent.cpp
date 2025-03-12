#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/PathComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

// TODO PathComponent:
// tangent mode for each node (auto, linear, Bezier)
// linked tangents on/off
// editing tangents (in a plane, needs new manipulator)


// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezPathComponentFlags, 1)
  EZ_BITFLAGS_CONSTANTS(ezPathComponentFlags::VisualizePath, ezPathComponentFlags::VisualizeUpDir)
EZ_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgPathChanged);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgPathChanged, 1, ezRTTIDefaultAllocator<ezMsgPathChanged>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezPathComponent::ControlPoint::Serialize(ezStreamWriter& s) const
{
  s << m_vPosition;
  s << m_vTangentIn;
  s << m_vTangentOut;
  s << m_Roll;

  return EZ_SUCCESS;
}

ezResult ezPathComponent::ControlPoint::Deserialize(ezStreamReader& s)
{
  s >> m_vPosition;
  s >> m_vTangentIn;
  s >> m_vTangentOut;
  s >> m_Roll;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPathComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_BITFLAGS_ACCESSOR_PROPERTY("Flags", ezPathComponentFlags, GetPathFlags, SetPathFlags)->AddAttributes(new ezDefaultValueAttribute(ezPathComponentFlags::VisualizePath)),
    EZ_ACCESSOR_PROPERTY("Closed", GetClosed,SetClosed),
    EZ_ACCESSOR_PROPERTY("Detail", GetLinearizationError, SetLinearizationError)->AddAttributes(new ezDefaultValueAttribute(0.01f), new ezClampValueAttribute(0.001f, 1.0f)),
    EZ_ARRAY_ACCESSOR_PROPERTY("Nodes", Nodes_GetCount, Nodes_GetNode, Nodes_SetNode, Nodes_Insert, Nodes_Remove),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgPathChanged, OnMsgPathChanged),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation/Paths"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezPathComponent::ezPathComponent() = default;
ezPathComponent::~ezPathComponent() = default;

void ezPathComponent::SerializeComponent(ezWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();
  s << m_PathFlags;
  s << m_bClosed;
  s << m_fLinearizationError;

  if (m_bControlPointsChanged && !m_bDisableControlPointUpdates)
  {
    ezDynamicArray<ControlPoint> controlPoints;
    FindControlPoints(controlPoints);
    ref_stream.GetStream().WriteArray(controlPoints).AssertSuccess();
  }
  else
  {
    ref_stream.GetStream().WriteArray(m_ControlPointRepresentation).AssertSuccess();
  }
}

void ezPathComponent::DeserializeComponent(ezWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();
  s >> m_PathFlags;
  s >> m_bClosed;
  s >> m_fLinearizationError;

  ref_stream.GetStream().ReadArray(m_ControlPointRepresentation).AssertSuccess();

  m_bDisableControlPointUpdates = true;
  m_bControlPointsChanged = false;
  m_bLinearizedRepresentationChanged = true;
}

void ezPathComponent::SetClosed(bool bClosed)
{
  if (m_bClosed == bClosed)
    return;

  m_bClosed = bClosed;
  m_bControlPointsChanged = true;
  m_bLinearizedRepresentationChanged = true;
}

void ezPathComponent::SetPathFlags(ezBitflags<ezPathComponentFlags> flags)
{
  if (m_PathFlags == flags)
    return;

  m_PathFlags = flags;

  if (IsActiveAndInitialized())
  {
    if (m_PathFlags.IsNoFlagSet())
      static_cast<ezPathComponentManager*>(GetOwningManager())->SetEnableUpdate(this, false);
    else
      static_cast<ezPathComponentManager*>(GetOwningManager())->SetEnableUpdate(this, true);
  }
}

void ezPathComponent::Nodes_SetNode(ezUInt32 i, const ezString& node)
{
  m_Nodes[i] = node;
  m_bControlPointsChanged = true;
  m_bLinearizedRepresentationChanged = true;
}

void ezPathComponent::Nodes_Insert(ezUInt32 uiIndex, const ezString& node)
{
  m_Nodes.InsertAt(uiIndex, node);
  m_bControlPointsChanged = true;
  m_bLinearizedRepresentationChanged = true;
}

void ezPathComponent::Nodes_Remove(ezUInt32 uiIndex)
{
  m_Nodes.RemoveAtAndCopy(uiIndex);
  m_bControlPointsChanged = true;
  m_bLinearizedRepresentationChanged = true;
}

void ezPathComponent::FindControlPoints(ezDynamicArray<ControlPoint>& out_ControlPoints) const
{
  auto& points = out_ControlPoints;

  points.Clear();

  if (m_Nodes.GetCount() <= 1)
    return;

  ezGameObject* pOwner = const_cast<ezGameObject*>(GetOwner());
  const ezTransform invTrans = pOwner->GetGlobalTransform().GetInverse();

  ezHybridArray<ezPathNodeTangentMode::StorageType, 64> tangentsIn;
  ezHybridArray<ezPathNodeTangentMode::StorageType, 64> tangentsOut;

  for (const ezString& sNode : m_Nodes)
  {
    const ezGameObject* pNodeObj = GetWorld()->SearchForObject(sNode, GetOwner(), ezGetStaticRTTI<ezPathNodeComponent>());
    if (pNodeObj == nullptr)
      continue;

    const ezPathNodeComponent* pNodeComp;
    if (!pNodeObj->TryGetComponentOfBaseType(pNodeComp))
      continue;

    auto& cp = points.ExpandAndGetRef();
    cp.m_vPosition = invTrans * pNodeObj->GetGlobalPosition();
    cp.m_Roll = pNodeComp->GetRoll();

    tangentsOut.PushBack(pNodeComp->GetTangentMode1().GetValue());
    tangentsIn.PushBack(pNodeComp->GetTangentMode2().GetValue());
  }

  const ezUInt32 uiNumPoints = points.GetCount();
  const ezUInt32 uiLastIdx = points.GetCount() - 1;

  if (uiNumPoints <= 1)
  {
    points.Clear();
    return;
  }

  ezUInt32 uiNumTangentsToUpdate = uiNumPoints;
  ezUInt32 uiPrevIdx = uiLastIdx - 1;
  ezUInt32 uiCurIdx = uiLastIdx;
  ezUInt32 uiNextIdx = 0;

  if (!m_bClosed)
  {
    const ezVec3 vStartTangent = (points[1].m_vPosition - points[0].m_vPosition) * 0.3333333333f;
    const ezVec3 vEndTangent = (points[uiLastIdx].m_vPosition - points[uiLastIdx - 1].m_vPosition) * 0.3333333333f;

    points[0].m_vTangentIn = vStartTangent;
    points[0].m_vTangentOut = -vStartTangent;

    points[uiLastIdx].m_vTangentIn = vEndTangent;
    points[uiLastIdx].m_vTangentOut = -vEndTangent;

    uiNumTangentsToUpdate = uiNumPoints - 2;
    uiPrevIdx = 0;
    uiCurIdx = 1;
    uiNextIdx = 2;
  }

  for (ezUInt32 i = 0; i < uiNumTangentsToUpdate; ++i)
  {
    auto& tCP = points[uiCurIdx];
    const auto& pCP = points[uiPrevIdx];
    const auto& nCP = points[uiNextIdx];

    const float fLength = ezMath::Max(0.001f, (nCP.m_vPosition - pCP.m_vPosition).GetLength());
    const float fLerpFactor = ezMath::Min(1.0f, (tCP.m_vPosition - pCP.m_vPosition).GetLength() / fLength);

    const ezVec3 dirP = (tCP.m_vPosition - pCP.m_vPosition) * 0.3333333333f;
    const ezVec3 dirN = (nCP.m_vPosition - tCP.m_vPosition) * 0.3333333333f;

    const ezVec3 tangent = ezMath::Lerp(dirP, dirN, fLerpFactor);

    switch (tangentsIn[uiCurIdx])
    {
      case ezPathNodeTangentMode::Auto:
        tCP.m_vTangentIn = tangent;
        break;
      case ezPathNodeTangentMode::Linear:
        tCP.m_vTangentIn = dirN;
        break;
    }

    switch (tangentsOut[uiCurIdx])
    {
      case ezPathNodeTangentMode::Auto:
        tCP.m_vTangentOut = -tangent;
        break;
      case ezPathNodeTangentMode::Linear:
        tCP.m_vTangentOut = -dirP;
        break;
    }

    uiPrevIdx = uiCurIdx;
    uiCurIdx = uiNextIdx;
    ++uiNextIdx;
  }
}

void ezPathComponent::EnsureControlPointRepresentationIsUpToDate()
{
  if (!m_bControlPointsChanged || m_bDisableControlPointUpdates)
    return;

  m_ControlPointRepresentation.Clear();

  if (!IsActive())
    return;

  FindControlPoints(m_ControlPointRepresentation);
  m_bControlPointsChanged = false;
}

void ezPathComponent::EnsureLinearizedRepresentationIsUpToDate()
{
  if (!m_bLinearizedRepresentationChanged)
    return;

  m_LinearizedRepresentation.Clear();

  if (!IsActive())
    return;

  EnsureControlPointRepresentationIsUpToDate();

  CreateLinearizedPathRepresentation(m_ControlPointRepresentation);
  m_bLinearizedRepresentationChanged = false;
}

void ezPathComponent::OnMsgPathChanged(ezMsgPathChanged& ref_msg)
{
  m_bControlPointsChanged = true;
  m_bLinearizedRepresentationChanged = true;
}

void ezPathComponent::DrawDebugVisualizations()
{
  if (m_PathFlags.AreNoneSet(ezPathComponentFlags::VisualizePath | ezPathComponentFlags::VisualizeUpDir))
    return;

  const bool bVisPath = m_PathFlags.IsSet(ezPathComponentFlags::VisualizePath);
  const bool bVisUp = m_PathFlags.IsSet(ezPathComponentFlags::VisualizeUpDir);

  EnsureLinearizedRepresentationIsUpToDate();

  if (m_LinearizedRepresentation.IsEmpty())
    return;

  ezHybridArray<ezDebugRendererLine, 32> lines;

  ezUInt32 uiPrev = 0;
  ezUInt32 uiNext = 1;

  for (; uiNext < m_LinearizedRepresentation.GetCount(); ++uiNext)
  {
    const auto& n0 = m_LinearizedRepresentation[uiPrev];
    const auto& n1 = m_LinearizedRepresentation[uiNext];

    if (bVisPath)
    {
      auto& line = lines.ExpandAndGetRef();
      line.m_start = n0.m_vPosition;
      line.m_end = n1.m_vPosition;
      line.m_startColor = ezColor::DarkRed;
      line.m_endColor = ezColor::DarkRed;
    }

    if (bVisUp)
    {
      auto& line = lines.ExpandAndGetRef();
      line.m_start = n0.m_vPosition;
      line.m_end = n0.m_vPosition + n0.m_vUpDirection * 0.25f;
      line.m_startColor = ezColor::Black;
      line.m_endColor = ezColor::LightBlue;
    }

    uiPrev = uiNext;
  }

  ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::White, GetOwner()->GetGlobalTransform());
}

void ezPathComponent::OnActivated()
{
  SUPER::OnActivated();

  if (m_PathFlags.IsAnyFlagSet())
  {
    static_cast<ezPathComponentManager*>(GetOwningManager())->SetEnableUpdate(this, true);
  }
}

void ezPathComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  if (m_PathFlags.IsAnyFlagSet())
  {
    // assume that if no flag is set, update is already disabled
    static_cast<ezPathComponentManager*>(GetOwningManager())->SetEnableUpdate(this, false);
  }
}

void ezPathComponent::LinearSampler::SetToStart()
{
  m_fSegmentFraction = 0.0f;
  m_uiSegmentNode = 0;
}

void ezPathComponent::SetLinearSamplerTo(LinearSampler& ref_sampler, float fDistance) const
{
  if (fDistance < 0.0f && m_LinearizedRepresentation.GetCount() >= 2)
  {
    ref_sampler.m_uiSegmentNode = m_LinearizedRepresentation.GetCount() - 1;
    ref_sampler.m_fSegmentFraction = 1.0f;
  }
  else
  {
    ref_sampler.m_uiSegmentNode = 0;
    ref_sampler.m_fSegmentFraction = 0.0f;
  }

  AdvanceLinearSamplerBy(ref_sampler, fDistance);
}

bool ezPathComponent::AdvanceLinearSamplerBy(LinearSampler& ref_sampler, float& inout_fAddDistance) const
{
  if (inout_fAddDistance == 0.0f || m_LinearizedRepresentation.IsEmpty())
  {
    inout_fAddDistance = 0.0f;
    return false;
  }

  if (m_LinearizedRepresentation.GetCount() == 1)
  {
    ref_sampler.SetToStart();
    inout_fAddDistance = 0.0f;
    return false;
  }

  if (inout_fAddDistance >= 0)
  {
    for (ezUInt32 i = ref_sampler.m_uiSegmentNode + 1; i < m_LinearizedRepresentation.GetCount(); ++i)
    {
      const auto& nd0 = m_LinearizedRepresentation[i - 1];
      const auto& nd1 = m_LinearizedRepresentation[i];

      const float fSegmentLength = (nd1.m_vPosition - nd0.m_vPosition).GetLength();
      const float fSegmentDistance = ref_sampler.m_fSegmentFraction * fSegmentLength;
      const float fRemainingSegmentDistance = fSegmentLength - fSegmentDistance;

      if (inout_fAddDistance >= fRemainingSegmentDistance)
      {
        inout_fAddDistance -= fRemainingSegmentDistance;
        ref_sampler.m_uiSegmentNode = i;
        ref_sampler.m_fSegmentFraction = 0.0f;
      }
      else
      {
        ref_sampler.m_fSegmentFraction = (fSegmentDistance + inout_fAddDistance) / fSegmentLength;
        return true;
      }
    }

    ref_sampler.m_uiSegmentNode = m_LinearizedRepresentation.GetCount() - 1;
    ref_sampler.m_fSegmentFraction = 1.0f;
    return false;
  }
  else
  {
    while (true)
    {
      ezUInt32 ic = ref_sampler.m_uiSegmentNode;
      ezUInt32 in = ezMath::Min(ref_sampler.m_uiSegmentNode + 1, m_LinearizedRepresentation.GetCount() - 1);

      const auto& nd0 = m_LinearizedRepresentation[ic];
      const auto& nd1 = m_LinearizedRepresentation[in];

      const float fSegmentLength = (nd1.m_vPosition - nd0.m_vPosition).GetLength();
      const float fSegmentDistance = ref_sampler.m_fSegmentFraction * fSegmentLength;
      const float fRemainingSegmentDistance = -fSegmentDistance;

      if (inout_fAddDistance <= fRemainingSegmentDistance)
      {
        inout_fAddDistance -= fRemainingSegmentDistance;

        if (ref_sampler.m_uiSegmentNode == 0)
        {
          ref_sampler.m_uiSegmentNode = 0;
          ref_sampler.m_fSegmentFraction = 0.0f;
          return false;
        }

        ref_sampler.m_uiSegmentNode--;
        ref_sampler.m_fSegmentFraction = 1.0f;
      }
      else
      {
        ref_sampler.m_fSegmentFraction = (fSegmentDistance + inout_fAddDistance) / fSegmentLength;
        return true;
      }
    }
  }
}

ezPathComponent::LinearizedElement ezPathComponent::SampleLinearizedRepresentation(const LinearSampler& sampler) const
{
  if (m_LinearizedRepresentation.IsEmpty())
    return {};

  if (sampler.m_uiSegmentNode + 1 >= m_LinearizedRepresentation.GetCount())
  {
    const ezUInt32 idx = m_LinearizedRepresentation.GetCount() - 1;

    return m_LinearizedRepresentation[idx];
  }

  const auto& nd0 = m_LinearizedRepresentation[sampler.m_uiSegmentNode];
  const auto& nd1 = m_LinearizedRepresentation[sampler.m_uiSegmentNode + 1];

  LinearizedElement res;
  res.m_vPosition = ezMath::Lerp(nd0.m_vPosition, nd1.m_vPosition, sampler.m_fSegmentFraction);
  res.m_vUpDirection = ezMath::Lerp(nd0.m_vUpDirection, nd1.m_vUpDirection, sampler.m_fSegmentFraction);

  return res;
}

void ezPathComponent::SetLinearizationError(float fError)
{
  if (m_fLinearizationError == fError)
    return;

  m_fLinearizationError = fError;
  m_bLinearizedRepresentationChanged = true;
}

static void ComputeCpDirs(const ezDynamicArray<ezPathComponent::ControlPoint>& points, bool bClosed, const ezCoordinateSystem& cs, ezDynamicArray<ezVec3>& inout_cpFwd, ezDynamicArray<ezVec3>& inout_cpUp)
{
  const ezUInt32 uiNumCPs = points.GetCount();
  inout_cpFwd.SetCount(uiNumCPs);
  inout_cpUp.SetCount(uiNumCPs);

  for (ezUInt32 uiCurPt = 0; uiCurPt < uiNumCPs; ++uiCurPt)
  {
    ezUInt32 uiPrevPt;
    ezUInt32 uiNextPt = uiCurPt + 1;

    if (bClosed)
    {
      if (uiCurPt == 0)
        uiPrevPt = uiNumCPs - 1;
      else
        uiPrevPt = uiCurPt - 1;

      uiNextPt %= uiNumCPs;
    }
    else
    {
      if (uiCurPt == 0)
        uiPrevPt = 0;
      else
        uiPrevPt = uiCurPt - 1;

      uiNextPt = ezMath::Min(uiCurPt + 1, uiNumCPs - 1);
    }

    const auto& cpP = points[uiPrevPt];
    const auto& cpC = points[uiCurPt];
    const auto& cpN = points[uiNextPt];

    const ezVec3 posPrev = ezMath::EvaluateBezierCurve(0.98f, cpP.m_vPosition, cpP.m_vPosition + cpP.m_vTangentIn, cpC.m_vPosition + cpC.m_vTangentOut, cpC.m_vPosition);
    const ezVec3 posNext = ezMath::EvaluateBezierCurve(0.02f, cpC.m_vPosition, cpC.m_vPosition + cpC.m_vTangentIn, cpN.m_vPosition + cpN.m_vTangentOut, cpN.m_vPosition);

    ezVec3 dirP = (posPrev - cpC.m_vPosition);
    ezVec3 dirN = (posNext - cpC.m_vPosition);
    dirP.NormalizeIfNotZero(ezVec3::MakeZero()).IgnoreResult();
    dirN.NormalizeIfNotZero(ezVec3::MakeZero()).IgnoreResult();

    ezVec3 dirAvg = dirP - dirN;
    dirAvg.NormalizeIfNotZero(cs.m_vForwardDir).IgnoreResult();

    ezVec3 dirUp = cs.m_vUpDir;
    dirUp.MakeOrthogonalTo(dirAvg);

    dirUp.NormalizeIfNotZero(cs.m_vUpDir).IgnoreResult();

    inout_cpFwd[uiCurPt] = dirAvg;
    inout_cpUp[uiCurPt] = dirUp;
  }
}

static double ComputePathLength(ezArrayPtr<ezPathComponent::LinearizedElement> points)
{
  double fLength = 0;
  for (ezUInt32 i = 1; i < points.GetCount(); ++i)
  {
    fLength += (points[i - 1].m_vPosition - points[i].m_vPosition).GetLength();
  }

  return fLength;
}

static ezVec3 ComputeTangentAt(float fT, const ezPathComponent::ControlPoint& cp0, const ezPathComponent::ControlPoint& cp1)
{
  const ezVec3 posPrev = ezMath::EvaluateBezierCurve(ezMath::Max(0.0f, fT - 0.02f), cp0.m_vPosition, cp0.m_vPosition + cp0.m_vTangentIn, cp1.m_vPosition + cp1.m_vTangentOut, cp1.m_vPosition);
  const ezVec3 posNext = ezMath::EvaluateBezierCurve(ezMath::Min(1.0f, fT + 0.02f), cp0.m_vPosition, cp0.m_vPosition + cp0.m_vTangentIn, cp1.m_vPosition + cp1.m_vTangentOut, cp1.m_vPosition);

  return (posNext - posPrev).GetNormalized();
}

static void InsertHalfPoint(ezDynamicArray<ezPathComponent::LinearizedElement>& ref_result, ezDynamicArray<ezVec3>& ref_tangents, const ezPathComponent::ControlPoint& cp0, const ezPathComponent::ControlPoint& cp1, float fLowerT, float fUpperT, const ezVec3& vLowerPos, const ezVec3& vUpperPos, float fDistSqr, ezInt32 iMinSteps, ezInt32 iMaxSteps)
{
  const float fHalfT = ezMath::Lerp(fLowerT, fUpperT, 0.5f);

  const ezVec3 vHalfPos = ezMath::EvaluateBezierCurve(fHalfT, cp0.m_vPosition, cp0.m_vPosition + cp0.m_vTangentIn, cp1.m_vPosition + cp1.m_vTangentOut, cp1.m_vPosition);

  if (iMinSteps <= 0)
  {
    const ezVec3 vInterpPos = ezMath::Lerp(vLowerPos, vUpperPos, 0.5f);

    if ((vHalfPos - vInterpPos).GetLengthSquared() < fDistSqr)
    {
      return;
    }
  }

  if (iMaxSteps > 0)
  {
    InsertHalfPoint(ref_result, ref_tangents, cp0, cp1, fLowerT, fHalfT, vLowerPos, vHalfPos, fDistSqr, iMinSteps - 1, iMaxSteps - 1);
  }

  ref_result.ExpandAndGetRef().m_vPosition = vHalfPos;
  ref_tangents.ExpandAndGetRef() = ComputeTangentAt(fHalfT, cp0, cp1);

  if (iMaxSteps > 0)
  {
    InsertHalfPoint(ref_result, ref_tangents, cp0, cp1, fHalfT, fUpperT, vHalfPos, vUpperPos, fDistSqr, iMinSteps - 1, iMaxSteps - 1);
  }
}

static void GeneratePathSegment(ezUInt32 uiCp0, ezUInt32 uiCp1, ezArrayPtr<const ezPathComponent::ControlPoint> points, ezArrayPtr<ezVec3> cpUp, ezArrayPtr<ezVec3> cpFwd, ezDynamicArray<ezPathComponent::LinearizedElement>& ref_result, ezDynamicArray<ezVec3>& ref_tangents, float fDistSqr)
{
  ref_tangents.Clear();

  const auto& cp0 = points[uiCp0];
  const auto& cp1 = points[uiCp1];

  ezInt32 iRollDiv = 0;
  float fToRoll = ezMath::Abs((cp1.m_Roll - cp0.m_Roll).GetDegree());
  while (fToRoll > 45.0f)
  {
    fToRoll *= 0.5f;
    iRollDiv++;
  }

  ref_result.ExpandAndGetRef().m_vPosition = cp0.m_vPosition;
  ref_tangents.ExpandAndGetRef() = -cpFwd[uiCp0];

  InsertHalfPoint(ref_result, ref_tangents, cp0, cp1, 0.0f, 1.0f, cp0.m_vPosition, cp1.m_vPosition, fDistSqr, ezMath::Max(1, iRollDiv), 7);

  ref_result.ExpandAndGetRef().m_vPosition = cp1.m_vPosition;
  ref_tangents.ExpandAndGetRef() = -cpFwd[uiCp1];
}

static void ComputeSegmentUpVector(ezArrayPtr<ezPathComponent::LinearizedElement> segmentElements, ezUInt32 uiCp0, ezUInt32 uiCp1, const ezArrayPtr<const ezPathComponent::ControlPoint> points, const ezArrayPtr<const ezVec3> cpUp, const ezArrayPtr<const ezVec3> tangents, const ezVec3& vWorldUp)
{
  const auto& cp0 = points[uiCp0];
  const auto& cp1 = points[uiCp1];

  const ezVec3 cp0up = cpUp[uiCp0];
  const ezVec3 cp1up = cpUp[uiCp1];

  const double fSegmentLength = ComputePathLength(segmentElements);

  if (fSegmentLength <= 0.00001f)
  {
    for (ezUInt32 t = 0; t < segmentElements.GetCount(); ++t)
    {
      segmentElements[t].m_vUpDirection = cp1up;
    }

    return;
  }

  const double fInvSegmentLength = 1.0 / fSegmentLength;

  double fCurDist = 0.0;
  ezVec3 vPrevPos = segmentElements[0].m_vPosition;


  for (ezUInt32 t = 0; t < segmentElements.GetCount(); ++t)
  {
    fCurDist += (segmentElements[t].m_vPosition - vPrevPos).GetLength();
    vPrevPos = segmentElements[t].m_vPosition;

    const float fLerpFactor = (float)(fCurDist * fInvSegmentLength);

    const ezAngle roll = ezMath::Lerp(cp0.m_Roll, cp1.m_Roll, fLerpFactor);

    ezQuat qRoll = ezQuat::MakeFromAxisAndAngle(tangents[t], roll);

    ezVec3 vLocalUp = ezMath::Lerp(cp0up, cp1up, fLerpFactor);
    vLocalUp.NormalizeIfNotZero(vWorldUp).IgnoreResult();

    segmentElements[t].m_vUpDirection = qRoll * vLocalUp;
  }
}

void ezPathComponent::CreateLinearizedPathRepresentation(const ezDynamicArray<ControlPoint>& points)
{
  m_LinearizedRepresentation.Clear();

  const ezUInt32 uiNumCPs = points.GetCount();

  if (uiNumCPs <= 1)
    return;

  ezHybridArray<ezVec3, 64> cpUp;
  ezHybridArray<ezVec3, 64> cpFwd;

  ezCoordinateSystem cs;
  GetWorld()->GetCoordinateSystem(GetOwner()->GetGlobalPosition(), cs);

  ComputeCpDirs(points, m_bClosed, cs, cpFwd, cpUp);

  const ezUInt32 uiNumCPsToUse = m_bClosed ? uiNumCPs + 1 : uiNumCPs;

  ezHybridArray<ezVec3, 64> tangents;

  for (ezUInt32 uiCurPt = 1; uiCurPt < uiNumCPsToUse; ++uiCurPt)
  {
    const ezUInt32 uiCp0 = uiCurPt - 1;
    const ezUInt32 uiCp1 = uiCurPt % uiNumCPs;

    const ezUInt32 uiFirstNewNode = m_LinearizedRepresentation.GetCount();
    GeneratePathSegment(uiCp0, uiCp1, points, cpUp, cpFwd, m_LinearizedRepresentation, tangents, ezMath::Square(m_fLinearizationError));

    ezArrayPtr<ezPathComponent::LinearizedElement> segmentElements = m_LinearizedRepresentation.GetArrayPtr().GetSubArray(uiFirstNewNode);

    ComputeSegmentUpVector(segmentElements, uiCp0, uiCp1, points, cpUp, tangents, cs.m_vUpDir);
  }

  m_fLinearizedLength = (float)ComputePathLength(m_LinearizedRepresentation);
}



//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPathNodeTangentMode, 1)
  EZ_ENUM_CONSTANTS(ezPathNodeTangentMode::Auto, ezPathNodeTangentMode::Linear)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezPathNodeComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Roll", GetRoll, SetRoll),
    EZ_ENUM_ACCESSOR_PROPERTY("Tangent1", ezPathNodeTangentMode, GetTangentMode1, SetTangentMode1),
    EZ_ENUM_ACCESSOR_PROPERTY("Tangent2", ezPathNodeTangentMode, GetTangentMode2, SetTangentMode2),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgTransformChanged, OnMsgTransformChanged),
    EZ_MESSAGE_HANDLER(ezMsgParentChanged, OnMsgParentChanged),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation/Paths"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezPathNodeComponent::ezPathNodeComponent() = default;
ezPathNodeComponent::~ezPathNodeComponent() = default;

void ezPathNodeComponent::SetRoll(ezAngle roll)
{
  if (m_Roll != roll)
  {
    m_Roll = roll;
    PathChanged();
  }
}

void ezPathNodeComponent::SetTangentMode1(ezEnum<ezPathNodeTangentMode> mode)
{
  if (m_TangentMode1 != mode)
  {
    m_TangentMode1 = mode;
    PathChanged();
  }
}

void ezPathNodeComponent::SetTangentMode2(ezEnum<ezPathNodeTangentMode> mode)
{
  if (m_TangentMode2 != mode)
  {
    m_TangentMode2 = mode;
    PathChanged();
  }
}

void ezPathNodeComponent::OnMsgTransformChanged(ezMsgTransformChanged& msg)
{
  PathChanged();
}

void ezPathNodeComponent::OnMsgParentChanged(ezMsgParentChanged& msg)
{
  if (msg.m_Type == ezMsgParentChanged::Type::ParentUnlinked)
  {
    ezGameObject* pOldParent = nullptr;
    if (GetWorld()->TryGetObject(msg.m_hParent, pOldParent))
    {
      ezMsgPathChanged msg2;
      pOldParent->SendEventMessage(msg2, this);
    }
  }
  else
  {
    PathChanged();
  }
}

void ezPathNodeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->EnableStaticTransformChangesNotifications();
  GetOwner()->EnableParentChangesNotifications();

  PathChanged();
}

void ezPathNodeComponent::OnDeactivated()
{
  ezMsgPathChanged msg2;
  GetOwner()->SendEventMessage(msg2, this);

  SUPER::OnDeactivated();
}

void ezPathNodeComponent::PathChanged()
{
  if (!IsActiveAndInitialized())
    return;

  ezMsgPathChanged msg2;
  GetOwner()->SendEventMessage(msg2, this);
}

//////////////////////////////////////////////////////////////////////////

ezPathComponentManager::ezPathComponentManager(ezWorld* pWorld)
  : ezComponentManager(pWorld)
{
}

void ezPathComponentManager::SetEnableUpdate(ezPathComponent* pThis, bool bEnable)
{
  if (bEnable)
  {
    if (!m_NeedUpdate.Contains(pThis))
      m_NeedUpdate.PushBack(pThis);
  }
  else
  {
    m_NeedUpdate.RemoveAndSwap(pThis);
  }
}

void ezPathComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezPathComponentManager::Update, this);
  desc.m_bOnlyUpdateWhenSimulating = false;
  desc.m_Phase = ezWorldUpdatePhase::PostTransform;

  this->RegisterUpdateFunction(desc);
}

void ezPathComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (ezPathComponent* pComponent : m_NeedUpdate)
  {
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->DrawDebugVisualizations();
    }
  }
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_PathComponent);
