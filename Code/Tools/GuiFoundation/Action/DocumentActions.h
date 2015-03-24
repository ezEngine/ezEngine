#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class EZ_GUIFOUNDATION_DLL ezDocumentActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const ezHashedString& sMapping, const ezHashedString& sPath);

  static ezActionDescriptorHandle s_hSave;
  static ezActionDescriptorHandle s_hSaveAs;
  static ezActionDescriptorHandle s_hSaveAll;
  static ezActionDescriptorHandle s_hClose;

private:
  static ezAction* CreateSaveAction(const ezActionContext& context);
  static ezAction* CreateSaveAsAction(const ezActionContext& context);
  static ezAction* CreateSaveAllAction(const ezActionContext& context);
  static ezAction* CreateCloseAction(const ezActionContext& context);
  static void DeleteDocumentAction(ezAction* pAction);
};


///
class EZ_GUIFOUNDATION_DLL ezDocumentAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentAction);
public:
  enum class DocumentButton
  {
    Save,
    SaveAs,
    SaveAll,
    Close
  };
  ezDocumentAction(const ezActionContext& context, const char* szName, DocumentButton button);
  ~ezDocumentAction();

  virtual ezResult Execute(const ezVariant& value) override;

private:
  void DocumentEventHandler(const ezDocumentBase::Event& e);

  DocumentButton m_ButtonType;
};