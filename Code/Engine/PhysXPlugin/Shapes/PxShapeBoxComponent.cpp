#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Shapes/PxShapeBoxComponent.h>
#include <PhysXPlugin/PhysXWorldModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxShapeBoxComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Half Extents", m_vHalfExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(0.5f))),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezPxShapeBoxComponent::ezPxShapeBoxComponent()
{
  m_vHalfExtents.Set(0.5f);
}


void ezPxShapeBoxComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();
  s << m_vHalfExtents;
}


void ezPxShapeBoxComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = stream.GetStream();
  s >> m_vHalfExtents;


}

void ezPxShapeBoxComponent::AddToActor(PxRigidActor* pActor, const ezTransform& ParentTransform)
{
  ezPhysXWorldModule* pModule = static_cast<ezPhysXWorldModule*>(GetManager()->GetUserData());

  const ezTransform OwnerTransform = GetOwner()->GetGlobalTransform();

  ezTransform LocalTransform;
  LocalTransform.SetLocalTransform(ParentTransform, OwnerTransform);

  ezQuat r;
  r.SetFromMat3(LocalTransform.m_Rotation);
  
  PxTransform t;
  t.p = PxVec3(LocalTransform.m_vPosition.x, LocalTransform.m_vPosition.y, LocalTransform.m_vPosition.z);
  t.q = PxQuat(r.v.x, r.v.y, r.v.z, r.w);

  auto pShape = pActor->createShape(PxBoxGeometry(m_vHalfExtents.x, m_vHalfExtents.y, m_vHalfExtents.z), *GetPxMaterial());
  pShape->setLocalPose(t);

  EZ_ASSERT_DEBUG(pShape != nullptr, "PhysX box shape creation failed");

  PxFilterData filter;
  filter.word0 = EZ_BIT(m_uiCollisionLayer);
  filter.word1 = ezPhysX::GetSingleton()->GetCollisionFilterConfig().GetFilterMask(m_uiCollisionLayer);
  filter.word2 = 0;
  filter.word3 = 0;
  pShape->setSimulationFilterData(filter);
  pShape->setQueryFilterData(filter);

  pShape->userData = GetOwner();
}
