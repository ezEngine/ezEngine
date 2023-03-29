#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <SampleGamePlugin/Components/DisplayMsgComponent.h>
#include <SampleGamePlugin/Messages/Messages.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(DisplayMsgComponent, 1, ezComponentMode::Static /* this component does not move the owner node */)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //}
  //EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("SampleGamePlugin"),
  }
  EZ_END_ATTRIBUTES;

  // BEGIN-DOCS-CODE-SNIPPET: message-handler-block
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgSetText, OnSetText),
    EZ_MESSAGE_HANDLER(ezMsgSetColor, OnSetColor)
  }
  EZ_END_MESSAGEHANDLERS;
  // END-DOCS-CODE-SNIPPET
}
EZ_END_COMPONENT_TYPE
// clang-format on

DisplayMsgComponent::DisplayMsgComponent() = default;
DisplayMsgComponent::~DisplayMsgComponent() = default;

void DisplayMsgComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
}

void DisplayMsgComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
}

void DisplayMsgComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
}

void DisplayMsgComponent::Update()
{
  const ezTransform ownerTransform = GetOwner()->GetGlobalTransform();

  ezDebugRenderer::Draw3DText(GetWorld(), m_sCurrentText.GetData(), ownerTransform.m_vPosition, m_TextColor, 32);
}

// BEGIN-DOCS-CODE-SNIPPET: message-handler-impl
void DisplayMsgComponent::OnSetText(ezMsgSetText& msg)
{
  m_sCurrentText = msg.m_sText;
}

void DisplayMsgComponent::OnSetColor(ezMsgSetColor& msg)
{
  m_TextColor = msg.m_Color;
}
// END-DOCS-CODE-SNIPPET
