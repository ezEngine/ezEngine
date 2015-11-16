#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/StaticMeshComponent.h>
#include <PhysXPlugin/PhysXSceneModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxStaticMeshComponent, 1);
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezPxStaticMeshComponent::ezPxStaticMeshComponent()
{
  m_pActor = nullptr;
}


void ezPxStaticMeshComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

}


void ezPxStaticMeshComponent::DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion)
{
  SUPER::DeserializeComponent(stream, uiTypeVersion);



}

ezResult ezPxStaticMeshComponent::Initialize()
{
  ezPhysXSceneModule* pModule = static_cast<ezPhysXSceneModule*>(GetManager()->GetUserData());

  const auto pos = GetOwner()->GetGlobalPosition();
  const auto rot = GetOwner()->GetGlobalRotation();

  PxTransform t = PxTransform::createIdentity();
  t.p = PxVec3(pos.x, pos.y, pos.z);
  t.q = PxQuat(rot.v.x, rot.v.y, rot.v.z, rot.w);
  m_pActor = pModule->GetPxApi()->createRigidStatic(t);
  EZ_ASSERT_DEBUG(m_pActor != nullptr, "PhysX actor creation failed");

  ezQuat qRot;
  qRot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(270));

  auto pShape = m_pActor->createShape(PxPlaneGeometry(), *pModule->GetDefaultMaterial());
  pShape->setLocalPose(PxTransform(PxQuat(qRot.v.x, qRot.v.y, qRot.v.z, qRot.w)));
  pModule->GetPxScene()->addActor(*m_pActor);
  return EZ_SUCCESS;
}

ezResult ezPxStaticMeshComponent::Deinitialize()
{
  ezPhysXSceneModule* pModule = static_cast<ezPhysXSceneModule*>(GetManager()->GetUserData());

  m_pActor->release();
  m_pActor = nullptr;

  return EZ_SUCCESS;
}

