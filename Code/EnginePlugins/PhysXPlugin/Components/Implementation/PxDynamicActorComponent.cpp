#include <PhysXPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <PhysXPlugin/Components/BreakableSheet.h>
#include <PhysXPlugin/Components/PxCenterOfMassComponent.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>

using namespace physx;

ezPxDynamicActorComponentManager::ezPxDynamicActorComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezPxDynamicActorComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

ezPxDynamicActorComponentManager::~ezPxDynamicActorComponentManager() {}

void ezPxDynamicActorComponentManager::UpdateKinematicActors()
{
  EZ_PROFILE_SCOPE("KinematicActors");

  for (auto pKinematicActorComponent : m_KinematicActorComponents)
  {
    if (PxRigidDynamic* pActor = pKinematicActorComponent->GetPxActor())
    {
      ezGameObject* pObject = pKinematicActorComponent->GetOwner();

      pObject->UpdateGlobalTransform();

      const ezVec3 pos = pObject->GetGlobalPosition();
      const ezQuat rot = pObject->GetGlobalRotation();

      PxTransform t = ezPxConversionUtils::ToTransform(pos, rot);
      pActor->setKinematicTarget(t);
    }
  }
}

void ezPxDynamicActorComponentManager::UpdateDynamicActors(ezArrayPtr<PxActor*> activeActors)
{
  EZ_PROFILE_SCOPE("DynamicActors");

  for (PxActor* activeActor : activeActors)
  {
    if (activeActor->getType() != PxActorType::eRIGID_DYNAMIC)
      continue;

    PxRigidDynamic* dynamicActor = static_cast<PxRigidDynamic*>(activeActor);

    ezPxDynamicActorComponent* pComponent = ezPxUserData::GetDynamicActorComponent(activeActor->userData);
    if (pComponent == nullptr)
    {
      // Check if this is a breakable sheet component piece
      if (ezBreakableSheetComponent* pSheetComponent = ezPxUserData::GetBreakableSheetComponent(activeActor->userData))
      {
        pSheetComponent->SetPieceTransform(dynamicActor->getGlobalPose(), ezPxUserData::GetAdditionalUserData(activeActor->userData));
      }

      continue;
    }

    if (pComponent->GetKinematic())
      continue;

    auto pose = dynamicActor->getGlobalPose();
    if (!pose.isSane())
    {
      // PhysX can completely fuck up poses and never recover
      // if that happens, force a non-NaN pose to prevent crashes down the line
      dynamicActor->setGlobalPose(ezPxConversionUtils::ToTransform(pComponent->GetOwner()->GetGlobalTransformSimd()));

      // ignore objects with bad data
      continue;
    }

    ezGameObject* pObject = pComponent->GetOwner();
    EZ_ASSERT_DEV(pObject != nullptr, "Owner must be still valid");

    // preserve scaling
    ezSimdTransform t = ezPxConversionUtils::ToSimdTransform(pose);
    t.m_Scale = ezSimdConversion::ToVec3(pObject->GetGlobalScaling());

    pObject->SetGlobalTransform(t);
  }
}

void ezPxDynamicActorComponentManager::UpdateMaxDepenetrationVelocity(float fMaxVelocity)
{
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    physx::PxRigidDynamic* pActor = it->GetPxActor();

    if (pActor != nullptr)
    {
      pActor->setMaxDepenetrationVelocity(fMaxVelocity);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxDynamicActorComponent, 3, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
      EZ_ACCESSOR_PROPERTY("Kinematic", GetKinematic, SetKinematic),
      EZ_ACCESSOR_PROPERTY("Mass", GetMass, SetMass)->AddAttributes(new ezSuffixAttribute(" kg")),
      EZ_MEMBER_PROPERTY("Density", m_fDensity)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezSuffixAttribute(" kg/m^3")),
      EZ_ACCESSOR_PROPERTY("DisableGravity", GetDisableGravity, SetDisableGravity),
      EZ_MEMBER_PROPERTY("LinearDamping", m_fLinearDamping)->AddAttributes(new ezDefaultValueAttribute(0.2f)),
      EZ_MEMBER_PROPERTY("AngularDamping", m_fAngularDamping)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
      EZ_MEMBER_PROPERTY("MaxContactImpulse", m_fMaxContactImpulse)->AddAttributes(new ezDefaultValueAttribute(100000.0f), new ezClampValueAttribute(0.0f, ezVariant())),
      EZ_ACCESSOR_PROPERTY("ContinuousCollisionDetection", GetContinuousCollisionDetection, SetContinuousCollisionDetection)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
      EZ_MESSAGE_HANDLER(ezMsgPhysicsAddForce, AddForceAtPos),
      EZ_MESSAGE_HANDLER(ezMsgPhysicsAddImpulse, AddImpulseAtPos),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(AddLinearForce, In, "vForce"),
    EZ_SCRIPT_FUNCTION_PROPERTY(AddLinearImpulse, In, "vImpulse"),
    EZ_SCRIPT_FUNCTION_PROPERTY(AddAngularForce, In, "vForce"),
    EZ_SCRIPT_FUNCTION_PROPERTY(AddAngularImpulse, In, "vImpulse"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetLocalCenterOfMass),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetGlobalCenterOfMass),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezPxDynamicActorComponent::ezPxDynamicActorComponent() = default;
ezPxDynamicActorComponent::~ezPxDynamicActorComponent() = default;

void ezPxDynamicActorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_bKinematic;
  s << m_fDensity;
  s << m_fMass;
  s << m_bDisableGravity;
  s << m_fLinearDamping;
  s << m_fAngularDamping;
  s << m_fMaxContactImpulse;
  s << m_bCCD;
}

void ezPxDynamicActorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_bKinematic;
  s >> m_fDensity;
  s >> m_fMass;
  s >> m_bDisableGravity;

  if (uiVersion >= 2)
  {
    s >> m_fLinearDamping;
    s >> m_fAngularDamping;
    s >> m_fMaxContactImpulse;
  }

  if (uiVersion >= 3)
  {
    s >> m_bCCD;
  }
}

void ezPxDynamicActorComponent::SetKinematic(bool b)
{
  if (m_bKinematic == b)
    return;

  m_bKinematic = b;

  if (m_bKinematic && m_pActor)
  {
    // do not insert this, until we actually have an actor pointer
    GetWorld()->GetOrCreateComponentManager<ezPxDynamicActorComponentManager>()->m_KinematicActorComponents.PushBack(this);
  }
  else
  {
    GetWorld()->GetOrCreateComponentManager<ezPxDynamicActorComponentManager>()->m_KinematicActorComponents.RemoveAndSwap(this);
  }

  if (m_pActor)
  {
    EZ_PX_WRITE_LOCK(*(m_pActor->getScene()));

    m_pActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, m_bKinematic);
  }
}

void ezPxDynamicActorComponent::SetDisableGravity(bool b)
{
  if (m_bDisableGravity == b)
    return;

  m_bDisableGravity = b;

  if (m_pActor)
  {
    EZ_PX_WRITE_LOCK(*(m_pActor->getScene()));

    m_pActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, m_bDisableGravity);
    m_pActor->wakeUp();
  }
}

void ezPxDynamicActorComponent::SetContinuousCollisionDetection(bool b)
{
  if (m_bCCD == b)
    return;

  m_bCCD = b;

  if (m_pActor)
  {
    EZ_PX_WRITE_LOCK(*(m_pActor->getScene()));

    m_pActor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, m_bCCD);
    m_pActor->wakeUp();
  }
}

void ezPxDynamicActorComponent::SetMass(float fMass)
{
  if (m_fMass == fMass)
    return;

  m_fMass = ezMath::Max(fMass, 0.0f);

  if (m_pActor)
  {
    EZ_PX_WRITE_LOCK(*(m_pActor->getScene()));

    m_pActor->setMass(m_fMass);
    m_pActor->wakeUp();
  }
}

void ezPxDynamicActorComponent::OnSimulationStarted()
{
  if (!IsActive())
    return;

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  const ezSimdTransform& globalTransform = GetOwner()->GetGlobalTransformSimd();

  PxTransform t = ezPxConversionUtils::ToTransform(globalTransform);
  m_pActor = ezPhysX::GetSingleton()->GetPhysXAPI()->createRigidDynamic(t);
  EZ_ASSERT_DEBUG(m_pActor != nullptr, "PhysX actor creation failed");

  ezPxUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);
  m_pActor->userData = pUserData;

  // PhysX does not get any scale value, so to correctly position child objects
  // we have to pretend that this parent object applies no scale on its children
  ezSimdTransform globalTransformNoScale = globalTransform;
  globalTransformNoScale.m_Scale.Set(1.0f);
  AddShapesFromObject(GetOwner(), m_pActor, globalTransformNoScale);

  if (m_pActor->getNbShapes() == 0 && !m_bKinematic)
  {
    m_pActor->release();
    m_pActor = nullptr;

    ezLog::Error("Rigid Body '{0}' does not have any shape components. Actor will be removed.", GetOwner()->GetName());
    return;
  }

  m_pActor->setLinearDamping(ezMath::Clamp(m_fLinearDamping, 0.0f, 1000.0f));
  m_pActor->setAngularDamping(ezMath::Clamp(m_fAngularDamping, 0.0f, 1000.0f));
  m_pActor->setMaxDepenetrationVelocity(pModule->GetMaxDepenetrationVelocity());
  m_pActor->setMaxContactImpulse(ezMath::Clamp(m_fMaxContactImpulse, 0.0f, ezMath::MaxValue<float>()));

  ezVec3 vCenterOfMass(0.0f);
  if (FindCenterOfMass(GetOwner(), vCenterOfMass))
  {
    ezSimdTransform CoMTransform = globalTransformNoScale;
    CoMTransform.Invert();

    vCenterOfMass = ezSimdConversion::ToVec3(CoMTransform.TransformPosition(ezSimdConversion::ToVec3(vCenterOfMass)));
  }

  PxVec3 pxCoM = ezPxConversionUtils::ToVec3(vCenterOfMass);

  if (m_fMass > 0.0f)
  {
    PxRigidBodyExt::setMassAndUpdateInertia(*m_pActor, m_fMass, &pxCoM);
  }
  else if (m_fDensity > 0.0f)
  {
    PxRigidBodyExt::updateMassAndInertia(*m_pActor, m_fDensity, &pxCoM);

    m_fMass = m_pActor->getMass();
  }
  else
  {
    ezLog::Warning("Rigid Body '{0}' neither has mass nor density set to valid values.", GetOwner()->GetName());
    PxRigidBodyExt::updateMassAndInertia(*m_pActor, 1.0f);
  }

  m_pActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, m_bDisableGravity);
  m_pActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, m_bKinematic);
  m_pActor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, m_bCCD);

  if (m_bKinematic)
  {
    GetWorld()->GetOrCreateComponentManager<ezPxDynamicActorComponentManager>()->m_KinematicActorComponents.PushBack(this);
  }

  {
    EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));
    pModule->GetPxScene()->addActor(*m_pActor);
  }
}

void ezPxDynamicActorComponent::OnDeactivated()
{
  if (m_bKinematic)
  {
    GetWorld()->GetOrCreateComponentManager<ezPxDynamicActorComponentManager>()->m_KinematicActorComponents.RemoveAndSwap(this);
  }

  if (m_pActor)
  {
    ezPhysXWorldModule* pModule = GetWorld()->GetModule<ezPhysXWorldModule>();

    if (pModule->GetPxScene())
    {
      EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));

      m_pActor->release();
    }

    m_pActor = nullptr;

    pModule->DeallocateUserData(m_uiUserDataIndex);
  }

  SUPER::OnDeactivated();
}

ezVec3 ezPxDynamicActorComponent::GetLocalCenterOfMass() const
{
  if (m_pActor != nullptr)
  {
    EZ_PX_READ_LOCK(*(m_pActor->getScene()));

    return ezPxConversionUtils::ToVec3(m_pActor->getCMassLocalPose().p);
  }

  return ezVec3::ZeroVector();
}

ezVec3 ezPxDynamicActorComponent::GetGlobalCenterOfMass() const
{
  if (m_pActor != nullptr)
  {
    EZ_PX_READ_LOCK(*(m_pActor->getScene()));

    const PxTransform globalPose = m_pActor->getGlobalPose();
    return ezPxConversionUtils::ToVec3(globalPose.transform(m_pActor->getCMassLocalPose().p));
  }

  return ezVec3::ZeroVector();
}

void ezPxDynamicActorComponent::AddLinearForce(const ezVec3& vForce)
{
  if (m_pActor != nullptr && !m_bKinematic)
  {
    EZ_PX_WRITE_LOCK(*(m_pActor->getScene()));

    m_pActor->addForce(ezPxConversionUtils::ToVec3(vForce), PxForceMode::eFORCE);
  }
}

void ezPxDynamicActorComponent::AddLinearImpulse(const ezVec3& vImpulse)
{
  if (m_pActor != nullptr && !m_bKinematic)
  {
    EZ_PX_WRITE_LOCK(*(m_pActor->getScene()));

    m_pActor->addForce(ezPxConversionUtils::ToVec3(vImpulse), PxForceMode::eIMPULSE);
  }
}

void ezPxDynamicActorComponent::AddAngularForce(const ezVec3& vForce)
{
  if (m_pActor != nullptr && !m_bKinematic)
  {
    EZ_PX_WRITE_LOCK(*(m_pActor->getScene()));

    m_pActor->addTorque(ezPxConversionUtils::ToVec3(vForce), PxForceMode::eFORCE);
  }
}

void ezPxDynamicActorComponent::AddAngularImpulse(const ezVec3& vImpulse)
{
  if (m_pActor != nullptr && !m_bKinematic)
  {
    EZ_PX_WRITE_LOCK(*(m_pActor->getScene()));

    m_pActor->addTorque(ezPxConversionUtils::ToVec3(vImpulse), PxForceMode::eIMPULSE);
  }
}

void ezPxDynamicActorComponent::AddForceAtPos(ezMsgPhysicsAddForce& msg)
{
  if (m_pActor != nullptr && !m_bKinematic)
  {
    EZ_PX_WRITE_LOCK(*m_pActor->getScene());

    PxRigidBodyExt::addForceAtPos(
      *m_pActor, ezPxConversionUtils::ToVec3(msg.m_vForce), ezPxConversionUtils::ToVec3(msg.m_vGlobalPosition), PxForceMode::eFORCE);
  }
}

void ezPxDynamicActorComponent::AddImpulseAtPos(ezMsgPhysicsAddImpulse& msg)
{
  if (m_pActor != nullptr && !m_bKinematic)
  {
    EZ_PX_WRITE_LOCK(*m_pActor->getScene());

    PxRigidBodyExt::addForceAtPos(
      *m_pActor, ezPxConversionUtils::ToVec3(msg.m_vImpulse), ezPxConversionUtils::ToVec3(msg.m_vGlobalPosition), PxForceMode::eIMPULSE);
  }
}

bool ezPxDynamicActorComponent::FindCenterOfMass(ezGameObject* pRoot, ezVec3& out_CoM) const
{
  ezPxCenterOfMassComponent* pCOM;
  if (pRoot->TryGetComponentOfBaseType<ezPxCenterOfMassComponent>(pCOM))
  {
    out_CoM = pRoot->GetGlobalPosition();
    return true;
  }
  else
  {
    auto it = pRoot->GetChildren();

    while (it.IsValid())
    {
      if (FindCenterOfMass(it, out_CoM))
        return true;

      ++it;
    }
  }

  return false;
}



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Components_Implementation_PxDynamicActorComponent);
