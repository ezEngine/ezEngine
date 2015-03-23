#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/ActionManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentAction, ezButtonAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

////////////////////////////////////////////////////////////////////////
// ezDocumentActions
////////////////////////////////////////////////////////////////////////

void ezDocumentActions::RegisterActions()
{
  ezHashedString sCategory;
  sCategory.Assign("Document");

  //s_hSave = ezActionManager::RegisterAction(ezActionDescriptor(ActionType::Action, ActionScope::Document, "Save", sCategory,
  //  &CreateSaveAction, &DeleteSaveAction));

}

void ezDocumentActions::UnregisterActions()
{
}

void ezDocumentActions::MapActions(const ezHashedString& sMapping)
{
}

ezAction* ezDocumentActions::CreateSaveAction(const ezActionContext& context)
{
  return EZ_DEFAULT_NEW(ezDocumentAction)("Save", ezDocumentAction::DocumentButton::Save);
}

void ezDocumentActions::DeleteSaveAction(ezAction* pAction)
{
  EZ_DEFAULT_DELETE(pAction);
}


////////////////////////////////////////////////////////////////////////
// ezDocumentAction
////////////////////////////////////////////////////////////////////////

ezDocumentAction::ezDocumentAction(const char* szName, DocumentButton button)
  : ezButtonAction(szName, false)
{

}

ezResult ezDocumentAction::Init(const ezActionContext& context)
{
  return EZ_SUCCESS;
}

ezResult ezDocumentAction::Execute(const ezVariant& value)
{
  return EZ_SUCCESS;
}

