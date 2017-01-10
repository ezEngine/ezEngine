#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Components/PxCenterOfMassComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>


ezPxDynamicActorComponentManager::ezPxDynamicActorComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezPxDynamicActorComponent, true>(pWorld)
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

  const float fInvDeltaTime = 1.0f / (float)(GetWorld()->GetClock().GetTimeDiff().GetSeconds());

  for (auto& activeTransform : activeTransforms)
  {
    ezPxDynamicActorComponent* pComponent = ezPxUserData::GetDynamicActorComponent(activeTransform.userData);
    if (pComponent == nullptr || pComponent->GetKinematic())
      continue;

    ezGameObject* pObject = pComponent->GetOwner();
    pObject->SetGlobalTransform(ezPxConversionUtils::ToTransform(activeTransform.actor2World));
    pObject->SetVelocity(ezPxConversionUtils::ToVec3(activeTransform.actor->isRigidDynamic()->getLinearVelocity()));
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezPxDynamicActorComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Kinematic", GetKinematic, SetKinematic),
    EZ_MEMBER_PROPERTY("Mass", m_fMass),
    EZ_MEMBER_PROPERTY("Density", m_fDensity)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("DisableGravity", GetDisableGravity, SetDisableGravity),
    EZ_MEMBER_PROPERTY("LinearDamping", m_fLinearDamping)->AddAttributes(new ezDefaultValueAttribute(0.1f)),
    EZ_MEMBER_PROPERTY("AngularDamping", m_fAngularDamping)->AddAttributes(new ezDefaultValueAttribute(0.05f)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

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

  if (m_bKinematic)
  {
    GetManager()->m_KinematicActorComponents.PushBack(this);
  }
  else
  {
    GetManager()->m_KinematicActorComponents.RemoveSwap(this);
  }

  if (m_pActor)
  {
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
    m_pActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, m_bDisableGravity);
  }
}

void ezPxDynamicActorComponent::OnSimulationStarted()
{
  if (!IsActive())
    return;

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  const auto pos = GetOwner()->GetGlobalPosition();
  const auto rot = GetOwner()->GetGlobalRotation();

  PxTransform t = PxTransform::createIdentity();
  t.p = PxVec3(pos.x, pos.y, pos.z);
  t.q = PxQuat(rot.v.x, rot.v.y, rot.v.z, rot.w);
  m_pActor = ezPhysX::GetSingleton()->GetPhysXAPI()->createRigidDynamic(t);
  EZ_ASSERT_DEBUG(m_pActor != nullptr, "PhysX actor creation failed");

  m_pActor->userData = &m_UserData;

  AddShapesFromObject(GetOwner(), m_pActor, GetOwner()->GetGlobalTransform());

  m_pActor->setLinearDamping(ezMath::Clamp(m_fLinearDamping, 0.0f, 1000.0f));
  m_pActor->setAngularDamping(ezMath::Clamp(m_fAngularDamping, 0.0f, 1000.0f));

  if (m_pActor->getNbShapes() == 0)
  {
    m_pActor->release();
    m_pActor = nullptr;

    ezLog::Error("Rigid Body '{0}' does not have any shape components. Actor will be removed.", GetOwner()->GetName());
    return;
  }

  ezVec3 vCenterOfMass(0.0f);
  if (FindCenterOfMass(GetOwner(), vCenterOfMass))
  {
    ezMat4 mTransform = GetOwner()->GetGlobalTransform().GetAsMat4();
    mTransform.Invert();

    vCenterOfMass = mTransform.TransformPosition(vCenterOfMass);
  }

  if (m_fMass > 0.0f)
  {
    PxRigidBodyExt::setMassAndUpdateInertia(*m_pActor, m_fMass, reinterpret_cast<PxVec3*>(&vCenterOfMass));
  }
  else if (m_fDensity > 0.0f)
  {
    PxRigidBodyExt::updateMassAndInertia(*m_pActor, m_fDensity, reinterpret_cast<PxVec3*>(&vCenterOfMass));
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
    EZ_PX_WRITE_LOCK(*(m_pActor->getScene()));

    m_pActor->release();
    m_pActor = nullptr;
  }
}

ezVec3 ezPxDynamicActorComponent::GetLocalCenterOfMass() const
{
  if (m_pActor != nullptr)
  {
    return ezPxConversionUtils::ToVec3(m_pActor->getCMassLocalPose().p);
  }

  return ezVec3::ZeroVector();
}

ezVec3 ezPxDynamicActorComponent::GetGlobalCenterOfMass() const
{
  if (m_pActor != nullptr)
  {
    const PxTransform globalPose = m_pActor->getGlobalPose();
    return ezPxConversionUtils::ToVec3(globalPose.transform(m_pActor->getCMassLocalPose().p));
  }

  return ezVec3::ZeroVector();
}

void ezPxDynamicActorComponent::AddLinearForce(const ezVec3& vForce)
{
  if (m_pActor != nullptr)
  {
    m_pActor->addForce(ezPxConversionUtils::ToVec3(vForce), PxForceMode::eFORCE);
  }
}

void ezPxDynamicActorComponent::AddLinearImpulse(const ezVec3& vImpulse)
{
  if (m_pActor != nullptr)
  {
    m_pActor->addForce(ezPxConversionUtils::ToVec3(vImpulse), PxForceMode::eIMPULSE);
  }
}

void ezPxDynamicActorComponent::AddAngularForce(const ezVec3& vForce)
{
  if (m_pActor != nullptr)
  {
    m_pActor->addTorque(ezPxConversionUtils::ToVec3(vForce), PxForceMode::eFORCE);
  }
}

void ezPxDynamicActorComponent::AddAngularImpulse(const ezVec3& vImpulse)
{
  if (m_pActor != nullptr)
  {
    m_pActor->addTorque(ezPxConversionUtils::ToVec3(vImpulse), PxForceMode::eIMPULSE);
  }
}

void ezPxDynamicActorComponent::AddForceAtPos(const ezVec3& vForce, const ezVec3& vPos)
{
  if (m_pActor != nullptr)
  {
    ezPhysX::addForceAtPos(*m_pActor, ezPxConversionUtils::ToVec3(vForce), ezPxConversionUtils::ToVec3(vPos), PxForceMode::eFORCE);
  }
}

void ezPxDynamicActorComponent::AddImpulseAtPos(const ezVec3& vImpulse, const ezVec3& vPos)
{
  if (m_pActor != nullptr)
  {
    ezPhysX::addForceAtPos(*m_pActor, ezPxConversionUtils::ToVec3(vImpulse), ezPxConversionUtils::ToVec3(vPos), PxForceMode::eIMPULSE);
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

