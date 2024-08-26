#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeBoxComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltShapeBoxComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("HalfExtents", GetHalfExtents, SetHalfExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(0.5f)), new ezClampValueAttribute(ezVec3(0), ezVariant())),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezBoxManipulatorAttribute("HalfExtents", 2.0f, true),
    new ezBoxVisualizerAttribute("HalfExtents", 2.0f),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltShapeBoxComponent::ezJoltShapeBoxComponent() = default;
ezJoltShapeBoxComponent::~ezJoltShapeBoxComponent() = default;

void ezJoltShapeBoxComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_vHalfExtents;
}

void ezJoltShapeBoxComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();
  s >> m_vHalfExtents;
}

void ezJoltShapeBoxComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(ezBoundingBoxSphere::MakeFromBox(ezBoundingBox::MakeFromMinMax(-m_vHalfExtents, m_vHalfExtents)), ezInvalidSpatialDataCategory);
}

void ezJoltShapeBoxComponent::ExtractGeometry(ezMsgExtractGeometry& ref_msg) const
{
  ref_msg.AddBox(GetOwner()->GetGlobalTransform(), m_vHalfExtents * 2.0f);
}

void ezJoltShapeBoxComponent::SetHalfExtents(const ezVec3& value)
{
  m_vHalfExtents = value.CompMax(ezVec3::MakeZero());

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezJoltShapeBoxComponent::CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial)
{
  // can't create boxes smaller than this
  ezVec3 size = m_vHalfExtents;
  size.x = ezMath::Max(size.x, JPH::cDefaultConvexRadius);
  size.y = ezMath::Max(size.y, JPH::cDefaultConvexRadius);
  size.z = ezMath::Max(size.z, JPH::cDefaultConvexRadius);

  auto pNewShape = new JPH::BoxShape(ezJoltConversionUtils::ToVec3(size));
  pNewShape->AddRef();
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<ezUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  ezJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape = pNewShape;
  sub.m_Transform = ezTransform::MakeLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeBoxComponent);
