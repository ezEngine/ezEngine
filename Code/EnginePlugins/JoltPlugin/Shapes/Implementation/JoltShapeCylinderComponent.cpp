#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeCylinderComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltShapeCylinderComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCylinderVisualizerAttribute(ezBasisAxis::PositiveZ, "Height", "Radius"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltShapeCylinderComponent::ezJoltShapeCylinderComponent() = default;
ezJoltShapeCylinderComponent::~ezJoltShapeCylinderComponent() = default;

void ezJoltShapeCylinderComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_fRadius;
  s << m_fHeight;
}

void ezJoltShapeCylinderComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = inout_stream.GetStream();
  s >> m_fRadius;
  s >> m_fHeight;
}

void ezJoltShapeCylinderComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  ezBoundingBox box = ezBoundingBox::MakeFromMinMax(ezVec3(-m_fRadius, -m_fRadius, -m_fHeight * 0.5f), ezVec3(m_fRadius, m_fRadius, m_fHeight * 0.5f));
  msg.AddBounds(ezBoundingBoxSphere::MakeFromBox(box), ezInvalidSpatialDataCategory);
}

void ezJoltShapeCylinderComponent::SetRadius(float f)
{
  m_fRadius = ezMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezJoltShapeCylinderComponent::SetHeight(float f)
{
  m_fHeight = ezMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezJoltShapeCylinderComponent::CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial)
{
  auto pNewShape = new JPH::CylinderShape(m_fHeight * 0.5f, m_fRadius);
  pNewShape->AddRef();
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<ezUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  const ezQuat qTilt = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveY, ezBasisAxis::PositiveZ);

  ezTransform tOwn = GetOwner()->GetGlobalTransform();
  tOwn.m_vScale.x = tOwn.m_vScale.z;
  tOwn.m_qRotation = tOwn.m_qRotation * qTilt;

  ezJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape = pNewShape;
  sub.m_Transform = ezTransform::MakeLocalTransform(rootTransform, tOwn);
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeCylinderComponent);
