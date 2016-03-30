#include <GameUtils/PCH.h>
#include <GameUtils/Components/InputComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>

EZ_IMPLEMENT_MESSAGE_TYPE(ezInputComponentMessage);

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezInputMessageGranularity, 1)
  EZ_ENUM_CONSTANT(ezInputMessageGranularity::PressOnly),
  EZ_ENUM_CONSTANT(ezInputMessageGranularity::PressAndRelease),
  EZ_ENUM_CONSTANT(ezInputMessageGranularity::PressReleaseAndDown),
  EZ_ENUM_CONSTANT(ezInputMessageGranularity::PressReleaseDownAndUp),
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_COMPONENT_TYPE(ezInputComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Input Set", m_sInputSet)->AddAttributes(new ezDynamicStringEnumAttribute("InputSet")),
    EZ_ENUM_MEMBER_PROPERTY("Granularity", ezInputMessageGranularity, m_Granularity),
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
    new ezCategoryAttribute("General"),
  EZ_END_ATTRIBUTES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezInputComponent::ezInputComponent()
{
}


void ezInputComponent::Update()
{
  if (m_sInputSet.IsEmpty())
    return;

  ezHybridArray<ezString, 24> AllActions;
  ezInputManager::GetAllInputActions(m_sInputSet, AllActions);

  ezInputComponentMessage msg;

  for (const ezString& actionName : AllActions)
  {
    msg.m_State = ezInputManager::GetInputActionState(m_sInputSet, actionName, &msg.m_fValue);

    if (msg.m_State == ezKeyState::Up && m_Granularity != ezInputMessageGranularity::PressReleaseDownAndUp)
      continue;
    if (msg.m_State == ezKeyState::Down && m_Granularity < ezInputMessageGranularity::PressReleaseAndDown)
      continue;
    if (msg.m_State == ezKeyState::Released && m_Granularity == ezInputMessageGranularity::PressOnly)
      continue;

    msg.m_szAction = actionName;

    // SendMessage, not PostMessage, because the string pointers would not be valid otherwise
    GetOwner()->SendMessage(msg);
  }
}

void ezInputComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_sInputSet;
  s << m_Granularity.GetValue();
}

void ezInputComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();


  s >> m_sInputSet;
  ezInputMessageGranularity::StorageType gran;
  s >> gran; m_Granularity.SetValue(gran);
}

