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

  static void MapActions(ezStringView sMapping, bool bDeleteAction, bool bAdvancedPasteActions);
  static void MapContextMenuActions(ezStringView sMapping);
  static void MapViewContextMenuActions(ezStringView sMapping);

  static ezActionDescriptorHandle s_hEditCategory;
  static ezActionDescriptorHandle s_hCopy;
  static ezActionDescriptorHandle s_hPaste;
  static ezActionDescriptorHandle s_hPasteAsChild;
  static ezActionDescriptorHandle s_hPasteAtOriginalLocation;
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
    PasteAtOriginalLocation,
    Delete,
  };
  ezEditAction(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezEditAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void SelectionEventHandler(const ezSelectionManagerEvent& e);

  ButtonType m_ButtonType;
};
