#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentAction, ezButtonAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

////////////////////////////////////////////////////////////////////////
// ezDocumentActions
////////////////////////////////////////////////////////////////////////

ezActionDescriptorHandle ezDocumentActions::s_hSave;
ezActionDescriptorHandle ezDocumentActions::s_hSaveAs;
ezActionDescriptorHandle ezDocumentActions::s_hSaveAll;
ezActionDescriptorHandle ezDocumentActions::s_hClose;

void ezDocumentActions::RegisterActions()
{
  ezHashedString sCategory;
  sCategory.Assign("Document");

  s_hSave     = ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Action, ezActionScope::Document, "Save", sCategory, &CreateSaveAction, &DeleteDocumentAction));
  s_hSaveAs   = ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Action, ezActionScope::Document, "Save As", sCategory, &CreateSaveAsAction, &DeleteDocumentAction));
  s_hSaveAll  = ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Action, ezActionScope::Document, "Save All", sCategory, &CreateSaveAllAction, &DeleteDocumentAction));
  s_hClose    = ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Action, ezActionScope::Document, "Close", sCategory, &CreateCloseAction, &DeleteDocumentAction));

}

void ezDocumentActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSave);
  ezActionManager::UnregisterAction(s_hSaveAs);
  ezActionManager::UnregisterAction(s_hSaveAll);
  ezActionManager::UnregisterAction(s_hClose);

  s_hSave.Invalidate();
  s_hSaveAs.Invalidate();
  s_hSaveAll.Invalidate();
  s_hClose.Invalidate();
}

void ezDocumentActions::MapActions(const ezHashedString& sMapping, const ezHashedString& sPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the documents actions failed!", sMapping.GetData());

  ezActionMapDescriptor desc;
  desc.m_sPath = sPath;

  desc.m_hAction = s_hSave;
  desc.m_fOrder = 1.0f;
  EZ_VERIFY(pMap->MapAction(desc).IsValid(), "Mapping failed");

  desc.m_hAction = s_hSaveAs;
  desc.m_fOrder = 2.0f;
  EZ_VERIFY(pMap->MapAction(desc).IsValid(), "Mapping failed");

  desc.m_hAction = s_hSaveAll;
  desc.m_fOrder = 3.0f;
  EZ_VERIFY(pMap->MapAction(desc).IsValid(), "Mapping failed");

  desc.m_hAction = s_hClose;
  desc.m_fOrder = 4.0f;
  EZ_VERIFY(pMap->MapAction(desc).IsValid(), "Mapping failed");
}

ezAction* ezDocumentActions::CreateSaveAction(const ezActionContext& context)
{
  return EZ_DEFAULT_NEW(ezDocumentAction)(context, "Save", ezDocumentAction::DocumentButton::Save);
}

ezAction* ezDocumentActions::CreateSaveAsAction(const ezActionContext& context)
{
  return EZ_DEFAULT_NEW(ezDocumentAction)(context, "Save As...", ezDocumentAction::DocumentButton::SaveAs);
}

ezAction* ezDocumentActions::CreateSaveAllAction(const ezActionContext& context)
{
  return EZ_DEFAULT_NEW(ezDocumentAction)(context, "Save All", ezDocumentAction::DocumentButton::SaveAll);
}

ezAction* ezDocumentActions::CreateCloseAction(const ezActionContext& context)
{
  return EZ_DEFAULT_NEW(ezDocumentAction)(context, "Close", ezDocumentAction::DocumentButton::Close);
}

void ezDocumentActions::DeleteDocumentAction(ezAction* pAction)
{
  EZ_DEFAULT_DELETE(pAction);
}


////////////////////////////////////////////////////////////////////////
// ezDocumentAction
////////////////////////////////////////////////////////////////////////

ezDocumentAction::ezDocumentAction(const ezActionContext& context, const char* szName, DocumentButton button)
  : ezButtonAction(context, szName, false)
{
  EZ_ASSERT_DEV(context.m_pDocument != nullptr, "Invalid context");

  m_ButtonType = button;

  m_Context.m_pDocument->m_EventsOne.AddEventHandler(ezMakeDelegate(&ezDocumentAction::DocumentEventHandler, this));

  if (m_ButtonType == DocumentButton::Save)
  {
    SetEnabled(m_Context.m_pDocument->IsModified());
  }
}

ezDocumentAction::~ezDocumentAction()
{
  m_Context.m_pDocument->m_EventsOne.RemoveEventHandler(ezMakeDelegate(&ezDocumentAction::DocumentEventHandler, this));
}

void ezDocumentAction::DocumentEventHandler(const ezDocumentBase::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentBase::Event::Type::DocumentSaved:
  case ezDocumentBase::Event::Type::ModifiedChanged:
    {
      if (m_ButtonType == DocumentButton::Save)
      {
        SetEnabled(m_Context.m_pDocument->IsModified());
        TriggerUpdate();
      }
    }
    break;
  }
}

ezResult ezDocumentAction::Execute(const ezVariant& value)
{
  switch (m_ButtonType)
  {
  case ezDocumentAction::DocumentButton::Save:
    if (m_Context.m_pDocument->SaveDocument().m_Result.Failed())
    {
      /// \todo Error box
    }
    break;

  case ezDocumentAction::DocumentButton::SaveAs:
    /// \todo Save as
    break;

  case ezDocumentAction::DocumentButton::SaveAll:
    {
      for (auto pMan : ezDocumentManagerBase::GetAllDocumentManagers())
      {
        for (auto pDoc : pMan->ezDocumentManagerBase::GetAllDocuments())
        {
          if (pDoc->SaveDocument().m_Result.Failed())
          {
            /// \todo Error box (or rather use the document window)
          }
        }
      }
    }
    break;

  case ezDocumentAction::DocumentButton::Close:
    /// \todo Use the window
    m_Context.m_pDocument->GetDocumentManager()->CloseDocument(m_Context.m_pDocument);
    break;
  }

  return EZ_SUCCESS;
}

