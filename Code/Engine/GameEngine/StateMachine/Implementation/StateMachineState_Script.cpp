#include <GameEngine/GameEnginePCH.h>

#include <Core/Scripting/ScriptWorldModule.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GameEngine/StateMachine/StateMachineState_Script.h>

namespace
{
  struct ScriptInstanceData
  {
    ezReflectedClass* m_pOwner = nullptr;

    ezStateMachineInstance* m_pStateMachineInstance = nullptr;
    const ezArrayMap<ezHashedString, ezVariant>* m_pParameters = nullptr;
    ezScriptClassResourceHandle m_hScriptClass;

    ezSharedPtr<ezScriptRTTI> m_pScriptType;
    ezUniquePtr<ezScriptInstance> m_pInstance;

    ~ScriptInstanceData()
    {
      ClearInstance();
    }

    void InstantiateScript(const ezStateMachineState* pFromState = nullptr)
    {
      ClearInstance();

      ezResourceLock<ezScriptClassResource> pScript(m_hScriptClass, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pScript.GetAcquireResult() != ezResourceAcquireResult::Final)
      {
        ezLog::Error("Failed to load script '{}'", m_hScriptClass.GetResourceID());
        return;
      }

      auto pScriptType = pScript->GetType();
      if (pScriptType == nullptr || pScriptType->IsDerivedFrom(ezGetStaticRTTI<ezStateMachineState>()) == false)
      {
        ezLog::Error("Script type '{}' is not a state machine state", pScriptType != nullptr ? pScriptType->GetTypeName() : "NULL");
        return;
      }

      m_pScriptType = pScriptType;

      m_pInstance = pScript->Instantiate(*m_pOwner, m_pStateMachineInstance->GetOwnerWorld());
      if (m_pInstance != nullptr)
      {
        m_pInstance->SetInstanceVariables(*m_pParameters);
      }

      if (ezWorld* pWorld = m_pStateMachineInstance->GetOwnerWorld())
      {
        pWorld->AddResourceReloadFunction(m_hScriptClass, ezComponentHandle(), this,
          [](ezWorld::ResourceReloadContext& context)
          {
            static_cast<ScriptInstanceData*>(context.m_pUserData)->ReloadScript();
          });
      }

      CallOnEnter(pFromState);
    }

    void ClearInstance()
    {
      CallOnExit(nullptr);

      if (m_pStateMachineInstance != nullptr)
      {
        if (ezWorld* pWorld = m_pStateMachineInstance->GetOwnerWorld())
        {
          auto pModule = pWorld->GetOrCreateModule<ezScriptWorldModule>();
          pModule->StopAndDeleteAllCoroutines(m_pInstance.Borrow());

          pWorld->RemoveResourceReloadFunction(m_hScriptClass, ezComponentHandle(), this);
        }
      }

      m_pInstance = nullptr;
      m_pScriptType = nullptr;
    }

    void ReloadScript()
    {
      InstantiateScript();
    }

    const ezAbstractFunctionProperty* GetScriptFunction(ezUInt32 uiFunctionIndex)
    {
      if (m_pScriptType != nullptr && m_pInstance != nullptr)
      {
        return m_pScriptType->GetFunctionByIndex(uiFunctionIndex);
      }

      return nullptr;
    }

    void CallOnEnter(const ezStateMachineState* pFromState)
    {
      if (auto pFunction = GetScriptFunction(ezStateMachineState_ScriptBaseClassFunctions::OnEnter))
      {
        ezVariant args[] = {m_pStateMachineInstance, pFromState};
        ezVariant returnValue;
        pFunction->Execute(m_pInstance.Borrow(), ezMakeArrayPtr(args), returnValue);
      }
    }

    void CallOnExit(const ezStateMachineState* pToState)
    {
      if (auto pFunction = GetScriptFunction(ezStateMachineState_ScriptBaseClassFunctions::OnExit))
      {
        ezVariant args[] = {m_pStateMachineInstance, pToState};
        ezVariant returnValue;
        pFunction->Execute(m_pInstance.Borrow(), ezMakeArrayPtr(args), returnValue);
      }
    }

    void CallUpdate(ezTime deltaTime)
    {
      if (auto pFunction = GetScriptFunction(ezStateMachineState_ScriptBaseClassFunctions::Update))
      {
        ezVariant args[] = {m_pStateMachineInstance, deltaTime};
        ezVariant returnValue;
        pFunction->Execute(m_pInstance.Borrow(), ezMakeArrayPtr(args), returnValue);
      }
    }
  };
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineState_Script, 1, ezRTTIDefaultAllocator<ezStateMachineState_Script>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("ScriptClass", GetScriptClassFile, SetScriptClassFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_ScriptClass")),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("ScriptClass")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStateMachineState_Script::ezStateMachineState_Script(ezStringView sName)
  : ezStateMachineState(sName)
{
}

ezStateMachineState_Script::~ezStateMachineState_Script() = default;

void ezStateMachineState_Script::OnEnter(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pFromState) const
{
  auto& instanceData = *static_cast<ScriptInstanceData*>(pInstanceData);

  if (instanceData.m_pInstance == nullptr)
  {
    instanceData.m_pOwner = const_cast<ezStateMachineState_Script*>(this);
    instanceData.m_pStateMachineInstance = &ref_instance;
    instanceData.m_pParameters = &m_Parameters;
    instanceData.m_hScriptClass = ezResourceManager::LoadResource<ezScriptClassResource>(m_sScriptClassFile);

    instanceData.InstantiateScript(pFromState);
  }
  else
  {
    instanceData.CallOnEnter(pFromState);
  }
}

void ezStateMachineState_Script::OnExit(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pToState) const
{
  auto& instanceData = *static_cast<ScriptInstanceData*>(pInstanceData);
  instanceData.CallOnExit(pToState);
}

void ezStateMachineState_Script::Update(ezStateMachineInstance& ref_instance, void* pInstanceData, ezTime deltaTime) const
{
  auto& instanceData = *static_cast<ScriptInstanceData*>(pInstanceData);
  instanceData.CallUpdate(deltaTime);
}

ezResult ezStateMachineState_Script::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sScriptClassFile;

  ezUInt16 uiNumParams = static_cast<ezUInt16>(m_Parameters.GetCount());
  inout_stream << uiNumParams;

  for (ezUInt32 p = 0; p < uiNumParams; ++p)
  {
    inout_stream << m_Parameters.GetKey(p);
    inout_stream << m_Parameters.GetValue(p);
  }

  return EZ_SUCCESS;
}

ezResult ezStateMachineState_Script::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_sScriptClassFile;

  ezUInt16 uiNumParams = 0;
  inout_stream >> uiNumParams;
  m_Parameters.Reserve(uiNumParams);

  ezHashedString key;
  ezVariant value;
  for (ezUInt32 p = 0; p < uiNumParams; ++p)
  {
    inout_stream >> key;
    inout_stream >> value;

    m_Parameters.Insert(key, value);
  }

  return EZ_SUCCESS;
}

bool ezStateMachineState_Script::GetInstanceDataDesc(ezInstanceDataDesc& out_desc)
{
  out_desc.FillFromType<ScriptInstanceData>();
  return true;
}

void ezStateMachineState_Script::SetScriptClassFile(const char* szFile)
{
  m_sScriptClassFile = szFile;

  // Note that we can't load the resource here directly. State machine states are instantiated during
  // state machine asset transform but the script class resource overwrites are not known there so the resource load would fail.
}

const char* ezStateMachineState_Script::GetScriptClassFile() const
{
  return m_sScriptClassFile;
}

const ezRangeView<const char*, ezUInt32> ezStateMachineState_Script::GetParameters() const
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

void ezStateMachineState_Script::SetParameter(const char* szKey, const ezVariant& value)
{
  ezHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != ezInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void ezStateMachineState_Script::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(ezTempHashedString(szKey)))
  {
  }
}

bool ezStateMachineState_Script::GetParameter(const char* szKey, ezVariant& out_value) const
{
  ezUInt32 it = m_Parameters.Find(szKey);

  if (it == ezInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachineState_Script);
