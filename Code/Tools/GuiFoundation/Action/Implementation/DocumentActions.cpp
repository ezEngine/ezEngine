#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <Foundation/IO/OSFile.h>
#include <QProcess>
#include <QDir>
#include <QClipboard>
#include <QMimeData>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

////////////////////////////////////////////////////////////////////////
// ezDocumentActions
////////////////////////////////////////////////////////////////////////

ezActionDescriptorHandle ezDocumentActions::s_hSaveCategory;
ezActionDescriptorHandle ezDocumentActions::s_hSave;
ezActionDescriptorHandle ezDocumentActions::s_hSaveAs;
ezActionDescriptorHandle ezDocumentActions::s_hSaveAll;
ezActionDescriptorHandle ezDocumentActions::s_hCloseCategory;
ezActionDescriptorHandle ezDocumentActions::s_hClose;
ezActionDescriptorHandle ezDocumentActions::s_hOpenContainingFolder;
ezActionDescriptorHandle ezDocumentActions::s_hUpdatePrefabs;
ezActionDescriptorHandle ezDocumentActions::s_hDocumentCategory;
ezActionDescriptorHandle ezDocumentActions::s_hCopyAssetGuid;

void ezDocumentActions::RegisterActions()
{
  s_hSaveCategory = EZ_REGISTER_CATEGORY("SaveCategory");
  s_hSave = EZ_REGISTER_ACTION_1("Document.Save", ezActionScope::Document, "Document", "Ctrl+S", ezDocumentAction, ezDocumentAction::ButtonType::Save);
  s_hSaveAll = EZ_REGISTER_ACTION_1("Document.SaveAll", ezActionScope::Document, "Document", "Ctrl+Shift+S", ezDocumentAction, ezDocumentAction::ButtonType::SaveAll);
  s_hSaveAs = EZ_REGISTER_ACTION_1("Document.SaveAs", ezActionScope::Document, "Document", "", ezDocumentAction, ezDocumentAction::ButtonType::SaveAs);
  s_hCloseCategory = EZ_REGISTER_CATEGORY("CloseCategory");
  s_hClose = EZ_REGISTER_ACTION_1("Document.Close", ezActionScope::Document, "Document", "Ctrl+W", ezDocumentAction, ezDocumentAction::ButtonType::Close);
  s_hOpenContainingFolder = EZ_REGISTER_ACTION_1("Document.OpenContainingFolder", ezActionScope::Document, "Document", "", ezDocumentAction, ezDocumentAction::ButtonType::OpenContainingFolder);
  s_hCopyAssetGuid = EZ_REGISTER_ACTION_1("Document.CopyAssetGuid", ezActionScope::Document, "Document", "", ezDocumentAction, ezDocumentAction::ButtonType::CopyAssetGuid);

  s_hDocumentCategory = EZ_REGISTER_CATEGORY("Tools.DocumentCategory");
  s_hUpdatePrefabs = EZ_REGISTER_ACTION_1("Prefabs.UpdateAll", ezActionScope::Document, "Scene", "Ctrl+Shift+P", ezDocumentAction, ezDocumentAction::ButtonType::UpdatePrefabs);
}

void ezDocumentActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSaveCategory);
  ezActionManager::UnregisterAction(s_hSave);
  ezActionManager::UnregisterAction(s_hSaveAs);
  ezActionManager::UnregisterAction(s_hSaveAll);
  ezActionManager::UnregisterAction(s_hCloseCategory);
  ezActionManager::UnregisterAction(s_hClose);
  ezActionManager::UnregisterAction(s_hOpenContainingFolder);
  ezActionManager::UnregisterAction(s_hCopyAssetGuid);
  ezActionManager::UnregisterAction(s_hDocumentCategory);
  ezActionManager::UnregisterAction(s_hUpdatePrefabs);
}

void ezDocumentActions::MapActions(const char* szMapping, const char* szPath, bool bForToolbar)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the documents actions failed!", szMapping);

  pMap->MapAction(s_hSaveCategory, szPath, 1.0f);
  ezStringBuilder sSubPath(szPath, "/SaveCategory");

  pMap->MapAction(s_hSave, sSubPath, 1.0f);
  //pMap->MapAction(s_hSaveAs, sSubPath, 2.0f);
  pMap->MapAction(s_hSaveAll, sSubPath, 3.0f);

  if (!bForToolbar)
  {
    sSubPath.Set(szPath, "/CloseCategory");
    pMap->MapAction(s_hCloseCategory, szPath, 2.0f);
    pMap->MapAction(s_hClose, sSubPath, 1.0f);
    pMap->MapAction(s_hCopyAssetGuid, sSubPath, 2.0f);
    pMap->MapAction(s_hOpenContainingFolder, sSubPath, 3.0f);
  }
}


void ezDocumentActions::MapToolsActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the documents actions failed!", szMapping);

  pMap->MapAction(s_hDocumentCategory, szPath, 1.0f);
  ezStringBuilder sSubPath(szPath, "/Tools.DocumentCategory");

  pMap->MapAction(s_hUpdatePrefabs, sSubPath, 1.0f);
}

////////////////////////////////////////////////////////////////////////
// ezDocumentAction
////////////////////////////////////////////////////////////////////////

ezDocumentAction::ezDocumentAction(const ezActionContext& context, const char* szName, ButtonType button)
  : ezButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
  case ezDocumentAction::ButtonType::Save:
    SetIconPath(":/GuiFoundation/Icons/Save16.png");
    break;
  case ezDocumentAction::ButtonType::SaveAs:
    SetIconPath("");
    break;
  case ezDocumentAction::ButtonType::SaveAll:
    SetIconPath(":/GuiFoundation/Icons/SaveAll16.png");
    break;
  case ezDocumentAction::ButtonType::Close:
    SetIconPath("");
    break;
  case ezDocumentAction::ButtonType::OpenContainingFolder:
    SetIconPath(":/GuiFoundation/Icons/OpenFolder16.png");
    break;
  case ezDocumentAction::ButtonType::CopyAssetGuid:
    //SetIconPath(":/GuiFoundation/Icons/OpenFolder16.png");
    break;
  case ezDocumentAction::ButtonType::UpdatePrefabs:
    SetIconPath(":/EditorPluginScene/PrefabUpdate.png");
    break;
  }

  if (context.m_pDocument == nullptr)
  {
    if (button == ButtonType::Save || button == ButtonType::SaveAs)
    {
      // for actions that require a document, hide them
      SetVisible(false);
    }
  }
  else
  {
    m_Context.m_pDocument->m_EventsOne.AddEventHandler(ezMakeDelegate(&ezDocumentAction::DocumentEventHandler, this));

    if (m_ButtonType == ButtonType::Save)
    {
      SetVisible(!m_Context.m_pDocument->IsReadOnly());
      SetEnabled(m_Context.m_pDocument->IsModified());
    }
  }
}

ezDocumentAction::~ezDocumentAction()
{
  if (m_Context.m_pDocument)
  {
    m_Context.m_pDocument->m_EventsOne.RemoveEventHandler(ezMakeDelegate(&ezDocumentAction::DocumentEventHandler, this));
  }
}

void ezDocumentAction::DocumentEventHandler(const ezDocumentEvent& e)
{
  switch (e.m_Type)
  {
  case ezDocumentEvent::Type::DocumentSaved:
  case ezDocumentEvent::Type::ModifiedChanged:
    {
      if (m_ButtonType == ButtonType::Save)
      {
        SetEnabled(m_Context.m_pDocument->IsModified());
      }
    }
    break;

  default:
    break;
  }
}

void ezDocumentAction::Execute(const ezVariant& value)
{
  switch (m_ButtonType)
  {
  case ezDocumentAction::ButtonType::Save:
    {
      ezQtDocumentWindow* pWnd = ezQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);
      pWnd->SaveDocument();
    }
    break;

  case ezDocumentAction::ButtonType::SaveAs:
    /// \todo Save as
    break;

  case ezDocumentAction::ButtonType::SaveAll:
    {
      for (auto pMan : ezDocumentManager::GetAllDocumentManagers())
      {
        for (auto pDoc : pMan->ezDocumentManager::GetAllDocuments())
        {
          ezQtDocumentWindow* pWnd = ezQtDocumentWindow::FindWindowByDocument(pDoc);

          if (pWnd->SaveDocument().m_Result.Failed())
            return;
        }
      }
    }
    break;

  case ezDocumentAction::ButtonType::Close:
    {
      ezQtDocumentWindow* pWnd = ezQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);

      if (!pWnd->CanCloseWindow())
        return;

      pWnd->CloseDocumentWindow();
    }
    break;

  case ezDocumentAction::ButtonType::OpenContainingFolder:
    {
      ezString sPath;

      if (!m_Context.m_pDocument)
      {
        if (ezToolsProject::IsProjectOpen())
          sPath = ezToolsProject::GetSingleton()->GetProjectFile();
        else
          sPath = ezOSFile::GetApplicationDirectory();
      }
      else
        sPath = m_Context.m_pDocument->GetDocumentPath();

      ezUIServices::OpenInExplorer(sPath);
    }
    break;

  case ezDocumentAction::ButtonType::CopyAssetGuid:
    {
      QString sGuid = QString::fromUtf8(ezConversionUtils::ToString(m_Context.m_pDocument->GetGuid()).GetData());

      QClipboard* clipboard = QApplication::clipboard();
      QMimeData* mimeData = new QMimeData();
      mimeData->setText(sGuid);
      clipboard->setMimeData(mimeData);
    }
    break;

  case ezDocumentAction::ButtonType::UpdatePrefabs:
    // TODO const cast
    const_cast<ezDocument*>(m_Context.m_pDocument)->UpdatePrefabs();
    return;
  }
}

