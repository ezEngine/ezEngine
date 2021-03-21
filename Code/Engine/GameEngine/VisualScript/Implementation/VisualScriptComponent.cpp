#include <GameEnginePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/VisualScript/VisualScriptComponent.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>

ezEvent<const ezVisualScriptComponentActivityEvent&> ezVisualScriptComponent::s_ActivityEvents;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezVisualScriptComponent, 5, ezComponentMode::Static);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Script", GetScriptFile, SetScriptFile)->AddAttributes(new ezAssetBrowserAttribute("Visual Script")),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("Script"), new ezExposeColorAlphaAttribute),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Scripting"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezVisualScriptComponent::ezVisualScriptComponent() = default;
ezVisualScriptComponent::~ezVisualScriptComponent() = default;

void ezVisualScriptComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

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

void ezVisualScriptComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

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
}

bool ezVisualScriptComponent::HandlesEventMessage(const ezEventMessage& msg) const
{
  if (m_Script)
  {
    return m_Script->HandlesEventMessage(msg);
  }

  return false;
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
      m_Script->Configure(m_hResource, this);
    }
  }

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
          m_Script->GetLocalVariables().StoreBool(param.m_sName, param.m_Value.Get<bool>());
        }
        else if (param.m_Value.IsA<ezString>())
        {
          m_Script->GetLocalVariables().StoreString(param.m_sName, param.m_Value.Get<ezString>());
        }
        else if (param.m_Value.IsNumber())
        {
          m_Script->GetLocalVariables().StoreDouble(param.m_sName, param.m_Value.ConvertTo<double>());
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

  m_Script->ExecuteScript(m_pActivity.Borrow());

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

bool ezVisualScriptComponent::OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg)
{
  return m_Script->HandleMessage(msg);
}

bool ezVisualScriptComponent::OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg) const
{
  return m_Script->HandleMessage(msg);
}

void ezVisualScriptComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  EnableUnhandledMessageHandler(true);

  m_Script = EZ_DEFAULT_NEW(ezVisualScriptInstance);

  if (m_hResource.IsValid())
  {
    m_Script->Configure(m_hResource, this);
  }
}

const ezRangeView<const char*, ezUInt32> ezVisualScriptComponent::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>([]() -> ezUInt32 { return 0; },
    [this]() -> ezUInt32 { return m_Params.GetCount(); }, [](ezUInt32& it) { ++it; },
    [this](const ezUInt32& it) -> const char* { return m_Params[it].m_sName.GetData(); });
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
