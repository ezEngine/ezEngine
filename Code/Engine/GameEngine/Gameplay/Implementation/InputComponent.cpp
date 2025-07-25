#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Gameplay/InputComponent.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezInputMessageGranularity, 1)
  EZ_ENUM_CONSTANT(ezInputMessageGranularity::PressOnly),
  EZ_ENUM_CONSTANT(ezInputMessageGranularity::PressAndRelease),
  EZ_ENUM_CONSTANT(ezInputMessageGranularity::PressReleaseAndDown),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgInputActionTriggered);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgInputActionTriggered, 1, ezRTTIDefaultAllocator<ezMsgInputActionTriggered>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("InputAction", GetInputAction, SetInputAction),
    EZ_MEMBER_PROPERTY("KeyPressValue", m_fKeyPressValue),
    EZ_ENUM_MEMBER_PROPERTY("TriggerState", ezTriggerState, m_TriggerState),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezInputComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InputSet", m_sInputSet)->AddAttributes(new ezDynamicStringEnumAttribute("InputSet")),
    EZ_ENUM_MEMBER_PROPERTY("Granularity", ezInputMessageGranularity, m_Granularity),
    EZ_MEMBER_PROPERTY("ForwardToBlackboard", m_bForwardToBlackboard),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Input"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGESENDERS
  {
    EZ_MESSAGE_SENDER(m_InputEventSender)
  }
  EZ_END_MESSAGESENDERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetCurrentInputState, In, "InputAction", In, "OnlyKeyPressed"),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezInputComponent::ezInputComponent() = default;
ezInputComponent::~ezInputComponent() = default;

static inline ezTriggerState::Enum ToTriggerState(ezKeyState::Enum s)
{
  switch (s)
  {
    case ezKeyState::Pressed:
      return ezTriggerState::Activated;

    case ezKeyState::Released:
      return ezTriggerState::Deactivated;

    default:
      return ezTriggerState::Continuing;
  }
}

void ezInputComponent::Update()
{
  if (m_sInputSet.IsEmpty())
    return;

  ezHybridArray<ezString, 32> AllActions;
  ezInputManager::GetAllInputActions(m_sInputSet, AllActions);

  ezMsgInputActionTriggered msg;

  ezBlackboard* pBlackboard = m_bForwardToBlackboard ? ezBlackboardComponent::FindBlackboard(GetOwner()) : nullptr;

  for (const ezString& actionName : AllActions)
  {
    float fValue = 0.0f;
    const ezKeyState::Enum state = ezInputManager::GetInputActionState(m_sInputSet, actionName, &fValue);

    if (pBlackboard)
    {
      pBlackboard->SetEntryValue(actionName, fValue);
    }

    if (state == ezKeyState::Up)
      continue;
    if (state == ezKeyState::Down && m_Granularity < ezInputMessageGranularity::PressReleaseAndDown)
      continue;
    if (state == ezKeyState::Released && m_Granularity == ezInputMessageGranularity::PressOnly)
      continue;

    msg.m_TriggerState = ToTriggerState(state);
    msg.m_sInputAction.Assign(actionName);
    msg.m_fKeyPressValue = fValue;

    m_InputEventSender.SendEventMessage(msg, this, GetOwner());
  }
}

float ezInputComponent::GetCurrentInputState(const char* szInputAction, bool bOnlyKeyPressed /*= false*/) const
{
  if (m_sInputSet.IsEmpty())
    return 0;

  float fValue = 0.0f;
  const ezKeyState::Enum state = ezInputManager::GetInputActionState(m_sInputSet, szInputAction, &fValue);

  if (bOnlyKeyPressed && state != ezKeyState::Pressed)
    return 0;

  return fValue;
}

void ezInputComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sInputSet;
  s << m_Granularity;

  // version 3
  s << m_bForwardToBlackboard;
}

void ezInputComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();


  s >> m_sInputSet;
  s >> m_Granularity;

  if (uiVersion >= 3)
  {
    s >> m_bForwardToBlackboard;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezInputComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezInputComponentPatch_1_2()
    : ezGraphPatch("ezInputComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Input Set", "InputSet");
  }
};

ezInputComponentPatch_1_2 g_ezInputComponentPatch_1_2;



EZ_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_InputComponent);
