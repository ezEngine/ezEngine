#include <PCH.h>
#include <GameEngine/Components/InputComponent.h>
#include <Core/Input/InputManager.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezInputMessageGranularity, 1)
  EZ_ENUM_CONSTANT(ezInputMessageGranularity::PressOnly),
  EZ_ENUM_CONSTANT(ezInputMessageGranularity::PressAndRelease),
  EZ_ENUM_CONSTANT(ezInputMessageGranularity::PressReleaseAndDown),
EZ_END_STATIC_REFLECTED_ENUM();

EZ_IMPLEMENT_MESSAGE_TYPE(ezInputEventMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInputEventMessage, 1, ezRTTIDefaultAllocator<ezInputEventMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_COMPONENT_TYPE(ezInputComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InputSet", m_sInputSet)->AddAttributes(new ezDynamicStringEnumAttribute("InputSet")),
    EZ_ENUM_MEMBER_PROPERTY("Granularity", ezInputMessageGranularity, m_Granularity),
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Input"),
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_MESSAGESENDERS
  {
    EZ_MESSAGE_SENDER(m_InputEventSender)
  }
  EZ_END_MESSAGESENDERS
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezInputComponent::ezInputComponent() {}
ezInputComponent::~ezInputComponent() {}

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

  ezHybridArray<ezString, 24> AllActions;
  ezInputManager::GetAllInputActions(m_sInputSet, AllActions);

  ezInputEventMessage msg;

  for (const ezString& actionName : AllActions)
  {
    float fValue = 0.0f;
    const ezKeyState::Enum state = ezInputManager::GetInputActionState(m_sInputSet, actionName, &fValue);

    if (state == ezKeyState::Up)
      continue;
    if (state == ezKeyState::Down && m_Granularity < ezInputMessageGranularity::PressReleaseAndDown)
      continue;
    if (state == ezKeyState::Released && m_Granularity == ezInputMessageGranularity::PressOnly)
      continue;

    msg.m_TriggerState = ToTriggerState(state);
    msg.m_uiInputActionHash = ezTempHashedString::ComputeHash(actionName.GetData());
    msg.m_fKeyPressValue = fValue;

    m_InputEventSender.SendMessage(msg, this, GetOwner());
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

  if (state != ezKeyState::Up)
  {
    return fValue;
  }

  return fValue;
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
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();


  s >> m_sInputSet;
  ezInputMessageGranularity::StorageType gran;
  s >> gran; m_Granularity.SetValue(gran);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezInputComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezInputComponentPatch_1_2()
    : ezGraphPatch("ezInputComponent", 2) {}

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Input Set", "InputSet");
  }
};

ezInputComponentPatch_1_2 g_ezInputComponentPatch_1_2;
