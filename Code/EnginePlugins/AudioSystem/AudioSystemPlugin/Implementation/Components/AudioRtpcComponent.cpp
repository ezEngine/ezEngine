#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Components/AudioRtpcComponent.h>
#include <AudioSystemPlugin/Core/AudioSystem.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

constexpr ezTypeVersion kVersion_AudioRtpcComponent = 1;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAudioRtpcComponent, kVersion_AudioRtpcComponent, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Rtpc", m_sRtpcName),
    EZ_MEMBER_PROPERTY("InitialValue", m_fInitialValue),

    EZ_ACCESSOR_PROPERTY_READ_ONLY("Value", GetValue)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SetValue, In, "Value", In, "Sync"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetValue),
  }
  EZ_END_FUNCTIONS;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgAudioSystemSetRtpcValue, OnSetValue),
  }
  EZ_END_MESSAGEHANDLERS;

  EZ_BEGIN_MESSAGESENDERS
  {
    EZ_MESSAGE_SENDER(m_ValueChangedEventSender),
  }
  EZ_END_MESSAGESENDERS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

void ezAudioRtpcComponent::Initialize()
{
  SUPER::Initialize();

  SetValue(m_fInitialValue, false);
}

void ezAudioRtpcComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s.WriteVersion(kVersion_AudioRtpcComponent);

  s << m_sRtpcName;
  s << m_fInitialValue;
}

void ezAudioRtpcComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  s.ReadVersion(kVersion_AudioRtpcComponent);

  s >> m_sRtpcName;
  s >> m_fInitialValue;
}

ezAudioRtpcComponent::ezAudioRtpcComponent()
  : ezAudioSystemComponent()
  , m_fInitialValue(0.0f)
  , m_fValue(0.0f)
{
}

ezAudioRtpcComponent::~ezAudioRtpcComponent() = default;

void ezAudioRtpcComponent::SetValue(float fValue, bool bSync)
{
  if (m_sRtpcName.IsEmpty())
    return;

  if (fValue == m_fValue)
    return; // No need to update...

  ezAudioSystemRequestSetRtpcValue request;

  request.m_uiEntityId = GetOwner()->GetHandle().GetInternalID().m_Data;
  request.m_uiObjectId = ezAudioSystem::GetSingleton()->GetRtpcId(m_sRtpcName);
  request.m_fValue = fValue;

  request.m_Callback = [this](const ezAudioSystemRequestSetRtpcValue& e)
  {
    if (e.m_eStatus.Failed())
      return;

    // Save the value in the component
    m_fValue = e.m_fValue;

    // Notify for the change
    ezMsgAudioSystemRtpcValueChanged msg;
    msg.m_fValue = e.m_fValue;

    // We are not in the writing thread, so posting the message for the next frame instead of sending...
    m_ValueChangedEventSender.PostEventMessage(msg, this, GetOwner(), ezTime::Zero(), ezObjectMsgQueueType::NextFrame);
  };

  ezVariant v(request);

  if (bSync)
  {
    ezAudioSystem::GetSingleton()->SendRequestSync(std::move(v));
  }
  else
  {
    ezAudioSystem::GetSingleton()->SendRequest(std::move(v));
  }
}

float ezAudioRtpcComponent::GetValue() const
{
  return m_fValue;
}

void ezAudioRtpcComponent::OnSetValue(ezMsgAudioSystemSetRtpcValue& msg)
{
  SetValue(msg.m_fValue, msg.m_bSync);
}

EZ_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Components_AudioRtpcComponent);
