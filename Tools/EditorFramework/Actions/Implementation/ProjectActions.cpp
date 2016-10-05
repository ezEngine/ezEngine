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
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/Dialogs/ShortcutEditorDlg.moc.h>
#include <EditorFramework/Dialogs/DataDirsDlg.moc.h>
#include <EditorFramework/Dialogs/EditorPluginConfigDlg.moc.h>
#include <EditorFramework/Dialogs/EnginePluginConfigDlg.moc.h>
#include <EditorFramework/Dialogs/PreferencesDlg.moc.h>
#include <EditorFramework/Dialogs/InputConfigDlg.moc.h>
#include <Dialogs/TagsDlg.moc.h>

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
ezActionDescriptorHandle ezProjectActions::s_hEditorSettingsMenu;
ezActionDescriptorHandle ezProjectActions::s_hProjectSettingsMenu;
ezActionDescriptorHandle ezProjectActions::s_hShortcutEditor;
ezActionDescriptorHandle ezProjectActions::s_hEditorPlugins;
ezActionDescriptorHandle ezProjectActions::s_hEnginePlugins;
ezActionDescriptorHandle ezProjectActions::s_hDataDirectories;
ezActionDescriptorHandle ezProjectActions::s_hInputConfig;
ezActionDescriptorHandle ezProjectActions::s_hPreferencesDlg;
ezActionDescriptorHandle ezProjectActions::s_hTagsDlg;

ezActionDescriptorHandle ezProjectActions::s_hToolsMenu;
ezActionDescriptorHandle ezProjectActions::s_hToolsCategory;
ezActionDescriptorHandle ezProjectActions::s_hReloadResources;
ezActionDescriptorHandle ezProjectActions::s_hReloadEngine;

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
  s_hEditorSettingsMenu = EZ_REGISTER_MENU_WITH_ICON("Menu.EditorSettings", ":/GuiFoundation/Icons/Settings16.png");
  s_hProjectSettingsMenu = EZ_REGISTER_MENU("Menu.ProjectSettings");

  s_hShortcutEditor = EZ_REGISTER_ACTION_1("Editor.Shortcuts", ezActionScope::Global, "Editor", "", ezProjectAction, ezProjectAction::ButtonType::Shortcuts);
  s_hEditorPlugins = EZ_REGISTER_ACTION_1("Editor.Plugins", ezActionScope::Global, "Editor", "", ezProjectAction, ezProjectAction::ButtonType::EditorPlugins);
  s_hEnginePlugins = EZ_REGISTER_ACTION_1("Engine.Plugins", ezActionScope::Global, "Editor", "", ezProjectAction, ezProjectAction::ButtonType::EnginePlugins);
  s_hPreferencesDlg = EZ_REGISTER_ACTION_1("Editor.ezQtPreferencesDlg", ezActionScope::Global, "Editor", "Ctrl+P", ezProjectAction, ezProjectAction::ButtonType::PreferencesDialog);
  s_hTagsDlg = EZ_REGISTER_ACTION_1("Engine.ezQtTagsDlg", ezActionScope::Global, "Editor", "", ezProjectAction, ezProjectAction::ButtonType::TagsDialog);

  s_hDataDirectories = EZ_REGISTER_ACTION_1("Project.DataDirectories", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::DataDirectories);
  s_hInputConfig = EZ_REGISTER_ACTION_1("Project.InputConfig", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::InputConfig);

  s_hToolsMenu = EZ_REGISTER_MENU("Menu.Tools");
  s_hToolsCategory = EZ_REGISTER_CATEGORY("ToolsCategory");
  s_hReloadResources = EZ_REGISTER_ACTION_1("Engine.ReloadResources", ezActionScope::Global, "Engine", "F4", ezProjectAction, ezProjectAction::ButtonType::ReloadResources);
  s_hReloadEngine = EZ_REGISTER_ACTION_1("Engine.ReloadEngine", ezActionScope::Global, "Engine", "Ctrl+Shift+F4", ezProjectAction, ezProjectAction::ButtonType::ReloadEngine);
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
  ezActionManager::UnregisterAction(s_hEditorSettingsMenu);
  ezActionManager::UnregisterAction(s_hProjectSettingsMenu);
  ezActionManager::UnregisterAction(s_hToolsMenu);
  ezActionManager::UnregisterAction(s_hToolsCategory);
  ezActionManager::UnregisterAction(s_hReloadResources);
  ezActionManager::UnregisterAction(s_hReloadEngine);
  ezActionManager::UnregisterAction(s_hShortcutEditor);
  ezActionManager::UnregisterAction(s_hEditorPlugins);
  ezActionManager::UnregisterAction(s_hEnginePlugins);
  ezActionManager::UnregisterAction(s_hPreferencesDlg);
  ezActionManager::UnregisterAction(s_hTagsDlg);
  ezActionManager::UnregisterAction(s_hDataDirectories);
  ezActionManager::UnregisterAction(s_hInputConfig);
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
  pMap->MapAction(s_hProjectSettingsMenu, "Menu.Editor/ProjectCategory", 1000.0f);

  pMap->MapAction(s_hSettingsCategory, "Menu.Editor", 3.0f);
  pMap->MapAction(s_hEditorSettingsMenu, "Menu.Editor/SettingsCategory", 1.0f);

  pMap->MapAction(s_hToolsMenu, "", 4.5f);
  pMap->MapAction(s_hToolsCategory, "Menu.Tools", 1.0f);
  pMap->MapAction(s_hReloadResources, "Menu.Tools/ToolsCategory", 1.0f);
  pMap->MapAction(s_hReloadEngine, "Menu.Tools/ToolsCategory", 2.0f);

  pMap->MapAction(s_hEditorPlugins, "Menu.Editor/SettingsCategory/Menu.EditorSettings", 1.0f);
  pMap->MapAction(s_hShortcutEditor, "Menu.Editor/SettingsCategory/Menu.EditorSettings", 2.0f);
  pMap->MapAction(s_hPreferencesDlg, "Menu.Editor/SettingsCategory/Menu.EditorSettings", 3.0f);

  pMap->MapAction(s_hDataDirectories, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 1.0f);
  pMap->MapAction(s_hEnginePlugins, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 2.0f);
  pMap->MapAction(s_hInputConfig, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 3.0f);
  pMap->MapAction(s_hTagsDlg, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 4.0f);
}

////////////////////////////////////////////////////////////////////////
// ezRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecentDocumentsMenuAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE


void ezRecentDocumentsMenuAction::GetEntries(ezHybridArray<ezLRUMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();

  if (ezQtEditorApp::GetSingleton()->GetRecentDocumentsList().GetFileList().IsEmpty())
    return;

  ezInt32 iMaxDocumentsToAdd = 10;
  for (ezString s : ezQtEditorApp::GetSingleton()->GetRecentDocumentsList().GetFileList())
  {
    QAction* pAction = nullptr;

    if (!ezOSFile::ExistsFile(s))
      continue;

    ezLRUMenuAction::Item item;

    const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (ezDocumentManager::FindDocumentTypeFromPath(s, false, pTypeDesc).Failed())
      continue;

    item.m_UserValue = s;
    item.m_Icon = ezQtUiServices::GetCachedIconResource(pTypeDesc->m_sIcon);

    if (ezToolsProject::IsProjectOpen())
    {
      ezString sRelativePath;
      if (!ezToolsProject::GetSingleton()->IsDocumentInAllowedRoot(s, &sRelativePath))
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
  ezQtEditorApp::GetSingleton()->OpenDocument(value.ConvertTo<ezString>());
}


////////////////////////////////////////////////////////////////////////
// ezRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecentProjectsMenuAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE


void ezRecentProjectsMenuAction::GetEntries(ezHybridArray<ezLRUMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();

  if (ezQtEditorApp::GetSingleton()->GetRecentProjectsList().GetFileList().IsEmpty())
    return;

  ezStringBuilder sTemp;

  for (ezString s : ezQtEditorApp::GetSingleton()->GetRecentProjectsList().GetFileList())
  {
    if (!ezOSFile::ExistsFile(s))
      continue;

    sTemp = s;
    sTemp.PathParentDirectory();
    sTemp.Trim("/");

    ezLRUMenuAction::Item item;
    item.m_sDisplay = sTemp;
    item.m_UserValue = s;

    out_Entries.PushBack(item);
  }
}

void ezRecentProjectsMenuAction::Execute(const ezVariant& value)
{
  ezQtEditorApp::GetSingleton()->OpenProject(value.ConvertTo<ezString>());
}

////////////////////////////////////////////////////////////////////////
// ezProjectAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProjectAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

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
  case ezProjectAction::ButtonType::ReloadResources:
    SetIconPath(":/GuiFoundation/Icons/ReloadResources16.png");
    break;
  case ezProjectAction::ButtonType::ReloadEngine:
    SetIconPath(":/GuiFoundation/Icons/ReloadEngine16.png");
    break;
  case ezProjectAction::ButtonType::DataDirectories:
    SetIconPath(":/EditorFramework/Icons/DataDirectories16.png");
    break;
  case ezProjectAction::ButtonType::InputConfig:
    SetIconPath(":/EditorFramework/Icons/Input16.png");
    break;
  case ezProjectAction::ButtonType::EditorPlugins:
    SetIconPath(":/EditorFramework/Icons/Plugins16.png");
    break;
  case ezProjectAction::ButtonType::EnginePlugins:
    SetIconPath(":/EditorFramework/Icons/Plugins16.png");
    break;
  case ezProjectAction::ButtonType::PreferencesDialog:
    SetIconPath(":/EditorFramework/Icons/StoredSettings16.png");
    break;
  case ezProjectAction::ButtonType::TagsDialog:
    SetIconPath(":/EditorFramework/Icons/Tag16.png");
    break;
  case ezProjectAction::ButtonType::Shortcuts:
    SetIconPath(":/GuiFoundation/Icons/Shortcuts16.png");
    break;
  }

  if (m_ButtonType == ButtonType::CloseProject ||
      m_ButtonType == ButtonType::DataDirectories ||
      m_ButtonType == ButtonType::EnginePlugins ||
      m_ButtonType == ButtonType::TagsDialog ||
      m_ButtonType == ButtonType::ReloadEngine ||
      m_ButtonType == ButtonType::ReloadResources ||
      m_ButtonType == ButtonType::InputConfig)
  {
    SetEnabled(ezToolsProject::IsProjectOpen());

    ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezProjectAction::ProjectEventHandler, this));
  }
}

ezProjectAction::~ezProjectAction()
{
  if (m_ButtonType == ButtonType::CloseProject ||
      m_ButtonType == ButtonType::DataDirectories ||
      m_ButtonType == ButtonType::EnginePlugins ||
      m_ButtonType == ButtonType::TagsDialog ||
      m_ButtonType == ButtonType::ReloadEngine ||
      m_ButtonType == ButtonType::ReloadResources ||
      m_ButtonType == ButtonType::InputConfig)
  {
    ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezProjectAction::ProjectEventHandler, this));
  }
}

void ezProjectAction::ProjectEventHandler(const ezToolsProjectEvent& e)
{
  SetEnabled(ezToolsProject::IsProjectOpen());
}

void ezProjectAction::Execute(const ezVariant& value)
{
  switch (m_ButtonType)
  {
  case ezProjectAction::ButtonType::CreateDocument:
    ezQtEditorApp::GetSingleton()->GuiCreateDocument();
    break;

  case ezProjectAction::ButtonType::OpenDocument:
    ezQtEditorApp::GetSingleton()->GuiOpenDocument();
    break;

  case ezProjectAction::ButtonType::CreateProject:
    ezQtEditorApp::GetSingleton()->GuiCreateProject();
    break;

  case ezProjectAction::ButtonType::OpenProject:
    ezQtEditorApp::GetSingleton()->GuiOpenProject();
    break;

  case ezProjectAction::ButtonType::CloseProject:
    {
      if (ezToolsProject::CanCloseProject())
        ezQtEditorApp::GetSingleton()->CloseProject();
    }
    break;

  case ezProjectAction::ButtonType::DataDirectories:
    {
      ezQtDataDirsDlg dlg(nullptr);
      dlg.exec();
    }
    break;

  case ezProjectAction::ButtonType::InputConfig:
    {
      ezQtInputConfigDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        ezToolsProject::BroadcastConfigChanged();
      }
    }
    break;

  case ezProjectAction::ButtonType::EditorPlugins:
    {
      ezQtEditorPluginConfigDlg dlg(nullptr);
      dlg.exec();
    }
    break;

  case ezProjectAction::ButtonType::EnginePlugins:
    {
      ezQtEnginePluginConfigDlg dlg(nullptr);
      dlg.exec();
    }
    break;

  case ezProjectAction::ButtonType::PreferencesDialog:
    {
      ezQtPreferencesDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        ezToolsProject::BroadcastConfigChanged();
      }
    }
    break;

  case ezProjectAction::ButtonType::TagsDialog:
    {
      ezQtTagsDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        ezToolsProject::BroadcastConfigChanged();
      }
    }
    break;

  case ezProjectAction::ButtonType::Shortcuts:
    {
      ezQtShortcutEditorDlg dlg(nullptr);
      dlg.exec();
    }
    break;

  case ezProjectAction::ButtonType::ReloadResources:
    {
      ezSimpleConfigMsgToEngine msg;
      msg.m_sWhatToDo = "ReloadResources";
      ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);

      if (m_Context.m_pDocument)
      {
        m_Context.m_pDocument->ShowDocumentStatus("Reloading Resources");
      }
    }
    break;

  case ezProjectAction::ButtonType::ReloadEngine:
    {
      ezEditorEngineProcessConnection::GetSingleton()->RestartProcess();
    }
    break;
  }

}

