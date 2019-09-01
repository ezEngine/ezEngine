#include <PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Shapes/PxShapeCapsuleComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxShapeCapsuleComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.01f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCapsuleManipulatorAttribute("Height", "Radius"),
    new ezCapsuleVisualizerAttribute("Height", "Radius"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

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


void ezPxShapeCapsuleComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(ezBoundingSphere(ezVec3(0, 0, -m_fHeight * 0.5f), m_fRadius), ezInvalidSpatialDataCategory);
  msg.AddBounds(ezBoundingSphere(ezVec3(0, 0, +m_fHeight * 0.5f), m_fRadius), ezInvalidSpatialDataCategory);
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

PxShape* ezPxShapeCapsuleComponent::CreateShape(PxRigidActor* pActor, PxTransform& out_ShapeTransform)
{
  const float fScale = GetOwner()->GetGlobalTransformSimd().GetMaxScale();

  out_ShapeTransform.q = PxQuat(ezAngle::Degree(90.0f).GetRadian(), PxVec3(0.0f, 1.0f, 0.0f));

  PxCapsuleGeometry capsule;
  capsule.radius = m_fRadius * fScale;
  capsule.halfHeight = m_fHeight * 0.5f * fScale;

  return pActor->createShape(capsule, *GetPxMaterial());
}



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Shapes_Implementation_PxShapeCapsuleComponent);
