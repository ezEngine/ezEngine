#include <GameUtils/PCH.h>
#include <GameUtils/Components/InputComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>

EZ_IMPLEMENT_MESSAGE_TYPE(ezInputComponentMessage);

EZ_BEGIN_COMPONENT_TYPE(ezInputComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Input Set", m_sInputSet)->AddAttributes(new ezDynamicStringEnumAttribute("InputSet")),
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
    new ezCategoryAttribute("Gameplay"),
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
    msg.m_szAction = actionName;
    msg.m_State = ezInputManager::GetInputActionState(m_sInputSet, actionName, &msg.m_fValue);

    // SendMessage, not PostMessage, because the string pointers would not be valid otherwise
    GetOwner()->SendMessage(msg);
  }
}

void ezInputComponent::SerializeComponent(ezWorldWriter& stream) const
{
  auto& s = stream.GetStream();

  s << m_sInputSet;
}

void ezInputComponent::DeserializeComponent(ezWorldReader& stream)
{
  auto& s = stream.GetStream();

  s >> m_sInputSet;
}

