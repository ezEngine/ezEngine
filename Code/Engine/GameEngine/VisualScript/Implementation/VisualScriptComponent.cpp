#include <PCH.h>
#include <GameEngine/VisualScript/VisualScriptComponent.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>

ezEvent<const ezVisualScriptComponentActivityEvent&> ezVisualScriptComponent::s_ActivityEvents;

EZ_BEGIN_COMPONENT_TYPE(ezVisualScriptComponent, 2);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Script", GetScriptFile, SetScriptFile)->AddAttributes(new ezAssetBrowserAttribute("Visual Script")),
    EZ_ACCESSOR_PROPERTY("HandleGlobalEvents", GetIsGlobalEventHandler, SetIsGlobalEventHandler),
    EZ_MEMBER_PROPERTY("DebugOutput", m_bEnableDebugOutput),
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Scripting"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE

ezVisualScriptComponent::ezVisualScriptComponent()
{
}

ezVisualScriptComponent::~ezVisualScriptComponent()
{
  m_hResource.Invalidate();
  m_Script.Reset();
}

void ezVisualScriptComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hResource;
  s << m_bGlobalEventHandler;
  /// \todo Store the current script state
}

void ezVisualScriptComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hResource;

  if (uiVersion >= 2)
  {
    s >> m_bGlobalEventHandler;
  }

  /// \todo Read script state
  SetIsGlobalEventHandler(m_bGlobalEventHandler);
}

void ezVisualScriptComponent::SetScriptFile(const char* szFile)
{
  ezVisualScriptResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezVisualScriptResource>(szFile);
  }

  SetScript(hResource);
}

const char* ezVisualScriptComponent::GetScriptFile() const
{
  if (!m_hResource.IsValid())
    return "";

  return m_hResource.GetResourceID();
}

void ezVisualScriptComponent::SetScript(const ezVisualScriptResourceHandle& hResource)
{
  m_hResource = hResource;
}

void ezVisualScriptComponent::SetIsGlobalEventHandler(bool enable)
{
  if (m_bGlobalEventHandler != enable)
  {
    m_bGlobalEventHandler = enable;
    EnableGlobalEventHandlerMode(m_bGlobalEventHandler);
  }
}

void ezVisualScriptComponent::Update()
{
  /// \todo Do we really need to tick scripts every frame?

  if (m_Script == nullptr)
  {
    EnableUnhandledMessageHandler(true);

    m_Script = EZ_DEFAULT_NEW(ezVisualScriptInstance);

    if (m_hResource.IsValid())
    {
      ezResourceLock<ezVisualScriptResource> pResource(m_hResource, ezResourceAcquireMode::NoFallback);

      m_Script->Configure(pResource->GetDescriptor(), GetOwner());
    }
  }

  if (m_bEnableDebugOutput != (m_pActivity != nullptr))
  {
    if (m_bEnableDebugOutput)
      m_pActivity = EZ_DEFAULT_NEW(ezVisualScriptInstanceActivity);
    else
      m_pActivity.Reset();
  }

  m_Script->ExecuteScript(m_pActivity.Borrow());

  if (m_bEnableDebugOutput && (!m_pActivity->IsEmpty() || !m_bHadEmptyActivity))
  {
    ezVisualScriptComponentActivityEvent e;
    e.m_pComponent = this;
    e.m_pActivity = m_pActivity.Borrow();

    s_ActivityEvents.Broadcast(e);

    // this is to send one 'empty' activity event (but not more), every time a script becomes inactive
    m_bHadEmptyActivity = m_pActivity->IsEmpty();
  }
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

void ezVisualScriptComponent::OnActivated()
{
  SUPER::OnActivated();

  EnableEventHandlerMode(true);
}

void ezVisualScriptComponent::OnDeactivated()
{
  EnableEventHandlerMode(false);

  SUPER::OnDeactivated();
}


