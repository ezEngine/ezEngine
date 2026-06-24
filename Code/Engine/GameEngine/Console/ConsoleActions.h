#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Delegate.h>
#include <GameEngine/GameEngineDLL.h>

// Allows to add a simple action that is shown in the console menu.
// To execute shortcuts, call `HandleInput` during your application's 'Run_ProcessApplicationInput' function.
class EZ_GAMEENGINE_DLL ezConsoleActions
{
public:
  using Action = ezDelegate<void()>;

  struct ezConsoleActionsDesc
  {
    ezString m_sInputSet;
    ezString m_sAction;
    ezString m_sMenu;
    Action m_Action;
  };

  // Adds an action. sInputSet and sAction should match what was passed into 'ezInputManager::SetInputActionConfig' and uniquely identifies the action. Actions are sorted by their menu and action name.
  static void AddAction(ezStringView sInputSet, ezStringView sAction, ezStringView sMenu, Action action);
  // Removes an action.
  static void RemoveAction(ezStringView sInputSet, ezStringView sAction);

  // Executes any action who's shortcut is 'ezKeyState::Pressed'.
  static void HandleInput();
  // Returns all actions sorted by menu first, then by action.
  static ezArrayPtr<const ezConsoleActionsDesc> GetActions();
  static void ClearActions();

private:
  static ezDynamicArray<ezConsoleActionsDesc> s_ConsoleActions;
};