#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Shapes/PxShapeCapsuleComponent.h>
#include <PhysXPlugin/PhysXWorldModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxShapeCapsuleComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.1f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezUpdateLocalBoundsMessage, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCapsuleManipulatorAttribute("Height", "Radius"),
    new ezCapsuleVisualizerAttribute("Height", "Radius"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPxShapeCapsuleComponent::ezPxShapeCapsuleComponent()
{
  m_fRadius = 0.5f;
  m_fHeight = 0.5f;
}


void ezPxShapeCapsuleComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();
  s << m_fRadius;
  s << m_fHeight;
}


void ezPxShapeCapsuleComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = stream.GetStream();
  s >> m_fRadius;
  s >> m_fHeight;
}


void ezPxShapeCapsuleComponent::OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const
{
  msg.m_ResultingLocalBounds.ExpandToInclude(ezBoundingSphere(ezVec3(0, 0, -m_fHeight * 0.5f), m_fRadius));
  msg.m_ResultingLocalBounds.ExpandToInclude(ezBoundingSphere(ezVec3(0, 0, +m_fHeight * 0.5f), m_fRadius));
}


void ezPxShapeCapsuleComponent::SetRadius(float value)
{
  m_fRadius = ezMath::Max(0.0f, value);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezPxShapeCapsuleComponent::SetHeight(float value)
{
  m_fHeight = ezMath::Max(0.0f, value);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezPxShapeCapsuleComponent::AddToActor(PxRigidActor* pActor, const ezTransform& ParentTransform)
{
  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

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

  auto pShape = pActor->createShape(PxCapsuleGeometry(m_fRadius, m_fHeight * 0.5f), *GetPxMaterial());
  pShape->setLocalPose(t);

  EZ_ASSERT_DEBUG(pShape != nullptr, "PhysX capsule shape creation failed");

  PxFilterData filter;
  filter.word0 = EZ_BIT(m_uiCollisionLayer);
  filter.word1 = ezPhysX::GetSingleton()->GetCollisionFilterConfig().GetFilterMask(m_uiCollisionLayer);
  filter.word2 = 0;
  filter.word3 = 0;
  pShape->setSimulationFilterData(filter);
  pShape->setQueryFilterData(filter);

  pShape->userData = GetOwner();
}



