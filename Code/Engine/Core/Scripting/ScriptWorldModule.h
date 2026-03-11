#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>

using ezScriptClassResourceHandle = ezTypedResourceHandle<class ezScriptClassResource>;
class ezScriptInstance;

/// World module responsible for script execution and coroutine management.
///
/// Handles the execution of script functions, manages script coroutines,
/// and provides scheduling for script update functions. This module ensures
/// scripts are properly integrated with the world update cycle.
class EZ_CORE_DLL ezScriptWorldModule : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezScriptWorldModule, ezWorldModule);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezScriptWorldModule);

public:
  ezScriptWorldModule(ezWorld* pWorld);
  ~ezScriptWorldModule();

  virtual void Initialize() override;
  virtual void WorldClear() override;

  /// Schedules a script function to be called at regular intervals.
  void AddUpdateFunctionToSchedule(const ezAbstractFunctionProperty* pFunction, void* pInstance, ezTime updateInterval, bool bOnlyWhenSimulating);

  /// Removes a previously scheduled script function from the scheduler.
  void RemoveUpdateFunctionToSchedule(const ezAbstractFunctionProperty* pFunction, void* pInstance);

  /// \name Coroutine Functions
  ///@{

  /// Creates a new coroutine of the specified type with the given name.
  ///
  /// Returns an invalid handle if the creationMode prevents creating a new coroutine
  /// and there is already a coroutine running with the same name on the given instance.
  ezScriptCoroutineHandle CreateCoroutine(const ezRTTI* pCoroutineType, ezStringView sName, ezScriptInstance& inout_instance, ezScriptCoroutineCreationMode::Enum creationMode, ezScriptCoroutine*& out_pCoroutine);

  /// Starts the coroutine with the given arguments.
  ///
  /// Calls the Start() function and then UpdateAndSchedule() once on the coroutine object.
  void StartCoroutine(ezScriptCoroutineHandle hCoroutine, ezArrayPtr<ezVariant> arguments);

  /// Stops and deletes the coroutine.
  ///
  /// Calls the Stop() function and deletes the coroutine on the next update cycle.
  void StopAndDeleteCoroutine(ezScriptCoroutineHandle hCoroutine);

  /// Stops and deletes all coroutines with the given name on the specified instance.
  void StopAndDeleteCoroutine(ezStringView sName, ezScriptInstance* pInstance);

  /// Stops and deletes all coroutines on the specified instance.
  void StopAndDeleteAllCoroutines(ezScriptInstance* pInstance);

  /// Returns whether the coroutine has finished or been stopped.
  bool IsCoroutineFinished(ezScriptCoroutineHandle hCoroutine) const;

  ///@}

  /// Returns a shared expression VM for custom script implementations.
  ///
  /// The VM is NOT thread safe - only execute one expression at a time.
  ezExpressionVM& GetSharedExpressionVM() { return m_SharedExpressionVM; }

  /// Context information for scheduled script functions.
  struct FunctionContext
  {
    /// Flags controlling when the function should be executed.
    enum Flags : ezUInt8
    {
      None,              ///< Execute always
      OnlyWhenSimulating ///< Execute only during simulation
    };

    ezPointerWithFlags<const ezAbstractFunctionProperty, 1> m_pFunctionAndFlags;
    void* m_pInstance = nullptr;

    bool operator==(const FunctionContext& other) const
    {
      return m_pFunctionAndFlags == other.m_pFunctionAndFlags && m_pInstance == other.m_pInstance;
    }
  };

private:
  void CallUpdateFunctions(const ezWorldModule::UpdateContext& context);

  ezIntervalScheduler<FunctionContext> m_Scheduler;

  ezIdTable<ezScriptCoroutineId, ezUniquePtr<ezScriptCoroutine>> m_RunningScriptCoroutines;
  ezHashTable<ezScriptInstance*, ezSmallArray<ezScriptCoroutineHandle, 8>> m_InstanceToScriptCoroutines;
  ezDynamicArray<ezUniquePtr<ezScriptCoroutine>> m_DeadScriptCoroutines;

  ezExpressionVM m_SharedExpressionVM;
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
