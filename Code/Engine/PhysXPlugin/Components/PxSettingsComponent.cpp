#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/PxSettingsComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxSettingsComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Object Gravity", GetObjectGravity, SetObjectGravity)->AddAttributes(new ezDefaultValueAttribute(ezVec3(0, 0, -9.81f))),
    EZ_ACCESSOR_PROPERTY("Character Gravity", GetCharacterGravity, SetCharacterGravity)->AddAttributes(new ezDefaultValueAttribute(ezVec3(0, 0, -12.0f))),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPxSettingsComponent::ezPxSettingsComponent()
{
  m_vObjectGravity.Set(0, 0, -9.81f);
  m_vCharacterGravity.Set(0, 0, -12.0f);
}


void ezPxSettingsComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_vObjectGravity;
  s << m_vCharacterGravity;
}


void ezPxSettingsComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_vObjectGravity;
  s >> m_vCharacterGravity;

}



