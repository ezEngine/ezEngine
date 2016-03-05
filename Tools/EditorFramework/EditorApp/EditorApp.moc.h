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

/// \brief Holds information about a plugin. Used for editor and engine plugins, where the user can configure whether to load them or not.
struct EZ_EDITORFRAMEWORK_DLL ezPluginInfo
{
  ezPluginInfo()
  {
    m_bAvailable = false;
    m_bActive = false;
    m_bToBeLoaded = false;
  }

  bool m_bAvailable; // exists on disk
  bool m_bActive; // currently loaded into the process
  bool m_bToBeLoaded; // supposed to be loaded into the process next restart

  bool operator==(const ezPluginInfo& rhs) const { return m_bAvailable == rhs.m_bAvailable && m_bActive == rhs.m_bActive && m_bToBeLoaded == rhs.m_bToBeLoaded; }
  bool operator!=(const ezPluginInfo& rhs) const { return !(*this == rhs); }
};

/// \brief Describes a set of plugins. Name is the plugin file name used to load it through ezPlugin, ezPluginInfo contains details about the loading state.
struct EZ_EDITORFRAMEWORK_DLL ezPluginSet
{
  ezMap<ezString, ezPluginInfo> m_Plugins;

  bool operator==(const ezPluginSet& rhs) const { return m_Plugins == rhs.m_Plugins; }
  bool operator!=(const ezPluginSet& rhs) const { return !(*this == rhs); }
};

/// \brief Maintains a list of recently used files.
class EZ_EDITORFRAMEWORK_DLL ezRecentFilesList
{
public:
  ezRecentFilesList(ezUInt32 uiMaxElements) { m_uiMaxElements = uiMaxElements; }

  /// \brief Moves the inserted file to the front.
  void Insert(const char* szFile);

  /// \brief Returns all files in the list.
  const ezDeque<ezString>& GetFileList() const { return m_Files; }

  /// \brief Clears the list
  void Clear() { m_Files.Clear(); }

  /// \brief Saves the recent files list to the given file. Uses a simple text file format (one line per item).
  void Save(const char* szFile);

  /// \brief Loads the recent files list from the given file. Uses a simple text file format (one line per item).
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
  ezString GetEditorDataFolder();

  const ezPluginSet& GetEditorPlugins() const { return s_EditorPlugins; }
  const ezPluginSet& GetEnginePlugins() const { return s_EnginePlugins; }

  ezPluginSet& GetEditorPlugins() { return s_EditorPlugins; }
  ezPluginSet& GetEnginePlugins() { return s_EnginePlugins; }

  void StoreEditorPluginsToBeLoaded();
  void StoreEnginePluginsToBeLoaded();

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

  void LoadEditorPlugins();
  void UnloadEditorPlugins();

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
  ezDocument* OpenDocumentImmediate(const char* szDocument, bool bRequestWindow = true, bool bAddToRecentFilesList = true);
  

  const ezApplicationFileSystemConfig& GetFileSystemConfig() const { return m_FileSystemConfig; }
  const ezApplicationPluginConfig& GetEnginePluginConfig() const { return m_EnginePluginConfig; }

  void SetFileSystemConfig(const ezApplicationFileSystemConfig& cfg);

  bool MakeDataDirectoryRelativePathAbsolute(ezString& sPath) const;
  bool MakePathDataDirectoryRelative(ezString& sPath) const;

  void AddRuntimePluginDependency(const char* szEditorPluginName, const char* szRuntimeDependency);

  ezStatus SaveTagRegistry();

  /// \brief Reads the known input slots from disk and adds them to the existing list.
  ///
  /// All input slots to be exposed by the editor are stored in 'Shared/Tools/ezEditor/InputSlots'
  /// as txt files. Each line names one input slot.
  void GetKnownInputSlots(ezDynamicArray<ezString>& slots) const;

private:
  ezString BuildDocumentTypeFileFilter(bool bForCreation);
  
  void GuiCreateOrOpenDocument(bool bCreate);
  void GuiCreateOrOpenProject(bool bCreate);

  ezDocument* CreateOrOpenDocument(bool bCreate, const char* szFile, bool bRequestWindow = true, bool bAddToRecentFilesList = true);
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
  void DocumentEventHandler(const ezDocumentEvent& e);
  void DocumentWindowEventHandler(const ezQtDocumentWindow::Event& e);
  void ProjectRequestHandler(ezToolsProject::Request& r);
  void ProjectEventHandler(const ezToolsProject::Event& r);
  void EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e);

  void DetectAvailableEditorPlugins();
  void DetectAvailableEnginePlugins();
  void ReadEditorPluginsToBeLoaded();
  void ReadEnginePluginConfig();
  void ReadTagRegistry();

  void SetupDataDirectories();
  void CreatePanels();

  ezString s_sUserName;

  ezSet<ezString> s_RestartRequiredReasons;
  ezSet<ezString> s_ReloadProjectRequiredReasons;

  ezPluginSet s_EditorPlugins;
  ezPluginSet s_EnginePlugins;

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

  ezMap<ezString, ezSet<ezString> > m_AdditionalRuntimePluginDependencies;

  // *** Recent Paths ***
  ezString m_sLastDocumentFolder;
  ezString m_sLastProjectFolder;
};