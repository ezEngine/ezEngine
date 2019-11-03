#include <TypeScriptPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Duktape/duktape.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>
#include <TypeScriptPlugin/Resources/JavaScriptResource.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezTypeScriptComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Script", GetJavaScriptResourceFile, SetJavaScriptResourceFile)->AddAttributes(new ezAssetBrowserAttribute("TypeScript")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezTypeScriptComponent::ezTypeScriptComponent() = default;
ezTypeScriptComponent::~ezTypeScriptComponent() = default;

void ezTypeScriptComponent::SerializeComponent(ezWorldWriter& stream) const
{
  auto& s = stream.GetStream();

  s << m_hJsResource;
}

void ezTypeScriptComponent::DeserializeComponent(ezWorldReader& stream)
{
  auto& s = stream.GetStream();

  s >> m_hJsResource;
}

bool ezTypeScriptComponent::OnUnhandledMessage(ezMessage& msg)
{
  return HandleUnhandledMessage(msg);
}

bool ezTypeScriptComponent::OnUnhandledMessage(ezMessage& msg) const
{
  return const_cast<ezTypeScriptComponent*>(this)->HandleUnhandledMessage(msg);
}

bool ezTypeScriptComponent::HandleUnhandledMessage(ezMessage& msg)
{
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  return binding.DeliverMessage(m_ComponentTypeInfo, this, msg);
}

bool ezTypeScriptComponent::CallTsFunc(const char* szFuncName)
{
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  ezDuktapeHelper duk(binding.GetDukTapeContext(), 0);

  binding.DukPutComponentObject(this); // [ comp ]

  if (duk.PrepareMethodCall(szFuncName).Succeeded()) // [ comp func comp ]
  {
    duk.CallPreparedMethod(); // [ comp result ]
    duk.PopStack(2);          // [ ]

    return true;
  }
  else
  {
    // remove 'this'   [ comp ]
    duk.PopStack(); // [ ]

    return false;
  }
}

void ezTypeScriptComponent::Initialize()
{
  // SUPER::Initialize() does nothing

  if (!GetWorld()->GetWorldSimulationEnabled())
    return;

  if (!GetUserFlag(UserFlag::InitializedTS))
  {
    SetUserFlag(UserFlag::InitializedTS, true);

    CallTsFunc("Initialize");
  }
}

void ezTypeScriptComponent::Deinitialize()
{
  // mirror what ezComponent::Deinitialize does, but make sure to CallTsFunc at the right time

  EZ_ASSERT_DEV(GetOwner() != nullptr, "Owner must still be valid");

  if (IsActive())
  {
    SetActive(false);
  }

  if (GetWorld()->GetWorldSimulationEnabled() && GetUserFlag(UserFlag::InitializedTS))
  {
    CallTsFunc("Deinitialize");
  }

  SetUserFlag(UserFlag::InitializedTS, false);

  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();
  binding.DeleteTsComponent(GetHandle());
}

void ezTypeScriptComponent::OnActivated()
{
  // SUPER::OnActivated() does nothing

  if (!GetWorld()->GetWorldSimulationEnabled())
    return;

  ezTypeScriptComponent::Initialize();

  SetUserFlag(UserFlag::OnActivatedTS, true);

  CallTsFunc("OnActivated");
}

void ezTypeScriptComponent::OnDeactivated()
{
  if (GetWorld()->GetWorldSimulationEnabled() && GetUserFlag(UserFlag::OnActivatedTS))
  {
    CallTsFunc("OnDeactivated");
  }

  SetUserFlag(UserFlag::OnActivatedTS, false);

  // SUPER::OnDeactivated() does nothing
}

void ezTypeScriptComponent::OnSimulationStarted()
{
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  if (binding.LoadComponent(m_hJsResource, m_ComponentTypeInfo).Succeeded())
  {
    ezUInt32 uiStashIdx = 0;
    binding.RegisterComponent(m_ComponentTypeInfo.Key(), GetHandle(), uiStashIdx);

    // if the TS component has any message handlers, we need to capture all messages and redirect them to the script
    EnableUnhandledMessageHandler(!m_ComponentTypeInfo.Value().m_MessageHandlers.IsEmpty());
  }

  ezTypeScriptComponent::OnActivated();

  CallTsFunc("OnSimulationStarted");
}


void ezTypeScriptComponent::Update(ezTypeScriptBinding& binding)
{
  if (GetUserFlag(UserFlag::NoTsTick))
    return;

  if (!CallTsFunc("Tick"))
  {
    SetUserFlag(UserFlag::NoTsTick, true);
  }
}

void ezTypeScriptComponent::SetJavaScriptResourceFile(const char* szFile)
{
  ezJavaScriptResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezJavaScriptResource>(szFile);
  }

  SetJavaScriptResource(hResource);
}

const char* ezTypeScriptComponent::GetJavaScriptResourceFile() const
{
  if (m_hJsResource.IsValid())
  {
    return m_hJsResource.GetResourceID();
  }

  return "";
}

void ezTypeScriptComponent::SetJavaScriptResource(const ezJavaScriptResourceHandle& hResource)
{
  m_hJsResource = hResource;
}

const ezJavaScriptResourceHandle& ezTypeScriptComponent::GetJavaScriptResource() const
{
  return m_hJsResource;
}
