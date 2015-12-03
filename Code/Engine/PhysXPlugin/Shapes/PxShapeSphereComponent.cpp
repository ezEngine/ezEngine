#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Shapes/PxShapeSphereComponent.h>
#include <PhysXPlugin/PhysXSceneModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxShapeSphereComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.5f)),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezPxShapeSphereComponent::ezPxShapeSphereComponent()
{
  m_fRadius = 0.5f;
}


void ezPxShapeSphereComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();
  s << m_fRadius;
}


void ezPxShapeSphereComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();
  s >> m_fRadius;

}

void ezPxShapeSphereComponent::AddToActor(PxRigidActor* pActor, const ezTransform& ParentTransform)
{
  ezPhysXSceneModule* pModule = static_cast<ezPhysXSceneModule*>(GetManager()->GetUserData());

  const ezTransform OwnerTransform = GetOwner()->GetGlobalTransform();

  ezTransform LocalTransform;
  LocalTransform.SetLocalTransform(ParentTransform, OwnerTransform);

  ezQuat r;
  r.SetFromMat3(LocalTransform.m_Rotation);

  PxTransform t;
  t.p = PxVec3(LocalTransform.m_vPosition.x, LocalTransform.m_vPosition.y, LocalTransform.m_vPosition.z);
  t.q = PxQuat(r.v.x, r.v.y, r.v.z, r.w);

  auto pShape = pActor->createShape(PxSphereGeometry(m_fRadius), *GetPxMaterial());
  pShape->setLocalPose(t);

  EZ_ASSERT_DEBUG(pShape != nullptr, "PhysX sphere shape creation failed");

  PxFilterData filter;
  filter.word0 = EZ_BIT(m_uiCollisionLayer);
  filter.word1 = ezPhysX::GetSingleton()->GetCollisionFilterConfig().GetFilterMask(m_uiCollisionLayer);
  filter.word2 = 0;
  filter.word3 = 0;
  pShape->setSimulationFilterData(filter);
  pShape->setQueryFilterData(filter);

  pShape->userData = GetOwner();
}
