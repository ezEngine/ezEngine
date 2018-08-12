#include <PCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCommandHistoryAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezCommandHistoryActions::s_hCommandHistoryCategory;
ezActionDescriptorHandle ezCommandHistoryActions::s_hUndo;
ezActionDescriptorHandle ezCommandHistoryActions::s_hRedo;

void ezCommandHistoryActions::RegisterActions()
{
  s_hCommandHistoryCategory = EZ_REGISTER_CATEGORY("CmdHistoryCategory");
  s_hUndo = EZ_REGISTER_ACTION_AND_DYNAMIC_MENU_1("Document.Undo", ezActionScope::Document, "Document", "Ctrl+Z", ezCommandHistoryAction,
                                                  ezCommandHistoryAction::ButtonType::Undo);
  s_hRedo = EZ_REGISTER_ACTION_AND_DYNAMIC_MENU_1("Document.Redo", ezActionScope::Document, "Document", "Ctrl+Y", ezCommandHistoryAction,
                                                  ezCommandHistoryAction::ButtonType::Redo);
}

void ezCommandHistoryActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCommandHistoryCategory);
  ezActionManager::UnregisterAction(s_hUndo);
  ezActionManager::UnregisterAction(s_hRedo);
}

void ezCommandHistoryActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/CmdHistoryCategory");

  pMap->MapAction(s_hCommandHistoryCategory, szPath, 3.0f);
  pMap->MapAction(s_hUndo, sSubPath, 1.0f);
  pMap->MapAction(s_hRedo, sSubPath, 2.0f);
}

ezCommandHistoryAction::ezCommandHistoryAction(const ezActionContext& context, const char* szName, ButtonType button)
    : ezDynamicActionAndMenuAction(context, szName, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case ezCommandHistoryAction::ButtonType::Undo:
      SetIconPath(":/GuiFoundation/Icons/Undo16.png");
      break;
    case ezCommandHistoryAction::ButtonType::Redo:
      SetIconPath(":/GuiFoundation/Icons/Redo16.png");
      break;
  }

  m_Context.m_pDocument->GetCommandHistory()->m_Events.AddEventHandler(
      ezMakeDelegate(&ezCommandHistoryAction::CommandHistoryEventHandler, this));

  UpdateState();
}

ezCommandHistoryAction::~ezCommandHistoryAction()
{
  m_Context.m_pDocument->GetCommandHistory()->m_Events.RemoveEventHandler(
      ezMakeDelegate(&ezCommandHistoryAction::CommandHistoryEventHandler, this));
}

void ezCommandHistoryAction::GetEntries(ezHybridArray<ezDynamicMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();

  ezCommandHistory* pHistory = m_Context.m_pDocument->GetCommandHistory();

  const ezUInt32 iCount = (m_ButtonType == ButtonType::Undo) ? pHistory->GetUndoStackSize() : pHistory->GetRedoStackSize();
  for (ezUInt32 i = 0; i < iCount; i++)
  {
    const ezCommandTransaction* pTransaction =
        (m_ButtonType == ButtonType::Undo) ? pHistory->GetUndoStackEntry(i) : pHistory->GetRedoStackEntry(i);
    ezDynamicMenuAction::Item entryItem;
    entryItem.m_sDisplay = pTransaction->m_sDisplayString;
    entryItem.m_UserValue = (ezUInt32)i + 1; // Number of steps to undo / redo.
    out_Entries.PushBack(entryItem);
  }
}

void ezCommandHistoryAction::Execute(const ezVariant& value)
{
  ezUInt32 iCount = value.IsValid() ? value.ConvertTo<ezUInt32>() : 1;

  switch (m_ButtonType)
  {
    case ButtonType::Undo:
    {
      EZ_ASSERT_DEV(m_Context.m_pDocument->GetCommandHistory()->CanUndo(), "The action should not be active");

      auto stat = m_Context.m_pDocument->GetCommandHistory()->Undo(iCount);
      ezQtUiServices::MessageBoxStatus(stat, "Could not execute the Undo operation");
    }
    break;

    case ButtonType::Redo:
    {
      EZ_ASSERT_DEV(m_Context.m_pDocument->GetCommandHistory()->CanRedo(), "The action should not be active");

      auto stat = m_Context.m_pDocument->GetCommandHistory()->Redo(iCount);
      ezQtUiServices::MessageBoxStatus(stat, "Could not execute the Redo operation");
    }
    break;
  }
}

void ezCommandHistoryAction::UpdateState()
{
  switch (m_ButtonType)
  {
    case ButtonType::Undo:
      SetAdditionalDisplayString(m_Context.m_pDocument->GetCommandHistory()->GetUndoDisplayString(), false);
      SetEnabled(m_Context.m_pDocument->GetCommandHistory()->CanUndo());
      break;

    case ButtonType::Redo:
      SetAdditionalDisplayString(m_Context.m_pDocument->GetCommandHistory()->GetRedoDisplayString(), false);
      SetEnabled(m_Context.m_pDocument->GetCommandHistory()->CanRedo());
      break;
  }
}

void ezCommandHistoryAction::CommandHistoryEventHandler(const ezCommandHistoryEvent& e)
{
  UpdateState();
}
