#include <SampleGamePluginPCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <SampleGamePlugin/Components/SendMsgComponent.h>
#include <SampleGamePlugin/Messages/Messages.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(SendMsgComponent, 1, ezComponentMode::Static /* this component does not move the owner node */)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Strings", m_TextArray)
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("SampleGamePlugin"),
  }
  EZ_END_ATTRIBUTES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgComponentInternalTrigger, OnSendText)
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

SendMsgComponent::SendMsgComponent() = default;
SendMsgComponent::~SendMsgComponent() = default;

void SendMsgComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s.WriteArray(m_TextArray);
}

void SendMsgComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  auto& s = stream.GetStream();

  s.ReadArray(m_TextArray);
}

void SendMsgComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // start sending strings shortly
  ezMsgComponentInternalTrigger msg;
  msg.m_uiUsageStringHash = ezTempHashedString::ComputeHash("SendNextString");
  PostMessage(msg, ezObjectMsgQueueType::NextFrame, ezTime::Milliseconds(100));
}

void SendMsgComponent::OnSendText(ezMsgComponentInternalTrigger& msg)
{
  if (msg.m_uiUsageStringHash = ezTempHashedString::ComputeHash("SendNextString"))
  {
    if (!m_TextArray.IsEmpty())
    {
      ezMsgSetText textMsg;
      textMsg.m_sText = m_TextArray[m_uiNextString % m_TextArray.GetCount()];

      // send the message to all components on this node and all child nodes
      GetOwner()->SendMessageRecursive(textMsg);

      m_uiNextString++;
    }


    // send the next string in a second
    PostMessage(msg, ezObjectMsgQueueType::NextFrame, ezTime::Seconds(2));
  }
}

//////////////////////////////////////////////////////////////////////////

SendMsgComponentManager::SendMsgComponentManager(ezWorld* pWorld)
  : ezComponentManager<class SendMsgComponent, ezBlockStorageType::Compact>(pWorld)
{
}
