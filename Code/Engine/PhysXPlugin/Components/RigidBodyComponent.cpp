#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/RigidBodyComponent.h>
#include <PhysXPlugin/PhysXSceneModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxRigidBodyComponent, 1);
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezPxRigidBodyComponent::ezPxRigidBodyComponent()
{
}


void ezPxRigidBodyComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

}


void ezPxRigidBodyComponent::DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion)
{
  SUPER::DeserializeComponent(stream, uiTypeVersion);



}


void ezPxRigidBodyComponent::Update()
{
  if (m_pActor == nullptr)
    return;

  const auto pose = m_pActor->getGlobalPose();

  ezQuat qRot(pose.q.x, pose.q.y, pose.q.z, pose.q.w);

  GetOwner()->SetGlobalTransform(ezTransform(ezVec3(pose.p.x, pose.p.y, pose.p.z), qRot));
}

ezResult ezPxRigidBodyComponent::Initialize()
{
  ezPhysXSceneModule* pModule = static_cast<ezPhysXSceneModule*>(GetManager()->GetUserData());

  const auto pos = GetOwner()->GetGlobalPosition();
  const auto rot = GetOwner()->GetGlobalRotation();

  PxTransform t = PxTransform::createIdentity();
  t.p = PxVec3(pos.x, pos.y, pos.z);
  t.q = PxQuat(rot.v.x, rot.v.y, rot.v.z, rot.w);
  m_pActor = pModule->GetPxApi()->createRigidDynamic(t);
  EZ_ASSERT_DEBUG(m_pActor != nullptr, "PhysX actor creation failed");

  PxRigidBodyExt::updateMassAndInertia(*m_pActor, 1.0f);

  pModule->GetPxScene()->addActor(*m_pActor);

  auto pShape = m_pActor->createShape(PxBoxGeometry(0.5f, 0.5f, 0.5f), *pModule->GetDefaultMaterial());
  //auto pShape = m_pActor->createShape(PxSphereGeometry(0.5f), *pModule->GetDefaultMaterial());
  EZ_ASSERT_DEBUG(pShape != nullptr, "PhysX shape creation failed");

  return EZ_SUCCESS;
}

ezResult ezPxRigidBodyComponent::Deinitialize()
{
  m_pActor->release();
  m_pActor = nullptr;

  return EZ_SUCCESS;
}

