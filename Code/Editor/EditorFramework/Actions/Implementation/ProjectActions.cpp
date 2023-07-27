#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/Dialogs/AssetProfilesDlg.moc.h>
#include <EditorFramework/Dialogs/CppProjectDlg.moc.h>
#include <EditorFramework/Dialogs/DataDirsDlg.moc.h>
#include <EditorFramework/Dialogs/ExportProjectDlg.moc.h>
#include <EditorFramework/Dialogs/InputConfigDlg.moc.h>
#include <EditorFramework/Dialogs/LaunchFileserveDlg.moc.h>
#include <EditorFramework/Dialogs/PluginSelectionDlg.moc.h>
#include <EditorFramework/Dialogs/PreferencesDlg.moc.h>
#include <EditorFramework/Dialogs/TagsDlg.moc.h>
#include <EditorFramework/Dialogs/WindowCfgDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/Dialogs/ShortcutEditorDlg.moc.h>

ezActionDescriptorHandle ezProjectActions::s_hCatProjectGeneral;
ezActionDescriptorHandle ezProjectActions::s_hCatProjectAssets;
ezActionDescriptorHandle ezProjectActions::s_hCatProjectConfig;
ezActionDescriptorHandle ezProjectActions::s_hCatProjectExternal;

ezActionDescriptorHandle ezProjectActions::s_hCatFilesGeneral;
ezActionDescriptorHandle ezProjectActions::s_hCatFileCommon;
ezActionDescriptorHandle ezProjectActions::s_hCatFileSpecial;
ezActionDescriptorHandle ezProjectActions::s_hCatAssetDoc;

ezActionDescriptorHandle ezProjectActions::s_hCreateDocument;
ezActionDescriptorHandle ezProjectActions::s_hOpenDocument;
ezActionDescriptorHandle ezProjectActions::s_hRecentDocuments;

ezActionDescriptorHandle ezProjectActions::s_hOpenDashboard;
ezActionDescriptorHandle ezProjectActions::s_hCreateProject;
ezActionDescriptorHandle ezProjectActions::s_hOpenProject;
ezActionDescriptorHandle ezProjectActions::s_hRecentProjects;
ezActionDescriptorHandle ezProjectActions::s_hCloseProject;
ezActionDescriptorHandle ezProjectActions::s_hDocsAndCommunity;

ezActionDescriptorHandle ezProjectActions::s_hCatProjectSettings;
ezActionDescriptorHandle ezProjectActions::s_hCatPluginSettings;
ezActionDescriptorHandle ezProjectActions::s_hShortcutEditor;
ezActionDescriptorHandle ezProjectActions::s_hDataDirectories;
ezActionDescriptorHandle ezProjectActions::s_hWindowConfig;
ezActionDescriptorHandle ezProjectActions::s_hInputConfig;
ezActionDescriptorHandle ezProjectActions::s_hPreferencesDlg;
ezActionDescriptorHandle ezProjectActions::s_hTagsConfig;
ezActionDescriptorHandle ezProjectActions::s_hImportAsset;
ezActionDescriptorHandle ezProjectActions::s_hAssetProfiles;
ezActionDescriptorHandle ezProjectActions::s_hExportProject;
ezActionDescriptorHandle ezProjectActions::s_hPluginSelection;
ezActionDescriptorHandle ezProjectActions::s_hClearAssetCaches;

ezActionDescriptorHandle ezProjectActions::s_hCatToolsExternal;
ezActionDescriptorHandle ezProjectActions::s_hCatToolsEditor;
ezActionDescriptorHandle ezProjectActions::s_hCatToolsDocument;
ezActionDescriptorHandle ezProjectActions::s_hCatEditorSettings;
ezActionDescriptorHandle ezProjectActions::s_hReloadResources;
ezActionDescriptorHandle ezProjectActions::s_hReloadEngine;
ezActionDescriptorHandle ezProjectActions::s_hLaunchFileserve;
ezActionDescriptorHandle ezProjectActions::s_hLaunchInspector;
ezActionDescriptorHandle ezProjectActions::s_hSaveProfiling;
ezActionDescriptorHandle ezProjectActions::s_hOpenVsCode;

ezActionDescriptorHandle ezProjectActions::s_hCppProjectMenu;
ezActionDescriptorHandle ezProjectActions::s_hSetupCppProject;
ezActionDescriptorHandle ezProjectActions::s_hOpenCppProject;
ezActionDescriptorHandle ezProjectActions::s_hCompileCppProject;

void ezProjectActions::RegisterActions()
{
  s_hCatProjectGeneral = EZ_REGISTER_CATEGORY("G.Project.General");
  s_hCatProjectAssets = EZ_REGISTER_CATEGORY("G.Project.Assets");
  s_hCatProjectExternal = EZ_REGISTER_CATEGORY("G.Project.External");
  s_hCatProjectConfig = EZ_REGISTER_CATEGORY("G.Project.Config");
  s_hCatEditorSettings = EZ_REGISTER_CATEGORY("G.Editor.Settings");

  s_hCatFilesGeneral = EZ_REGISTER_CATEGORY("G.Files.General");
  s_hCatFileCommon = EZ_REGISTER_CATEGORY("G.File.Common");
  s_hCatFileSpecial = EZ_REGISTER_CATEGORY("G.File.Special");
  s_hCatAssetDoc = EZ_REGISTER_CATEGORY("G.AssetDoc");


  s_hOpenDashboard = EZ_REGISTER_ACTION_1("Editor.OpenDashboard", ezActionScope::Global, "Editor", "Ctrl+Shift+D", ezProjectAction, ezProjectAction::ButtonType::OpenDashboard);

  s_hCreateProject = EZ_REGISTER_ACTION_1("Project.Create", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::CreateProject);

  s_hOpenProject = EZ_REGISTER_ACTION_1("Project.Open", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::OpenProject);

  s_hRecentProjects = EZ_REGISTER_DYNAMIC_MENU("Project.RecentProjects.Menu", ezRecentProjectsMenuAction, "");
  s_hCloseProject = EZ_REGISTER_ACTION_1("Project.Close", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::CloseProject);

  s_hImportAsset = EZ_REGISTER_ACTION_1("Project.ImportAsset", ezActionScope::Global, "Project", "Ctrl+I", ezProjectAction, ezProjectAction::ButtonType::ImportAsset);
  s_hClearAssetCaches = EZ_REGISTER_ACTION_1("Project.ClearAssetCaches", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::ClearAssetCaches);

  s_hExportProject = EZ_REGISTER_ACTION_1("Project.ExportProject", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::ExportProject);

  s_hCppProjectMenu = EZ_REGISTER_MENU("G.Project.Cpp");
  {
    s_hSetupCppProject = EZ_REGISTER_ACTION_1("Project.SetupCppProject", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::SetupCppProject);
    s_hOpenCppProject = EZ_REGISTER_ACTION_1("Project.OpenCppProject", ezActionScope::Global, "Project", "Ctrl+Shift+O", ezProjectAction, ezProjectAction::ButtonType::OpenCppProject);
    s_hCompileCppProject = EZ_REGISTER_ACTION_1("Project.CompileCppProject", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::CompileCppProject);
  }

  s_hCatProjectSettings = EZ_REGISTER_MENU("G.Project.Settings");

  s_hPluginSelection = EZ_REGISTER_ACTION_1("Project.PluginSelection", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::PluginSelection);
  s_hDataDirectories = EZ_REGISTER_ACTION_1("Project.DataDirectories", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::DataDirectories);
  s_hTagsConfig = EZ_REGISTER_ACTION_1("Engine.Tags", ezActionScope::Global, "Editor", "", ezProjectAction, ezProjectAction::ButtonType::TagsDialog);
  s_hInputConfig = EZ_REGISTER_ACTION_1("Project.InputConfig", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::InputConfig);
  s_hWindowConfig = EZ_REGISTER_ACTION_1("Project.WindowConfig", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::WindowConfig);
  s_hAssetProfiles = EZ_REGISTER_ACTION_1("Project.AssetProfiles", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::AssetProfiles);

  s_hCatPluginSettings = EZ_REGISTER_MENU("G.Plugins.Settings");

  //////////////////////////////////////////////////////////////////////////

  s_hCreateDocument = EZ_REGISTER_ACTION_1("Document.Create", ezActionScope::Global, "Project", "Ctrl+N", ezProjectAction, ezProjectAction::ButtonType::CreateDocument);
  s_hOpenDocument = EZ_REGISTER_ACTION_1("Document.Open", ezActionScope::Global, "Project", "", ezProjectAction, ezProjectAction::ButtonType::OpenDocument);
  s_hRecentDocuments = EZ_REGISTER_DYNAMIC_MENU("Project.RecentDocuments.Menu", ezRecentDocumentsMenuAction, "");

  s_hShortcutEditor = EZ_REGISTER_ACTION_1("Editor.Shortcuts", ezActionScope::Global, "Editor", "", ezProjectAction, ezProjectAction::ButtonType::Shortcuts);
  s_hPreferencesDlg = EZ_REGISTER_ACTION_1("Editor.Preferences", ezActionScope::Global, "Editor", "", ezProjectAction, ezProjectAction::ButtonType::PreferencesDialog);

  s_hCatToolsExternal = EZ_REGISTER_CATEGORY("G.Tools.External");
  s_hCatToolsEditor = EZ_REGISTER_CATEGORY("G.Tools.Editor");
  s_hCatToolsDocument = EZ_REGISTER_CATEGORY("G.Tools.Document");

  s_hReloadResources = EZ_REGISTER_ACTION_1("Engine.ReloadResources", ezActionScope::Global, "Engine", "F4", ezProjectAction, ezProjectAction::ButtonType::ReloadResources);
  s_hReloadEngine = EZ_REGISTER_ACTION_1("Engine.ReloadEngine", ezActionScope::Global, "Engine", "Ctrl+Shift+F4", ezProjectAction, ezProjectAction::ButtonType::ReloadEngine);
  s_hLaunchFileserve = EZ_REGISTER_ACTION_1("Editor.LaunchFileserve", ezActionScope::Global, "Engine", "", ezProjectAction, ezProjectAction::ButtonType::LaunchFileserve);
  s_hLaunchInspector = EZ_REGISTER_ACTION_1("Editor.LaunchInspector", ezActionScope::Global, "Engine", "", ezProjectAction, ezProjectAction::ButtonType::LaunchInspector);
  s_hSaveProfiling = EZ_REGISTER_ACTION_1("Editor.SaveProfiling", ezActionScope::Global, "Engine", "Ctrl+Alt+P", ezProjectAction, ezProjectAction::ButtonType::SaveProfiling);
  s_hOpenVsCode = EZ_REGISTER_ACTION_1("Editor.OpenVsCode", ezActionScope::Global, "Project", "Ctrl+Alt+O", ezProjectAction, ezProjectAction::ButtonType::OpenVsCode);



  s_hDocsAndCommunity = EZ_REGISTER_ACTION_1("Editor.DocsAndCommunity", ezActionScope::Global, "Editor", "", ezProjectAction, ezProjectAction::ButtonType::ShowDocsAndCommunity);
}

void ezProjectActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCatProjectGeneral);
  ezActionManager::UnregisterAction(s_hCatProjectAssets);
  ezActionManager::UnregisterAction(s_hCatProjectConfig);
  ezActionManager::UnregisterAction(s_hCatProjectExternal);

  ezActionManager::UnregisterAction(s_hCatFilesGeneral);
  ezActionManager::UnregisterAction(s_hCatFileCommon);
  ezActionManager::UnregisterAction(s_hCatFileSpecial);
  ezActionManager::UnregisterAction(s_hCatAssetDoc);

  ezActionManager::UnregisterAction(s_hCreateDocument);
  ezActionManager::UnregisterAction(s_hOpenDocument);
  ezActionManager::UnregisterAction(s_hRecentDocuments);
  ezActionManager::UnregisterAction(s_hOpenDashboard);
  ezActionManager::UnregisterAction(s_hDocsAndCommunity);
  ezActionManager::UnregisterAction(s_hCreateProject);
  ezActionManager::UnregisterAction(s_hOpenProject);
  ezActionManager::UnregisterAction(s_hRecentProjects);
  ezActionManager::UnregisterAction(s_hCloseProject);
  ezActionManager::UnregisterAction(s_hCatProjectSettings);
  ezActionManager::UnregisterAction(s_hCatPluginSettings);
  ezActionManager::UnregisterAction(s_hCatToolsExternal);
  ezActionManager::UnregisterAction(s_hCatToolsEditor);
  ezActionManager::UnregisterAction(s_hCatToolsDocument);
  ezActionManager::UnregisterAction(s_hCatEditorSettings);
  ezActionManager::UnregisterAction(s_hReloadResources);
  ezActionManager::UnregisterAction(s_hReloadEngine);
  ezActionManager::UnregisterAction(s_hLaunchFileserve);
  ezActionManager::UnregisterAction(s_hLaunchInspector);
  ezActionManager::UnregisterAction(s_hSaveProfiling);
  ezActionManager::UnregisterAction(s_hOpenVsCode);
  ezActionManager::UnregisterAction(s_hShortcutEditor);
  ezActionManager::UnregisterAction(s_hPreferencesDlg);
  ezActionManager::UnregisterAction(s_hTagsConfig);
  ezActionManager::UnregisterAction(s_hDataDirectories);
  ezActionManager::UnregisterAction(s_hWindowConfig);
  ezActionManager::UnregisterAction(s_hImportAsset);
  ezActionManager::UnregisterAction(s_hClearAssetCaches);
  ezActionManager::UnregisterAction(s_hInputConfig);
  ezActionManager::UnregisterAction(s_hAssetProfiles);
  ezActionManager::UnregisterAction(s_hCppProjectMenu);
  ezActionManager::UnregisterAction(s_hSetupCppProject);
  ezActionManager::UnregisterAction(s_hOpenCppProject);
  ezActionManager::UnregisterAction(s_hCompileCppProject);
  ezActionManager::UnregisterAction(s_hExportProject);
  ezActionManager::UnregisterAction(s_hPluginSelection);
}

void ezProjectActions::MapActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  ezStringBuilder sPath;

  // Add categories
  pMap->MapAction(s_hCatProjectGeneral, "G.Project", 1.0f);
  pMap->MapAction(s_hCatProjectConfig, "G.Project", 2.0f);
  pMap->MapAction(s_hCatProjectAssets, "G.Project", 4.0f);
  pMap->MapAction(s_hCatProjectExternal, "G.Project", 5.0f);

  pMap->MapAction(s_hCatToolsExternal, "G.Tools", 1.0f);
  pMap->MapAction(s_hCatToolsEditor, "G.Tools", 2.0f);
  pMap->MapAction(s_hCatToolsDocument, "G.Tools", 3.0f);
  pMap->MapAction(s_hCatEditorSettings, "G.Tools", 1000.0f);

  pMap->MapAction(s_hCatProjectSettings, "G.Project.Config", 1.0f);
  pMap->MapAction(s_hCatPluginSettings, "G.Project.Config", 1.0f);

  if (pMap->SearchPathForAction("G.File", sPath).Succeeded())
  {
    pMap->MapAction(s_hCatFilesGeneral, sPath, 1.0f);
    pMap->MapAction(s_hCatFileCommon, sPath, 2.0f);
    pMap->MapAction(s_hCatAssetDoc, sPath, 3.0f);
    pMap->MapAction(s_hCatFileSpecial, sPath, 4.0f);
  }

  // Add actions
  pMap->MapAction(s_hOpenDashboard, "G.Project.General", 1.0f);
  // pMap->MapAction(s_hCreateProject, "G.Project.General", 2.0f); // use dashboard
  // pMap->MapAction(s_hOpenProject, "G.Project.General", 3.0f);   // use dashboard
  // pMap->MapAction(s_hRecentProjects, "G.Project.General", 4.0f);// use dashboard
  pMap->MapAction(s_hCloseProject, "G.Project.General", 5.0f);

  pMap->MapAction(s_hImportAsset, "G.Project.Assets", 1.0f);
  pMap->MapAction(s_hClearAssetCaches, "G.Project.Assets", 5.0f);

  pMap->MapAction(s_hDataDirectories, "G.Project.Settings", 1.0f);
  pMap->MapAction(s_hInputConfig, "G.Project.Settings", 2.0f);
  pMap->MapAction(s_hWindowConfig, "G.Project.Settings", 3.0f);
  pMap->MapAction(s_hTagsConfig, "G.Project.Settings", 4.0f);
  pMap->MapAction(s_hAssetProfiles, "G.Project.Settings", 5.0f);

  pMap->MapAction(s_hPluginSelection, "G.Plugins.Settings", -1000.0f);

  pMap->MapAction(s_hCppProjectMenu, "G.Project.External", 1.0f);
  pMap->MapAction(s_hSetupCppProject, "G.Project.Cpp", 1.0f);
  pMap->MapAction(s_hOpenCppProject, "G.Project.Cpp", 2.0f);
  pMap->MapAction(s_hCompileCppProject, "G.Project.Cpp", 3.0f);
  pMap->MapAction(s_hExportProject, "G.Project.External", 10.0f);

  pMap->MapAction(s_hOpenVsCode, "G.Tools.External", 1.0f);
  pMap->MapAction(s_hLaunchInspector, "G.Tools.External", 2.0f);
  pMap->MapAction(s_hLaunchFileserve, "G.Tools.External", 3.0f);

  pMap->MapAction(s_hReloadResources, "G.Tools.Editor", 1.0f);
  pMap->MapAction(s_hReloadEngine, "G.Tools.Editor", 2.0f);
  pMap->MapAction(s_hSaveProfiling, "G.Tools.Editor", 3.0f);

  pMap->MapAction(s_hShortcutEditor, "G.Editor.Settings", 1.0f);
  pMap->MapAction(s_hPreferencesDlg, "G.Editor.Settings", 2.0f);

  if (pMap->SearchPathForAction("G.Help", sPath).Succeeded())
  {
    pMap->MapAction(s_hDocsAndCommunity, sPath, 0.0f);
  }

  if (pMap->SearchPathForAction("G.File.Common", sPath).Succeeded())
  {
    pMap->MapAction(s_hCreateDocument, sPath, 1.0f);
    pMap->MapAction(s_hOpenDocument, sPath, 2.0f);
    pMap->MapAction(s_hRecentDocuments, sPath, 3.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// ezRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecentDocumentsMenuAction, 0, ezRTTINoAllocator)
  ;
EZ_END_DYNAMIC_REFLECTED_TYPE;


void ezRecentDocumentsMenuAction::GetEntries(ezHybridArray<ezDynamicMenuAction::Item, 16>& out_entries)
{
  out_entries.Clear();

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

      out_entries.PushBack(item);
    }
    else
    {
      item.m_sDisplay = file.m_File;

      out_entries.PushBack(item);
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
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezRecentProjectsMenuAction::GetEntries(ezHybridArray<ezDynamicMenuAction::Item, 16>& out_entries)
{
  out_entries.Clear();

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

    out_entries.PushBack(item);
  }
}

void ezRecentProjectsMenuAction::Execute(const ezVariant& value)
{
  ezQtEditorApp::GetSingleton()->OpenProject(value.ConvertTo<ezString>()).IgnoreResult();
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
    case ezProjectAction::ButtonType::OpenDashboard:
      SetIconPath(":/GuiFoundation/Icons/Project16.png");
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
    case ezProjectAction::ButtonType::PluginSelection:
      SetIconPath(":/EditorFramework/Icons/Plugins16.png");
      break;
    case ezProjectAction::ButtonType::PreferencesDialog:
      SetIconPath(":/EditorFramework/Icons/StoredSettings16.png");
      break;
    case ezProjectAction::ButtonType::TagsDialog:
      SetIconPath(":/EditorFramework/Icons/Tag16.png");
      break;
    case ezProjectAction::ButtonType::ExportProject:
      // TODO: SetIconPath(":/EditorFramework/Icons/Tag16.png");
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
    case ezProjectAction::ButtonType::SaveProfiling:
      // no icon
      break;
    case ezProjectAction::ButtonType::SetupCppProject:
      SetIconPath(":/EditorFramework/Icons/VisualStudio.svg");
      break;
    case ezProjectAction::ButtonType::OpenCppProject:
      // SetIconPath(":/EditorFramework/Icons/VisualStudio.svg"); // TODO
      break;
    case ezProjectAction::ButtonType::CompileCppProject:
      // SetIconPath(":/EditorFramework/Icons/VisualStudio.svg"); // TODO
      break;
    case ezProjectAction::ButtonType::ShowDocsAndCommunity:
      // SetIconPath(":/GuiFoundation/Icons/Project16.png"); // TODO
      break;
    case ezProjectAction::ButtonType::ClearAssetCaches:
      // SetIconPath(":/GuiFoundation/Icons/Project16.png"); // TODO
      break;
  }

  if (m_ButtonType == ButtonType::CloseProject ||
      m_ButtonType == ButtonType::DataDirectories ||
      m_ButtonType == ButtonType::WindowConfig ||
      m_ButtonType == ButtonType::ImportAsset ||
      m_ButtonType == ButtonType::TagsDialog ||
      m_ButtonType == ButtonType::ReloadEngine ||
      m_ButtonType == ButtonType::ReloadResources ||
      m_ButtonType == ButtonType::LaunchFileserve ||
      m_ButtonType == ButtonType::LaunchInspector ||
      m_ButtonType == ButtonType::OpenVsCode ||
      m_ButtonType == ButtonType::InputConfig ||
      m_ButtonType == ButtonType::AssetProfiles ||
      m_ButtonType == ButtonType::SetupCppProject ||
      m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject ||
      m_ButtonType == ButtonType::ExportProject ||
      m_ButtonType == ButtonType::ClearAssetCaches ||
      m_ButtonType == ButtonType::PluginSelection)
  {
    SetEnabled(ezToolsProject::IsProjectOpen());

    ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezProjectAction::ProjectEventHandler, this));
  }

  if (m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject)
  {
    SetEnabled(ezCppProject::ExistsProjectCMakeListsTxt());

    ezCppProject::s_ChangeEvents.AddEventHandler(ezMakeDelegate(&ezProjectAction::CppEventHandler, this));
  }
}

ezProjectAction::~ezProjectAction()
{
  if (m_ButtonType == ButtonType::CloseProject ||
      m_ButtonType == ButtonType::DataDirectories ||
      m_ButtonType == ButtonType::WindowConfig ||
      m_ButtonType == ButtonType::ImportAsset ||
      m_ButtonType == ButtonType::TagsDialog ||
      m_ButtonType == ButtonType::ReloadEngine ||
      m_ButtonType == ButtonType::ReloadResources ||
      m_ButtonType == ButtonType::LaunchFileserve ||
      m_ButtonType == ButtonType::LaunchInspector ||
      m_ButtonType == ButtonType::OpenVsCode ||
      m_ButtonType == ButtonType::InputConfig ||
      m_ButtonType == ButtonType::AssetProfiles ||
      m_ButtonType == ButtonType::SetupCppProject ||
      m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject ||
      m_ButtonType == ButtonType::ExportProject ||
      m_ButtonType == ButtonType::ClearAssetCaches ||
      m_ButtonType == ButtonType::PluginSelection)
  {
    ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezProjectAction::ProjectEventHandler, this));
  }

  if (m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject)
  {
    ezCppProject::s_ChangeEvents.RemoveEventHandler(ezMakeDelegate(&ezProjectAction::CppEventHandler, this));
  }
}

void ezProjectAction::ProjectEventHandler(const ezToolsProjectEvent& e)
{
  if (m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject)
  {
    SetEnabled(ezCppProject::ExistsProjectCMakeListsTxt());
  }
  else
  {
    SetEnabled(ezToolsProject::IsProjectOpen());
  }
}

void ezProjectAction::CppEventHandler(const ezCppSettings& e)
{
  SetEnabled(ezCppProject::ExistsProjectCMakeListsTxt());
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

    case ezProjectAction::ButtonType::OpenDashboard:
      ezQtEditorApp::GetSingleton()->GuiOpenDashboard();
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

    case ezProjectAction::ButtonType::PluginSelection:
    {
      ezQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(ezOSFile::GetApplicationDirectory());

      ezCppSettings cppSettings;
      if (cppSettings.Load().Succeeded())
      {
        ezQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(ezCppProject::GetPluginSourceDir(cppSettings));
      }

      ezQtPluginSelectionDlg dlg(&ezQtEditorApp::GetSingleton()->GetPluginBundles());
      dlg.exec();

      ezToolsProject::SaveProjectState();
    }
    break;

    case ezProjectAction::ButtonType::PreferencesDialog:
    {
      ezQtPreferencesDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        // save modified preferences right away
        ezToolsProject::SaveProjectState();

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

    case ezProjectAction::ButtonType::ExportProject:
    {
      ezQtExportProjectDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case ezProjectAction::ButtonType::ClearAssetCaches:
    {
      auto res = ezQtUiServices::GetSingleton()->MessageBoxQuestion("Delete ALL cached asset files?\n\n* 'Yes All' deletes everything and takes a long time to re-process. This is rarely needed.\n* 'No All' only deletes assets that are likely to make problems.", QMessageBox::StandardButton::YesAll | QMessageBox::StandardButton::NoAll | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Cancel);

      if (res == QMessageBox::StandardButton::Cancel)
        break;

      if (res == QMessageBox::StandardButton::YesAll)
        ezAssetCurator::GetSingleton()->ClearAssetCaches(ezAssetDocumentManager::Perfect);
      else
        ezAssetCurator::GetSingleton()->ClearAssetCaches(ezAssetDocumentManager::Unknown);
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
      ezQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Reloading Resources...", ezTime::Seconds(5));

      ezSimpleConfigMsgToEngine msg;
      msg.m_sWhatToDo = "ReloadResources";
      msg.m_sPayload = "ReloadAllResources";
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
      ezQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Launching FileServe...", ezTime::Seconds(5));

      ezQtLaunchFileserveDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case ezProjectAction::ButtonType::LaunchInspector:
    {
      ezQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Launching ezInspector...", ezTime::Seconds(5));

      ezQtEditorApp::GetSingleton()->RunInspector();
    }
    break;

    case ezProjectAction::ButtonType::ReloadEngine:
    {
      ezEditorEngineProcessConnection::GetSingleton()->RestartProcess().IgnoreResult();
    }
    break;

    case ezProjectAction::ButtonType::SaveProfiling:
    {
      const char* szEditorProfilingFile = ":appdata/profilingEditor.json";
      {
        // Start capturing profiling data on engine process
        ezSimpleConfigMsgToEngine msg;
        msg.m_sWhatToDo = "SaveProfiling";
        msg.m_sPayload = ":appdata/profilingEngine.json";
        ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
      }
      {
        // Capture profiling data on editor process
        ezFileWriter fileWriter;
        if (fileWriter.Open(szEditorProfilingFile) == EZ_SUCCESS)
        {
          ezProfilingSystem::ProfilingData profilingData;
          ezProfilingSystem::Capture(profilingData);
          // Set sort index to -1 so that the editor is always on top when opening the trace.
          profilingData.m_uiProcessSortIndex = -1;
          if (profilingData.Write(fileWriter).Failed())
          {
            ezLog::Error("Failed to write editor profiling capture: {}.", szEditorProfilingFile);
            return;
          }

          ezLog::Info("Editor profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
        }
        else
        {
          ezLog::Error("Could not write profiling capture to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
        }
      }
      ezStringBuilder sEngineProfilingFile;
      {
        // Wait for engine process response
        auto callback = [&](ezProcessMessage* pMsg) -> bool
        {
          auto pSimpleCfg = static_cast<ezSaveProfilingResponseToEditor*>(pMsg);
          sEngineProfilingFile = pSimpleCfg->m_sProfilingFile;
          return true;
        };
        ezProcessCommunicationChannel::WaitForMessageCallback cb = callback;

        if (ezEditorEngineProcessConnection::GetSingleton()->WaitForMessage(ezGetStaticRTTI<ezSaveProfilingResponseToEditor>(), ezTime::Seconds(15), &cb).Failed())
        {
          ezLog::Error("Timeout while waiting for engine process to create profiling capture. Captures will not be merged.");
          return;
        }
        if (sEngineProfilingFile.IsEmpty())
        {
          ezLog::Error("Engine process failed to create profiling file.");
          return;
        }
      }

      // Merge editor and engine profiling files by simply merging the arrays inside
      {
        ezString sEngineProfilingJson;
        {
          ezFileReader reader;
          if (reader.Open(sEngineProfilingFile).Failed())
          {
            ezLog::Error("Failed to read engine profiling capture: {}.", sEngineProfilingFile);
            return;
          }
          sEngineProfilingJson.ReadAll(reader);
        }
        ezString sEditorProfilingJson;
        {
          ezFileReader reader;
          if (reader.Open(szEditorProfilingFile).Failed())
          {
            ezLog::Error("Failed to read editor profiling capture: {}.", sEngineProfilingFile);
            return;
          }
          sEditorProfilingJson.ReadAll(reader);
        }

        ezStringBuilder sMergedProfilingJson;
        {
          // Just glue the array together
          sMergedProfilingJson.Reserve(sEngineProfilingJson.GetElementCount() + 1 + sEditorProfilingJson.GetElementCount());
          const char* szEndArray = sEngineProfilingJson.FindLastSubString("]");
          sMergedProfilingJson.Append(ezStringView(sEngineProfilingJson.GetData(), szEndArray - sEngineProfilingJson.GetData()));
          sMergedProfilingJson.Append(",");
          const char* szStartArray = sEditorProfilingJson.FindSubString("[") + 1;
          sMergedProfilingJson.Append(ezStringView(szStartArray, sEditorProfilingJson.GetElementCount() - (szStartArray - sEditorProfilingJson.GetData())));
        }
        ezStringBuilder sMergedFile;
        const ezDateTime dt = ezTimestamp::CurrentTimestamp();
        sMergedFile.AppendFormat(":appdata/profiling_{0}-{1}-{2}_{3}-{4}-{5}-{6}.json", dt.GetYear(), ezArgU(dt.GetMonth(), 2, true), ezArgU(dt.GetDay(), 2, true), ezArgU(dt.GetHour(), 2, true), ezArgU(dt.GetMinute(), 2, true), ezArgU(dt.GetSecond(), 2, true), ezArgU(dt.GetMicroseconds() / 1000, 3, true));
        ezFileWriter fileWriter;
        if (fileWriter.Open(sMergedFile).Failed() || fileWriter.WriteBytes(sMergedProfilingJson.GetData(), sMergedProfilingJson.GetElementCount()).Failed())
        {
          ezLog::Error("Failed to write merged profiling capture: {}.", sMergedFile);
          return;
        }
        ezLog::Info("Merged profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
        ezQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(ezFmt("Merged profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData()), ezTime::Seconds(5.0));
      }
    }
    break;

    case ezProjectAction::ButtonType::OpenVsCode:
    {
      QStringList args;

      for (const auto& dd : ezQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
      {
        ezStringBuilder path;
        ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, path).IgnoreResult();

        args.append(QString::fromUtf8(path, path.GetElementCount()));
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

    case ezProjectAction::ButtonType::SetupCppProject:
    {
      ezQtCppProjectDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case ezProjectAction::ButtonType::OpenCppProject:
    {
      ezCppSettings cpp;
      cpp.Load().IgnoreResult();

      if (ezCppProject::ExistsProjectCMakeListsTxt())
      {
        if (ezCppProject::RunCMakeIfNecessary(cpp).Failed())
        {
          ezQtUiServices::GetSingleton()->MessageBoxWarning("Generating the C++ solution failed.");
        }
        else if (!ezQtUiServices::OpenFileInDefaultProgram(ezCppProject::GetSolutionPath(cpp)))
        {
          ezQtUiServices::GetSingleton()->MessageBoxWarning("Opening the solution failed.");
        }
      }
      else
      {
        ezQtUiServices::GetSingleton()->MessageBoxInformation("C++ code has not been set up, opening a solution is not possible.");
      }
    }
    break;

    case ezProjectAction::ButtonType::CompileCppProject:
    {
      ezCppSettings cpp;
      cpp.Load().IgnoreResult();

      if (ezCppProject::ExistsProjectCMakeListsTxt())
      {
        if (ezCppProject::BuildCodeIfNecessary(cpp).Succeeded())
        {
          ezQtUiServices::GetSingleton()->MessageBoxInformation("Successfully compiled the C++ code.");
        }
        else
        {
          ezQtUiServices::GetSingleton()->MessageBoxWarning("Compiling the code failed. See log for details.");
        }
      }
      else
      {
        ezQtUiServices::GetSingleton()->MessageBoxInformation("C++ code has not been set up, compilation is not necessary.");
      }
    }
    break;

    case ezProjectAction::ButtonType::ShowDocsAndCommunity:
      ezQtEditorApp::GetSingleton()->GuiOpenDocsAndCommunity();
      break;
  }
}
