#include <PCH.h>
#include <PhysXPlugin/Shapes/PxShapeSphereComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

using namespace physx;

EZ_BEGIN_COMPONENT_TYPE(ezPxShapeSphereComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezUpdateLocalBoundsMessage, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS
  EZ_BEGIN_ATTRIBUTES
  {
    new ezSphereManipulatorAttribute("Radius"),
    new ezSphereVisualizerAttribute("Radius"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

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
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = stream.GetStream();
  s >> m_fRadius;

}


void ezPxShapeSphereComponent::OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const
{
  msg.m_ResultingLocalBounds.ExpandToInclude(ezBoundingSphere(ezVec3::ZeroVector(), m_fRadius));
}


void ezPxShapeSphereComponent::SetRadius(float f)
{
  m_fRadius = ezMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

PxShape* ezPxShapeSphereComponent::CreateShape(PxRigidActor* pActor, PxTransform& out_ShapeTransform)
{
  float fScale = GetOwner()->GetGlobalTransformSimd().GetMaxScale();

  return pActor->createShape(PxSphereGeometry(m_fRadius * fScale), *GetPxMaterial());
}



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Shapes_Implementation_PxShapeSphereComponent);

