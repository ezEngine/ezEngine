#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptWorldModule.h>

template <>
struct ezHashHelper<ezScriptWorldModule::FunctionContext>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezScriptWorldModule::FunctionContext& value)
  {
    ezUInt32 hash = ezHashHelper<const void*>::Hash(value.m_pFunction);
    hash = ezHashingUtils::CombineHashValues32(hash, ezHashHelper<void*>::Hash(value.m_pInstance));
    return hash;
  }

  EZ_ALWAYS_INLINE static bool Equal(const ezScriptWorldModule::FunctionContext& a, const ezScriptWorldModule::FunctionContext& b) { return a == b; }
};

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezScriptWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScriptWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezScriptWorldModule::ezScriptWorldModule(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezScriptWorldModule::ResourceEventHandler, this));
}

ezScriptWorldModule::~ezScriptWorldModule()
{
  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezScriptWorldModule::ResourceEventHandler, this));
}

void ezScriptWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezScriptWorldModule::CallUpdateFunctions, this);
    updateDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(updateDesc);
  }

  {
    auto updateDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezScriptWorldModule::ReloadScripts, this);
    updateDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    updateDesc.m_fPriority = 10000.0f;

    RegisterUpdateFunction(updateDesc);
  }
}

void ezScriptWorldModule::AddUpdateFunctionToSchedule(const ezAbstractFunctionProperty* pFunction, void* pInstance, ezTime updateInterval)
{
  FunctionContext context;
  context.m_pFunction = pFunction;
  context.m_pInstance = pInstance;

  m_Scheduler.AddOrUpdateWork(context, updateInterval);
}

void ezScriptWorldModule::RemoveUpdateFunctionToSchedule(const ezAbstractFunctionProperty* pFunction, void* pInstance)
{
  FunctionContext context;
  context.m_pFunction = pFunction;
  context.m_pInstance = pInstance;

  m_Scheduler.RemoveWork(context);
}

void ezScriptWorldModule::AddScriptReloadFunction(ezScriptClassResourceHandle hScript, ReloadFunction function)
{
  if (hScript.IsValid() == false)
    return;

  m_ReloadFunctions[hScript].PushBack(function);
}

void ezScriptWorldModule::RemoveScriptReloadFunction(ezScriptClassResourceHandle hScript, void* pInstance)
{
  ReloadFunctionList* pReloadFunctions = nullptr;
  if (m_ReloadFunctions.TryGetValue(hScript, pReloadFunctions))
  {
    for (ezUInt32 i = 0; i < pReloadFunctions->GetCount(); ++i)
    {
      if ((*pReloadFunctions)[i].GetClassInstance() == pInstance)
      {
        pReloadFunctions->RemoveAtAndSwap(i);
        break;
      }
    }
  }
}

void ezScriptWorldModule::CallUpdateFunctions(const ezWorldModule::UpdateContext& context)
{
  const ezTime deltaTime = GetWorld()->GetClock().GetTimeDiff();
  m_Scheduler.Update(deltaTime, [this](const FunctionContext& context, ezTime deltaTime)
    {
    ezVariant returnValue;
    context.m_pFunction->Execute(context.m_pInstance, ezArrayPtr<ezVariant>(), returnValue); });
}

void ezScriptWorldModule::ReloadScripts(const ezWorldModule::UpdateContext& context)
{
  for (auto hScript : m_NeedReload)
  {
    if (m_ReloadFunctions.TryGetValue(hScript, m_TempReloadFunctions))
    {
      for (auto& reloadFunction : m_TempReloadFunctions)
      {
        reloadFunction();
      }
    }
  }

  m_NeedReload.Clear();
}

void ezScriptWorldModule::ResourceEventHandler(const ezResourceEvent& e)
{
  if (e.m_Type != ezResourceEvent::Type::ResourceContentUnloading)
    return;

  if (auto pResource = ezDynamicCast<const ezScriptClassResource*>(e.m_pResource))
  {
    ezScriptClassResourceHandle hScript = pResource->GetResourceHandle();
    m_NeedReload.Insert(hScript);
  }
}
