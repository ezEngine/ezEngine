#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class ezDocumentActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const ezHashedString& sMapping);

  static ezActionHandle s_hSave;
  static ezActionHandle s_hSaveAs;
  static ezActionHandle s_hSaveAll;
  static ezActionHandle s_hClose;

private:
  ezAction* CreateSaveAction(const ezActionContext& context);
  void DeleteSaveAction(ezAction* pAction);
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
  ezDocumentAction(const char* szName, DocumentButton button);

  virtual ezResult Init(const ezActionContext& context) override;
  virtual ezResult Execute(const ezVariant& value) override;

private:

};