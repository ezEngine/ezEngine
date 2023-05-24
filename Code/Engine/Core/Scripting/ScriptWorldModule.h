#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>

using ezScriptClassResourceHandle = ezTypedResourceHandle<class ezScriptClassResource>;

class EZ_CORE_DLL ezScriptWorldModule : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezScriptWorldModule, ezWorldModule);

public:
  ezScriptWorldModule(ezWorld* pWorld);
  ~ezScriptWorldModule();

  virtual void Initialize() override;

  void AddUpdateFunctionToSchedule(const ezAbstractFunctionProperty* pFunction, void* pInstance, ezTime updateInterval);
  void RemoveUpdateFunctionToSchedule(const ezAbstractFunctionProperty* pFunction, void* pInstance);

  using ReloadFunction = ezDelegate<void()>;
  void AddScriptReloadFunction(ezScriptClassResourceHandle hScript, ReloadFunction function);
  void RemoveScriptReloadFunction(ezScriptClassResourceHandle hScript, void* pInstance);

  struct FunctionContext
  {
    const ezAbstractFunctionProperty* m_pFunction = nullptr;
    void* m_pInstance = nullptr;

    bool operator==(const FunctionContext& other) const
    {
      return m_pFunction == other.m_pFunction && m_pInstance == other.m_pInstance;
    }
  };

private:
  void CallUpdateFunctions(const ezWorldModule::UpdateContext& context);
  void ReloadScripts(const ezWorldModule::UpdateContext& context);
  void ResourceEventHandler(const ezResourceEvent& e);

  ezIntervalScheduler<FunctionContext> m_Scheduler;

  using ReloadFunctionList = ezHybridArray<ReloadFunction, 8>;
  ezHashTable<ezScriptClassResourceHandle, ReloadFunctionList> m_ReloadFunctions;
  ezHashSet<ezScriptClassResourceHandle> m_NeedReload;
  ReloadFunctionList m_TempReloadFunctions;
};

//////////////////////////////////////////////////////////////////////////

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
