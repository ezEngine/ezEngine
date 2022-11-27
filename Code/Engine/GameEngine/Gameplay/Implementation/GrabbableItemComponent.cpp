#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GrabbableItemComponent.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGrabbableItemGrabPoint, ezNoBase, 1, ezRTTIDefaultAllocator<ezGrabbableItemGrabPoint>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("LocalPosition", m_vLocalPosition),
    EZ_MEMBER_PROPERTY("LocalRotation", m_qLocalRotation),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezTransformManipulatorAttribute("LocalPosition", "LocalRotation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE


EZ_BEGIN_COMPONENT_TYPE(ezGrabbableItemComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_ACCESSOR_PROPERTY("GrabPoints", GrabPoints_GetCount, GrabPoints_GetValue, GrabPoints_SetValue, GrabPoints_Insert, GrabPoints_Remove),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezGrabbableItemComponent::ezGrabbableItemComponent() = default;
ezGrabbableItemComponent::~ezGrabbableItemComponent() = default;

ezUInt32 ezGrabbableItemComponent::GrabPoints_GetCount() const
{
  return m_GrabPoints.GetCount();
}

ezGrabbableItemGrabPoint ezGrabbableItemComponent::GrabPoints_GetValue(ezUInt32 uiIndex) const
{
  return m_GrabPoints[uiIndex];
}

void ezGrabbableItemComponent::GrabPoints_SetValue(ezUInt32 uiIndex, ezGrabbableItemGrabPoint value)
{
  m_GrabPoints[uiIndex] = value;
}

void ezGrabbableItemComponent::GrabPoints_Insert(ezUInt32 uiIndex, ezGrabbableItemGrabPoint value)
{
  m_GrabPoints.Insert(value, uiIndex);
}

void ezGrabbableItemComponent::GrabPoints_Remove(ezUInt32 uiIndex)
{
  m_GrabPoints.RemoveAtAndCopy(uiIndex);
}

void ezGrabbableItemComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  const ezUInt8 uiNumGrabPoints = static_cast<ezUInt8>(m_GrabPoints.GetCount());
  s << uiNumGrabPoints;
  for (const auto& gb : m_GrabPoints)
  {
    s << gb.m_vLocalPosition;
    s << gb.m_qLocalRotation;
  }
}

void ezGrabbableItemComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  ezUInt8 uiNumGrabPoints;
  s >> uiNumGrabPoints;
  m_GrabPoints.SetCount(uiNumGrabPoints);
  for (auto& gb : m_GrabPoints)
  {
    s >> gb.m_vLocalPosition;
    s >> gb.m_qLocalRotation;
  }
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_GrabbableItemComponent);
