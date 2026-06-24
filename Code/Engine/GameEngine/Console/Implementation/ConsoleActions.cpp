#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/Startup.h>
#include <GameEngine/Console/ConsoleActions.h>

ezDynamicArray<ezConsoleActions::ezConsoleActionsDesc> ezConsoleActions::s_ConsoleActions;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(GameEngine, ConsoleActions)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezConsoleActions::ClearActions();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  ezInt32 CompareConsoleActions(const ezConsoleActions::ezConsoleActionsDesc& lhs, const ezConsoleActions::ezConsoleActionsDesc& rhs)
  {
    const ezInt32 iMenuCompare = lhs.m_sMenu.Compare(rhs.m_sMenu);
    if (iMenuCompare != 0)
      return iMenuCompare;

    return lhs.m_sAction.Compare(rhs.m_sAction);
  }
} // namespace

void ezConsoleActions::AddAction(ezStringView sInputSet, ezStringView sAction, ezStringView sMenu, Action action)
{
  ezConsoleActionsDesc desc;
  desc.m_sInputSet = sInputSet;
  desc.m_sAction = sAction;
  desc.m_sMenu = sMenu;
  desc.m_Action = std::move(action);

  RemoveAction(sInputSet, sAction);

  ezUInt32 uiInsertIndex = 0;
  while (uiInsertIndex < s_ConsoleActions.GetCount() && CompareConsoleActions(s_ConsoleActions[uiInsertIndex], desc) <= 0)
  {
    ++uiInsertIndex;
  }

  s_ConsoleActions.InsertAt(uiInsertIndex, std::move(desc));
}

void ezConsoleActions::RemoveAction(ezStringView sInputSet, ezStringView sAction)
{
  for (ezUInt32 uiActionIndex = 0; uiActionIndex < s_ConsoleActions.GetCount(); ++uiActionIndex)
  {
    if (s_ConsoleActions[uiActionIndex].m_sInputSet.IsEqual(sInputSet) && s_ConsoleActions[uiActionIndex].m_sAction.IsEqual(sAction))
    {
      s_ConsoleActions.RemoveAtAndCopy(uiActionIndex);
      return;
    }
  }
}

void ezConsoleActions::HandleInput()
{
  for (auto& desc : s_ConsoleActions)
  {
    if (ezInputManager::GetInputActionState(desc.m_sInputSet, desc.m_sAction) == ezKeyState::Pressed && desc.m_Action.IsValid())
    {
      desc.m_Action();
    }
  }
}

ezArrayPtr<const ezConsoleActions::ezConsoleActionsDesc> ezConsoleActions::GetActions()
{
  return s_ConsoleActions;
}

void ezConsoleActions::ClearActions()
{
  s_ConsoleActions.Clear();
}