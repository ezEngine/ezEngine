#pragma once

#include <EditorFramework/Plugin.h>
#include <ToolsFoundation/Settings/Settings.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Communication/Event.h>
#include <QApplication>
#include <Foundation/Logging/HTMLWriter.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Core/Application/Config/FileSystemConfig.h>
#include <Core/Application/Config/PluginConfig.h>

class QMainWindow;
class QWidget;

struct EZ_EDITORFRAMEWORK_DLL ezPluginSet
{
  ezSet<ezString> m_Plugins;

  bool operator==(const ezPluginSet& rhs) const { return m_Plugins == rhs.m_Plugins; }
  bool operator!=(const ezPluginSet& rhs) const { return !(*this == rhs); }
};

class EZ_EDITORFRAMEWORK_DLL ezRecentFilesList
{
public:
  ezRecentFilesList(ezUInt32 uiMaxElements) { m_uiMaxElements = uiMaxElements; }

  void Insert(const char* szFile);
  const ezDeque<ezString>& GetFileList() const { return m_Files; }
  void Clear() { m_Files.Clear(); }

  void Save(const char* szFile);
  void Load(const char* szFile);

private:
  ezUInt32 m_uiMaxElements;
  ezDeque<ezString> m_Files;
};

class EZ_EDITORFRAMEWORK_DLL ezQtEditorApp : public QObject
{
  Q_OBJECT

  static ezQtEditorApp* s_pInstance;

public:
  ezQtEditorApp();
  ~ezQtEditorApp();

  static ezQtEditorApp* GetInstance() { return s_pInstance; }

  const ezString& GetApplicationUserName() { return s_sUserName; }

  const ezPluginSet& GetEditorPluginsAvailable();
  const ezPluginSet& GetEditorPluginsActive() { return s_EditorPluginsActive; }
  const ezPluginSet& GetEditorPluginsToBeLoaded() { return s_EditorPluginsToBeLoaded; }
  void SetEditorPluginsToBeLoaded(const ezPluginSet& plugins);

  void AddRestartRequiredReason(const char* szReason);
  const ezSet<ezString>& GetRestartRequiredReasons() { return s_RestartRequiredReasons; }

  void AddReloadProjectRequiredReason(const char* szReason);
  const ezSet<ezString>& GetReloadProjectRequiredReason() { return s_ReloadProjectRequiredReasons; }

  void RegisterPluginNameForSettings(const char* szPluginName);
  const ezSet<ezString>& GetRegisteredPluginNamesForSettings() { return s_SettingsPluginNames; }
  ezSettings& GetEditorSettings(const char* szPluginName = "-Main-");
  ezSettings& GetProjectSettings(const char* szPluginName = "-Main-");
  ezSettings& GetDocumentSettings(const ezDocument* pDocument, const char* szPluginName = "-Main-");
  ezSettings& GetDocumentSettings(const char* szDocument, const char* szPlugin = "-Main-");
  void SaveSettings();

  void InitQt(int argc, char** argv);
  void StartupEditor(const char* szAppName, const char* szUserName);
  void ShutdownEditor();
  ezInt32 RunEditor();
  void DeInitQt();

  void LoadPlugins();
  void UnloadPlugins();

  ezRecentFilesList& GetRecentProjectsList()   { return s_RecentProjects;  }
  ezRecentFilesList& GetRecentDocumentsList()  { return s_RecentDocuments; }

  ezString GetDocumentDataFolder(const char* szDocument);

  ezEditorEngineProcessConnection* GetEngineViewProcess() { return s_pEngineViewProcess; }

  void ShowSettingsDocument();
  void CloseSettingsDocument();

  void CloseProject();
  void OpenProject(const char* szProject);

  void GuiCreateDocument();
  void GuiOpenDocument();

  void GuiCreateProject();
  void GuiOpenProject();

  void OpenDocument(const char* szDocument);
  ezDocument* OpenDocumentImmediate(const char* szDocument, bool bRequestWindow = true);
  

  const ezApplicationFileSystemConfig& GetFileSystemConfig() const { return m_FileSystemConfig; }
  const ezApplicationPluginConfig& GetEnginePluginConfig() const { return m_EnginePluginConfig; }

  void SetFileSystemConfig(const ezApplicationFileSystemConfig& cfg);
  void SetEnginePluginConfig(const ezApplicationPluginConfig& cfg);

  bool MakeDataDirectoryRelativePathAbsolute(ezString& sPath) const;
  bool MakePathDataDirectoryRelative(ezString& sPath) const;

private:
  ezString BuildDocumentTypeFileFilter(bool bForCreation);
  
  void GuiCreateOrOpenDocument(bool bCreate);
  void GuiCreateOrOpenProject(bool bCreate);

  ezDocument* CreateOrOpenDocument(bool bCreate, const char* szFile, bool bRequestWindow = true);
  void CreateOrOpenProject(bool bCreate, const char* szFile);

private slots:
  void SlotTimedUpdate();
  void SlotQueuedCloseProject();
  void SlotQueuedOpenProject(QString sProject);
  void SlotQueuedOpenDocument(QString sProject);
  void SlotQueuedCreateOrOpenProject(bool bCreate);

private:
  ezSettings& GetSettings(ezMap<ezString, ezSettings>& SettingsMap, const char* szPlugin, const char* szSearchPath);

  void UpdateGlobalStatusBarMessage();

  void DocumentManagerRequestHandler(ezDocumentManager::Request& r);
  void DocumentManagerEventHandler(const ezDocumentManager::Event& r);
  void DocumentEventHandler(const ezDocument::Event& e);
  void DocumentWindowEventHandler(const ezQtDocumentWindow::Event& e);
  void ProjectRequestHandler(ezToolsProject::Request& r);
  void ProjectEventHandler(const ezToolsProject::Event& r);
  void EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e);

  void ReadPluginsToBeLoaded();
  void ReadEnginePluginConfig();

  void SetupDataDirectories();
  void CreatePanels();

  ezString s_sUserName;

  ezSet<ezString> s_RestartRequiredReasons;
  ezSet<ezString> s_ReloadProjectRequiredReasons;

  ezPluginSet s_EditorPluginsAvailable;
  ezPluginSet s_EditorPluginsActive;
  ezPluginSet s_EditorPluginsToBeLoaded;

  ezSet<ezString> s_SettingsPluginNames;
  ezMap<ezString, ezSettings> s_EditorSettings;
  ezMap<ezString, ezSettings> s_ProjectSettings;
  ezMap<ezString, ezMap<ezString, ezSettings> > s_DocumentSettings;

  void StoreSettings(const ezMap<ezString, ezSettings>& settings, const char* szFolder);
  void SaveDocumentSettings(const ezDocument* pDocument);

  void SaveRecentFiles();
  void LoadRecentFiles();

  ezRecentFilesList s_RecentProjects;
  ezRecentFilesList s_RecentDocuments;

  QApplication* s_pQtApplication;
  ezEditorEngineProcessConnection* s_pEngineViewProcess;
  QTimer* m_pTimer;

  ezLogWriter::HTML m_LogHTML;

  ezAssetCurator m_AssetCurator;

  ezApplicationFileSystemConfig m_FileSystemConfig;
  ezApplicationPluginConfig m_EnginePluginConfig;

  // *** Recent Paths ***
  ezString m_sLastDocumentFolder;
  ezString m_sLastProjectFolder;
};