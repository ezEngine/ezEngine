#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/PxShapeCapsuleComponent.h>
#include <PhysXPlugin/PhysXSceneModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxShapeCapsuleComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.5f)),
    EZ_MEMBER_PROPERTY("Half Height", m_fHalfHeight)->AddAttributes(new ezDefaultValueAttribute(0.25f)),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezPxShapeCapsuleComponent::ezPxShapeCapsuleComponent()
{
  m_fRadius = 0.5f;
  m_fHalfHeight = 0.25f;
}


void ezPxShapeCapsuleComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();
  s << m_fRadius;
  s << m_fHalfHeight;
}


void ezPxShapeCapsuleComponent::DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion)
{
  SUPER::DeserializeComponent(stream, uiTypeVersion);

  auto& s = stream.GetStream();
  s >> m_fRadius;
  s >> m_fHalfHeight;
}

void ezPxShapeCapsuleComponent::AddToActor(PxRigidActor* pActor, const ezTransform& ParentTransform)
{
  ezPhysXSceneModule* pModule = static_cast<ezPhysXSceneModule*>(GetManager()->GetUserData());

  const ezTransform OwnerTransform = GetOwner()->GetGlobalTransform();

  ezTransform LocalTransform;
  LocalTransform.SetLocalTransform(ParentTransform, OwnerTransform);

  ezQuat r;
  r.SetFromMat3(LocalTransform.m_Rotation);

  ezQuat qRot;
  qRot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(270));

  r = qRot * r;

  PxTransform t;
  t.p = PxVec3(LocalTransform.m_vPosition.x, LocalTransform.m_vPosition.y, LocalTransform.m_vPosition.z);
  t.q = PxQuat(r.v.x, r.v.y, r.v.z, r.w);

  /// \todo Material
  auto pShape = pActor->createShape(PxCapsuleGeometry(m_fRadius, m_fHalfHeight), *ezPhysX::GetSingleton()->GetDefaultMaterial());
  pShape->setLocalPose(t);

  EZ_ASSERT_DEBUG(pShape != nullptr, "PhysX capsule shape creation failed");
}



