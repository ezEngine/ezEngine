#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezScriptComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("UpdateInterval", GetUpdateInterval, SetUpdateInterval)->AddAttributes(new ezClampValueAttribute(ezTime::MakeZero(), ezVariant())),
    EZ_RESOURCE_ACCESSOR_PROPERTY("ScriptClass", GetScriptClass, SetScriptClass)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_ScriptClass", ezDependencyFlags::Package)),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("ScriptClass")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SetScriptVariable, In, "Name", In, "Value"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetScriptVariable, In, "Name"),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetUpdateInterval, In, "interval"),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Scripting"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezScriptComponent::ezScriptComponent() = default;
ezScriptComponent::~ezScriptComponent() = default;

void ezScriptComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hScriptClass;
  s << m_UpdateInterval;

  ezUInt16 uiNumParams = static_cast<ezUInt16>(m_Parameters.GetCount());
  s << uiNumParams;

  for (ezUInt32 p = 0; p < uiNumParams; ++p)
  {
    s << m_Parameters.GetKey(p);
    s << m_Parameters.GetValue(p);
  }
}

void ezScriptComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hScriptClass;
  s >> m_UpdateInterval;

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

void ezScriptComponent::Initialize()
{
  SUPER::Initialize();

  if (m_hScriptClass.IsValid())
  {
    InstantiateScript(false);
  }
}

void ezScriptComponent::Deinitialize()
{
  SUPER::Deinitialize();

  ClearInstance(false);
}

void ezScriptComponent::OnActivated()
{
  SUPER::OnActivated();

  CallScriptFunction(ezComponent_ScriptBaseClassFunctions::OnActivated);

  AddUpdateFunctionToSchedule();
}

void ezScriptComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  CallScriptFunction(ezComponent_ScriptBaseClassFunctions::OnDeactivated);

  RemoveUpdateFunctionToSchedule();
}

void ezScriptComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  CallScriptFunction(ezComponent_ScriptBaseClassFunctions::OnSimulationStarted);
}

void ezScriptComponent::SetScriptVariable(const ezHashedString& sName, const ezVariant& value)
{
  if (m_pInstance != nullptr)
  {
    m_pInstance->SetInstanceVariable(sName, value);
  }
}

ezVariant ezScriptComponent::GetScriptVariable(const ezHashedString& sName) const
{
  if (m_pInstance != nullptr)
  {
    return m_pInstance->GetInstanceVariable(sName);
  }

  return ezVariant();
}

void ezScriptComponent::SetScriptClass(const ezScriptClassResourceHandle& hScript)
{
  if (m_hScriptClass == hScript)
    return;

  if (IsInitialized())
  {
    ClearInstance(IsActiveAndInitialized());
  }

  m_hScriptClass = hScript;

  if (IsInitialized() && m_hScriptClass.IsValid())
  {
    InstantiateScript(IsActiveAndInitialized());
  }
}

void ezScriptComponent::SetUpdateInterval(ezTime interval)
{
  m_UpdateInterval = interval;

  AddUpdateFunctionToSchedule();
}

ezTime ezScriptComponent::GetUpdateInterval() const
{
  return m_UpdateInterval;
}

void ezScriptComponent::BroadcastEventMsg(ezEventMessage& ref_msg)
{
  const ezRTTI* pType = ref_msg.GetDynamicRTTI();

  for (auto& sender : m_EventSenders)
  {
    if (sender.m_pMsgType == pType)
    {
      sender.m_Sender.SendEventMessage(ref_msg, this, GetOwner()->GetParent());
      return;
    }
  }

  auto& sender = m_EventSenders.ExpandAndGetRef();
  sender.m_pMsgType = pType;
  sender.m_Sender.SendEventMessage(ref_msg, this, GetOwner()->GetParent());
}

const ezRangeView<const char*, ezUInt32> ezScriptComponent::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>([]() -> ezUInt32
    { return 0; },
    [this]() -> ezUInt32
    { return m_Parameters.GetCount(); },
    [](ezUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const ezUInt32& uiIt) -> const char*
    { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void ezScriptComponent::SetParameter(const char* szKey, const ezVariant& value)
{
  ezHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != ezInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;

  if (IsInitialized() && m_hScriptClass.IsValid())
  {
    InstantiateScript(IsActiveAndInitialized());
  }
}

void ezScriptComponent::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(ezTempHashedString(szKey)))
  {
    if (IsInitialized() && m_hScriptClass.IsValid())
    {
      InstantiateScript(IsActiveAndInitialized());
    }
  }
}

bool ezScriptComponent::GetParameter(const char* szKey, ezVariant& out_value) const
{
  ezUInt32 it = m_Parameters.Find(szKey);

  if (it == ezInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

void ezScriptComponent::InstantiateScript(bool bActivate)
{
  ClearInstance(IsActiveAndInitialized());

  ezResourceLock<ezScriptClassResource> pScript(m_hScriptClass, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pScript.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    ezLog::Error("Failed to load script '{}'", GetScriptClass().GetResourceIdOrDescription());
    return;
  }

  auto pScriptType = pScript->GetType();
  if (pScriptType == nullptr || pScriptType->IsDerivedFrom(ezGetStaticRTTI<ezComponent>()) == false)
  {
    ezLog::Error("Script type '{}' is not a component", pScriptType != nullptr ? pScriptType->GetTypeName() : "NULL");
    return;
  }

  m_pScriptType = pScriptType;
  m_pMessageDispatchType = pScriptType;

  m_pInstance = pScript->Instantiate(*this, GetWorld());
  if (m_pInstance != nullptr)
  {
    m_pInstance->SetInstanceVariables(m_Parameters);
  }

  GetWorld()->AddResourceReloadFunction(m_hScriptClass, GetHandle(), nullptr,
    [](const ezWorld::ResourceReloadContext& context)
    {
      ezStaticCast<ezScriptComponent*>(context.m_pComponent)->ReloadScript();
    });

  CallScriptFunction(ezComponent_ScriptBaseClassFunctions::Initialize);
  if (bActivate)
  {
    CallScriptFunction(ezComponent_ScriptBaseClassFunctions::OnActivated);

    if (GetWorld()->GetWorldSimulationEnabled())
    {
      CallScriptFunction(ezComponent_ScriptBaseClassFunctions::OnSimulationStarted);
    }
  }

  AddUpdateFunctionToSchedule();
}

void ezScriptComponent::ClearInstance(bool bDeactivate)
{
  if (bDeactivate)
  {
    CallScriptFunction(ezComponent_ScriptBaseClassFunctions::OnDeactivated);
  }
  CallScriptFunction(ezComponent_ScriptBaseClassFunctions::Deinitialize);

  RemoveUpdateFunctionToSchedule();

  auto pModule = GetWorld()->GetOrCreateModule<ezScriptWorldModule>();
  pModule->StopAndDeleteAllCoroutines(m_pInstance.Borrow());

  GetWorld()->RemoveResourceReloadFunction(m_hScriptClass, GetHandle(), nullptr);

  m_pInstance = nullptr;
  m_pScriptType = nullptr;

  m_pMessageDispatchType = GetDynamicRTTI();
}

void ezScriptComponent::AddUpdateFunctionToSchedule()
{
  if (IsActiveAndInitialized() == false)
    return;

  auto pModule = GetWorld()->GetOrCreateModule<ezScriptWorldModule>();
  if (auto pUpdateFunction = GetScriptFunction(ezComponent_ScriptBaseClassFunctions::Update))
  {
    const bool bOnlyWhenSimulating = true;
    pModule->AddUpdateFunctionToSchedule(pUpdateFunction, m_pInstance.Borrow(), m_UpdateInterval, bOnlyWhenSimulating);
  }
}

void ezScriptComponent::RemoveUpdateFunctionToSchedule()
{
  auto pModule = GetWorld()->GetOrCreateModule<ezScriptWorldModule>();
  if (auto pUpdateFunction = GetScriptFunction(ezComponent_ScriptBaseClassFunctions::Update))
  {
    pModule->RemoveUpdateFunctionToSchedule(pUpdateFunction, m_pInstance.Borrow());
  }
}

const ezAbstractFunctionProperty* ezScriptComponent::GetScriptFunction(ezUInt32 uiFunctionIndex)
{
  if (m_pScriptType != nullptr && m_pInstance != nullptr)
  {
    return m_pScriptType->GetFunctionByIndex(uiFunctionIndex);
  }

  return nullptr;
}

void ezScriptComponent::CallScriptFunction(ezUInt32 uiFunctionIndex)
{
  if (auto pFunction = GetScriptFunction(uiFunctionIndex))
  {
    ezVariant returnValue;
    pFunction->Execute(m_pInstance.Borrow(), ezArrayPtr<ezVariant>(), returnValue);
  }
}

void ezScriptComponent::ReloadScript()
{
  InstantiateScript(IsActiveAndInitialized());
}

EZ_STATICLINK_FILE(Core, Core_Scripting_Implementation_ScriptComponent);
