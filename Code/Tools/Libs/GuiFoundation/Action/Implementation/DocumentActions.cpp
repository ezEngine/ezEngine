#include <GuiFoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <QClipboard>
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
ezActionDescriptorHandle ezDocumentActions::s_hCloseCategory;
ezActionDescriptorHandle ezDocumentActions::s_hClose;
ezActionDescriptorHandle ezDocumentActions::s_hOpenContainingFolder;
ezActionDescriptorHandle ezDocumentActions::s_hCopyAssetGuid;
ezActionDescriptorHandle ezDocumentActions::s_hMoveDocumentWindow;
ezActionDescriptorHandle ezDocumentActions::s_hUpdatePrefabs;
ezActionDescriptorHandle ezDocumentActions::s_hDocumentCategory;

void ezDocumentActions::RegisterActions()
{
  s_hSaveCategory = EZ_REGISTER_CATEGORY("SaveCategory");
  s_hSave = EZ_REGISTER_ACTION_1(
    "Document.Save", ezActionScope::Document, "Document", "Ctrl+S", ezDocumentAction, ezDocumentAction::ButtonType::Save);
  s_hSaveAll = EZ_REGISTER_ACTION_1(
    "Document.SaveAll", ezActionScope::Document, "Document", "Ctrl+Shift+S", ezDocumentAction, ezDocumentAction::ButtonType::SaveAll);
  s_hSaveAs = EZ_REGISTER_ACTION_1(
    "Document.SaveAs", ezActionScope::Document, "Document", "", ezDocumentAction, ezDocumentAction::ButtonType::SaveAs);
  s_hCloseCategory = EZ_REGISTER_CATEGORY("CloseCategory");
  s_hClose = EZ_REGISTER_ACTION_1(
    "Document.Close", ezActionScope::Document, "Document", "Ctrl+W", ezDocumentAction, ezDocumentAction::ButtonType::Close);
  s_hOpenContainingFolder = EZ_REGISTER_ACTION_1("Document.OpenContainingFolder", ezActionScope::Document, "Document", "", ezDocumentAction,
    ezDocumentAction::ButtonType::OpenContainingFolder);
  s_hCopyAssetGuid = EZ_REGISTER_ACTION_1(
    "Document.CopyAssetGuid", ezActionScope::Document, "Document", "", ezDocumentAction, ezDocumentAction::ButtonType::CopyAssetGuid);
  s_hMoveDocumentWindow =
    EZ_REGISTER_DYNAMIC_MENU("Document.MoveWindow", ezContainerWindowMenuAction, ":/GuiFoundation/Icons/Window16.png");

  s_hDocumentCategory = EZ_REGISTER_CATEGORY("Tools.DocumentCategory");
  s_hUpdatePrefabs = EZ_REGISTER_ACTION_1(
    "Prefabs.UpdateAll", ezActionScope::Document, "Scene", "Ctrl+Shift+P", ezDocumentAction, ezDocumentAction::ButtonType::UpdatePrefabs);
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
  ezActionManager::UnregisterAction(s_hMoveDocumentWindow);
  ezActionManager::UnregisterAction(s_hDocumentCategory);
  ezActionManager::UnregisterAction(s_hUpdatePrefabs);
}

void ezDocumentActions::MapActions(const char* szMapping, const char* szPath, bool bForToolbar)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", szMapping);

  pMap->MapAction(s_hSaveCategory, szPath, 1.0f);
  ezStringBuilder sSubPath(szPath, "/SaveCategory");

  pMap->MapAction(s_hSave, sSubPath, 1.0f);
  pMap->MapAction(s_hSaveAll, sSubPath, 3.0f);

  if (!bForToolbar)
  {
    pMap->MapAction(s_hSaveAs, sSubPath, 2.0f);

    pMap->MapAction(s_hMoveDocumentWindow, sSubPath, 4.0f);
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
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", szMapping);

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
      SetIconPath(":/GuiFoundation/Icons/DocumentGuid16.png");
      break;
    case ezDocumentAction::ButtonType::UpdatePrefabs:
      SetIconPath(":/EditorPluginScene/Icons/PrefabUpdate16.png");
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
    {
      ezQtDocumentWindow* pWnd = ezQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);
      if (pWnd->SaveDocument().Succeeded())
      {
        auto* desc = m_Context.m_pDocument->GetDocumentTypeDescriptor();
        ezStringBuilder sAllFilters;
        sAllFilters.Append(desc->m_sDocumentTypeName, " (*.", desc->m_sFileExtension, ")");
        QString sSelectedExt;
        ezString sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Create Document"),
          m_Context.m_pDocument->GetDocumentPath(), QString::fromUtf8(sAllFilters.GetData()), &sSelectedExt,
          QFileDialog::Option::DontResolveSymlinks)
                           .toUtf8()
                           .data();

        if (!sFile.IsEmpty())
        {
          ezUuid newDoc;
          newDoc.CreateNewUuid();
          ezStatus res =
            m_Context.m_pDocument->GetDocumentManager()->CloneDocument(m_Context.m_pDocument->GetDocumentPath(), sFile, newDoc);

          if (res.Failed())
          {
            ezStringBuilder s;
            s.Format("Failed to save document: \n'{0}'", sFile);
            ezQtUiServices::MessageBoxStatus(res, s);
          }
          else
          {
            const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
            if (ezDocumentManager::FindDocumentTypeFromPath(sFile, false, pTypeDesc).Succeeded())
            {
              ezDocument* pDocument = nullptr;
              m_Context.m_pDocument->GetDocumentManager()->OpenDocument(pTypeDesc->m_sDocumentTypeName, sFile, pDocument);
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
    }
    break;

    case ezDocumentAction::ButtonType::UpdatePrefabs:
      // TODO const cast
      const_cast<ezDocument*>(m_Context.m_pDocument)->UpdatePrefabs();
      return;
  }
}


////////////////////////////////////////////////////////////////////////
// ezContainerWindowMenuAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezContainerWindowMenuAction, 1, ezRTTINoAllocator)
  ;
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezContainerWindowMenuAction::ezContainerWindowMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath)
  : ezDynamicMenuAction(context, szName, szIconPath)
{
}

void ezContainerWindowMenuAction::GetEntries(ezHybridArray<ezDynamicMenuAction::Item, 16>& out_Entries)
{
  ezQtDocumentWindow* pView = ezQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);

  ezStringBuilder tmp;

  out_Entries.Clear();
  const auto& containers = ezQtContainerWindow::GetAllContainerWindows();
  for (ezUInt32 i = 0; i < containers.GetCount(); i++)
  {
    ezQtContainerWindow* pContainer = containers[i];
    ezDynamicMenuAction::Item entryItem;

    if (i == 0)
      entryItem.m_sDisplay = "Main";
    else
      entryItem.m_sDisplay = ezConversionUtils::ToString(i, tmp);

    entryItem.m_CheckState = (pView->GetContainerWindow() == pContainer) ? ezDynamicMenuAction::Item::CheckMark::Checked
                                                                         : ezDynamicMenuAction::Item::CheckMark::Unchecked;
    entryItem.m_UserValue = (ezInt32)i;
    entryItem.m_Icon = QIcon(QStringLiteral(":/GuiFoundation/Icons/Window16.png"));
    out_Entries.PushBack(entryItem);
  }
  {
    ezDynamicMenuAction::Item separator;
    separator.m_ItemFlags = ezDynamicMenuAction::Item::ItemFlags::Separator;
    out_Entries.PushBack(separator);
  }
  {
    ezDynamicMenuAction::Item newContainer;
    newContainer.m_sDisplay = ezTranslate("Document.NewWindow");
    newContainer.m_CheckState = ezDynamicMenuAction::Item::CheckMark::NotCheckable;
    newContainer.m_UserValue = (ezInt32)-1;
    newContainer.m_Icon = QIcon(QStringLiteral(":/GuiFoundation/Icons/Window16.png"));
    out_Entries.PushBack(newContainer);
  }
}

void ezContainerWindowMenuAction::Execute(const ezVariant& value)
{
  ezQtDocumentWindow* pView = ezQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);

  ezInt32 iIndex = value.ConvertTo<ezInt32>();
  if (iIndex == -1)
  {
    ezQtContainerWindow* pContainer = ezQtContainerWindow::CreateNewContainerWindow();
    pContainer->MoveDocumentWindowToContainer(pView);
    pContainer->show();
    pView->EnsureVisible();
  }
  else
  {
    ezQtContainerWindow::GetAllContainerWindows()[iIndex]->MoveDocumentWindowToContainer(pView);
    pView->EnsureVisible();
  }
}
