#pragma once


#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/RendererCoreDLL.h>

class ezBlackboard;
class ezCVar;
struct ezCVarEvent;

/// Tracks CVars that configure render pipelines, i.e. those that start with "Rendering.Pipeline.".
class EZ_RENDERERCORE_DLL ezPipelineCVarRegistry
{
public:
  /// Returns all CVars that start with "Rendering.Pipeline."
  static ezArrayPtr<ezCVar* const> GetCVars();

  /// Applies all "Rendering.Pipeline." CVars to the given blackboard.
  static void ApplyCVarsToBlackboard(ezBlackboard* pBlackboard);
  /// Applies the given CVar to the given blackboard. Note that pCVar must start with "Rendering.Pipeline.".
  static void ApplyCVarToBlackboard(ezBlackboard* pBlackboard, const ezCVar* pCVar);

  /// Fired every time a CVar that starts with "Rendering.Pipeline." changes.
  static ezCVar::CVarEvents s_CVarChangedEvent;

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, PipelineCVarRegistry);

  static void CVarListChangedEventHandler(const ezCVarEvent& e);
  static void CVarChangedEventHandler(const ezCVarEvent& e);
  static void UpdateCVars();
  static void ClearCVars();

  static ezDynamicArray<ezCVar*> s_CVars;
  static ezDynamicArray<ezEventSubscriptionID> s_CVarEventSubscriptions;
  static ezHashTable<const ezCVar*, ezHashedString> s_CVarStrings;
};
