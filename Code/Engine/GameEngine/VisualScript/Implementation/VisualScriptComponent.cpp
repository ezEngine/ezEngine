#include <PCH.h>
#include <GameEngine/VisualScript/VisualScriptComponent.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>

EZ_BEGIN_COMPONENT_TYPE(ezVisualScriptComponent, 1);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Script", GetScriptFile, SetScriptFile)->AddAttributes(new ezAssetBrowserAttribute("Visual Script")),
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

  /// \todo Store the current script state
}

void ezVisualScriptComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hResource;

  /// \todo Read script state
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

      m_Script->Configure(pResource->GetDescriptor());
    }
    else
    {
      /// \todo Remove dummy code
      m_Script->Configure();
    }
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


