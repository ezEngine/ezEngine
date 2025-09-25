#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameComponentsPlugin/Camera/ThirdPersonViewComponent.h>

ezThirdPersonViewComponentManager::ezThirdPersonViewComponentManager(ezWorld* pWorld)
  : ezComponentManager(pWorld)
{
}

ezThirdPersonViewComponentManager::~ezThirdPersonViewComponentManager() = default;

void ezThirdPersonViewComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezThirdPersonViewComponentManager::Update, this);
    desc.m_Phase = ezWorldUpdatePhase::PostTransform;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void ezThirdPersonViewComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  // Iterate through all components managed by this manager and update them
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->Update();
    }
  }
}


// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezThirdPersonViewComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("TargetObject", GetTargetObject, SetTargetObject)->AddAttributes(new ezDefaultValueAttribute(ezStringView(".."))),
    EZ_MEMBER_PROPERTY("TargetOffsetHigh", m_vTargetOffsetHigh),
    EZ_MEMBER_PROPERTY("TargetOffsetLow", m_vTargetOffsetLow),
    EZ_MEMBER_PROPERTY("MinDistance", m_fMinDistance)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.05f, 10.0f)),
    EZ_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new ezDefaultValueAttribute(3.0f), new ezClampValueAttribute(0.5f, 100.0f)),
    EZ_MEMBER_PROPERTY("MaxDistanceUp", m_fMaxDistanceUp)->AddAttributes(new ezDefaultValueAttribute(3.0f), new ezClampValueAttribute(0.5f, 100.0f)),
    EZ_MEMBER_PROPERTY("MaxDistanceDown", m_fMaxDistanceDown)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.5f, 100.0f)),
    EZ_MEMBER_PROPERTY("MinUpRotation", m_MinUpRotation)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(-70)), new ezClampValueAttribute(ezAngle::MakeFromDegree(-85), ezAngle::MakeFromDegree(70))),
    EZ_MEMBER_PROPERTY("MaxUpRotation", m_MaxUpRotation)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(+80)), new ezClampValueAttribute(ezAngle::MakeFromDegree(-70), ezAngle::MakeFromDegree(85))),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("SweepWidth", m_fSweepWidth)->AddAttributes(new ezDefaultValueAttribute(0.4f), new ezClampValueAttribute(0.05f, 2.0f)),
    EZ_MEMBER_PROPERTY("ZoomInSpeed", m_fZoomInSpeed)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.01f, 1000.0f)),
    EZ_MEMBER_PROPERTY("ZoomOutSpeed", m_fZoomOutSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.01f, 1000.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(RotateUp, In, "angle"),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezThirdPersonViewComponent::ezThirdPersonViewComponent() = default;
ezThirdPersonViewComponent::~ezThirdPersonViewComponent() = default;

void ezThirdPersonViewComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  // version 1
  s << m_sTargetObject;
  s << m_vTargetOffsetHigh;
  s << m_vTargetOffsetLow;
  s << m_fMinDistance;
  s << m_fMaxDistance;
  s << m_MinUpRotation;
  s << m_MaxUpRotation;
  s << m_fMaxDistanceUp;
  s << m_fMaxDistanceDown;
  s << m_uiCollisionLayer;
  s << m_fSweepWidth;
  s << m_fZoomInSpeed;
  s << m_fZoomOutSpeed;
}

void ezThirdPersonViewComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  // version 1
  s >> m_sTargetObject;
  s >> m_vTargetOffsetHigh;
  s >> m_vTargetOffsetLow;
  s >> m_fMinDistance;
  s >> m_fMaxDistance;
  s >> m_MinUpRotation;
  s >> m_MaxUpRotation;
  s >> m_fMaxDistanceUp;
  s >> m_fMaxDistanceDown;
  s >> m_uiCollisionLayer;
  s >> m_fSweepWidth;
  s >> m_fZoomInSpeed;
  s >> m_fZoomOutSpeed;
}

void ezThirdPersonViewComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_CurUpRotation = ezMath::Lerp(m_MinUpRotation, m_MaxUpRotation, 0.5f);

  SetTargetObject(m_sTargetObject);
}

void ezThirdPersonViewComponent::SetTargetObject(const char* szTargetObject)
{
  m_sTargetObject = szTargetObject;
  m_hTargetObject.Invalidate();

  if (IsActiveAndSimulating() && !m_sTargetObject.IsEmpty())
  {
    if (ezGameObject* pTargetObject = GetWorld()->SearchForObject(m_sTargetObject, GetOwner()))
    {
      m_hTargetObject = pTargetObject->GetHandle();
    }
  }
}

const char* ezThirdPersonViewComponent::GetTargetObject() const
{
  return m_sTargetObject;
}

void ezThirdPersonViewComponent::RotateUp(ezAngle angle)
{
  m_RotateUp += angle;
}

void ezThirdPersonViewComponent::Update()
{
  if (m_hTargetObject.IsInvalidated())
    return;

  ezGameObject* pTarget = nullptr;
  if (!GetWorld()->TryGetObject(m_hTargetObject, pTarget))
    return;

  const ezTransform tTarget = pTarget->GetGlobalTransform();

  m_CurUpRotation += m_RotateUp;
  m_CurUpRotation = ezMath::Clamp(m_CurUpRotation, m_MinUpRotation, m_MaxUpRotation);
  m_RotateUp = {};

  float fCurMaxDistance = m_fMaxDistance;

  const ezVec3 vOffsetCenter = ezMath::Lerp(m_vTargetOffsetLow, m_vTargetOffsetHigh, 0.5f);
  ezVec3 vOffset = vOffsetCenter;
  if (m_CurUpRotation.GetRadian() > 0)
  {
    const float fLerp = ezMath::Unlerp(0.0f, m_MaxUpRotation.GetRadian(), m_CurUpRotation.GetRadian());
    vOffset = ezMath::Lerp(vOffsetCenter, m_vTargetOffsetHigh, fLerp);
    fCurMaxDistance = ezMath::Lerp(m_fMaxDistance, m_fMaxDistanceUp, fLerp);
  }
  else
  {
    const float fLerp = ezMath::Unlerp(0.0f, m_MinUpRotation.GetRadian(), m_CurUpRotation.GetRadian());
    vOffset = ezMath::Lerp(vOffsetCenter, m_vTargetOffsetLow, fLerp);
    fCurMaxDistance = ezMath::Lerp(m_fMaxDistance, m_fMaxDistanceDown, fLerp);
  }

  const ezVec3 vLookAtPos = tTarget.m_vPosition + tTarget.m_qRotation * vOffset;

  const ezQuat qRotUp = ezQuat::MakeFromAxisAndAngle(ezVec3::MakeAxisY(), m_CurUpRotation);
  const ezQuat qRotTotal = tTarget.m_qRotation * qRotUp;

  const ezVec3 vSweepDir = qRotTotal * ezVec3(-1, 0, 0);
  float fNewDistance = fCurMaxDistance;

  if (ezPhysicsWorldModuleInterface* pPhysics = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>())
  {
    const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();

    ezPhysicsCastResult res1;
    ezPhysicsQueryParameters params;
    params.m_ShapeTypes = ezPhysicsShapeType::Static;
    params.m_uiCollisionLayer = m_uiCollisionLayer;
    if (pPhysics->SweepTestSphere(res1, m_fSweepWidth * 0.5f, vLookAtPos, vSweepDir, fNewDistance, params))
    {
      fNewDistance = ezMath::Max(m_fMinDistance, res1.m_fDistance);
    }

    if (m_fCurDistance > fNewDistance)
    {
      m_fCurDistance = ezMath::Lerp(m_fCurDistance, fNewDistance, ezMath::Min(1.0f, tDiff.AsFloatInSeconds() * m_fZoomInSpeed));
    }
    else
    {
      m_fCurDistance = ezMath::Lerp(m_fCurDistance, fNewDistance, ezMath::Min(1.0f, tDiff.AsFloatInSeconds() * m_fZoomOutSpeed));
    }
  }

  const ezVec3 vLookFromPos = vLookAtPos + m_fCurDistance * vSweepDir;
  GetOwner()->SetGlobalTransformToLookAt(vLookFromPos, vLookAtPos);
}


EZ_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Camera_Implementation_ThirdPersonViewComponent);
