#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>

///
class EZ_GUIFOUNDATION_DLL ezCommandHistoryActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hCommandHistoryCategory;
  static ezActionDescriptorHandle s_hUndo;
  static ezActionDescriptorHandle s_hRedo;

};


///
class EZ_GUIFOUNDATION_DLL ezCommandHistoryAction : public ezDynamicActionAndMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCommandHistoryAction, ezDynamicActionAndMenuAction);
public:

  enum class ButtonType
  {
    Undo,
    Redo,
  };

  ezCommandHistoryAction(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezCommandHistoryAction();

  virtual void Execute(const ezVariant& value) override;
  virtual void GetEntries(ezHybridArray<ezDynamicMenuAction::Item, 16>& out_Entries) override;

private:
  void UpdateState();
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);

  ButtonType m_ButtonType;
};
