#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <QClipboard>
#include <QFileDialog>
#include <QMimeData>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Project/ToolsProject.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentAction, 1, ezRTTINoAllocator)
  ;
EZ_END_DYNAMIC_REFLECTED_TYPE;

////////////////////////////////////////////////////////////////////////
// ezDocumentActions
////////////////////////////////////////////////////////////////////////

ezActionDescriptorHandle ezDocumentActions::s_hSaveCategory;
ezActionDescriptorHandle ezDocumentActions::s_hSave;
ezActionDescriptorHandle ezDocumentActions::s_hSaveAs;
ezActionDescriptorHandle ezDocumentActions::s_hSaveAll;
ezActionDescriptorHandle ezDocumentActions::s_hClose;
ezActionDescriptorHandle ezDocumentActions::s_hCloseAll;
ezActionDescriptorHandle ezDocumentActions::s_hCloseAllButThis;
ezActionDescriptorHandle ezDocumentActions::s_hOpenContainingFolder;
ezActionDescriptorHandle ezDocumentActions::s_hCopyAssetGuid;
ezActionDescriptorHandle ezDocumentActions::s_hUpdatePrefabs;

void ezDocumentActions::RegisterActions()
{
  s_hSaveCategory = EZ_REGISTER_CATEGORY("SaveCategory");
  s_hSave = EZ_REGISTER_ACTION_1("Document.Save", ezActionScope::Document, "Document", "Ctrl+S", ezDocumentAction, ezDocumentAction::ButtonType::Save);
  s_hSaveAll = EZ_REGISTER_ACTION_1("Document.SaveAll", ezActionScope::Document, "Document", "Ctrl+Shift+S", ezDocumentAction, ezDocumentAction::ButtonType::SaveAll);
  s_hSaveAs = EZ_REGISTER_ACTION_1("Document.SaveAs", ezActionScope::Document, "Document", "", ezDocumentAction, ezDocumentAction::ButtonType::SaveAs);
  s_hClose = EZ_REGISTER_ACTION_1("Document.Close", ezActionScope::Document, "Document", "Ctrl+W", ezDocumentAction, ezDocumentAction::ButtonType::Close);
  s_hCloseAll = EZ_REGISTER_ACTION_1("Document.CloseAll", ezActionScope::Document, "Document", "Ctrl+Shift+W", ezDocumentAction, ezDocumentAction::ButtonType::CloseAll);
  s_hCloseAllButThis = EZ_REGISTER_ACTION_1("Document.CloseAllButThis", ezActionScope::Document, "Document", "Shift+Alt+W", ezDocumentAction, ezDocumentAction::ButtonType::CloseAllButThis);
  s_hOpenContainingFolder = EZ_REGISTER_ACTION_1("Document.OpenContainingFolder", ezActionScope::Document, "Document", "", ezDocumentAction, ezDocumentAction::ButtonType::OpenContainingFolder);
  s_hCopyAssetGuid = EZ_REGISTER_ACTION_1("Document.CopyAssetGuid", ezActionScope::Document, "Document", "", ezDocumentAction, ezDocumentAction::ButtonType::CopyAssetGuid);
  s_hUpdatePrefabs = EZ_REGISTER_ACTION_1("Prefabs.UpdateAll", ezActionScope::Document, "Scene", "Ctrl+Shift+P", ezDocumentAction, ezDocumentAction::ButtonType::UpdatePrefabs);
}

void ezDocumentActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSaveCategory);
  ezActionManager::UnregisterAction(s_hSave);
  ezActionManager::UnregisterAction(s_hSaveAs);
  ezActionManager::UnregisterAction(s_hSaveAll);
  ezActionManager::UnregisterAction(s_hClose);
  ezActionManager::UnregisterAction(s_hCloseAll);
  ezActionManager::UnregisterAction(s_hCloseAllButThis);
  ezActionManager::UnregisterAction(s_hOpenContainingFolder);
  ezActionManager::UnregisterAction(s_hCopyAssetGuid);
  ezActionManager::UnregisterAction(s_hUpdatePrefabs);
}

void ezDocumentActions::MapMenuActions(ezStringView sMapping, ezStringView sTargetMenu)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hSave, sTargetMenu, 5.0f);
  pMap->MapAction(s_hSaveAs, sTargetMenu, 6.0f);
  pMap->MapAction(s_hSaveAll, sTargetMenu, 7.0f);
  pMap->MapAction(s_hClose, sTargetMenu, 8.0f);
  pMap->MapAction(s_hCloseAll, sTargetMenu, 9.0f);
  pMap->MapAction(s_hCloseAllButThis, sTargetMenu, 10.0f);
  pMap->MapAction(s_hOpenContainingFolder, sTargetMenu, 11.0f);
  pMap->MapAction(s_hCopyAssetGuid, sTargetMenu, 12.0f);
}

void ezDocumentActions::MapToolbarActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hSaveCategory, "", 1.0f);
  ezStringView sSubPath = "SaveCategory";

  pMap->MapAction(s_hSave, sSubPath, 1.0f);
  pMap->MapAction(s_hSaveAll, sSubPath, 3.0f);
}


void ezDocumentActions::MapToolsActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hUpdatePrefabs, "G.Tools.Document", 1.0f);
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
      SetIconPath(":/GuiFoundation/Icons/Save.svg");
      break;
    case ezDocumentAction::ButtonType::SaveAs:
      SetIconPath("");
      break;
    case ezDocumentAction::ButtonType::SaveAll:
      SetIconPath(":/GuiFoundation/Icons/SaveAll.svg");
      break;
    case ezDocumentAction::ButtonType::Close:
      SetIconPath("");
      break;
    case ezDocumentAction::ButtonType::CloseAll:
      SetIconPath("");
      break;
    case ezDocumentAction::ButtonType::CloseAllButThis:
      SetIconPath("");
      break;
    case ezDocumentAction::ButtonType::OpenContainingFolder:
      SetIconPath(":/GuiFoundation/Icons/OpenFolder.svg");
      break;
    case ezDocumentAction::ButtonType::CopyAssetGuid:
      SetIconPath(":/GuiFoundation/Icons/Guid.svg");
      break;
    case ezDocumentAction::ButtonType::UpdatePrefabs:
      SetIconPath(":/EditorPluginScene/Icons/PrefabUpdate.svg");
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
      pWnd->SaveDocument().LogFailure();
    }
    break;

    case ezDocumentAction::ButtonType::SaveAs:
    {
      ezQtDocumentWindow* pWnd = ezQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);
      if (pWnd->SaveDocument().Succeeded())
      {
        auto* desc = m_Context.m_pDocument->GetDocumentTypeDescriptor();
        ezStringBuilder sAllFilters;
        sAllFilters.Append(desc->m_sDocumentTypeName, " (*.", desc->m_sFileExtension, ")");
        QString sSelectedExt;
        ezString sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Create Document"),
          ezMakeQString(m_Context.m_pDocument->GetDocumentPath()), QString::fromUtf8(sAllFilters.GetData()), &sSelectedExt, QFileDialog::Option::DontResolveSymlinks)
                           .toUtf8()
                           .data();

        if (!sFile.IsEmpty())
        {
          ezUuid newDoc = ezUuid::MakeUuid();
          ezStatus res = m_Context.m_pDocument->GetDocumentManager()->CloneDocument(m_Context.m_pDocument->GetDocumentPath(), sFile, newDoc);

          if (res.Failed())
          {
            ezStringBuilder s;
            s.SetFormat("Failed to save document: \n'{0}'", sFile);
            ezQtUiServices::MessageBoxStatus(res, s);
          }
          else
          {
            const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
            if (ezDocumentManager::FindDocumentTypeFromPath(sFile, false, pTypeDesc).Succeeded())
            {
              ezDocument* pDocument = nullptr;
              m_Context.m_pDocument->GetDocumentManager()->OpenDocument(pTypeDesc->m_sDocumentTypeName, sFile, pDocument).LogFailure();
            }
          }
        }
      }
    }
    break;

    case ezDocumentAction::ButtonType::SaveAll:
    {
      ezToolsProject::GetSingleton()->BroadcastSaveAll();
    }
    break;

    case ezDocumentAction::ButtonType::Close:
    {
      ezQtDocumentWindow* pWindow = ezQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);

      if (!pWindow->CanCloseWindow())
        return;

      pWindow->CloseDocumentWindow();
    }
    break;

    case ezDocumentAction::ButtonType::CloseAll:
    {
      auto& documentWindows = ezQtDocumentWindow::GetAllDocumentWindows();
      for (ezQtDocumentWindow* pWindow : documentWindows)
      {
        if (!pWindow->CanCloseWindow())
          continue;

        // Prevent closing the document root window.
        if (ezStringUtils::Compare(pWindow->GetUniqueName(), "Settings") == 0)
          continue;

        pWindow->CloseDocumentWindow();
      }
    }
    break;

    case ezDocumentAction::ButtonType::CloseAllButThis:
    {
      ezQtDocumentWindow* pThisWindow = ezQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);

      auto& documentWindows = ezQtDocumentWindow::GetAllDocumentWindows();
      for (ezQtDocumentWindow* pWindow : documentWindows)
      {
        if (!pWindow->CanCloseWindow() || pWindow == pThisWindow)
          continue;

        // Prevent closing the document root window.
        if (ezStringUtils::Compare(pWindow->GetUniqueName(), "Settings") == 0)
          continue;

        pWindow->CloseDocumentWindow();
      }
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

      ezQtUiServices::OpenInExplorer(sPath, true);
    }
    break;

    case ezDocumentAction::ButtonType::CopyAssetGuid:
    {
      ezStringBuilder sGuid;
      ezConversionUtils::ToString(m_Context.m_pDocument->GetGuid(), sGuid);

      QClipboard* clipboard = QApplication::clipboard();
      QMimeData* mimeData = new QMimeData();
      mimeData->setText(sGuid.GetData());
      clipboard->setMimeData(mimeData);

      ezQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(ezFmt("Copied asset GUID: {}", sGuid), ezTime::MakeFromSeconds(5));
    }
    break;

    case ezDocumentAction::ButtonType::UpdatePrefabs:
      // TODO const cast
      const_cast<ezDocument*>(m_Context.m_pDocument)->UpdatePrefabs();
      return;
  }
}
