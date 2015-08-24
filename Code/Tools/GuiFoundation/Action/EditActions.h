#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
///
class EZ_GUIFOUNDATION_DLL ezEditActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hEditCategory;
  static ezActionDescriptorHandle s_hCopy;
  static ezActionDescriptorHandle s_hPaste;
  static ezActionDescriptorHandle s_hDelete;
};


///
class EZ_GUIFOUNDATION_DLL ezEditAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditAction);
public:
  enum class ButtonType
  {
    Copy,
    Paste,
    Delete,
  };
  ezEditAction(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezEditAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void SelectionEventHandler(const ezSelectionManager::Event& e);

  ButtonType m_ButtonType;
};