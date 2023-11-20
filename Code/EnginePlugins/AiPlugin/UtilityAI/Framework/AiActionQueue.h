#pragma once

#include <AiPlugin/UtilityAI/Framework/AiAction.h>

class ezLogInterface;

class EZ_AIPLUGIN_DLL ezAiActionQueue
{
public:
  ezAiActionQueue();
  ~ezAiActionQueue();

  /// \brief Returns true when no action is currently queued.
  bool IsEmpty() const;

  /// \brief Destroys all actions immediately and clears the queue.
  ///
  /// This is likely to leave the game object in an intermediate (broken) state,
  /// since actions won't clean up after themselves.
  ///
  /// This should only be called when the action queue isn't needed anymore, at all,
  /// and the proper state of the the target object is enforced with other means.
  void InterruptAndClear();

  /// \brief Instructs all currently queued actions to cancel their current or future work.
  ///
  /// This does not remove any action (not even the ones that aren't started yet),
  /// and it doesn't guarantee that actions won't do anything anymore.
  /// In fact for some actions it is quite important that they do something,
  /// even when canceled, to leave the object in a clean state.
  void CancelCurrentActions(ezGameObject& owner);

  /// \brief Appends an action for execution after the previous actions are finished.
  void QueueAction(ezAiAction* pAction);

  /// \brief Executes the first action in the queue.
  ///
  /// If the action finishes successfully, the next action gets executed.
  /// If an action fails to execute the entire queue gets canceled via CancelCurrentActions().
  void Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog);

  void PrintDebugInfo(ezGameObject& owner);

private:
  ezHybridArray<ezAiAction*, 16> m_Queue;
};
