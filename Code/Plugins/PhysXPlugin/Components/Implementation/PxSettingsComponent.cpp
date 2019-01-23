#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxSettingsComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxSettingsComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("ObjectGravity", GetObjectGravity, SetObjectGravity)->AddAttributes(new ezDefaultValueAttribute(ezVec3(0, 0, -9.81f))),
    EZ_ACCESSOR_PROPERTY("CharacterGravity", GetCharacterGravity, SetCharacterGravity)->AddAttributes(new ezDefaultValueAttribute(ezVec3(0, 0, -12.0f))),
    EZ_ACCESSOR_PROPERTY("MaxDepenetrationVelocity", GetMaxDepenetrationVelocity, SetMaxDepenetrationVelocity)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ENUM_ACCESSOR_PROPERTY("SteppingMode", ezPxSteppingMode, GetSteppingMode, SetSteppingMode),
    EZ_ACCESSOR_PROPERTY("FixedFrameRate", GetFixedFrameRate, SetFixedFrameRate)->AddAttributes(new ezDefaultValueAttribute(60.0f), new ezClampValueAttribute(1.0f, 1000.0f)),
    EZ_ACCESSOR_PROPERTY("MaxSubSteps", GetMaxSubSteps, SetMaxSubSteps)->AddAttributes(new ezDefaultValueAttribute(4), new ezClampValueAttribute(1, 100)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxSettingsComponent::ezPxSettingsComponent()
    : m_Settings()
{
}

ezPxSettingsComponent::~ezPxSettingsComponent() {}

void ezPxSettingsComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_Settings.m_vObjectGravity;
  s << m_Settings.m_vCharacterGravity;
  s << m_Settings.m_fMaxDepenetrationVelocity;
  s << m_Settings.m_SteppingMode;
  s << m_Settings.m_fFixedFrameRate;
  s << m_Settings.m_uiMaxSubSteps;
}


void ezPxSettingsComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_Settings.m_vObjectGravity;
  s >> m_Settings.m_vCharacterGravity;

  if (uiVersion >= 3)
  {
    s >> m_Settings.m_fMaxDepenetrationVelocity;
  }

  if (uiVersion >= 2)
  {
    s >> m_Settings.m_SteppingMode;
    s >> m_Settings.m_fFixedFrameRate;
    s >> m_Settings.m_uiMaxSubSteps;
  }
}

void ezPxSettingsComponent::SetObjectGravity(const ezVec3& v)
{
  m_Settings.m_vObjectGravity = v;
  SetModified(EZ_BIT(0));
}

void ezPxSettingsComponent::SetCharacterGravity(const ezVec3& v)
{
  m_Settings.m_vCharacterGravity = v;
  SetModified(EZ_BIT(1));
}

void ezPxSettingsComponent::SetMaxDepenetrationVelocity(float fMaxVelocity)
{
  m_Settings.m_fMaxDepenetrationVelocity = fMaxVelocity;
  SetModified(EZ_BIT(2));
}

void ezPxSettingsComponent::SetSteppingMode(ezPxSteppingMode::Enum mode)
{
  m_Settings.m_SteppingMode = mode;
  SetModified(EZ_BIT(3));
}

void ezPxSettingsComponent::SetFixedFrameRate(float fFixedFrameRate)
{
  m_Settings.m_fFixedFrameRate = fFixedFrameRate;
  SetModified(EZ_BIT(4));
}

void ezPxSettingsComponent::SetMaxSubSteps(ezUInt32 uiMaxSubSteps)
{
  m_Settings.m_uiMaxSubSteps = uiMaxSubSteps;
  SetModified(EZ_BIT(5));
}



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Components_Implementation_PxSettingsComponent);
