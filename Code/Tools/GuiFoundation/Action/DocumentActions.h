#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class EZ_GUIFOUNDATION_DLL ezDocumentActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath, bool bForToolbar);

  static ezActionDescriptorHandle s_hSaveCategory;
  static ezActionDescriptorHandle s_hSave;
  static ezActionDescriptorHandle s_hSaveAs;
  static ezActionDescriptorHandle s_hSaveAll;

  static ezActionDescriptorHandle s_hCloseCategory;
  static ezActionDescriptorHandle s_hClose;
  static ezActionDescriptorHandle s_hOpenContainingFolder;
};


///
class EZ_GUIFOUNDATION_DLL ezDocumentAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentAction);
public:
  enum class ButtonType
  {
    Save,
    SaveAs,
    SaveAll,
    Close,
    OpenContainingFolder
  };
  ezDocumentAction(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezDocumentAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void DocumentEventHandler(const ezDocumentBase::Event& e);

  ButtonType m_ButtonType;
};