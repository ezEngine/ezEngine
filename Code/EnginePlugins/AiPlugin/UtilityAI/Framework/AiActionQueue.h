#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/UtilityAI/Framework/AiAction.h>

class ezLogInterface;

/// Sequence of ezAiAction objects that are executed one after another.
///
/// Each NPC has one queue. The active behavior fills it and usually checks whether it is empty to know that it is done.
/// Takes ownership of queued actions and destroys them once they are finished.
class EZ_AIPLUGIN_DLL ezAiActionQueue
{
public:
  ezAiActionQueue();
  ~ezAiActionQueue();

  bool IsEmpty() const;

  /// Destroys all actions immediately and clears the queue.
  ///
  /// This is likely to leave the game object in an intermediate (broken) state,
  /// since actions won't clean up after themselves.
  ///
  /// This should only be called when the action queue isn't needed anymore, at all,
  /// and the proper state of the the target object is enforced with other means.
  void InterruptAndClear();

  /// Instructs all currently queued actions to cancel their current or future work.
  ///
  /// This does not remove any action (not even the ones that aren't started yet),
  /// and it doesn't guarantee that actions won't do anything anymore.
  /// In fact for some actions it is quite important that they do something,
  /// even when canceled, to leave the object in a clean state.
  ///
  /// Behaviors typically call this in OnActivateBehavior() before queuing their own actions.
  /// The canceled actions stay in front of the new ones and usually finish within the next Execute().
  void CancelCurrentActions(ezGameObject& owner);

  /// Appends an action for execution after the previous actions are finished.
  ///
  /// The queue takes ownership. pAction must come from the action's Create() function.
  void QueueAction(ezAiAction* pAction);

  /// Executes the first action in the queue.
  ///
  /// If the action returns Finished, it is destroyed and the next action gets executed in the same frame.
  /// This continues until an action returns Succeded (needs more time) or the queue is empty.
  /// If an action fails, an error is logged and the entire queue gets canceled via CancelCurrentActions().
  /// The description of the failed action is then written to out_pFailedAction, if given.
  void Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog, ezStringBuilder* out_pFailedAction = nullptr);

  /// Draws the description of the current action as screen-space info text.
  void PrintDebugInfo(ezGameObject& owner);

  /// Writes the description of the currently executing action and the number of actions after it.
  void GetDebugDesc(ezStringBuilder& out_sText) const;

private:
  ezHybridArray<ezAiAction*, 16> m_Queue;
};
