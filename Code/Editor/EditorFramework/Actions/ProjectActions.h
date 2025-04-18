#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class ezCppSettings;

///
class EZ_EDITORFRAMEWORK_DLL ezProjectActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(ezStringView sMapping, const ezBitflags<ezStandardMenuTypes> menus = ezStandardMenuTypes::Default);

  static ezActionDescriptorHandle s_hCatProjectGeneral;
  static ezActionDescriptorHandle s_hCatProjectAssets;
  static ezActionDescriptorHandle s_hCatProjectConfig;
  static ezActionDescriptorHandle s_hCatProjectExternal;

  static ezActionDescriptorHandle s_hCatFilesGeneral;
  static ezActionDescriptorHandle s_hCatFileCommon;
  static ezActionDescriptorHandle s_hCatFileSpecial;

  static ezActionDescriptorHandle s_hCreateDocument;
  static ezActionDescriptorHandle s_hOpenDocument;
  static ezActionDescriptorHandle s_hRecentDocuments;

  static ezActionDescriptorHandle s_hOpenDashboard;
  static ezActionDescriptorHandle s_hCreateProject;
  static ezActionDescriptorHandle s_hOpenProject;
  static ezActionDescriptorHandle s_hRecentProjects;
  static ezActionDescriptorHandle s_hCloseProject;

  static ezActionDescriptorHandle s_hDocsAndCommunity;

  static ezActionDescriptorHandle s_hCatProjectSettings;
  static ezActionDescriptorHandle s_hCatPluginSettings;
  static ezActionDescriptorHandle s_hShortcutEditor;
  static ezActionDescriptorHandle s_hDataDirectories;
  static ezActionDescriptorHandle s_hWindowConfig;
  static ezActionDescriptorHandle s_hInputConfig;
  static ezActionDescriptorHandle s_hPreferencesDlg;
  static ezActionDescriptorHandle s_hTagsConfig;
  static ezActionDescriptorHandle s_hAssetProfiles;
  static ezActionDescriptorHandle s_hExportProject;
  static ezActionDescriptorHandle s_hPluginSelection;

  static ezActionDescriptorHandle s_hCatToolsExternal;
  static ezActionDescriptorHandle s_hCatToolsEditor;
  static ezActionDescriptorHandle s_hCatToolsDocument;
  static ezActionDescriptorHandle s_hCatEditorSettings;
  static ezActionDescriptorHandle s_hReloadResources;
  static ezActionDescriptorHandle s_hReloadEngine;
  static ezActionDescriptorHandle s_hLaunchFileserve;
  static ezActionDescriptorHandle s_hLaunchInspector;
  static ezActionDescriptorHandle s_hLaunchTracy;
  static ezActionDescriptorHandle s_hSaveProfiling;
  static ezActionDescriptorHandle s_hOpenVsCode;
  static ezActionDescriptorHandle s_hImportAsset;
  static ezActionDescriptorHandle s_hClearAssetCaches;

  static ezActionDescriptorHandle s_hCppProjectMenu;
  static ezActionDescriptorHandle s_hSetupCppProject;
  static ezActionDescriptorHandle s_hOpenCppProject;
  static ezActionDescriptorHandle s_hCompileCppProject;
  static ezActionDescriptorHandle s_hRegenerateCppSolution;
};

///
class EZ_EDITORFRAMEWORK_DLL ezRecentDocumentsMenuAction : public ezDynamicMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRecentDocumentsMenuAction, ezDynamicMenuAction);

public:
  ezRecentDocumentsMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath)
    : ezDynamicMenuAction(context, szName, szIconPath)
  {
  }
  virtual void GetEntries(ezDynamicArray<Item>& out_entries) override;
  virtual void Execute(const ezVariant& value) override;
};

///
class EZ_EDITORFRAMEWORK_DLL ezRecentProjectsMenuAction : public ezDynamicMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRecentProjectsMenuAction, ezDynamicMenuAction);

public:
  ezRecentProjectsMenuAction(const ezActionContext& context, const char* szName, const char* szIconPath)
    : ezDynamicMenuAction(context, szName, szIconPath)
  {
  }
  virtual void GetEntries(ezDynamicArray<Item>& out_entries) override;
  virtual void Execute(const ezVariant& value) override;
};

///
class EZ_EDITORFRAMEWORK_DLL ezProjectAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProjectAction, ezButtonAction);

public:
  enum class ButtonType
  {
    CreateDocument,
    OpenDocument,
    OpenDashboard,
    CreateProject,
    OpenProject,
    CloseProject,
    ReloadResources,
    ReloadEngine,
    LaunchFileserve,
    LaunchInspector,
    LaunchTracy,
    SaveProfiling,
    OpenVsCode,
    Shortcuts,
    DataDirectories,
    WindowConfig,
    InputConfig,
    PreferencesDialog,
    TagsDialog,
    ImportAsset,
    AssetProfiles,
    SetupCppProject,
    OpenCppProject,
    CompileCppProject,
    RegenerateCppSolution,
    ShowDocsAndCommunity,
    ExportProject,
    PluginSelection,
    ClearAssetCaches,
  };

  ezProjectAction(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezProjectAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void ProjectEventHandler(const ezToolsProjectEvent& e);
  void CppEventHandler(const ezCppSettings& e);

  ButtonType m_ButtonType;
};
