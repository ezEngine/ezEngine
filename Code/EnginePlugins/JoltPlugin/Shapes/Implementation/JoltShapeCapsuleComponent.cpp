#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeCapsuleComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltShapeCapsuleComponent, 1, ezComponentMode::Static)
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
    new ezCapsuleManipulatorAttribute("Height", "Radius"),
    new ezCapsuleVisualizerAttribute("Height", "Radius"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltShapeCapsuleComponent::ezJoltShapeCapsuleComponent() = default;
ezJoltShapeCapsuleComponent::~ezJoltShapeCapsuleComponent() = default;

void ezJoltShapeCapsuleComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();
  s << m_fRadius;
  s << m_fHeight;
}

void ezJoltShapeCapsuleComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = stream.GetStream();
  s >> m_fRadius;
  s >> m_fHeight;
}

void ezJoltShapeCapsuleComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(ezBoundingSphere(ezVec3(0, 0, -m_fHeight * 0.5f), m_fRadius), ezInvalidSpatialDataCategory);
  msg.AddBounds(ezBoundingSphere(ezVec3(0, 0, +m_fHeight * 0.5f), m_fRadius), ezInvalidSpatialDataCategory);
}

void ezJoltShapeCapsuleComponent::SetRadius(float f)
{
  m_fRadius = ezMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezJoltShapeCapsuleComponent::SetHeight(float f)
{
  m_fHeight = ezMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezJoltShapeCapsuleComponent::CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial)
{
  auto pNewShape = new JPH::CapsuleShape(m_fHeight * 0.5f, m_fRadius);
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<ezUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  auto pRotShapeSet = new JPH::RotatedTranslatedShapeSettings(JPH::Vec3::sZero(), JPH::Quat::sRotation(JPH::Vec3::sAxisX(), ezAngle::Degree(90).GetRadian()), pNewShape);
  pRotShapeSet->AddRef();

  JPH::Shape* pRotShape = pRotShapeSet->Create().Get().GetPtr();
  pRotShape->SetUserData(reinterpret_cast<ezUInt64>(GetUserData()));

  ezJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape = pRotShape;
  sub.m_Transform.SetLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
}
