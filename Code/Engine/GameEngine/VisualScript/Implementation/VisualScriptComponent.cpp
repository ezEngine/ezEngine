#include <PCH.h>
#include <GameEngine/VisualScript/VisualScriptComponent.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

EZ_BEGIN_COMPONENT_TYPE(ezVisualScriptComponent, 1);
EZ_END_COMPONENT_TYPE

ezVisualScriptComponent::ezVisualScriptComponent()
{
}


ezVisualScriptComponent::~ezVisualScriptComponent()
{
  m_Script.Reset();
}

void ezVisualScriptComponent::Update()
{
  if (m_Script == nullptr)
  {
    EnableUnhandledMessageHandler(true);

    m_Script = EZ_DEFAULT_NEW(ezVisualScriptInstance);
    m_Script->Configure();
  }

  m_Script->ExecuteScript();
}

bool ezVisualScriptComponent::OnUnhandledMessage(ezMessage& msg)
{
  m_Script->HandleMessage(msg);

  return false;
}

bool ezVisualScriptComponent::OnUnhandledMessage(ezMessage& msg) const
{
  m_Script->HandleMessage(msg);

  return false;
}


