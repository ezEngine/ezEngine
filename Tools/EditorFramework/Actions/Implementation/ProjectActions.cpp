#include <PCH.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <EditorFramework/Settings/SettingsTab.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

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
  s_hEditorMenu = EZ_REGISTER_MENU("Menu.Editor");

  s_hDocumentCategory = EZ_REGISTER_CATEGORY("DocumentCategory");
  s_hCreateDocument = EZ_REGISTER_ACTION_1("Document.Create", ezActionScope::Global, "Project", "Ctrl+N", ezProjectAction, ezProjectAction::ButtonType::CreateDocument);
  s_hOpenDocument = EZ_REGISTER_ACTION_1("Document.Open", ezActionScope::Global, "Project", "Ctrl+O", ezProjectAction, ezProjectAction::ButtonType::OpenDocument);
  s_hRecentDocuments = EZ_REGISTER_LRU_MENU("Project.RecentDocuments.Menu", ezRecentDocumentsMenuAction, "");

  s_hProjectCategory = EZ_REGISTER_CATEGORY("ProjectCategory");
  s_hCreateProject = EZ_REGISTER_ACTION_1("Project.Create", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::CreateProject);
  s_hOpenProject = EZ_REGISTER_ACTION_1("Project.Open", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::OpenProject);
  s_hRecentProjects = EZ_REGISTER_LRU_MENU("Project.RecentProjects.Menu", ezRecentProjectsMenuAction, "");
  s_hCloseProject = EZ_REGISTER_ACTION_1("Project.Close", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::CloseProject);

  s_hSettingsCategory = EZ_REGISTER_CATEGORY("SettingsCategory");
  s_hProjectSettings = EZ_REGISTER_ACTION_1("Settings.Open", ezActionScope::Global, "Settings", "", ezProjectAction, ezProjectAction::ButtonType::ProjectSettings);

  s_hToolsMenu = EZ_REGISTER_MENU("Menu.Tools");
  s_hEngineCategory = EZ_REGISTER_CATEGORY("EngineCategory");
  s_hReloadResources = EZ_REGISTER_ACTION_1("Engine.ReloadResources", ezActionScope::Global, "Engine", "F5", ezProjectAction, ezProjectAction::ButtonType::ReloadResources);
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

  pMap->MapAction(s_hDocumentCategory, "Menu.Editor", 1.0f);
  pMap->MapAction(s_hCreateDocument, "Menu.Editor/DocumentCategory", 1.0f);
  pMap->MapAction(s_hOpenDocument, "Menu.Editor/DocumentCategory", 2.0f);
  pMap->MapAction(s_hRecentDocuments, "Menu.Editor/DocumentCategory", 3.0f);

  pMap->MapAction(s_hProjectCategory, "Menu.Editor", 2.0f);
  pMap->MapAction(s_hCreateProject, "Menu.Editor/ProjectCategory", 1.0f);
  pMap->MapAction(s_hOpenProject, "Menu.Editor/ProjectCategory", 2.0f);
  pMap->MapAction(s_hRecentProjects, "Menu.Editor/ProjectCategory", 3.0f);
  pMap->MapAction(s_hCloseProject, "Menu.Editor/ProjectCategory", 4.0f);

  pMap->MapAction(s_hSettingsCategory, "Menu.Editor", 3.0f);
  pMap->MapAction(s_hProjectSettings, "Menu.Editor/SettingsCategory", 1.0f);

  pMap->MapAction(s_hToolsMenu, "", 4.0f);
  pMap->MapAction(s_hEngineCategory, "Menu.Tools", 1.0f);
  pMap->MapAction(s_hReloadResources, "Menu.Tools/EngineCategory", 1.0f);
}

////////////////////////////////////////////////////////////////////////
// ezRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecentDocumentsMenuAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();


void ezRecentDocumentsMenuAction::GetEntries(ezHybridArray<ezLRUMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();

  if (ezQtEditorApp::GetInstance()->GetRecentDocumentsList().GetFileList().IsEmpty())
    return;

  ezInt32 iMaxDocumentsToAdd = 10;
  for (ezString s : ezQtEditorApp::GetInstance()->GetRecentDocumentsList().GetFileList())
  {
    QAction* pAction = nullptr;

    if (!ezOSFile::ExistsFile(s))
      continue;

    ezLRUMenuAction::Item item;

    ezDocumentManager* pManager;
    ezDocumentTypeDescriptor td;
    ezDocumentManager::FindDocumentTypeFromPath(s, false, pManager, &td);

    item.m_UserValue = s;
    item.m_Icon = ezUIServices::GetCachedIconResource(td.m_sIcon);

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
  ezQtEditorApp::GetInstance()->OpenDocument(value.ConvertTo<ezString>());
}


////////////////////////////////////////////////////////////////////////
// ezRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecentProjectsMenuAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();


void ezRecentProjectsMenuAction::GetEntries(ezHybridArray<ezLRUMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();

  if (ezQtEditorApp::GetInstance()->GetRecentProjectsList().GetFileList().IsEmpty())
    return;

  for (ezString s : ezQtEditorApp::GetInstance()->GetRecentProjectsList().GetFileList())
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
  ezQtEditorApp::GetInstance()->OpenProject(value.ConvertTo<ezString>());
}

////////////////////////////////////////////////////////////////////////
// ezProjectAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProjectAction, 1, ezRTTINoAllocator);
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
    ezQtEditorApp::GetInstance()->GuiCreateDocument();
    break;

  case ezProjectAction::ButtonType::OpenDocument:
    ezQtEditorApp::GetInstance()->GuiOpenDocument();
    break;

  case ezProjectAction::ButtonType::CreateProject:
    ezQtEditorApp::GetInstance()->GuiCreateProject();
    break;

  case ezProjectAction::ButtonType::OpenProject:
    ezQtEditorApp::GetInstance()->GuiOpenProject();
    break;

  case ezProjectAction::ButtonType::CloseProject:
    {
      if (ezToolsProject::CanCloseProject())
        ezQtEditorApp::GetInstance()->CloseProject();
    }
    break;

  case ezProjectAction::ButtonType::ProjectSettings:
    ezQtEditorApp::GetInstance()->ShowSettingsDocument();
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

