#include <GameEnginePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/VisualScript/VisualScriptComponent.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>

ezEvent<const ezVisualScriptComponentActivityEvent&> ezVisualScriptComponent::s_ActivityEvents;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezVisualScriptComponent, 3, ezComponentMode::Static);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Script", GetScriptFile, SetScriptFile)->AddAttributes(new ezAssetBrowserAttribute("Visual Script")),
    EZ_ACCESSOR_PROPERTY("HandleGlobalEvents", GetIsGlobalEventHandler, SetIsGlobalEventHandler),
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

ezVisualScriptComponent::ezVisualScriptComponent() {}

ezVisualScriptComponent::~ezVisualScriptComponent()
{
  m_hResource.Invalidate();
  m_Script.Clear();
}

void ezVisualScriptComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hResource;
  s << m_bGlobalEventHandler;
  /// \todo Store the current script state

  // Version 3
  s << m_NumberParams.GetCount();
  for (ezUInt32 i = 0; i < m_NumberParams.GetCount(); ++i)
  {
    s << m_NumberParams[i].m_sName;
    s << m_NumberParams[i].m_Value;
  }
  s << m_BoolParams.GetCount();
  for (ezUInt32 i = 0; i < m_BoolParams.GetCount(); ++i)
  {
    s << m_BoolParams[i].m_sName;
    s << m_BoolParams[i].m_Value;
  }
}

void ezVisualScriptComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hResource;

  bool globalEventHandler = false; // dummy to prevent early out in SetIsGlobalEventHandler
  if (uiVersion >= 2)
  {
    s >> globalEventHandler;
  }

  if (uiVersion >= 3)
  {
    ezUInt32 numNums, numBools;

    s >> numNums;
    m_NumberParams.SetCount(numNums);

    for (ezUInt32 i = 0; i < m_NumberParams.GetCount(); ++i)
    {
      s >> m_NumberParams[i].m_sName;
      s >> m_NumberParams[i].m_Value;
    }

    m_bNumberParamsChanged = numNums > 0;

    s >> numBools;
    m_BoolParams.SetCount(numBools);

    for (ezUInt32 i = 0; i < m_BoolParams.GetCount(); ++i)
    {
      s >> m_BoolParams[i].m_sName;
      s >> m_BoolParams[i].m_Value;
    }

    m_bBoolParamsChanged = numBools > 0;
  }

  /// \todo Read script state
  SetIsGlobalEventHandler(globalEventHandler);
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
    SetGlobalEventHandlerMode(m_bGlobalEventHandler);
  }
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
      m_Script->Configure(m_hResource, GetOwner());
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
    if (m_bNumberParamsChanged)
    {
      m_bNumberParamsChanged = false;

      for (ezUInt32 i = 0; i < m_NumberParams.GetCount(); ++i)
      {
        const auto& e = m_NumberParams[i];
        m_Script->GetLocalVariables().StoreDouble(e.m_sName, e.m_Value);
      }

      // TODO: atm we clear this, because it stores only the initial state, and any mutated state is stored
      // in the VS instance, which we do not sync with
      // thefore if we don't clear, modifying ANY value would reset ALL values to the start value
      m_NumberParams.Clear();
    }

    if (m_bBoolParamsChanged)
    {
      m_bBoolParamsChanged = false;

      for (ezUInt32 i = 0; i < m_BoolParams.GetCount(); ++i)
      {
        const auto& e = m_BoolParams[i];
        m_Script->GetLocalVariables().StoreBool(e.m_sName, e.m_Value);
      }

      // TODO: atm we clear this, because it stores only the initial state, and any mutated state is stored
      // in the VS instance, which we do not sync with
      // thefore if we don't clear, modifying ANY value would reset ALL values to the start value
      m_BoolParams.Clear();
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

bool ezVisualScriptComponent::OnUnhandledMessage(ezMessage& msg)
{
  return m_Script->HandleMessage(msg);
}

bool ezVisualScriptComponent::OnUnhandledMessage(ezMessage& msg) const
{
  return m_Script->HandleMessage(msg);
}

const ezRangeView<const char*, ezUInt32> ezVisualScriptComponent::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>([]() -> ezUInt32 { return 0; },
    [this]() -> ezUInt32 { return m_NumberParams.GetCount() + m_BoolParams.GetCount(); }, [](ezUInt32& it) { ++it; },
    [this](const ezUInt32& it) -> const char* {
      if (it < m_NumberParams.GetCount())
        return m_NumberParams[it].m_sName.GetData();
      else
        return m_BoolParams[it - m_NumberParams.GetCount()].m_sName.GetData();
    });
}

void ezVisualScriptComponent::SetParameter(const char* szKey, const ezVariant& var)
{
  const ezTempHashedString th(szKey);
  if (var.CanConvertTo<float>())
  {
    float value = var.ConvertTo<float>();

    for (ezUInt32 i = 0; i < m_NumberParams.GetCount(); ++i)
    {
      if (m_NumberParams[i].m_sName == th)
      {
        if (m_NumberParams[i].m_Value != value)
        {
          m_bNumberParamsChanged = true;
          m_NumberParams[i].m_Value = value;
        }
        return;
      }
    }

    m_bNumberParamsChanged = true;
    auto& e = m_NumberParams.ExpandAndGetRef();
    e.m_sName.Assign(szKey);
    e.m_Value = value;

    return;
  }

  if (var.CanConvertTo<bool>())
  {
    bool value = var.ConvertTo<bool>();

    for (ezUInt32 i = 0; i < m_BoolParams.GetCount(); ++i)
    {
      if (m_BoolParams[i].m_sName == th)
      {
        if (m_BoolParams[i].m_Value != value)
        {
          m_bBoolParamsChanged = true;
          m_BoolParams[i].m_Value = value;
        }
        return;
      }
    }

    m_bBoolParamsChanged = true;
    auto& e = m_BoolParams.ExpandAndGetRef();
    e.m_sName.Assign(szKey);
    e.m_Value = value;

    return;
  }
}

void ezVisualScriptComponent::RemoveParameter(const char* szKey)
{
  const ezTempHashedString th(szKey);

  for (ezUInt32 i = 0; i < m_NumberParams.GetCount(); ++i)
  {
    if (m_NumberParams[i].m_sName == th)
    {
      m_NumberParams.RemoveAtAndSwap(i);
      return;
    }
  }

  for (ezUInt32 i = 0; i < m_BoolParams.GetCount(); ++i)
  {
    if (m_BoolParams[i].m_sName == th)
    {
      m_BoolParams.RemoveAtAndSwap(i);
      return;
    }
  }
}

bool ezVisualScriptComponent::GetParameter(const char* szKey, ezVariant& out_value) const
{
  const ezTempHashedString th(szKey);

  for (const auto& e : m_NumberParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  for (const auto& e : m_BoolParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  return false;
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Implementation_VisualScriptComponent);
