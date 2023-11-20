#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/UtilityAI/Framework/AiContext.h>
#include <Core/Scripting/ScriptClassResource.h>
#include <Core/World/Declarations.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/Reflection.h>

class ezAiPerception;
class ezAiActionQueue;

/// What scripts of AI nodes (ezAiConsiderationScript, ezAiBehaviorScript) get passed. Gives access to the agent and receives the results.
///
/// Created on the stack for a single script call, scripts must not store it.
class EZ_AIPLUGIN_DLL ezAiScriptContext
{
public:
  ezAiScriptContext() = default;
  ezAiScriptContext(const ezAiContext& ctx);

  /// The game object of the agent. Null if it was deleted.
  ezGameObject* GetOwner() const;

  /// Same clock as ezAiPerceptionTarget::GetLastSeen().
  ezTime GetNow() const;

  /// The perceptions of the node's 'Perceptions' pin, if it has one.
  ezUInt32 GetPerceptionCount() const;
  /// Not const, because reflected functions can't return const pointers. Scripts only read perceptions.
  ezAiPerception* GetPerception(ezUInt32 uiIndex) const;

  /// For considerations: the raw value, which is then mapped through the input range and the curve. 0 if the script doesn't set it.
  void SetValue(float fValue);

  /// For script behaviors, in 'CanRunBehavior': false prevents the behavior from running with this candidate. True if the script doesn't call it.
  void SetCanRun(bool bCanRun);

  /// For script behaviors, in 'OnReactivateBehavior': cancels the queued actions and calls 'OnActivateBehavior' again with the new candidate.
  void Restart();

  ezWorld* m_pWorld = nullptr;
  ezGameObjectHandle m_hOwner;
  ezAiActionQueue* m_pActionQueue = nullptr; ///< Only set where a script may queue actions (script behaviors, when activated or reactivated).
  ezHybridArray<const ezAiPerception*, 16> m_Perceptions;
  float m_fValue = 0.0f;
  bool m_bCanRun = true;
  bool m_bRestart = false;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_AIPLUGIN_DLL, ezAiScriptContext);

/// Per-agent instance of the script class of an AI node, lives in the node's instance data.
///
/// The script is instantiated on first use, because the script class resource can't be loaded while the controller asset is transformed.
/// The script owner is the node itself (shared by all agents), so the scripts reach the agent through ezAiScriptContext.
/// Reloads the script when the resource changes.
class EZ_AIPLUGIN_DLL ezAiScriptInstance
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAiScriptInstance);

public:
  ezAiScriptInstance();
  ~ezAiScriptInstance();

  /// Returns the script function with the given ezScriptBaseClassFunctionAttribute index, or null if the script doesn't implement it.
  ///
  /// Instantiates the script on the first call. Logs an error once, if the script can't be loaded or isn't derived from pBaseType.
  const ezAbstractFunctionProperty* GetFunction(const ezAiContext& ctx, ezUInt32 uiFunctionIndex, ezReflectedClass& ref_owner, const ezRTTI* pBaseType, ezStringView sScriptClass, const ezArrayMap<ezHashedString, ezVariant>& parameters);

  /// Only valid after GetFunction() returned a function.
  ezScriptInstance* GetScriptInstance() { return m_pInstance.Borrow(); }

  /// Calls the function with the given arguments.
  ///
  /// AI scripts must not wait, they queue actions instead (see ezAiScriptActions). A script function that contains coroutine nodes
  /// (in any branch) is a coroutine, whose nodes up to the first wait run during this call. If it still waits afterwards,
  /// it is stopped and a warning is logged, the rest of the function doesn't run.
  void Call(const ezAiContext& ctx, const ezAbstractFunctionProperty* pFunction, ezArrayPtr<ezVariant> arguments);

private:
  void Instantiate();
  void Clear();

  ezWorld* m_pWorld = nullptr;
  ezReflectedClass* m_pOwner = nullptr;
  const ezRTTI* m_pBaseType = nullptr;
  const ezArrayMap<ezHashedString, ezVariant>* m_pParameters = nullptr;
  ezScriptClassResourceHandle m_hScriptClass;
  bool m_bFailed = false;
  bool m_bWarnedAboutCoroutine = false;

  ezSharedPtr<ezScriptRTTI> m_pScriptType;
  ezUniquePtr<ezScriptInstance> m_pInstance;
};
