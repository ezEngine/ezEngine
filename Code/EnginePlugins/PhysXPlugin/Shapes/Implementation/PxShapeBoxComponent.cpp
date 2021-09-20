#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Shapes/PxShapeBoxComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <extensions/PxRigidActorExt.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxShapeBoxComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0), ezVariant())),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezBoxManipulatorAttribute("Extents"),
    new ezBoxVisualizerAttribute("Extents"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxShapeBoxComponent::ezPxShapeBoxComponent() = default;
ezPxShapeBoxComponent::~ezPxShapeBoxComponent() = default;

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

void ezPxShapeBoxComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(ezBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f), ezInvalidSpatialDataCategory);
}

void ezPxShapeBoxComponent::ExtractGeometry(ezMsgExtractGeometry& msg) const
{
  EZ_ASSERT_NOT_IMPLEMENTED;
#if 0
  const ezVec3 vScale = ezSimdConversion::ToVec3(GetOwner()->GetGlobalTransformSimd().m_Scale.Abs());

  auto& box = msg.m_pWorldGeometry->m_BoxShapes.ExpandAndGetRef();
  box.m_vPosition = GetOwner()->GetGlobalPosition();
  box.m_qRotation = GetOwner()->GetGlobalRotation();
  box.m_vHalfExtents = m_vExtents.CompMul(vScale) * 0.5f;
#endif
}

void ezPxShapeBoxComponent::SetExtents(const ezVec3& value)
{
  m_vExtents = value.CompMax(ezVec3::ZeroVector());

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezPxShapeBoxComponent::CreateShapes(ezDynamicArray<physx::PxShape*>& out_Shapes, physx::PxRigidActor* pActor, physx::PxTransform& out_ShapeTransform)
{
  ezVec3 vScale = ezSimdConversion::ToVec3(GetOwner()->GetGlobalTransformSimd().m_Scale.Abs());

  PxBoxGeometry box;
  box.halfExtents = ezPxConversionUtils::ToVec3(m_vExtents.CompMul(vScale) * 0.5f);

  out_Shapes.PushBack(PxRigidActorExt::createExclusiveShape(*pActor, box, *GetPxMaterial()));
}



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Shapes_Implementation_PxShapeBoxComponent);
