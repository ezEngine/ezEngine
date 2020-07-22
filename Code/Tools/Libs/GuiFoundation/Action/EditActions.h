#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
///
class EZ_GUIFOUNDATION_DLL ezEditActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath, bool bDeleteAction, bool bPasteAsChildAction);
  static void MapContextMenuActions(const char* szMapping, const char* szPath);
  static void MapViewContextMenuActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hEditCategory;
  static ezActionDescriptorHandle s_hCopy;
  static ezActionDescriptorHandle s_hPaste;
  static ezActionDescriptorHandle s_hPasteAsChild;
  static ezActionDescriptorHandle s_hDelete;
};


///
class EZ_GUIFOUNDATION_DLL ezEditAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditAction, ezButtonAction);

public:
  enum class ButtonType
  {
    Copy,
    Paste,
    PasteAsChild,
    Delete,
  };
  ezEditAction(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezEditAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void SelectionEventHandler(const ezSelectionManagerEvent& e);

  ButtonType m_ButtonType;
};
