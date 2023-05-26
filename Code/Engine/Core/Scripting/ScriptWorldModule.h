#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>

using ezScriptClassResourceHandle = ezTypedResourceHandle<class ezScriptClassResource>;
class ezScriptInstance;

class EZ_CORE_DLL ezScriptWorldModule : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezScriptWorldModule, ezWorldModule);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezScriptWorldModule);

public:
  ezScriptWorldModule(ezWorld* pWorld);
  ~ezScriptWorldModule();

  virtual void Initialize() override;

  void AddUpdateFunctionToSchedule(const ezAbstractFunctionProperty* pFunction, void* pInstance, ezTime updateInterval, bool bOnlyWhenSimulating);
  void RemoveUpdateFunctionToSchedule(const ezAbstractFunctionProperty* pFunction, void* pInstance);

  /// \name Coroutine Functions
  ///@{

  /// \brief Creates a new coroutine of pCoroutineType with the given name. If the creationMode prevents creating a new coroutine,
  /// this function will return an invalid handle and a nullptr in out_pCoroutine if there is already a coroutine running
  /// with the same name on the given instance.
  ezScriptCoroutineHandle CreateCoroutine(const ezRTTI* pCoroutineType, ezStringView sName, ezScriptInstance& inout_instance, ezScriptCoroutineCreationMode::Enum creationMode, ezScriptCoroutine*& out_pCoroutine);

  /// \brief Starts the coroutine with the given arguments. This will call the Start() function and then UpdateAndSchedule() once on the coroutine object.
  void StartCoroutine(ezScriptCoroutineHandle hCoroutine, ezArrayPtr<ezVariant> arguments);

  /// \brief Stops and deletes the coroutine. This will call the Stop() function and will delete the coroutine on next update of the script world module.
  void StopAndDeleteCoroutine(ezScriptCoroutineHandle hCoroutine);

  /// \brief Stops and deletes all coroutines with the given name on pInstance.
  void StopAndDeleteCoroutine(ezStringView sName, ezScriptInstance* pInstance);

  /// \brief Stops and deletes all coroutines on pInstance.
  void StopAndDeleteAllCoroutines(ezScriptInstance* pInstance);

  /// \brief Returns whether the coroutine has already finished or has been stopped.
  bool IsCoroutineFinished(ezScriptCoroutineHandle hCoroutine) const;

  ///@}
  /// \name Script Reload Functions
  ///@{

  using ReloadFunction = ezDelegate<void()>;
  void AddScriptReloadFunction(ezScriptClassResourceHandle hScript, ReloadFunction function);
  void RemoveScriptReloadFunction(ezScriptClassResourceHandle hScript, ReloadFunction function);

  ///@}

  struct FunctionContext
  {
    ezPointerWithFlags<const ezAbstractFunctionProperty, 1> m_pFunctionAndFlags;
    void* m_pInstance = nullptr;

    bool operator==(const FunctionContext& other) const
    {
      return m_pFunctionAndFlags.GetPtr() == other.m_pFunctionAndFlags.GetPtr() && m_pInstance == other.m_pInstance;
    }
  };

private:
  void CallUpdateFunctions(const ezWorldModule::UpdateContext& context);
  void ReloadScripts(const ezWorldModule::UpdateContext& context);
  void ResourceEventHandler(const ezResourceEvent& e);

  ezIntervalScheduler<FunctionContext> m_Scheduler;

  ezIdTable<ezScriptCoroutineId, ezUniquePtr<ezScriptCoroutine>> m_RunningScriptCoroutines;
  ezHashTable<ezScriptInstance*, ezSmallArray<ezScriptCoroutineHandle, 8>> m_InstanceToScriptCoroutines;
  ezDynamicArray<ezUniquePtr<ezScriptCoroutine>> m_DeadScriptCoroutines;

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
    ezUInt32 hash = ezHashHelper<const void*>::Hash(value.m_pFunctionAndFlags);
    hash = ezHashingUtils::CombineHashValues32(hash, ezHashHelper<void*>::Hash(value.m_pInstance));
    return hash;
  }

  EZ_ALWAYS_INLINE static bool Equal(const ezScriptWorldModule::FunctionContext& a, const ezScriptWorldModule::FunctionContext& b) { return a == b; }
};
