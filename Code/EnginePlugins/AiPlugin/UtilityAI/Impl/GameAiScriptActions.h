#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Core/World/Declarations.h>
#include <Foundation/Reflection/Reflection.h>

class ezAiScriptContext;

/// Script functions that queue the built-in actions (see GameAiActions.h) in the agent's ezAiActionQueue.
///
/// Only work with the context passed to 'OnActivateBehavior' and 'OnReactivateBehavior' of an ezAiBehaviorScript,
/// elsewhere they log a warning and do nothing. The actions run one after another, after the script function returned.
/// The behavior stays active until the queue is empty. Positions are copied when the action is queued.
class EZ_AIPLUGIN_DLL ezAiScriptActions
{
public:
  static void QueueNavigateTo(ezAiScriptContext* pContext, const ezVec3& vTargetPosition, float fSpeed, bool bAllowPartialPath);

  /// Follows the object while it moves. Fails if the object is deleted.
  static void QueueNavigateToObject(ezAiScriptContext* pContext, ezGameObjectHandle hTargetObject, float fSpeed, bool bAllowPartialPath);

  /// See ezAiActionStopWalking.
  static void QueueStopWalking(ezAiScriptContext* pContext);

  /// See ezAiActionTurnTowards: has no effect while the agent walks.
  static void QueueTurnTowards(ezAiScriptContext* pContext, const ezVec3& vTargetPosition);

  static void QueueWait(ezAiScriptContext* pContext, ezTime duration);

  static void QueueBlackboardSet(ezAiScriptContext* pContext, ezStringView sEntryName, const ezVariant& value);

  /// Waits until the entry equals the value (or doesn't, if bEquals is false). A missing entry counts as equal. There is no timeout.
  static void QueueBlackboardWait(ezAiScriptContext* pContext, ezStringView sEntryName, const ezVariant& value, bool bEquals);

  /// See ezAiActionBlackboardSetAndWait, e.g. to trigger an animation and wait until the animation graph resets the entry.
  static void QueueBlackboardSetAndWait(ezAiScriptContext* pContext, ezStringView sEntryName, const ezVariant& setValue, const ezVariant& waitValue, ezTime timeout);

  /// Writes the text to the log when the action runs. For testing.
  static void QueueLog(ezAiScriptContext* pContext, ezStringView sText);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_AIPLUGIN_DLL, ezAiScriptActions);
