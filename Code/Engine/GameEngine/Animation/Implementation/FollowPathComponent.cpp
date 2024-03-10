#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameEngine/Animation/FollowPathComponent.h>
#include <GameEngine/Animation/PathComponent.h>

#include <RendererCore/Debug/DebugRenderer.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezFollowPathMode, 1)
  EZ_ENUM_CONSTANTS(ezFollowPathMode::OnlyPosition, ezFollowPathMode::AlignUpZ, ezFollowPathMode::FullRotation)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezFollowPathComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Path", DummyGetter, SetPathObject)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_ACCESSOR_PROPERTY("StartDistance", GetDistanceAlongPath, SetDistanceAlongPath)->AddAttributes(new ezClampValueAttribute(0.0f, {})),
    EZ_ACCESSOR_PROPERTY("Running", IsRunning, SetRunning)->AddAttributes(new ezDefaultValueAttribute(true)), // Whether the animation should start right away.
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezPropertyAnimMode, m_Mode),
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("LookAhead", m_fLookAhead)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("Smoothing", m_fSmoothing)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ENUM_MEMBER_PROPERTY("FollowMode", ezFollowPathMode, m_FollowMode),  
    EZ_MEMBER_PROPERTY("TiltAmount", m_fTiltAmount)->AddAttributes(new ezDefaultValueAttribute(5.0f)),
    EZ_MEMBER_PROPERTY("MaxTilt", m_MaxTilt)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(30.0f)), new ezClampValueAttribute(ezAngle::MakeFromDegree(0.0f), ezAngle::MakeFromDegree(90.0f))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SetDirectionForwards, In, "Forwards"),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsDirectionForwards),
    EZ_SCRIPT_FUNCTION_PROPERTY(ToggleDirection),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation/Paths"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezFollowPathComponent::ezFollowPathComponent() = default;
ezFollowPathComponent::~ezFollowPathComponent() = default;

void ezFollowPathComponent::Update(bool bForce)
{
  if (!bForce && (!m_bIsRunning || m_fSpeed == 0.0f))
    return;

  if (m_hPathObject.IsInvalidated())
    return;

  ezWorld* pWorld = GetWorld();

  ezGameObject* pPathObject = nullptr;
  if (!pWorld->TryGetObject(m_hPathObject, pPathObject))
  {
    // no need to retry this again
    m_hPathObject.Invalidate();
    return;
  }

  ezPathComponent* pPathComponent;
  if (!pPathObject->TryGetComponentOfBaseType(pPathComponent))
    return;

  pPathComponent->EnsureLinearizedRepresentationIsUpToDate();

  auto& clock = pWorld->GetClock();

  float fToAdvance = m_fSpeed * clock.GetTimeDiff().AsFloatInSeconds();

  if (!m_bIsRunningForwards)
  {
    fToAdvance = -fToAdvance;
  }

  {
    if (!pPathComponent->AdvanceLinearSamplerBy(m_PathSampler, fToAdvance) && fToAdvance != 0.0f)
    {
      ezMsgAnimationReachedEnd msg;
      m_ReachedEndEvent.SendEventMessage(msg, this, GetOwner());

      if (m_Mode == ezPropertyAnimMode::Loop)
      {
        pPathComponent->SetLinearSamplerTo(m_PathSampler, fToAdvance);
      }
      else if (m_Mode == ezPropertyAnimMode::BackAndForth)
      {
        m_bIsRunningForwards = !m_bIsRunningForwards;
        fToAdvance = -fToAdvance;
        pPathComponent->AdvanceLinearSamplerBy(m_PathSampler, fToAdvance);
      }
      else
      {
        m_bIsRunning = false;
      }
    }
  }

  ezPathComponent::LinearSampler samplerAhead;

  float fLookAhead = ezMath::Max(m_fLookAhead, 0.02f);

  {
    samplerAhead = m_PathSampler;
    if (!pPathComponent->AdvanceLinearSamplerBy(samplerAhead, fLookAhead) && fLookAhead != 0.0f)
    {
      if (m_Mode == ezPropertyAnimMode::Loop)
      {
        pPathComponent->SetLinearSamplerTo(samplerAhead, fLookAhead);
      }
    }
  }

  auto transform = pPathComponent->SampleLinearizedRepresentation(m_PathSampler);
  auto transformAhead = pPathComponent->SampleLinearizedRepresentation(samplerAhead);

  if (m_bLastStateValid)
  {
    const float fSmoothing = ezMath::Clamp(m_fSmoothing, 0.0f, 0.99f);

    transform.m_vPosition = ezMath::Lerp(transform.m_vPosition, m_vLastPosition, fSmoothing);
    transform.m_vUpDirection = ezMath::Lerp(transform.m_vUpDirection, m_vLastUpDir, fSmoothing);
    transformAhead.m_vPosition = ezMath::Lerp(transformAhead.m_vPosition, m_vLastTargetPosition, fSmoothing);
  }

  ezVec3 vTarget = transformAhead.m_vPosition - transform.m_vPosition;
  if (m_FollowMode == ezFollowPathMode::AlignUpZ)
  {
    const ezPlane plane = ezPlane::MakeFromNormalAndPoint(ezVec3::MakeAxisZ(), transform.m_vPosition);
    vTarget = plane.GetCoplanarDirection(vTarget);
  }
  vTarget.NormalizeIfNotZero(ezVec3::MakeAxisX()).IgnoreResult();

  ezVec3 vUp = (m_FollowMode == ezFollowPathMode::FullRotation) ? transform.m_vUpDirection : ezVec3::MakeAxisZ();
  ezVec3 vRight = vTarget.CrossRH(vUp);
  vRight.NormalizeIfNotZero(ezVec3::MakeAxisY()).IgnoreResult();

  vUp = vRight.CrossRH(vTarget);
  vUp.NormalizeIfNotZero(ezVec3::MakeAxisZ()).IgnoreResult();

  // check if we want to tilt the platform when turning
  ezAngle deltaAngle = ezAngle::MakeFromDegree(0.0f);
  if (m_FollowMode == ezFollowPathMode::AlignUpZ && !ezMath::IsZero(m_fTiltAmount, 0.0001f) && !ezMath::IsZero(m_MaxTilt.GetDegree(), 0.0001f))
  {
    if (m_bLastStateValid)
    {
      ezVec3 vLastTarget = m_vLastTargetPosition - m_vLastPosition;
      {
        const ezPlane plane = ezPlane::MakeFromNormalAndPoint(ezVec3::MakeAxisZ(), transform.m_vPosition);
        vLastTarget = plane.GetCoplanarDirection(vLastTarget);
        vLastTarget.NormalizeIfNotZero(ezVec3::MakeAxisX()).IgnoreResult();
      }

      const float fTiltStrength = ezMath::Sign((vTarget - vLastTarget).Dot(vRight)) * ezMath::Sign(m_fTiltAmount);
      ezAngle tiltAngle = ezMath::Min(vLastTarget.GetAngleBetween(vTarget) * ezMath::Abs(m_fTiltAmount), m_MaxTilt);
      deltaAngle = ezMath::Lerp(tiltAngle * fTiltStrength, m_LastTiltAngle, 0.85f); // this smooths out the tilting from being jittery

      ezQuat rot = ezQuat::MakeFromAxisAndAngle(vTarget, deltaAngle);
      vUp = rot * vUp;
      vRight = rot * vRight;
    }
  }

  {
    m_bLastStateValid = true;
    m_vLastPosition = transform.m_vPosition;
    m_vLastUpDir = transform.m_vUpDirection;
    m_vLastTargetPosition = transformAhead.m_vPosition;
    m_LastTiltAngle = deltaAngle;
  }

  ezMat3 mRot = ezMat3::MakeIdentity();
  if (m_FollowMode != ezFollowPathMode::OnlyPosition)
  {
    mRot.SetColumn(0, vTarget);
    mRot.SetColumn(1, -vRight);
    mRot.SetColumn(2, vUp);
  }

  ezTransform tFinal;
  tFinal.m_vPosition = transform.m_vPosition;
  tFinal.m_vScale.Set(1);
  tFinal.m_qRotation = ezQuat::MakeFromMat3(mRot);

  GetOwner()->SetGlobalTransform(pPathObject->GetGlobalTransform() * tFinal);
}

void ezFollowPathComponent::SetPathObject(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hPathObject = resolver(szReference, GetHandle(), "Path");
}

void ezFollowPathComponent::SetDistanceAlongPath(float fDistance)
{
  m_bLastStateValid = false;
  m_fStartDistance = fDistance;

  if (IsActiveAndInitialized())
  {
    if (m_hPathObject.IsInvalidated())
      return;

    ezWorld* pWorld = GetWorld();

    ezGameObject* pPathObject = nullptr;
    if (!pWorld->TryGetObject(m_hPathObject, pPathObject))
      return;

    ezPathComponent* pPathComponent = nullptr;
    if (!pPathObject->TryGetComponentOfBaseType(pPathComponent))
      return;

    pPathComponent->EnsureLinearizedRepresentationIsUpToDate();

    pPathComponent->SetLinearSamplerTo(m_PathSampler, m_fStartDistance);

    Update(true);
  }
}

float ezFollowPathComponent::GetDistanceAlongPath() const
{
  return m_fStartDistance;
}

void ezFollowPathComponent::SerializeComponent(ezWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();

  ref_stream.WriteGameObjectHandle(m_hPathObject);

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

void ezFollowPathComponent::DeserializeComponent(ezWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();

  m_hPathObject = ref_stream.ReadGameObjectHandle();

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

void ezFollowPathComponent::OnActivated()
{
  SUPER::OnActivated();

  // initialize sampler
  SetDistanceAlongPath(m_fStartDistance);
}

void ezFollowPathComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // if no path reference was set, search the parent objects for a path
  if (m_hPathObject.IsInvalidated())
  {
    ezGameObject* pParent = GetOwner()->GetParent();
    while (pParent != nullptr)
    {
      ezPathComponent* pPath = nullptr;
      if (pParent->TryGetComponentOfBaseType(pPath))
      {
        m_hPathObject = pPath->GetOwner()->GetHandle();
        break;
      }

      pParent = pParent->GetParent();
    }
  }

  // initialize sampler
  SetDistanceAlongPath(m_fStartDistance);
}

bool ezFollowPathComponent::IsRunning(void) const
{
  return m_bIsRunning;
}

void ezFollowPathComponent::SetRunning(bool b)
{
  m_bIsRunning = b;
}

void ezFollowPathComponent::SetDirectionForwards(bool bForwards)
{
  m_bIsRunningForwards = bForwards;
}

void ezFollowPathComponent::ToggleDirection()
{
  m_bIsRunningForwards = !m_bIsRunningForwards;
}

bool ezFollowPathComponent::IsDirectionForwards() const
{
  return m_bIsRunningForwards;
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_FollowPathComponent);
