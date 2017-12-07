#include <PCH.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Components/PxCenterOfMassComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>

using namespace physx;

ezPxDynamicActorComponentManager::ezPxDynamicActorComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezPxDynamicActorComponent, ezBlockStorageType::FreeList>(pWorld)
{

}

ezPxDynamicActorComponentManager::~ezPxDynamicActorComponentManager()
{

}

void ezPxDynamicActorComponentManager::UpdateKinematicActors()
{
  EZ_PROFILE("KinematicActors");

  for (auto pKinematicActorComponent : m_KinematicActorComponents)
  {
    if (PxRigidDynamic* pActor = pKinematicActorComponent->GetActor())
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

void ezPxDynamicActorComponentManager::UpdateDynamicActors(ezArrayPtr<const PxActiveTransform> activeTransforms)
{
  EZ_PROFILE("DynamicActors");

  for (auto& activeTransform : activeTransforms)
  {
    ezPxDynamicActorComponent* pComponent = ezPxUserData::GetDynamicActorComponent(activeTransform.userData);
    if (pComponent == nullptr)
    {
      // Check if this is a breakable sheet component piece
      ezBreakableSheetComponent* pSheetComponent = ezPxUserData::GetBreakableSheetComponent(activeTransform.userData);
      if (pSheetComponent)
      {
        pSheetComponent->SetPieceTransform(activeTransform.actor2World, ezPxUserData::GetAdditionalUserData(activeTransform.userData));
      }

      continue;
    }

    if(pComponent->GetKinematic())
      continue;

    ezGameObject* pObject = pComponent->GetOwner();

    // this can happen when the object was deleted in the meantime
    if (pObject == nullptr)
      continue;

    // preserve scaling
    ezSimdTransform t = ezPxConversionUtils::ToSimdTransform(activeTransform.actor2World);
    t.m_Scale = ezSimdConversion::ToVec3(pObject->GetGlobalScaling());

    pObject->SetGlobalTransform(t);
  }
}

void ezPxDynamicActorComponentManager::UpdateMaxDepenetrationVelocity(float fMaxVelocity)
{
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    physx::PxRigidDynamic* pActor = it->GetActor();

    if (pActor != nullptr)
    {
      pActor->setMaxDepenetrationVelocity(fMaxVelocity);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezPxDynamicActorComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Kinematic", GetKinematic, SetKinematic),
    EZ_ACCESSOR_PROPERTY("Mass", GetMass, SetMass),
    EZ_MEMBER_PROPERTY("Density", m_fDensity)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("DisableGravity", GetDisableGravity, SetDisableGravity),
    EZ_MEMBER_PROPERTY("LinearDamping", m_fLinearDamping)->AddAttributes(new ezDefaultValueAttribute(0.1f)),
    EZ_MEMBER_PROPERTY("AngularDamping", m_fAngularDamping)->AddAttributes(new ezDefaultValueAttribute(0.05f)),
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezPhysicsAddForceMsg, AddForceAtPos),
    EZ_MESSAGE_HANDLER(ezPhysicsAddImpulseMsg, AddImpulseAtPos),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_COMPONENT_TYPE

ezPxDynamicActorComponent::ezPxDynamicActorComponent()
  : m_UserData(this)
{
  m_bKinematic = false;
  m_bDisableGravity = false;
  m_pActor = nullptr;

  m_fLinearDamping = 0.1f;
  m_fAngularDamping = 0.05f;
  m_fDensity = 1.0f;
  m_fMass = 0.0f;
}

void ezPxDynamicActorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_bKinematic;
  s << m_fDensity;
  s << m_fMass;
  s << m_bDisableGravity;
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
}

void ezPxDynamicActorComponent::SetKinematic(bool b)
{
  if (m_bKinematic == b)
    return;

  m_bKinematic = b;

  if (m_bKinematic && m_pActor)
  {
    // do not insert this, until we actually have an actor pointer
    GetManager()->m_KinematicActorComponents.PushBack(this);
  }
  else
  {
    GetManager()->m_KinematicActorComponents.RemoveSwap(this);
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

void ezPxDynamicActorComponent::SetMass(float fMass)
{
  if (m_fMass == fMass)
    return;

  m_fMass = fMass;

  if (m_pActor)
  {
    EZ_PX_WRITE_LOCK(*(m_pActor->getScene()));

    m_pActor->setMass(m_fMass);
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

  m_pActor->userData = &m_UserData;

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
  }
  else
  {
    ezLog::Warning("Rigid Body '{0}' neither has mass nor density set to valid values.", GetOwner()->GetName());
    PxRigidBodyExt::updateMassAndInertia(*m_pActor, 1.0f);
  }

  m_pActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, m_bDisableGravity);
  m_pActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, m_bKinematic);

  if (m_bKinematic)
  {
    GetManager()->m_KinematicActorComponents.PushBack(this);
  }

  {
    EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));
    pModule->GetPxScene()->addActor(*m_pActor);
  }
}

void ezPxDynamicActorComponent::Deinitialize()
{
  if (m_bKinematic)
  {
    GetManager()->m_KinematicActorComponents.RemoveSwap(this);
  }

  if (m_pActor)
  {
    ezPhysXWorldModule* pModule = GetWorld()->GetModule<ezPhysXWorldModule>();
    EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));

    m_pActor->release();
    m_pActor = nullptr;
  }
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

void ezPxDynamicActorComponent::AddForceAtPos(ezPhysicsAddForceMsg& msg)
{
  if (m_pActor != nullptr && !m_bKinematic)
  {
    EZ_PX_WRITE_LOCK(*m_pActor->getScene());

    PxRigidBodyExt::addForceAtPos(*m_pActor, ezPxConversionUtils::ToVec3(msg.m_vForce), ezPxConversionUtils::ToVec3(msg.m_vGlobalPosition), PxForceMode::eFORCE);
  }
}

void ezPxDynamicActorComponent::AddImpulseAtPos(ezPhysicsAddImpulseMsg& msg)
{
  if (m_pActor != nullptr && !m_bKinematic)
  {
    EZ_PX_WRITE_LOCK(*m_pActor->getScene());

    PxRigidBodyExt::addForceAtPos(*m_pActor, ezPxConversionUtils::ToVec3(msg.m_vImpulse), ezPxConversionUtils::ToVec3(msg.m_vGlobalPosition), PxForceMode::eIMPULSE);
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

