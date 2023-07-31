#include <GameEngine/GameEnginePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/VisualScript/VisualScriptComponent.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>

ezEvent<const ezVisualScriptComponentActivityEvent&> ezVisualScriptComponent::s_ActivityEvents;

//////////////////////////////////////////////////////////////////////////

ezVisualScriptComponentManager::ezVisualScriptComponentManager(ezWorld* pWorld)
  : ezComponentManager<ComponentType, ezBlockStorageType::Compact>(pWorld)
{
  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezVisualScriptComponentManager::ResourceEventHandler, this));
}

ezVisualScriptComponentManager::~ezVisualScriptComponentManager()
{
  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezVisualScriptComponentManager::ResourceEventHandler, this));
}

void ezVisualScriptComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezVisualScriptComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void ezVisualScriptComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  // Don't handle resource reload events during play the game since that would mess up script state
  if (GetWorld()->GetWorldSimulationEnabled())
    return;

  if (e.m_Type == ezResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezVisualScriptResource>())
  {
    ezVisualScriptResourceHandle hScript((ezVisualScriptResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hScript)
      {
        m_ComponentsToUpdate.Insert(it->GetHandle());
      }
    }
  }
}

void ezVisualScriptComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  {
    for (auto hComp : m_ComponentsToUpdate)
    {
      ezVisualScriptComponent* pComponent = nullptr;
      if (!TryGetComponent(hComp, pComponent))
        continue;

      pComponent->InitScriptInstance();
    }

    m_ComponentsToUpdate.Clear();
  }

  if (GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      ComponentType* pComponent = it;
      if (pComponent->IsActiveAndInitialized())
      {
        pComponent->Update();
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezVisualScriptComponent, 5, ezComponentMode::Static);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Script", GetScriptFile, SetScriptFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Code_VisualScript", ezDependencyFlags::Package)),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("Script"), new ezExposeColorAlphaAttribute),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Scripting"),
    new ezColorAttribute(ezColorScheme::Scripting),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezVisualScriptComponent::ezVisualScriptComponent() = default;
ezVisualScriptComponent::ezVisualScriptComponent(ezVisualScriptComponent&& other) = default;
ezVisualScriptComponent::~ezVisualScriptComponent() = default;

ezVisualScriptComponent& ezVisualScriptComponent::operator=(ezVisualScriptComponent&& other) = default;

void ezVisualScriptComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hResource;
  /// \todo Store the current script state

  // Version 5
  s << m_Params.GetCount();
  for (ezUInt32 i = 0; i < m_Params.GetCount(); ++i)
  {
    s << m_Params[i].m_sName;
    s << m_Params[i].m_Value;
  }
}

void ezVisualScriptComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hResource;

  if (uiVersion == 3)
  {
    bool globalEventHandler = false; // dummy to prevent early out in SetIsGlobalEventHandler
    s >> globalEventHandler;
    SetGlobalEventHandlerMode(globalEventHandler);
  }

  if (uiVersion >= 3 && uiVersion < 5)
  {
    m_Params.Clear();

    ezUInt32 numNums, numBools;

    s >> numNums;
    for (ezUInt32 i = 0; i < numNums; ++i)
    {
      auto& param = m_Params.ExpandAndGetRef();
      s >> param.m_sName;

      double value;
      s >> value;
      param.m_Value = value;
    }

    s >> numBools;
    for (ezUInt32 i = 0; i < numBools; ++i)
    {
      auto& param = m_Params.ExpandAndGetRef();
      s >> param.m_sName;

      bool value;
      s >> value;
      param.m_Value = value;
    }

    m_bParamsChanged = !m_Params.IsEmpty();
  }

  if (uiVersion >= 5)
  {
    ezUInt32 numParams = 0;
    s >> numParams;
    m_Params.SetCount(numParams);

    for (ezUInt32 i = 0; i < m_Params.GetCount(); ++i)
    {
      s >> m_Params[i].m_sName;
      s >> m_Params[i].m_Value;
    }
  }

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

  if (m_pScriptInstance != nullptr)
  {
    InitScriptInstance();
  }
}

bool ezVisualScriptComponent::HandlesMessage(const ezMessage& msg) const
{
  if (m_pScriptInstance)
  {
    return m_pScriptInstance->HandlesMessage(msg);
  }

  return false;
}

void ezVisualScriptComponent::Update()
{
  /// \todo Do we really need to tick scripts every frame?

  EZ_ASSERT_DEV(m_pScriptInstance != nullptr, "Script instance should have been created at this point");

  const bool bEnableDebugOutput = GetDebugOutput();

  if (bEnableDebugOutput != (m_pActivity != nullptr))
  {
    if (bEnableDebugOutput)
      m_pActivity = EZ_DEFAULT_NEW(ezVisualScriptInstanceActivity);
    else
      m_pActivity.Clear();
  }

  // Script Parameters
  {
    if (m_bParamsChanged)
    {
      m_bParamsChanged = false;

      for (auto& param : m_Params)
      {
        if (param.m_Value.IsA<bool>())
        {
          m_pScriptInstance->GetLocalVariables().StoreBool(param.m_sName, param.m_Value.Get<bool>());
        }
        else if (param.m_Value.IsA<ezString>())
        {
          m_pScriptInstance->GetLocalVariables().StoreString(param.m_sName, param.m_Value.Get<ezString>());
        }
        else if (param.m_Value.IsNumber())
        {
          m_pScriptInstance->GetLocalVariables().StoreDouble(param.m_sName, param.m_Value.ConvertTo<double>());
        }
        else
        {
          EZ_ASSERT_NOT_IMPLEMENTED;
        }
      }

      // TODO: atm we clear this, because it stores only the initial state, and any mutated state is stored
      // in the VS instance, which we do not sync with
      // therefore if we don't clear, modifying ANY value would reset ALL values to the start value
      m_Params.Clear();
    }
  }

  m_pScriptInstance->ExecuteScript(m_pActivity.Borrow());

  if (bEnableDebugOutput && (!m_pActivity->IsEmpty() || !m_bHadEmptyActivity))
  {
    ezVisualScriptComponentActivityEvent e;
    e.m_pComponent = this;
    e.m_pActivity = m_pActivity.Borrow();

    s_ActivityEvents.Broadcast(e);

    // this is to send one 'empty' activity event (but not more), every time a script becomes inactive
    m_bHadEmptyActivity = m_pActivity->IsEmpty();
  }
}

void ezVisualScriptComponent::InitScriptInstance()
{
  m_pScriptInstance = EZ_DEFAULT_NEW(ezVisualScriptInstance);

  if (m_hResource.IsValid())
  {
    m_pScriptInstance->Configure(m_hResource, this);
  }

  m_bParamsChanged = true;
}

bool ezVisualScriptComponent::OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg)
{
  return m_pScriptInstance->HandleMessage(msg);
}

bool ezVisualScriptComponent::OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg) const
{
  return m_pScriptInstance->HandleMessage(msg);
}

void ezVisualScriptComponent::Initialize()
{
  SUPER::Initialize();

  EnableUnhandledMessageHandler(true);

  InitScriptInstance();
}

const ezRangeView<const char*, ezUInt32> ezVisualScriptComponent::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>([]() -> ezUInt32 { return 0; },
    [this]() -> ezUInt32 { return m_Params.GetCount(); }, [](ezUInt32& ref_uiIt) { ++ref_uiIt; },
    [this](const ezUInt32& uiIt) -> const char* { return m_Params[uiIt].m_sName.GetData(); });
}

void ezVisualScriptComponent::SetParameter(const char* szKey, const ezVariant& value)
{
  const ezTempHashedString th(szKey);

  for (auto& param : m_Params)
  {
    if (param.m_sName == th)
    {
      if (param.m_Value != value)
      {
        m_bParamsChanged = true;
        param.m_Value = value;
      }
      return;
    }
  }

  m_bParamsChanged = true;
  auto& param = m_Params.ExpandAndGetRef();
  param.m_sName.Assign(szKey);
  param.m_Value = value;
}

void ezVisualScriptComponent::RemoveParameter(const char* szKey)
{
  const ezTempHashedString th(szKey);

  for (ezUInt32 i = 0; i < m_Params.GetCount(); ++i)
  {
    if (m_Params[i].m_sName == th)
    {
      m_Params.RemoveAtAndSwap(i);
      return;
    }
  }
}

bool ezVisualScriptComponent::GetParameter(const char* szKey, ezVariant& out_value) const
{
  const ezTempHashedString th(szKey);

  for (const auto& param : m_Params)
  {
    if (param.m_sName == th)
    {
      out_value = param.m_Value;
      return true;
    }
  }

  return false;
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Implementation_VisualScriptComponent);
