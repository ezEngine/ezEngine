#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class EZ_GUIFOUNDATION_DLL ezDocumentActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath, bool bForToolbar);
  static void MapToolsActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hSaveCategory;
  static ezActionDescriptorHandle s_hSave;
  static ezActionDescriptorHandle s_hSaveAs;
  static ezActionDescriptorHandle s_hSaveAll;

  static ezActionDescriptorHandle s_hCloseCategory;
  static ezActionDescriptorHandle s_hClose;
  static ezActionDescriptorHandle s_hOpenContainingFolder;
  static ezActionDescriptorHandle s_hCopyAssetGuid;
  static ezActionDescriptorHandle s_hMoveDocumentWindow;

  static ezActionDescriptorHandle s_hUpdatePrefabs;
  static ezActionDescriptorHandle s_hDocumentCategory;
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
    OpenContainingFolder,
    UpdatePrefabs,
    CopyAssetGuid,
  };
  ezDocumentAction(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezDocumentAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void DocumentEventHandler(const ezDocumentEvent& e);

  ButtonType m_ButtonType;
};

/// \brief Action to move document windows between containers.
class EZ_GUIFOUNDATION_DLL ezContainerWindowMenuAction : public ezDynamicMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezContainerWindowMenuAction, ezDynamicMenuAction);
public:
  ezContainerWindowMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath);
  virtual void GetEntries(ezHybridArray<ezDynamicMenuAction::Item, 16>& out_Entries) override;
  virtual void Execute(const ezVariant& value) override;
};
