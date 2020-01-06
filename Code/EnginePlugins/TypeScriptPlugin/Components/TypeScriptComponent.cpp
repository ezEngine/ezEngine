#include <TypeScriptPluginPCH.h>

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Duktape/duktape.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgTypeScriptMsgProxy);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgTypeScriptMsgProxy, 1, ezRTTIDefaultAllocator<ezMsgTypeScriptMsgProxy>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezTypeScriptComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Script", GetTypeScriptComponentFile, SetTypeScriptComponentFile)->AddAttributes(new ezAssetBrowserAttribute("TypeScript")),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("Script")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Scripting"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgTypeScriptMsgProxy, OnMsgTypeScriptMsgProxy)
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezTypeScriptComponent::ezTypeScriptComponent() = default;
ezTypeScriptComponent::~ezTypeScriptComponent() = default;

void ezTypeScriptComponent::SerializeComponent(ezWorldWriter& stream) const
{
  auto& s = stream.GetStream();

  s << m_TypeScriptComponentGuid;

  // version 3
  ezUInt16 uiNumParams = m_Parameters.GetCount();
  s << uiNumParams;

  for (ezUInt32 p = 0; p < uiNumParams; ++p)
  {
    s << m_Parameters.GetKey(p);
    s << m_Parameters.GetValue(p);
  }
}

void ezTypeScriptComponent::DeserializeComponent(ezWorldReader& stream)
{
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  if (uiVersion == 1)
  {
    ezTypelessResourceHandle hDummy;
    s >> hDummy;
  }
  else
  {
    s >> m_TypeScriptComponentGuid;
  }

  if (uiVersion >= 3)
  {
    ezUInt16 uiNumParams = 0;
    s >> uiNumParams;
    m_Parameters.Reserve(uiNumParams);

    ezHashedString key;
    ezVariant value;

    for (ezUInt32 p = 0; p < uiNumParams; ++p)
    {
      s >> key;
      s >> value;

      m_Parameters.Insert(key, value);
    }
  }
}

bool ezTypeScriptComponent::OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg)
{
  return HandleUnhandledMessage(msg, bWasPostedMsg);
}

bool ezTypeScriptComponent::OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg) const
{
  return const_cast<ezTypeScriptComponent*>(this)->HandleUnhandledMessage(msg, bWasPostedMsg);
}

bool ezTypeScriptComponent::HandleUnhandledMessage(ezMessage& msg, bool bWasPostedMsg)
{
  if (GetUserFlag(UserFlag::ScriptFailure))
    return false;

  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  return binding.DeliverMessage(m_ComponentTypeInfo, this, msg, bWasPostedMsg == false);
}

bool ezTypeScriptComponent::HandlesEventMessage(const ezEventMessage& msg) const
{
  ezTypeScriptBinding& binding = static_cast<const ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  return binding.HasMessageHandler(m_ComponentTypeInfo, msg.GetDynamicRTTI());
}

void ezTypeScriptComponent::BroadcastEventMsg(ezEventMessage& msg)
{
  const ezRTTI* pType = msg.GetDynamicRTTI();

  for (auto& sender : m_EventSenders)
  {
    if (sender.m_pMsgType == pType)
    {
      sender.m_Sender.SendMessage(msg, this, GetOwner()->GetParent());
      return;
    }
  }

  auto& sender = m_EventSenders.ExpandAndGetRef();
  sender.m_pMsgType = pType;
  sender.m_Sender.SendMessage(msg, this, GetOwner()->GetParent());
}

bool ezTypeScriptComponent::CallTsFunc(const char* szFuncName)
{
  if (GetUserFlag(UserFlag::ScriptFailure))
    return false;

  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  ezDuktapeHelper duk(binding.GetDukTapeContext());

  binding.DukPutComponentObject(this); // [ comp ]

  if (duk.PrepareMethodCall(szFuncName).Succeeded()) // [ comp func comp ]
  {
    duk.CallPreparedMethod(); // [ comp result ]
    duk.PopStack(2);          // [ ]

    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
  }
  else
  {
    // remove 'this'   [ comp ]
    duk.PopStack(); // [ ]

    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, false, 0);
  }
}

void ezTypeScriptComponent::SetExposedVariables()
{
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  ezDuktapeHelper duk(binding.GetDukTapeContext());

  binding.DukPutComponentObject(this); // [ comp ]

  for (ezUInt32 p = 0; p < m_Parameters.GetCount(); ++p)
  {
    const auto& pair = m_Parameters.GetPair(p);

    ezTypeScriptBinding::SetVariantProperty(duk, pair.key.GetString(), -1, pair.value); // [ comp ]
  }

  duk.PopStack(); // [ ]

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void ezTypeScriptComponent::Initialize()
{
  // SUPER::Initialize() does nothing

  if (!GetUserFlag(UserFlag::SimStartedTS))
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

  if (GetUserFlag(UserFlag::InitializedTS))
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

  if (!GetUserFlag(UserFlag::SimStartedTS))
    return;

  ezTypeScriptComponent::Initialize();

  SetUserFlag(UserFlag::OnActivatedTS, true);

  CallTsFunc("OnActivated");
}

void ezTypeScriptComponent::OnDeactivated()
{
  if (GetUserFlag(UserFlag::OnActivatedTS))
  {
    CallTsFunc("OnDeactivated");
  }

  SetUserFlag(UserFlag::OnActivatedTS, false);

  // SUPER::OnDeactivated() does nothing
}

void ezTypeScriptComponent::OnSimulationStarted()
{
  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  SetUserFlag(UserFlag::SimStartedTS, true);

  if (binding.LoadComponent(m_TypeScriptComponentGuid, m_ComponentTypeInfo).Failed())
  {
    SetUserFlag(UserFlag::ScriptFailure, true);
    return;
  }

  ezUInt32 uiStashIdx = 0;
  if (binding.RegisterComponent(m_ComponentTypeInfo.Value().m_sComponentTypeName, GetHandle(), uiStashIdx).Failed())
  {
    SetUserFlag(UserFlag::ScriptFailure, true);
    return;
  }

  // if the TS component has any message handlers, we need to capture all messages and redirect them to the script
  EnableUnhandledMessageHandler(!m_ComponentTypeInfo.Value().m_MessageHandlers.IsEmpty());

  SetExposedVariables();

  ezTypeScriptComponent::OnActivated();

  CallTsFunc("OnSimulationStarted");
}

void ezTypeScriptComponent::Update(ezTypeScriptBinding& binding)
{
  if (GetUserFlag(UserFlag::ScriptFailure))
    return;

  if (GetUserFlag(UserFlag::NoTsTick))
    return;

  const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  if (m_LastUpdate + m_UpdateInterval > tNow)
    return;

  m_LastUpdate = tNow;

  ezDuktapeHelper duk(binding.GetDukTapeContext());

  binding.DukPutComponentObject(this); // [ comp ]

  if (duk.PrepareMethodCall("Tick").Succeeded()) // [ comp func comp ]
  {
    duk.CallPreparedMethod(); // [ comp result ]
    duk.PopStack(2);          // [ ]
  }
  else
  {
    // remove 'this'   [ comp ]
    duk.PopStack(); // [ ]

    SetUserFlag(UserFlag::NoTsTick, true);
  }

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void ezTypeScriptComponent::SetTypeScriptComponentFile(const char* szFile)
{
  if (ezConversionUtils::IsStringUuid(szFile))
  {
    SetTypeScriptComponentGuid(ezConversionUtils::ConvertStringToUuid(szFile));
  }
  else
  {
    SetTypeScriptComponentGuid(ezUuid());
  }
}

const char* ezTypeScriptComponent::GetTypeScriptComponentFile() const
{
  if (m_TypeScriptComponentGuid.IsValid())
  {
    static ezStringBuilder sGuid; // need dummy storage
    return ezConversionUtils::ToString(m_TypeScriptComponentGuid, sGuid);
  }

  return "";
}

void ezTypeScriptComponent::SetTypeScriptComponentGuid(const ezUuid& hResource)
{
  m_TypeScriptComponentGuid = hResource;
}

const ezUuid& ezTypeScriptComponent::GetTypeScriptComponentGuid() const
{
  return m_TypeScriptComponentGuid;
}

void ezTypeScriptComponent::OnMsgTypeScriptMsgProxy(ezMsgTypeScriptMsgProxy& msg)
{
  if (GetUserFlag(UserFlag::ScriptFailure))
    return;

  ezTypeScriptBinding& binding = static_cast<ezTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  binding.DeliverTsMessage(m_ComponentTypeInfo, this, msg);
}

const ezRangeView<const char*, ezUInt32> ezTypeScriptComponent::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>([]() -> ezUInt32 { return 0; }, [this]() -> ezUInt32 { return m_Parameters.GetCount(); },
    [](ezUInt32& it) { ++it; }, [this](const ezUInt32& it) -> const char* { return m_Parameters.GetKey(it).GetString().GetData(); });
}

void ezTypeScriptComponent::SetParameter(const char* szKey, const ezVariant& value)
{
  ezHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != ezInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;

  //GetWorld()->GetComponentManager<ezTypeScriptComponentManager>()->AddToUpdateList(this);
}

void ezTypeScriptComponent::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(ezTempHashedString(szKey)))
  {
    //GetWorld()->GetComponentManager<ezTypeScriptComponentManager>()->AddToUpdateList(this);
  }
}

bool ezTypeScriptComponent::GetParameter(const char* szKey, ezVariant& out_value) const
{
  ezUInt32 it = m_Parameters.Find(szKey);

  if (it == ezInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}
