#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

///
class EZ_GUIFOUNDATION_DLL ezDocumentActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(ezStringView sMapping, ezStringView sTargetMenu);
  static void MapToolbarActions(ezStringView sMapping);
  static void MapToolsActions(ezStringView sMapping);

  static ezActionDescriptorHandle s_hSaveCategory;
  static ezActionDescriptorHandle s_hSave;
  static ezActionDescriptorHandle s_hSaveAs;
  static ezActionDescriptorHandle s_hSaveAll;

  static ezActionDescriptorHandle s_hClose;
  static ezActionDescriptorHandle s_hCloseAll;
  static ezActionDescriptorHandle s_hCloseAllButThis;

  static ezActionDescriptorHandle s_hOpenContainingFolder;

  static ezActionDescriptorHandle s_hUpdatePrefabs;
};


/// \brief Standard document actions.
class EZ_GUIFOUNDATION_DLL ezDocumentAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentAction, ezButtonAction);

public:
  enum class ButtonType
  {
    Save,
    SaveAs,
    SaveAll,
    Close,
    CloseAll,
    CloseAllButThis,
    OpenContainingFolder,
    UpdatePrefabs,
  };
  ezDocumentAction(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezDocumentAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void DocumentEventHandler(const ezDocumentEvent& e);

  ButtonType m_ButtonType;
};
