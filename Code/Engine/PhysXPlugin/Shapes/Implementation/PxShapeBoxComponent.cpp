#include <PCH.h>
#include <PhysXPlugin/Shapes/PxShapeBoxComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/Messages/BuildNavMeshMessage.h>
#include <GameEngine/AI/NavMesh/NavMeshDescription.h>

using namespace physx;

EZ_BEGIN_COMPONENT_TYPE(ezPxShapeBoxComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0), ezVariant())),
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezUpdateLocalBoundsMessage, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS
  EZ_BEGIN_ATTRIBUTES
  {
    new ezBoxManipulatorAttribute("Extents"),
    new ezBoxVisualizerAttribute("Extents"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPxShapeBoxComponent::ezPxShapeBoxComponent()
{
  m_vExtents.Set(1.0f);
}

void ezPxShapeBoxComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();
  s << m_vExtents;
}


void ezPxShapeBoxComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = stream.GetStream();
  s >> m_vExtents;
}

void ezPxShapeBoxComponent::OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const
{
  msg.AddBounds(ezBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f));
}

void ezPxShapeBoxComponent::AddToNavMesh(ezBuildNavMeshMessage& msg) const
{
  const ezVec3 vScale = ezSimdConversion::ToVec3(GetOwner()->GetGlobalTransformSimd().m_Scale.Abs());

  ezNavMeshBoxObstacle& box = msg.m_pNavMeshDescription->m_BoxObstacles.ExpandAndGetRef();
  box.m_vPosition = GetOwner()->GetGlobalPosition();
  box.m_qRotation = GetOwner()->GetGlobalRotation();
  box.m_vHalfExtents = m_vExtents.CompMult(vScale) * 0.5f;
}

void ezPxShapeBoxComponent::SetExtents(const ezVec3& value)
{
  m_vExtents = value.CompMax(ezVec3::ZeroVector());

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

PxShape* ezPxShapeBoxComponent::CreateShape(PxRigidActor* pActor, PxTransform& out_ShapeTransform)
{
  ezVec3 vScale = ezSimdConversion::ToVec3(GetOwner()->GetGlobalTransformSimd().m_Scale.Abs());

  PxBoxGeometry box;
  box.halfExtents = ezPxConversionUtils::ToVec3(m_vExtents.CompMult(vScale) * 0.5f);

  return pActor->createShape(box, *GetPxMaterial());
}



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Shapes_Implementation_PxShapeBoxComponent);

