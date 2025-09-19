#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameEngine/Animation/FollowSplineComponent.h>
#include <RendererCore/Components/SplineComponent.h>

#include <RendererCore/Debug/DebugRenderer.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezFollowSplineMode, 1)
  EZ_ENUM_CONSTANTS(ezFollowSplineMode::OnlyPosition, ezFollowSplineMode::AlignUpZ, ezFollowSplineMode::FullRotation)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezFollowSplineComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Spline", DummyGetter, SetSplineObject)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_ACCESSOR_PROPERTY("StartDistance", GetStartDistance, SetStartDistance)->AddAttributes(new ezClampValueAttribute(0.0f, {})),
    EZ_ACCESSOR_PROPERTY("Running", IsRunning, SetRunning)->AddAttributes(new ezDefaultValueAttribute(true)), // Whether the animation should start right away.
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezPropertyAnimMode, m_Mode),
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("LookAhead", m_fLookAhead)->AddAttributes(new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("Smoothing", m_fSmoothing)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ENUM_MEMBER_PROPERTY("FollowMode", ezFollowSplineMode, m_FollowMode),  
    EZ_MEMBER_PROPERTY("TiltAmount", m_fTiltAmount)->AddAttributes(new ezDefaultValueAttribute(5.0f)),
    EZ_MEMBER_PROPERTY("MaxTilt", m_MaxTilt)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(30.0f)), new ezClampValueAttribute(ezAngle::MakeFromDegree(0.0f), ezAngle::MakeFromDegree(90.0f))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SetCurrentDistance, In, "Distance"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetCurrentDistance),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetDirectionForwards, In, "Forwards"),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsDirectionForwards),
    EZ_SCRIPT_FUNCTION_PROPERTY(ToggleDirection),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezFollowSplineComponent::ezFollowSplineComponent() = default;
ezFollowSplineComponent::~ezFollowSplineComponent() = default;

void ezFollowSplineComponent::SerializeComponent(ezWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();

  ref_stream.WriteGameObjectHandle(m_hSplineObject);

  s << m_fStartDistance;
  s << m_fSpeed;
  s << m_fLookAhead;
  s << m_Mode;
  s << m_fSmoothing;
  s << m_bIsRunning;
  s << m_bIsRunningForwards;
  s << m_FollowMode;
  s << m_fTiltAmount;
  s << m_MaxTilt;
}

void ezFollowSplineComponent::DeserializeComponent(ezWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();

  m_hSplineObject = ref_stream.ReadGameObjectHandle();

  s >> m_fStartDistance;
  s >> m_fSpeed;
  s >> m_fLookAhead;
  s >> m_Mode;
  s >> m_fSmoothing;
  s >> m_bIsRunning;
  s >> m_bIsRunningForwards;
  s >> m_FollowMode;
  s >> m_fTiltAmount;
  s >> m_MaxTilt;
}

void ezFollowSplineComponent::OnActivated()
{
  SUPER::OnActivated();

  SetCurrentDistance(m_fStartDistance);
}

void ezFollowSplineComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // if no spline reference was set, search the parent objects for a spline
  if (m_hSplineObject.IsInvalidated())
  {
    ezGameObject* pParent = GetOwner()->GetParent();
    while (pParent != nullptr)
    {
      ezSplineComponent* pSpline = nullptr;
      if (pParent->TryGetComponentOfBaseType(pSpline))
      {
        m_hSplineObject = pSpline->GetOwner()->GetHandle();
        break;
      }

      pParent = pParent->GetParent();
    }
  }

  SetCurrentDistance(m_fStartDistance);
}

void ezFollowSplineComponent::SetSplineObject(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hSplineObject = resolver(szReference, GetHandle(), "Spline");
}

void ezFollowSplineComponent::SetStartDistance(float fDistance)
{
  m_bLastStateValid = false;
  m_fStartDistance = fDistance;

  if (IsActiveAndInitialized())
  {
    SetCurrentDistance(m_fStartDistance);
  }
}

void ezFollowSplineComponent::SetCurrentDistance(float fDistance)
{
  m_fCurrentDistance = ezMath::Max(fDistance, 0.0f);

  if (IsActiveAndInitialized())
  {
    ezGameObject* pSplineObject = nullptr;
    if (!GetWorld()->TryGetObject(m_hSplineObject, pSplineObject))
      return;

    ezSplineComponent* pSplineComponent;
    if (!pSplineObject->TryGetComponentOfBaseType(pSplineComponent))
      return;

    m_fCurrentDistance = ezMath::Min(m_fCurrentDistance, pSplineComponent->GetTotalLength());

    Update(true);
  }
}

void ezFollowSplineComponent::SetRunning(bool b)
{
  m_bIsRunning = b;
}

void ezFollowSplineComponent::SetDirectionForwards(bool bForwards)
{
  m_bIsRunningForwards = bForwards;
}

void ezFollowSplineComponent::ToggleDirection()
{
  m_bIsRunningForwards = !m_bIsRunningForwards;
}

void ezFollowSplineComponent::Update(bool bForce)
{
  if (!bForce && (!m_bIsRunning || m_fSpeed == 0.0f))
    return;

  if (m_hSplineObject.IsInvalidated())
    return;

  ezWorld* pWorld = GetWorld();

  ezGameObject* pSplineObject = nullptr;
  if (!pWorld->TryGetObject(m_hSplineObject, pSplineObject))
  {
    // no need to retry this again
    m_hSplineObject.Invalidate();
    return;
  }

  ezSplineComponent* pSplineComponent;
  if (!pSplineObject->TryGetComponentOfBaseType(pSplineComponent))
    return;

  auto& clock = pWorld->GetClock();

  float fToAdvance = m_fSpeed * clock.GetTimeDiff().AsFloatInSeconds();

  if (!m_bIsRunningForwards)
  {
    fToAdvance = -fToAdvance;
  }

  if (fToAdvance != 0.0f)
  {
    const float fTotalLength = pSplineComponent->GetTotalLength();

    bool bReachedEnd = false;
    const float fNewDistance = m_fCurrentDistance + fToAdvance;
    if (fToAdvance > 0.0f && fNewDistance >= fTotalLength)
    {
      bReachedEnd = true;
      m_fCurrentDistance = fTotalLength;
      fToAdvance = fNewDistance - fTotalLength;
    }
    else if (fToAdvance < 0.0f && fNewDistance <= 0.0f)
    {
      bReachedEnd = true;
      m_fCurrentDistance = 0.0f;
      fToAdvance = fNewDistance;
    }
    else
    {
      m_fCurrentDistance = fNewDistance;
    }

    if (bReachedEnd)
    {
      ezMsgAnimationReachedEnd msg;
      m_ReachedEndEvent.SendEventMessage(msg, this, GetOwner());

      if (m_Mode == ezPropertyAnimMode::Loop)
      {
        m_fCurrentDistance = fToAdvance;
      }
      else if (m_Mode == ezPropertyAnimMode::BackAndForth)
      {
        m_bIsRunningForwards = !m_bIsRunningForwards;
        fToAdvance = -fToAdvance;
        m_fCurrentDistance += fToAdvance;
      }
      else
      {
        m_bIsRunning = false;
      }
    }
  }

  const float fKey = pSplineComponent->GetKeyAtDistance(m_fCurrentDistance);
  ezVec3 vPosition = pSplineComponent->GetPositionAtKey(fKey);
  ezVec3 vUpDir = pSplineComponent->GetUpDirAtKey(fKey);

  ezVec3 vForwardDir;
  if (m_fLookAhead > 0.0f)
  {
    float fLookAhead = ezMath::Max(m_fLookAhead, 0.02f);
    float fLookAheadDistance = m_fCurrentDistance + fLookAhead;
    if (fLookAheadDistance > pSplineComponent->GetTotalLength() && m_Mode == ezPropertyAnimMode::Loop)
    {
      fLookAheadDistance -= pSplineComponent->GetTotalLength();
    }

    const ezVec3 vLookAheadPosition = pSplineComponent->GetPositionAtDistance(fLookAheadDistance);
    vForwardDir = vLookAheadPosition - vPosition;
  }
  else
  {
    vForwardDir = pSplineComponent->GetForwardDirAtKey(fKey);
  }

  if (m_bLastStateValid)
  {
    const float fSmoothing = ezMath::Clamp(m_fSmoothing, 0.0f, 0.99f);

    vPosition = ezMath::Lerp(vPosition, m_vLastPosition, fSmoothing);
    vUpDir = ezMath::Lerp(vUpDir, m_vLastUpDir, fSmoothing);
    vForwardDir = ezMath::Lerp(vForwardDir, m_vLastForwardDir, fSmoothing);
  }

  if (m_FollowMode == ezFollowSplineMode::AlignUpZ)
  {
    const ezPlane plane = ezPlane::MakeFromNormalAndPoint(ezVec3::MakeAxisZ(), vPosition);
    vForwardDir = plane.GetCoplanarDirection(vForwardDir);
  }
  vForwardDir.NormalizeIfNotZero(ezVec3::MakeAxisX()).IgnoreResult();

  vUpDir = (m_FollowMode == ezFollowSplineMode::FullRotation) ? vUpDir : ezVec3::MakeAxisZ();
  ezVec3 vRightDir = vUpDir.CrossRH(vForwardDir);
  vRightDir.NormalizeIfNotZero(ezVec3::MakeAxisY()).IgnoreResult();

  vUpDir = vForwardDir.CrossRH(vRightDir);
  vUpDir.NormalizeIfNotZero(ezVec3::MakeAxisZ()).IgnoreResult();

  // check if we want to tilt the platform when turning
  ezAngle deltaAngle = ezAngle::MakeFromDegree(0.0f);
  if (m_FollowMode == ezFollowSplineMode::AlignUpZ && !ezMath::IsZero(m_fTiltAmount, 0.0001f) && !ezMath::IsZero(m_MaxTilt.GetDegree(), 0.0001f))
  {
    if (m_bLastStateValid)
    {
      ezVec3 vLastForwardDir = m_vLastForwardDir;
      {
        const ezPlane plane = ezPlane::MakeFromNormalAndPoint(ezVec3::MakeAxisZ(), vPosition);
        vLastForwardDir = plane.GetCoplanarDirection(vLastForwardDir);
        vLastForwardDir.NormalizeIfNotZero(ezVec3::MakeAxisX()).IgnoreResult();
      }

      const float fTiltStrength = ezMath::Sign((vLastForwardDir - vForwardDir).Dot(vRightDir)) * ezMath::Sign(m_fTiltAmount);
      ezAngle tiltAngle = ezMath::Min(vLastForwardDir.GetAngleBetween(vForwardDir) * ezMath::Abs(m_fTiltAmount), m_MaxTilt);
      deltaAngle = ezMath::Lerp(tiltAngle * fTiltStrength, m_LastTiltAngle, 0.85f); // this smooths out the tilting from being jittery

      ezQuat rot = ezQuat::MakeFromAxisAndAngle(vForwardDir, deltaAngle);
      vUpDir = rot * vUpDir;
      vRightDir = rot * vRightDir;
    }
  }

  {
    m_bLastStateValid = true;
    m_vLastPosition = vPosition;
    m_vLastForwardDir = vForwardDir;
    m_vLastUpDir = vUpDir;
    m_LastTiltAngle = deltaAngle;
  }

  ezMat3 mRot = ezMat3::MakeIdentity();
  if (m_FollowMode != ezFollowSplineMode::OnlyPosition)
  {
    mRot.SetColumn(0, vForwardDir);
    mRot.SetColumn(1, vRightDir);
    mRot.SetColumn(2, vUpDir);
  }

  ezTransform tFinal;
  tFinal.m_vPosition = vPosition;
  tFinal.m_vScale = GetOwner()->GetLocalScaling() * GetOwner()->GetLocalUniformScaling();
  tFinal.m_qRotation = ezQuat::MakeFromMat3(mRot);

  GetOwner()->SetGlobalTransform(pSplineObject->GetGlobalTransform() * tFinal);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezFollowPathComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezFollowPathComponentPatch_1_2()
    : ezGraphPatch("ezFollowPathComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("ezFollowSplineComponent");

    pNode->RenameProperty("Path", "Spline");

    auto* pFollowMode = pNode->FindProperty("FollowMode");
    if (pFollowMode && pFollowMode->m_Value.IsA<ezString>())
    {
      ezStringBuilder sFollowMode = pFollowMode->m_Value.Get<ezString>();
      sFollowMode.ReplaceAll("Path", "Spline");
      pNode->ChangeProperty("FollowMode", sFollowMode.GetView());
    }
  }
};

ezFollowPathComponentPatch_1_2 g_ezFollowPathComponentPatch_1_2;


EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_FollowSplineComponent);
