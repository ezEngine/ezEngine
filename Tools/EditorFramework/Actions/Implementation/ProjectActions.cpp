#include <PCH.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <EditorFramework/Settings/SettingsTab.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <Foundation/IO/OSFile.h>

ezActionDescriptorHandle ezProjectActions::s_hEditorMenu;

ezActionDescriptorHandle ezProjectActions::s_hDocumentCategory;
ezActionDescriptorHandle ezProjectActions::s_hCreateDocument;
ezActionDescriptorHandle ezProjectActions::s_hOpenDocument;
ezActionDescriptorHandle ezProjectActions::s_hRecentDocuments;

ezActionDescriptorHandle ezProjectActions::s_hProjectCategory;
ezActionDescriptorHandle ezProjectActions::s_hCreateProject;
ezActionDescriptorHandle ezProjectActions::s_hOpenProject;
ezActionDescriptorHandle ezProjectActions::s_hRecentProjects;
ezActionDescriptorHandle ezProjectActions::s_hCloseProject;

ezActionDescriptorHandle ezProjectActions::s_hSettingsCategory;
ezActionDescriptorHandle ezProjectActions::s_hProjectSettings;

ezActionDescriptorHandle ezProjectActions::s_hToolsMenu;
ezActionDescriptorHandle ezProjectActions::s_hEngineCategory;
ezActionDescriptorHandle ezProjectActions::s_hReloadResources;

void ezProjectActions::RegisterActions()
{
  s_hEditorMenu = EZ_REGISTER_MENU("EditorMenu", "Editor");

  s_hDocumentCategory = EZ_REGISTER_CATEGORY("DocumentCategory");
  s_hCreateDocument = EZ_REGISTER_ACTION_1("Create Document", "Create Document", ezActionScope::Global, "Project", "Ctrl+N", ezProjectAction, ezProjectAction::ButtonType::CreateDocument);
  s_hOpenDocument = EZ_REGISTER_ACTION_1("Open Document", "Open Document", ezActionScope::Global, "Project", "Ctrl+O", ezProjectAction, ezProjectAction::ButtonType::OpenDocument);
  s_hRecentDocuments = EZ_REGISTER_LRU_MENU("Recent Documents", "Recent Documents", ezRecentDocumentsMenuAction, "");

  s_hProjectCategory = EZ_REGISTER_CATEGORY("ProjectCategory");
  s_hCreateProject = EZ_REGISTER_ACTION_1("Create Project", "Create Project", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::CreateProject);
  s_hOpenProject = EZ_REGISTER_ACTION_1("Open Project", "Open Project", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::OpenProject);
  s_hRecentProjects = EZ_REGISTER_LRU_MENU("Recent Projects", "Recent Projects", ezRecentProjectsMenuAction, "");
  s_hCloseProject = EZ_REGISTER_ACTION_1("Close Project", "Close Project", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::CloseProject);

  s_hSettingsCategory = EZ_REGISTER_CATEGORY("SettingsCategory");
  s_hProjectSettings = EZ_REGISTER_ACTION_1("Settings", "Settings", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::ProjectSettings);

  s_hToolsMenu = EZ_REGISTER_MENU("ToolsMenu", "Tools");
  s_hEngineCategory = EZ_REGISTER_CATEGORY("EngineCategory");
  s_hReloadResources = EZ_REGISTER_ACTION_1("EngineReloadResources", "Reload Resources", ezActionScope::Global, "Engine", "F5", ezProjectAction, ezProjectAction::ButtonType::ReloadResources);
}

void ezProjectActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hEditorMenu);
  ezActionManager::UnregisterAction(s_hDocumentCategory);
  ezActionManager::UnregisterAction(s_hCreateDocument);
  ezActionManager::UnregisterAction(s_hOpenDocument);
  ezActionManager::UnregisterAction(s_hRecentDocuments);
  ezActionManager::UnregisterAction(s_hProjectCategory);
  ezActionManager::UnregisterAction(s_hCreateProject);
  ezActionManager::UnregisterAction(s_hOpenProject);
  ezActionManager::UnregisterAction(s_hRecentProjects);
  ezActionManager::UnregisterAction(s_hCloseProject);
  ezActionManager::UnregisterAction(s_hSettingsCategory);
  ezActionManager::UnregisterAction(s_hProjectSettings);
  ezActionManager::UnregisterAction(s_hToolsMenu);
  ezActionManager::UnregisterAction(s_hEngineCategory);
  ezActionManager::UnregisterAction(s_hReloadResources);
}

void ezProjectActions::MapActions(const char* szMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hEditorMenu, "", -1000000000.0f);

  pMap->MapAction(s_hDocumentCategory, "EditorMenu", 1.0f);
  pMap->MapAction(s_hCreateDocument, "EditorMenu/DocumentCategory", 1.0f);
  pMap->MapAction(s_hOpenDocument, "EditorMenu/DocumentCategory", 2.0f);
  pMap->MapAction(s_hRecentDocuments, "EditorMenu/DocumentCategory", 3.0f);

  pMap->MapAction(s_hProjectCategory, "EditorMenu", 2.0f);
  pMap->MapAction(s_hCreateProject, "EditorMenu/ProjectCategory", 1.0f);
  pMap->MapAction(s_hOpenProject, "EditorMenu/ProjectCategory", 2.0f);
  pMap->MapAction(s_hRecentProjects, "EditorMenu/ProjectCategory", 3.0f);
  pMap->MapAction(s_hCloseProject, "EditorMenu/ProjectCategory", 4.0f);

  pMap->MapAction(s_hSettingsCategory, "EditorMenu", 3.0f);
  pMap->MapAction(s_hProjectSettings, "EditorMenu/SettingsCategory", 1.0f);

  pMap->MapAction(s_hToolsMenu, "", 4.0f);
  pMap->MapAction(s_hEngineCategory, "ToolsMenu", 1.0f);
  pMap->MapAction(s_hReloadResources, "ToolsMenu/EngineCategory", 1.0f);
}

////////////////////////////////////////////////////////////////////////
// ezRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecentDocumentsMenuAction, ezLRUMenuAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();


void ezRecentDocumentsMenuAction::GetEntries(ezHybridArray<ezLRUMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();

  if (ezEditorApp::GetInstance()->GetRecentDocumentsList().GetFileList().IsEmpty())
    return;

  ezInt32 iMaxDocumentsToAdd = 10;
  for (ezString s : ezEditorApp::GetInstance()->GetRecentDocumentsList().GetFileList())
  {
    QAction* pAction = nullptr;

    if (!ezOSFile::ExistsFile(s))
      continue;

    ezLRUMenuAction::Item item;

    ezDocumentManagerBase* pManager;
    ezDocumentTypeDescriptor td;
    ezDocumentManagerBase::FindDocumentTypeFromPath(s, false, pManager, &td);

    item.m_UserValue = s;
    item.m_Icon = QIcon(QString::fromUtf8(td.m_sIcon.GetData()));

    if (ezToolsProject::IsProjectOpen())
    {
      ezString sRelativePath;
      if (!ezToolsProject::GetInstance()->IsDocumentInAllowedRoot(s, &sRelativePath))
        continue;

      item.m_sDisplay = sRelativePath;

      out_Entries.PushBack(item);
    }
    else
    {
      item.m_sDisplay = s;

      out_Entries.PushBack(item);
    }

    --iMaxDocumentsToAdd;

    if (iMaxDocumentsToAdd <= 0)
      break;
  }
}

void ezRecentDocumentsMenuAction::Execute(const ezVariant& value)
{
  ezEditorApp::GetInstance()->OpenDocument(value.ConvertTo<ezString>());
}


////////////////////////////////////////////////////////////////////////
// ezRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecentProjectsMenuAction, ezLRUMenuAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();


void ezRecentProjectsMenuAction::GetEntries(ezHybridArray<ezLRUMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();

  if (ezEditorApp::GetInstance()->GetRecentProjectsList().GetFileList().IsEmpty())
    return;

  for (ezString s : ezEditorApp::GetInstance()->GetRecentProjectsList().GetFileList())
  {
    if (!ezOSFile::ExistsFile(s))
      continue;

    ezLRUMenuAction::Item item;
    item.m_sDisplay = s;
    item.m_UserValue = s;

    out_Entries.PushBack(item);
  }
}

void ezRecentProjectsMenuAction::Execute(const ezVariant& value)
{
  ezEditorApp::GetInstance()->OpenProject(value.ConvertTo<ezString>());
}

////////////////////////////////////////////////////////////////////////
// ezProjectAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProjectAction, ezButtonAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezProjectAction::ezProjectAction(const ezActionContext& context, const char* szName, ButtonType button)
  : ezButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
  case ezProjectAction::ButtonType::CreateDocument:
    SetIconPath(":/GuiFoundation/Icons/DocumentAdd16.png");
    break;
  case ezProjectAction::ButtonType::OpenDocument:
    SetIconPath(":/GuiFoundation/Icons/Document16.png");
    break;
  case ezProjectAction::ButtonType::CreateProject:
    SetIconPath(":/GuiFoundation/Icons/ProjectAdd16.png");
    break;
  case ezProjectAction::ButtonType::OpenProject:
    SetIconPath(":/GuiFoundation/Icons/Project16.png");
    break;
  case ezProjectAction::ButtonType::CloseProject:
    SetIconPath(":/GuiFoundation/Icons/ProjectClose16.png");
    break;
  case ezProjectAction::ButtonType::ProjectSettings:
    SetIconPath(":/GuiFoundation/Icons/Settings16.png");
    break;
  case ezProjectAction::ButtonType::ReloadResources:
    SetIconPath(":/GuiFoundation/Icons/ReloadResources16.png");
    break;
  }

  if (m_ButtonType == ButtonType::CloseProject)
  {
    SetEnabled(ezToolsProject::IsProjectOpen());

    ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezProjectAction::ProjectEventHandler, this));
  }
}

ezProjectAction::~ezProjectAction()
{
  if (m_ButtonType == ButtonType::CloseProject)
  {
    ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezProjectAction::ProjectEventHandler, this));
  }
}

void ezProjectAction::ProjectEventHandler(const ezToolsProject::Event& e)
{
  SetEnabled(ezToolsProject::IsProjectOpen());
}

void ezProjectAction::Execute(const ezVariant& value)
{
  switch (m_ButtonType)
  {
  case ezProjectAction::ButtonType::CreateDocument:
    ezEditorApp::GetInstance()->GuiCreateDocument();
    break;

  case ezProjectAction::ButtonType::OpenDocument:
    ezEditorApp::GetInstance()->GuiOpenDocument();
    break;

  case ezProjectAction::ButtonType::CreateProject:
    ezEditorApp::GetInstance()->GuiCreateProject();
    break;

  case ezProjectAction::ButtonType::OpenProject:
    ezEditorApp::GetInstance()->GuiOpenProject();
    break;

  case ezProjectAction::ButtonType::CloseProject:
    {
      if (ezToolsProject::CanCloseProject())
        ezEditorApp::GetInstance()->CloseProject();
    }
    break;

  case ezProjectAction::ButtonType::ProjectSettings:
    ezEditorApp::GetInstance()->ShowSettingsDocument();
    break;

  case ezProjectAction::ButtonType::ReloadResources:
    {
      ezSimpleConfigMsgToEngine msg;
      msg.m_sWhatToDo = "ReloadResources";
      ezEditorEngineProcessConnection::GetInstance()->SendMessage(&msg);
    }
    break;
  }

}

