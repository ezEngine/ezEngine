#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/SplineComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezSplineComponentFlags, 1)
  EZ_BITFLAGS_CONSTANTS(ezSplineComponentFlags::VisualizeSpline, ezSplineComponentFlags::VisualizeUpDir, ezSplineComponentFlags::VisualizeTangents)
EZ_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgSplineChanged);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgSplineChanged, 1, ezRTTIDefaultAllocator<ezMsgSplineChanged>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezSplineComponentManager::ezSplineComponentManager(ezWorld* pWorld)
  : ezComponentManager(pWorld)
{
}

void ezSplineComponentManager::SetEnableUpdate(ezSplineComponent* pThis, bool bEnable)
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

void ezSplineComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezSplineComponentManager::Update, this);
  desc.m_bOnlyUpdateWhenSimulating = false;
  desc.m_Phase = ezWorldUpdatePhase::PostTransform;

  this->RegisterUpdateFunction(desc);
}

void ezSplineComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (ezSplineComponent* pComponent : m_NeedUpdate)
  {
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->DrawDebugVisualizations(pComponent->GetSplineFlags());
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSplineComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_BITFLAGS_ACCESSOR_PROPERTY("Flags", ezSplineComponentFlags, GetSplineFlags, SetSplineFlags),
    EZ_ACCESSOR_PROPERTY("Closed", GetClosed, SetClosed),
    EZ_MEMBER_PROPERTY("EditNodes", m_uiDummy),
    EZ_ARRAY_ACCESSOR_PROPERTY("Nodes", Nodes_GetCount, Nodes_GetNode, Nodes_SetNode, Nodes_Insert, Nodes_Remove),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgSplineChanged, OnMsgSplineChanged),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetPositionAtKey, In, "Key"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetForwardDirAtKey, In, "Key"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetUpDirAtKey, In, "Key"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetScaleAtKey, In, "Key"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetTransformAtKey, In, "Key"),

    EZ_SCRIPT_FUNCTION_PROPERTY(GetTotalLength),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetKeyAtDistance, In, "Distance"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetPositionAtDistance, In, "Distance"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetForwardDirAtDistance, In, "Distance"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetUpDirAtDistance, In, "Distance"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetScaleAtDistance, In, "Distance"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetTransformAtDistance, In, "Distance"),

    EZ_SCRIPT_FUNCTION_PROPERTY(GetChangeCounter),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Utilities/Splines"),
    new ezSplineManipulatorAttribute("Nodes", "Closed", "EditNodes"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSplineComponent::ezSplineComponent() = default;
ezSplineComponent::~ezSplineComponent() = default;

void ezSplineComponent::SerializeComponent(ezWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();
  s << m_SplineFlags;
  m_Spline.Serialize(s).AssertSuccess();
}

void ezSplineComponent::DeserializeComponent(ezWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();
  s >> m_SplineFlags;
  m_Spline.Deserialize(s).AssertSuccess();
}

void ezSplineComponent::OnActivated()
{
  SUPER::OnActivated();

  if (m_SplineFlags.IsAnyFlagSet())
  {
    static_cast<ezSplineComponentManager*>(GetOwningManager())->SetEnableUpdate(this, true);
  }

  UpdateSpline();
}

void ezSplineComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  if (m_SplineFlags.IsAnyFlagSet())
  {
    // assume that if no flag is set, update is already disabled
    static_cast<ezSplineComponentManager*>(GetOwningManager())->SetEnableUpdate(this, false);
  }
}

void ezSplineComponent::OnMsgSplineChanged(ezMsgSplineChanged& ref_msg)
{
  UpdateSpline();
}

void ezSplineComponent::SetClosed(bool bClosed)
{
  if (m_Spline.m_bClosed == bClosed)
    return;

  m_Spline.m_bClosed = bClosed;

  UpdateSpline();
}

void ezSplineComponent::SetSplineFlags(ezBitflags<ezSplineComponentFlags> flags)
{
  if (m_SplineFlags == flags)
    return;

  m_SplineFlags = flags;

  if (IsActiveAndInitialized())
  {
    static_cast<ezSplineComponentManager*>(GetOwningManager())->SetEnableUpdate(this, m_SplineFlags.IsAnyFlagSet());
  }
}

ezVec3 ezSplineComponent::GetPositionAtKey(float fKey) const
{
  return ezSimdConversion::ToVec3(m_Spline.EvaluatePosition(fKey));
}

ezVec3 ezSplineComponent::GetForwardDirAtKey(float fKey) const
{
  return ezSimdConversion::ToVec3(m_Spline.EvaluateDerivative(fKey).GetNormalized<3>());
}

ezVec3 ezSplineComponent::GetUpDirAtKey(float fKey) const
{
  return ezSimdConversion::ToVec3(m_Spline.EvaluateUpDirection(fKey));
}

ezVec3 ezSplineComponent::GetScaleAtKey(float fKey) const
{
  return ezSimdConversion::ToVec3(m_Spline.EvaluateScale(fKey));
}

ezTransform ezSplineComponent::GetTransformAtKey(float fKey) const
{
  return ezSimdConversion::ToTransform(m_Spline.EvaluateTransform(fKey));
}

float ezSplineComponent::GetKeyAtDistance(float fDistance) const
{
  if (m_DistanceToKey.IsEmpty())
    return 0.0f;

  const ezUInt32 uiUpperIndex = ezMath::Min(m_DistanceToKey.UpperBound(fDistance), m_DistanceToKey.GetCount() - 1);
  const ezUInt32 uiLowerIndex = uiUpperIndex > 0 ? uiUpperIndex - 1 : 0;

  const float fLowerDistance = m_DistanceToKey.GetKey(uiLowerIndex);
  const float fUpperDistance = m_DistanceToKey.GetKey(uiUpperIndex);
  const float fLowerKey = m_DistanceToKey.GetValue(uiLowerIndex);
  const float fUpperKey = m_DistanceToKey.GetValue(uiUpperIndex);

  return ezMath::Lerp(fLowerKey, fUpperKey, ezMath::Saturate(ezMath::Unlerp(fLowerDistance, fUpperDistance, fDistance)));
}

ezVec3 ezSplineComponent::GetPositionAtDistance(float fDistance) const
{
  const float fKey = GetKeyAtDistance(fDistance);
  return GetPositionAtKey(fKey);
}

ezVec3 ezSplineComponent::GetForwardDirAtDistance(float fDistance) const
{
  const float fKey = GetKeyAtDistance(fDistance);
  return GetForwardDirAtKey(fKey);
}

ezVec3 ezSplineComponent::GetUpDirAtDistance(float fDistance) const
{
  const float fKey = GetKeyAtDistance(fDistance);
  return GetUpDirAtKey(fKey);
}

ezVec3 ezSplineComponent::GetScaleAtDistance(float fDistance) const
{
  const float fKey = GetKeyAtDistance(fDistance);
  return GetScaleAtKey(fKey);
}

ezTransform ezSplineComponent::GetTransformAtDistance(float fDistance) const
{
  const float fKey = GetKeyAtDistance(fDistance);
  return GetTransformAtKey(fKey);
}

void ezSplineComponent::SetSpline(ezSpline&& spline)
{
  ezUInt32 uiOldChangeCounter = m_Spline.m_uiChangeCounter;
  m_Spline = std::move(spline);
  m_Spline.m_uiChangeCounter = uiOldChangeCounter + 1;

  CreateDistanceToKeyRemapping();
}

void ezSplineComponent::Nodes_SetNode(ezUInt32 uiIndex, const ezHashedString& sNodeName)
{
  m_Nodes[uiIndex] = sNodeName;

  UpdateSpline();
}

void ezSplineComponent::Nodes_Insert(ezUInt32 uiIndex, const ezHashedString& sNodeName)
{
  m_Nodes.InsertAt(uiIndex, sNodeName);

  UpdateSpline();
}

void ezSplineComponent::Nodes_Remove(ezUInt32 uiIndex)
{
  const ezHashedString& sNodeName = m_Nodes[uiIndex];
  if (ezSplineNodeComponent* pNodeComponent = FindNodeComponent(sNodeName))
  {
    pNodeComponent->m_uiNodeIndex = ezSmallInvalidIndex;
  }

  m_Nodes.RemoveAtAndCopy(uiIndex);

  UpdateSpline();
}

void ezSplineComponent::UpdateSpline()
{
  if (!IsActiveAndInitialized())
    return;

  if (GetUniqueID() != ezInvalidIndex)
  {
    // Only in Editor
    UpdateFromNodeObjects();
  }

  CreateDistanceToKeyRemapping();
}

ezSplineNodeComponent* ezSplineComponent::FindNodeComponent(const ezHashedString& sNodeName)
{
  ezGameObject* pNodeObj = GetWorld()->SearchForObject(sNodeName, GetOwner(), ezGetStaticRTTI<ezSplineNodeComponent>());
  if (pNodeObj == nullptr)
    return nullptr;

  ezSplineNodeComponent* pNodeComponent = nullptr;
  bool _ = pNodeObj->TryGetComponentOfBaseType(pNodeComponent);
  return pNodeComponent;
}

void ezSplineComponent::UpdateFromNodeObjects()
{
  auto& points = m_Spline.m_ControlPoints;
  points.Clear();

  if (m_Nodes.GetCount() < 2)
    return;

  for (const ezHashedString& sNode : m_Nodes)
  {
    ezSplineNodeComponent* pNodeComponent = FindNodeComponent(sNode);
    if (pNodeComponent == nullptr)
      continue;

    pNodeComponent->m_uiNodeIndex = points.GetCount();

    const ezSimdTransform localNodeTransform = pNodeComponent->GetOwner()->GetLocalTransformSimd();

    auto& cp = points.ExpandAndGetRef();
    cp.SetPosition(localNodeTransform.m_Position);
    cp.SetTangentIn(ezSimdConversion::ToVec3(pNodeComponent->GetCustomTangentIn()), pNodeComponent->GetTangentModeIn());

    if (pNodeComponent->GetLinkCustomTangents() && pNodeComponent->GetTangentModeIn() == ezSplineTangentMode::Custom && pNodeComponent->GetTangentModeOut() == ezSplineTangentMode::Custom)
    {
      cp.SetTangentOut(ezSimdConversion::ToVec3(-pNodeComponent->GetCustomTangentIn()), pNodeComponent->GetTangentModeOut());
    }
    else
    {
      cp.SetTangentOut(ezSimdConversion::ToVec3(pNodeComponent->GetCustomTangentOut()), pNodeComponent->GetTangentModeOut());
    }

    cp.SetRoll(pNodeComponent->GetRoll());
    cp.SetScale(localNodeTransform.m_Scale);
  }

  if (points.GetCount() < 2)
  {
    points.Clear();
    return;
  }

  ezCoordinateSystem coordinateSystem;
  GetWorld()->GetCoordinateSystem(GetOwner()->GetGlobalPosition(), coordinateSystem);

  m_Spline.CalculateUpDirAndAutoTangents(ezSimdConversion::ToVec3(coordinateSystem.m_vUpDir), ezSimdConversion::ToVec3(coordinateSystem.m_vForwardDir));

  ++m_Spline.m_uiChangeCounter;
}

void ezSplineComponent::InsertHalfPoint(ezDynamicArray<float>& ref_Ts, ezUInt32 uiCp0, float fLowerT, float fUpperT, const ezSimdVec4f& vLowerPos, const ezSimdVec4f& vUpperPos, float fDistSqr, ezInt32 iMinSteps, ezInt32 iMaxSteps) const
{
  const float fHalfT = ezMath::Lerp(fLowerT, fUpperT, 0.5f);

  const ezSimdVec4f vHalfPos = m_Spline.EvaluatePosition(uiCp0, fHalfT);

  if (iMinSteps <= 0)
  {
    const ezSimdVec4f vInterpPos = ezSimdVec4f::Lerp(vLowerPos, vUpperPos, ezSimdVec4f(0.5f));
    if ((vHalfPos - vInterpPos).GetLengthSquared<3>() < fDistSqr)
    {
      return;
    }
  }

  if (iMaxSteps > 0)
  {
    InsertHalfPoint(ref_Ts, uiCp0, fLowerT, fHalfT, vLowerPos, vHalfPos, fDistSqr, iMinSteps - 1, iMaxSteps - 1);
  }

  ref_Ts.PushBack(fHalfT);

  if (iMaxSteps > 0)
  {
    InsertHalfPoint(ref_Ts, uiCp0, fHalfT, fUpperT, vHalfPos, vUpperPos, fDistSqr, iMinSteps - 1, iMaxSteps - 1);
  }
}

void ezSplineComponent::CreateDistanceToKeyRemapping()
{
  m_DistanceToKey.Clear();
  m_fTotalLength = 0.0f;

  const auto& points = m_Spline.m_ControlPoints;
  const ezUInt32 uiNumCPs = points.GetCount();
  if (uiNumCPs < 2)
    return;

  m_DistanceToKey.Insert(0.0f, 0.0f);
  constexpr float fMaxErrorSqr = ezMath::Square(0.1f);

  ezHybridArray<float, 64> segmentTs;
  const ezUInt32 uiNumSegments = m_Spline.m_bClosed ? uiNumCPs : uiNumCPs - 1;
  for (ezUInt32 uiSegment = 0; uiSegment < uiNumSegments; ++uiSegment)
  {
    segmentTs.Clear();
    segmentTs.PushBack(1.0f);

    const ezUInt32 uiCp0 = uiSegment;
    const ezUInt32 uiCp1 = (uiCp0 + 1 < uiNumCPs) ? uiCp0 + 1 : 0;
    const auto& vLowerPos = points[uiCp0].m_vPos;
    const auto& vUpperPos = points[uiCp1].m_vPos;

    InsertHalfPoint(segmentTs, uiCp0, 0.0f, 1.0f, vLowerPos, vUpperPos, fMaxErrorSqr, 0, 7);
    segmentTs.Sort();

    ezSimdVec4f vLastPos = vLowerPos;
    for (float t : segmentTs)
    {
      const ezSimdVec4f vCurPos = m_Spline.EvaluatePosition(uiCp0, t);
      m_fTotalLength += (vLastPos - vCurPos).GetLength<3>();

      m_DistanceToKey.Insert(m_fTotalLength, t + uiSegment);

      vLastPos = vCurPos;
    }
  }
}

void ezSplineComponent::DrawDebugVisualizations(ezBitflags<ezSplineComponentFlags> flags) const
{
  if (flags.IsNoFlagSet())
    return;

  if (m_DistanceToKey.IsEmpty())
    return;

  const bool bVisPath = flags.IsSet(ezSplineComponentFlags::VisualizeSpline);
  const bool bVisUp = flags.IsSet(ezSplineComponentFlags::VisualizeUpDir);

  ezHybridArray<ezDebugRendererLine, 32> lines;
  ezColor c = ezColorScheme::DarkUI(ezColorScheme::Red);
  ezColor cUp = ezColorScheme::LightUI(ezColorScheme::Blue);

  ezVec3 lastPos = GetPositionAtKey(0);
  float fLastKey = 0.0f;
  for (ezUInt32 i = 1; i < m_DistanceToKey.GetCount(); ++i)
  {
    const float fKey = m_DistanceToKey.GetValue(i);

    const float fSubStep = 1.0f / 4.0f;
    for (float fT = fSubStep; fT <= 1.01f; fT += fSubStep)
    {
      const float fSubKey = ezMath::Lerp(fLastKey, fKey, fT);
      const ezVec3 curPos = GetPositionAtKey(fSubKey);

      if (bVisPath)
      {
        auto& line = lines.ExpandAndGetRef();
        line.m_start = lastPos;
        line.m_end = curPos;
        line.m_startColor = c;
        line.m_endColor = c;
      }

      if (bVisUp)
      {
        auto& line = lines.ExpandAndGetRef();
        line.m_start = curPos;
        line.m_end = curPos + GetUpDirAtKey(fSubKey) * 0.25f;
        line.m_startColor = cUp;
        line.m_endColor = cUp;
      }

      lastPos = curPos;
    }

    fLastKey = fKey;
  }

  ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::White, GetOwner()->GetGlobalTransform());

  const bool bVisTangents = flags.IsSet(ezSplineComponentFlags::VisualizeTangents);
  if (bVisTangents)
  {
    for (ezUInt32 i = 0; i < m_Spline.m_ControlPoints.GetCount(); ++i)
    {
      DrawDebugTangents(i);
    }
  }
}

void ezSplineComponent::DrawDebugTangents(ezUInt32 uiPointIndex, ezSplineTangentMode::Enum tangentModeIn /*= ezSplineTangentMode::Default*/, ezSplineTangentMode::Enum tangentModeOut /*= ezSplineTangentMode::Default*/) const
{
  if (uiPointIndex < m_Spline.m_ControlPoints.GetCount())
  {
    const ezSpline::ControlPoint& cp = m_Spline.m_ControlPoints[uiPointIndex];

    const ezColor tInColor = ezColorScheme::DarkUI(tangentModeIn == ezSplineTangentMode::Custom ? ezColorScheme::Grape : ezColorScheme::Gray);
    const ezColor tOutColor = ezColorScheme::DarkUI(tangentModeOut == ezSplineTangentMode::Custom ? ezColorScheme::Grape : ezColorScheme::Gray);
    const ezVec3 vTangentIn = ezSimdConversion::ToVec3(cp.m_vPosTangentIn);
    const ezVec3 vTangentOut = ezSimdConversion::ToVec3(cp.m_vPosTangentOut);

    const ezVec3 vGlobalPos = ezSimdConversion::ToVec3(GetOwner()->GetGlobalTransformSimd().TransformPosition(cp.m_vPos));
    const ezTransform t = ezTransform::Make(vGlobalPos);

    ezDebugRenderer::DrawLineSphere(GetWorld(), ezBoundingSphere::MakeFromCenterAndRadius(vTangentIn, 0.05f), tInColor, t);
    ezDebugRenderer::DrawLineSphere(GetWorld(), ezBoundingSphere::MakeFromCenterAndRadius(vTangentOut, 0.05f), tOutColor, t);

    ezHybridArray<ezDebugRendererLine, 2> lines;
    lines.PushBack(ezDebugRendererLine(ezVec3::MakeZero(), vTangentIn, tInColor));
    lines.PushBack(ezDebugRendererLine(ezVec3::MakeZero(), vTangentOut, tOutColor));
    ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::White, t);
  }
}

bool ezSplineComponent::DrawSplineOnSelection() const
{
  const ezUInt16 uiFrame = static_cast<ezUInt16>(ezRenderWorld::GetFrameCounter());
  if (m_uiExtractedFrame == uiFrame)
  {
    return false; // already drawn this frame
  }

  m_uiExtractedFrame = uiFrame;

  if (!m_SplineFlags.IsSet(ezSplineComponentFlags::VisualizeSpline))
  {
    DrawDebugVisualizations(ezSplineComponentFlags::VisualizeSpline);
  }

  return true;
}

void ezSplineComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != ezDefaultRenderDataCategories::Selection)
    return;

  DrawSplineOnSelection();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSplineNodeComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Roll", GetRoll, SetRoll),
    EZ_ENUM_ACCESSOR_PROPERTY("TangentModeIn", ezSplineTangentMode, GetTangentModeIn, SetTangentModeIn),
    EZ_ACCESSOR_PROPERTY("CustomTangentIn", GetCustomTangentIn, SetCustomTangentIn),
    EZ_ENUM_ACCESSOR_PROPERTY("TangentModeOut", ezSplineTangentMode, GetTangentModeOut, SetTangentModeOut),
    EZ_ACCESSOR_PROPERTY("CustomTangentOut", GetCustomTangentOut, SetCustomTangentOut),
    EZ_ACCESSOR_PROPERTY("LinkCustomTangents", GetLinkCustomTangents, SetLinkCustomTangents),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgTransformChanged, OnMsgTransformChanged),
    EZ_MESSAGE_HANDLER(ezMsgParentChanged, OnMsgParentChanged),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Utilities/Splines"),
    new ezSplineTangentManipulatorAttribute("TangentModeIn", "CustomTangentIn"),
    new ezSplineTangentManipulatorAttribute("TangentModeOut", "CustomTangentOut"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSplineNodeComponent::ezSplineNodeComponent() = default;
ezSplineNodeComponent::~ezSplineNodeComponent() = default;

void ezSplineNodeComponent::SetRoll(ezAngle roll)
{
  if (m_Roll != roll)
  {
    m_Roll = roll;
    SplineChanged();
  }
}

void ezSplineNodeComponent::SetTangentModeIn(ezEnum<ezSplineTangentMode> mode)
{
  if (m_TangentModeIn != mode)
  {
    m_TangentModeIn = mode;
    SplineChanged();
  }
}

void ezSplineNodeComponent::SetTangentModeOut(ezEnum<ezSplineTangentMode> mode)
{
  if (m_TangentModeOut != mode)
  {
    m_TangentModeOut = mode;
    SplineChanged();
  }
}

void ezSplineNodeComponent::SetCustomTangentIn(const ezVec3& vTangent)
{
  if (m_vCustomTangentIn != vTangent)
  {
    m_vCustomTangentIn = vTangent;
    SplineChanged();
  }
}

void ezSplineNodeComponent::SetCustomTangentOut(const ezVec3& vTangent)
{
  if (m_vCustomTangentOut != vTangent)
  {
    m_vCustomTangentOut = vTangent;
    SplineChanged();
  }
}

void ezSplineNodeComponent::SetLinkCustomTangents(bool bLink)
{
  if (bLink != GetLinkCustomTangents())
  {
    SetUserFlag(0, bLink);
    SplineChanged();
  }
}

bool ezSplineNodeComponent::GetLinkCustomTangents() const
{
  return GetUserFlag(0);
}

void ezSplineNodeComponent::OnMsgTransformChanged(ezMsgTransformChanged& msg)
{
  SplineChanged();
}

void ezSplineNodeComponent::OnMsgParentChanged(ezMsgParentChanged& msg)
{
  if (msg.m_Type == ezMsgParentChanged::Type::ParentUnlinked)
  {
    ezGameObject* pOldParent = nullptr;
    if (GetWorld()->TryGetObject(msg.m_hParent, pOldParent))
    {
      ezMsgSplineChanged msg2;
      pOldParent->SendEventMessage(msg2, this);
    }
  }
  else
  {
    SplineChanged();
  }
}

void ezSplineNodeComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != ezDefaultRenderDataCategories::Selection)
    return;

  const ezGameObject* pParent = GetOwner()->GetParent();
  const ezSplineComponent* pSplineComponent = nullptr;

  while (pParent != nullptr && !pParent->TryGetComponentOfBaseType(pSplineComponent))
  {
    pParent = pParent->GetParent();
  }

  if (pSplineComponent != nullptr && pSplineComponent->DrawSplineOnSelection())
  {
    pSplineComponent->DrawDebugTangents(m_uiNodeIndex, m_TangentModeIn, m_TangentModeOut);
  }
}

void ezSplineNodeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->EnableStaticTransformChangesNotifications();
  GetOwner()->EnableParentChangesNotifications();
}

void ezSplineNodeComponent::SplineChanged()
{
  if (!IsActiveAndInitialized())
    return;

  ezMsgSplineChanged msg;
  GetOwner()->SendEventMessage(msg, this);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezPathComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezPathComponentPatch_1_2()
    : ezGraphPatch("ezPathComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("ezSplineComponent");

    auto* pFlags = pNode->FindProperty("Flags");
    if (pFlags && pFlags->m_Value.IsA<ezString>())
    {
      ezStringBuilder sFlags = pFlags->m_Value.Get<ezString>();
      sFlags.ReplaceAll("Path", "Spline");
      pNode->ChangeProperty("Flags", sFlags.GetView());
    }
  }
};

ezPathComponentPatch_1_2 g_ezPathComponentPatch_1_2;

class ezPathNodeComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezPathNodeComponentPatch_1_2()
    : ezGraphPatch("ezPathNodeComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("ezSplineNodeComponent");
  }
};

ezPathNodeComponentPatch_1_2 g_ezPathNodeComponentPatch_1_2;


EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SplineComponent);
