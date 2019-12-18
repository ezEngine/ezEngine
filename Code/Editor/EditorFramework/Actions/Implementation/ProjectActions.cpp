#include <EditorFrameworkPCH.h>

#include <Assets/AssetCurator.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Dialogs/AssetProfilesDlg.moc.h>
#include <EditorFramework/Dialogs/DataDirsDlg.moc.h>
#include <EditorFramework/Dialogs/EditorPluginConfigDlg.moc.h>
#include <EditorFramework/Dialogs/EnginePluginConfigDlg.moc.h>
#include <EditorFramework/Dialogs/InputConfigDlg.moc.h>
#include <EditorFramework/Dialogs/LaunchFileserveDlg.moc.h>
#include <EditorFramework/Dialogs/PreferencesDlg.moc.h>
#include <EditorFramework/Dialogs/TagsDlg.moc.h>
#include <EditorFramework/Dialogs/WindowCfgDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Settings/SettingsTab.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/Dialogs/ShortcutEditorDlg.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QFileDialog>
#include <QProcess>
#include <QStandardPaths>
#include <ToolsFoundation/Project/ToolsProject.h>

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
ezActionDescriptorHandle ezProjectActions::s_hWindowConfig;
ezActionDescriptorHandle ezProjectActions::s_hInputConfig;
ezActionDescriptorHandle ezProjectActions::s_hPreferencesDlg;
ezActionDescriptorHandle ezProjectActions::s_hTagsDlg;
ezActionDescriptorHandle ezProjectActions::s_hImportAsset;
ezActionDescriptorHandle ezProjectActions::s_hAssetProfiles;

ezActionDescriptorHandle ezProjectActions::s_hToolsMenu;
ezActionDescriptorHandle ezProjectActions::s_hToolsCategory;
ezActionDescriptorHandle ezProjectActions::s_hReloadResources;
ezActionDescriptorHandle ezProjectActions::s_hReloadEngine;
ezActionDescriptorHandle ezProjectActions::s_hLaunchFileserve;
ezActionDescriptorHandle ezProjectActions::s_hLaunchInspector;
ezActionDescriptorHandle ezProjectActions::s_hSaveProfiling;
ezActionDescriptorHandle ezProjectActions::s_hOpenVsCode;

void ezProjectActions::RegisterActions()
{
  s_hEditorMenu = EZ_REGISTER_MENU("Menu.Editor");

  s_hDocumentCategory = EZ_REGISTER_CATEGORY("DocumentCategory");
  s_hCreateDocument = EZ_REGISTER_ACTION_1(
    "Document.Create", ezActionScope::Global, "Project", "Ctrl+N", ezProjectAction, ezProjectAction::ButtonType::CreateDocument);
  s_hOpenDocument =
    EZ_REGISTER_ACTION_1("Document.Open", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::OpenDocument);
  s_hRecentDocuments = EZ_REGISTER_DYNAMIC_MENU("Project.RecentDocuments.Menu", ezRecentDocumentsMenuAction, "");

  s_hProjectCategory = EZ_REGISTER_CATEGORY("ProjectCategory");
  s_hCreateProject = EZ_REGISTER_ACTION_1(
    "Project.Create", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::CreateProject);
  s_hOpenProject =
    EZ_REGISTER_ACTION_1("Project.Open", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::OpenProject);
  s_hRecentProjects = EZ_REGISTER_DYNAMIC_MENU("Project.RecentProjects.Menu", ezRecentProjectsMenuAction, "");
  s_hCloseProject =
    EZ_REGISTER_ACTION_1("Project.Close", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::CloseProject);

  s_hSettingsCategory = EZ_REGISTER_CATEGORY("SettingsCategory");
  s_hEditorSettingsMenu = EZ_REGISTER_MENU_WITH_ICON("Menu.EditorSettings", ":/GuiFoundation/Icons/Settings16.png");
  s_hProjectSettingsMenu = EZ_REGISTER_MENU("Menu.ProjectSettings");

  s_hShortcutEditor =
    EZ_REGISTER_ACTION_1("Editor.Shortcuts", ezActionScope::Global, "Editor", "", ezProjectAction, ezProjectAction::ButtonType::Shortcuts);
  s_hEditorPlugins = EZ_REGISTER_ACTION_1(
    "Editor.Plugins", ezActionScope::Global, "Editor", "", ezProjectAction, ezProjectAction::ButtonType::EditorPlugins);
  s_hEnginePlugins = EZ_REGISTER_ACTION_1(
    "Engine.Plugins", ezActionScope::Global, "Editor", "", ezProjectAction, ezProjectAction::ButtonType::EnginePlugins);
  s_hPreferencesDlg = EZ_REGISTER_ACTION_1(
    "Editor.Preferences", ezActionScope::Global, "Editor", "", ezProjectAction, ezProjectAction::ButtonType::PreferencesDialog);
  s_hTagsDlg =
    EZ_REGISTER_ACTION_1("Engine.Tags", ezActionScope::Global, "Editor", "", ezProjectAction, ezProjectAction::ButtonType::TagsDialog);

  s_hDataDirectories = EZ_REGISTER_ACTION_1(
    "Project.DataDirectories", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::DataDirectories);
  s_hInputConfig = EZ_REGISTER_ACTION_1(
    "Project.InputConfig", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::InputConfig);
  s_hWindowConfig = EZ_REGISTER_ACTION_1(
    "Project.WindowConfig", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::WindowConfig);
  s_hImportAsset = EZ_REGISTER_ACTION_1(
    "Project.ImportAsset", ezActionScope::Global, "Project", "Ctrl+I", ezProjectAction, ezProjectAction::ButtonType::ImportAsset);
  s_hAssetProfiles = EZ_REGISTER_ACTION_1(
    "Project.AssetProfiles", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::AssetProfiles);

  s_hToolsMenu = EZ_REGISTER_MENU("Menu.Tools");
  s_hToolsCategory = EZ_REGISTER_CATEGORY("ToolsCategory");
  s_hReloadResources = EZ_REGISTER_ACTION_1(
    "Engine.ReloadResources", ezActionScope::Global, "Engine", "F4", ezProjectAction, ezProjectAction::ButtonType::ReloadResources);
  s_hReloadEngine = EZ_REGISTER_ACTION_1(
    "Engine.ReloadEngine", ezActionScope::Global, "Engine", "Ctrl+Shift+F4", ezProjectAction, ezProjectAction::ButtonType::ReloadEngine);
  s_hLaunchFileserve = EZ_REGISTER_ACTION_1(
    "Editor.LaunchFileserve", ezActionScope::Global, "Engine", "", ezProjectAction, ezProjectAction::ButtonType::LaunchFileserve);
  s_hLaunchInspector = EZ_REGISTER_ACTION_1(
    "Editor.LaunchInspector", ezActionScope::Global, "Engine", "", ezProjectAction, ezProjectAction::ButtonType::LaunchInspector);
  s_hSaveProfiling = EZ_REGISTER_ACTION_1(
    "Editor.SaveProfiling", ezActionScope::Global, "Engine", "Alt+S", ezProjectAction, ezProjectAction::ButtonType::SaveProfiling);
  s_hOpenVsCode = EZ_REGISTER_ACTION_1(
    "Editor.OpenVsCode", ezActionScope::Global, "Project", "Ctrl+Alt+O", ezProjectAction, ezProjectAction::ButtonType::OpenVsCode);
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
  ezActionManager::UnregisterAction(s_hLaunchFileserve);
  ezActionManager::UnregisterAction(s_hLaunchInspector);
  ezActionManager::UnregisterAction(s_hSaveProfiling);
  ezActionManager::UnregisterAction(s_hOpenVsCode);
  ezActionManager::UnregisterAction(s_hShortcutEditor);
  ezActionManager::UnregisterAction(s_hEditorPlugins);
  ezActionManager::UnregisterAction(s_hEnginePlugins);
  ezActionManager::UnregisterAction(s_hPreferencesDlg);
  ezActionManager::UnregisterAction(s_hTagsDlg);
  ezActionManager::UnregisterAction(s_hDataDirectories);
  ezActionManager::UnregisterAction(s_hWindowConfig);
  ezActionManager::UnregisterAction(s_hImportAsset);
  ezActionManager::UnregisterAction(s_hInputConfig);
  ezActionManager::UnregisterAction(s_hAssetProfiles);
}

void ezProjectActions::MapActions(const char* szMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hEditorMenu, "", -1000000000.0f);

  pMap->MapAction(s_hDocumentCategory, "Menu.Editor", 1.0f);
  pMap->MapAction(s_hCreateDocument, "Menu.Editor/DocumentCategory", 1.0f);
  pMap->MapAction(s_hOpenDocument, "Menu.Editor/DocumentCategory", 2.0f);
  pMap->MapAction(s_hImportAsset, "Menu.Editor/DocumentCategory", 3.0f);
  pMap->MapAction(s_hRecentDocuments, "Menu.Editor/DocumentCategory", 4.0f);

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
  pMap->MapAction(s_hLaunchFileserve, "Menu.Tools/ToolsCategory", 3.0f);
  pMap->MapAction(s_hLaunchInspector, "Menu.Tools/ToolsCategory", 3.5f);
  pMap->MapAction(s_hSaveProfiling, "Menu.Tools/ToolsCategory", 4.0f);
  pMap->MapAction(s_hOpenVsCode, "Menu.Tools/ToolsCategory", 5.0f);

  pMap->MapAction(s_hEditorPlugins, "Menu.Editor/SettingsCategory/Menu.EditorSettings", 1.0f);
  pMap->MapAction(s_hShortcutEditor, "Menu.Editor/SettingsCategory/Menu.EditorSettings", 2.0f);
  pMap->MapAction(s_hPreferencesDlg, "Menu.Editor/SettingsCategory/Menu.EditorSettings", 3.0f);

  pMap->MapAction(s_hDataDirectories, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 1.0f);
  pMap->MapAction(s_hEnginePlugins, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 2.0f);
  pMap->MapAction(s_hInputConfig, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 3.0f);
  pMap->MapAction(s_hTagsDlg, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 4.0f);
  pMap->MapAction(s_hWindowConfig, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 5.0f);
  pMap->MapAction(s_hAssetProfiles, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 6.0f);
}

////////////////////////////////////////////////////////////////////////
// ezRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecentDocumentsMenuAction, 0, ezRTTINoAllocator)
  ;
EZ_END_DYNAMIC_REFLECTED_TYPE;


void ezRecentDocumentsMenuAction::GetEntries(ezHybridArray<ezDynamicMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();

  if (ezQtEditorApp::GetSingleton()->GetRecentDocumentsList().GetFileList().IsEmpty())
    return;

  ezInt32 iMaxDocumentsToAdd = 10;
  for (auto file : ezQtEditorApp::GetSingleton()->GetRecentDocumentsList().GetFileList())
  {
    QAction* pAction = nullptr;

    if (!ezOSFile::ExistsFile(file.m_File))
      continue;

    ezDynamicMenuAction::Item item;

    const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (ezDocumentManager::FindDocumentTypeFromPath(file.m_File, false, pTypeDesc).Failed())
      continue;

    item.m_UserValue = file.m_File;
    item.m_Icon = ezQtUiServices::GetCachedIconResource(pTypeDesc->m_sIcon);

    if (ezToolsProject::IsProjectOpen())
    {
      ezString sRelativePath;
      if (!ezToolsProject::GetSingleton()->IsDocumentInAllowedRoot(file.m_File, &sRelativePath))
        continue;

      item.m_sDisplay = sRelativePath;

      out_Entries.PushBack(item);
    }
    else
    {
      item.m_sDisplay = file.m_File;

      out_Entries.PushBack(item);
    }

    --iMaxDocumentsToAdd;

    if (iMaxDocumentsToAdd <= 0)
      break;
  }
}

void ezRecentDocumentsMenuAction::Execute(const ezVariant& value)
{
  ezQtEditorApp::GetSingleton()->OpenDocumentQueued(value.ConvertTo<ezString>());
}


////////////////////////////////////////////////////////////////////////
// ezRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecentProjectsMenuAction, 1, ezRTTINoAllocator)
  ;
EZ_END_DYNAMIC_REFLECTED_TYPE;


void ezRecentProjectsMenuAction::GetEntries(ezHybridArray<ezDynamicMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();

  if (ezQtEditorApp::GetSingleton()->GetRecentProjectsList().GetFileList().IsEmpty())
    return;

  ezStringBuilder sTemp;

  for (auto file : ezQtEditorApp::GetSingleton()->GetRecentProjectsList().GetFileList())
  {
    if (!ezOSFile::ExistsFile(file.m_File))
      continue;

    sTemp = file.m_File;
    sTemp.PathParentDirectory();
    sTemp.Trim("/");

    ezDynamicMenuAction::Item item;
    item.m_sDisplay = sTemp;
    item.m_UserValue = file.m_File;

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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProjectAction, 1, ezRTTINoAllocator)
  ;
EZ_END_DYNAMIC_REFLECTED_TYPE;

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
    case ezProjectAction::ButtonType::LaunchFileserve:
      SetIconPath(":/EditorFramework/Icons/Fileserve16.png");
      break;
    case ezProjectAction::ButtonType::LaunchInspector:
      SetIconPath(":/EditorFramework/Icons/Inspector16.png");
      break;
    case ezProjectAction::ButtonType::ReloadEngine:
      SetIconPath(":/GuiFoundation/Icons/ReloadEngine16.png");
      break;
    case ezProjectAction::ButtonType::DataDirectories:
      SetIconPath(":/EditorFramework/Icons/DataDirectories16.png");
      break;
    case ezProjectAction::ButtonType::WindowConfig:
      SetIconPath(":/EditorFramework/Icons/WindowConfig16.png");
      break;
    case ezProjectAction::ButtonType::ImportAsset:
      SetIconPath(":/GuiFoundation/Icons/DocumentImport16.png");
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
    case ezProjectAction::ButtonType::AssetProfiles:
      SetIconPath(":/EditorFramework/Icons/AssetProfiles16.png");
      break;
    case ezProjectAction::ButtonType::OpenVsCode:
      SetIconPath(":/GuiFoundation/Icons/vscode16.png");
      break;
  }

  if (m_ButtonType == ButtonType::CloseProject || m_ButtonType == ButtonType::DataDirectories || m_ButtonType == ButtonType::WindowConfig ||
      m_ButtonType == ButtonType::ImportAsset || m_ButtonType == ButtonType::EnginePlugins || m_ButtonType == ButtonType::TagsDialog ||
      m_ButtonType == ButtonType::ReloadEngine || m_ButtonType == ButtonType::ReloadResources ||
      m_ButtonType == ButtonType::LaunchFileserve || m_ButtonType == ButtonType::LaunchInspector || m_ButtonType == ButtonType::OpenVsCode ||
      m_ButtonType == ButtonType::InputConfig || m_ButtonType == ButtonType::AssetProfiles)
  {
    SetEnabled(ezToolsProject::IsProjectOpen());

    ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezProjectAction::ProjectEventHandler, this));
  }
}

ezProjectAction::~ezProjectAction()
{
  if (m_ButtonType == ButtonType::CloseProject || m_ButtonType == ButtonType::DataDirectories || m_ButtonType == ButtonType::WindowConfig ||
      m_ButtonType == ButtonType::ImportAsset || m_ButtonType == ButtonType::EnginePlugins || m_ButtonType == ButtonType::TagsDialog ||
      m_ButtonType == ButtonType::ReloadEngine || m_ButtonType == ButtonType::ReloadResources ||
      m_ButtonType == ButtonType::LaunchFileserve || m_ButtonType == ButtonType::LaunchInspector || m_ButtonType == ButtonType::OpenVsCode ||
      m_ButtonType == ButtonType::InputConfig || m_ButtonType == ButtonType::AssetProfiles)
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

    case ezProjectAction::ButtonType::WindowConfig:
    {
      ezQtWindowCfgDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case ezProjectAction::ButtonType::ImportAsset:
    {
      ezAssetDocumentGenerator::ImportAssets();
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
        // save modified preferences right away
        ezQtEditorApp::GetSingleton()->SaveSettings();

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

      ezEditorAppEvent e;
      e.m_Type = ezEditorAppEvent::Type::ReloadResources;
      ezQtEditorApp::GetSingleton()->m_Events.Broadcast(e);

      if (m_Context.m_pDocument)
      {
        m_Context.m_pDocument->ShowDocumentStatus("Reloading Resources");
      }

      ezTranslator::ReloadAllTranslators();
    }
    break;

    case ezProjectAction::ButtonType::LaunchFileserve:
    {
      ezQtLaunchFileserveDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case ezProjectAction::ButtonType::LaunchInspector:
    {
      ezQtEditorApp::GetSingleton()->RunInspector();
    }
    break;

    case ezProjectAction::ButtonType::ReloadEngine:
    {
      ezEditorEngineProcessConnection::GetSingleton()->RestartProcess();
    }
    break;

    case ezProjectAction::ButtonType::SaveProfiling:
    {
      ezFileWriter fileWriter;
      if (fileWriter.Open(":appdata/profiling.json") == EZ_SUCCESS)
      {
        ezProfilingSystem::ProfilingData profilingData = ezProfilingSystem::Capture();
        profilingData.Write(fileWriter);

        ezLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
        ezQtUiServices::GetSingleton()->ShowAllDocumentsStatusBarMessage(
          ezFmt("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData()), ezTime::Seconds(5.0));
      }
      else
      {
        ezLog::Error("Could not write profiling capture to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
      }
    }
    break;

    case ezProjectAction::ButtonType::OpenVsCode:
    {
      QStringList args;

      for (const auto& dd : ezQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
      {
        ezStringBuilder path;
        ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, path);

        args.append(QString::fromUtf8(path));
      }

      const ezStatus res = ezQtUiServices::OpenInVsCode(args);

      ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Failed to open VS Code");
    }
    break;

    case ezProjectAction::ButtonType::AssetProfiles:
    {
      ezQtAssetProfilesDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        // we need to force the asset status reevaluation because when the profile settings have changed,
        // we need to figure out which assets are now out of date
        ezAssetCurator::GetSingleton()->SetActiveAssetProfileByIndex(dlg.m_uiActiveConfig, true);

        // makes the scene re-select the current objects, which updates which enum values are shown in the property grid
        ezToolsProject::BroadcastConfigChanged();
      }
    }
    break;
  }
}
