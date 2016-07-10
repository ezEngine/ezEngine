#pragma once

#include <EditorFramework/Plugin.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Basics/RecentFilesList.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Communication/Event.h>
#include <QApplication>
#include <Foundation/Logging/HTMLWriter.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Core/Application/Config/FileSystemConfig.h>
#include <Core/Application/Config/PluginConfig.h>
#include <Foundation/Types/UniquePtr.h>
#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <Foundation/Configuration/Singleton.h>

class QMainWindow;
class QWidget;
class ezProgress;
class ezQtProgressbar;
class ezQtEditorApp;

struct EZ_EDITORFRAMEWORK_DLL ezEditorAppEvent
{
  enum class Type
  {
    BeforeApplyDataDirectories, ///< Sent after data directory config was loaded, but before it is applied. Allows to add custom dependencies at the right moment.
  };

  Type m_Type;
  ezQtEditorApp* m_pSender;
};

class EZ_EDITORFRAMEWORK_DLL ezQtEditorApp : public QObject
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtEditorApp);

public:
  ezQtEditorApp();
  ~ezQtEditorApp();

  ezEvent<const ezEditorAppEvent&> m_Events;

  /// \brief Returns the folder in which the tools binaries can be found. If enabled in the preferences, it uses the pre-compiled tools, otherwise the currently compiled ones.
  ezString GetExternalToolsFolder();

  /// \brief Can be set via the command line option '-safe'. In this mode the editor will not automatically load recent documents
  bool IsInSafeMode() const { return m_bSafeMode; }

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

  void SaveSettings();

  /// \brief Writes a file containing all the currently open documents
  void SaveOpenDocumentsList();

  /// \brief Reads the list of last open documents in the current project.
  ezRecentFilesList LoadOpenDocumentsList();

  void InitQt(int argc, char** argv);
  void StartupEditor();
  void ShutdownEditor();
  ezInt32 RunEditor();
  void DeInitQt();

  void LoadEditorPlugins();
  void UnloadEditorPlugins();

  ezRecentFilesList& GetRecentProjectsList()   { return s_RecentProjects;  }
  ezRecentFilesList& GetRecentDocumentsList()  { return s_RecentDocuments; }

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

  /// \brief Starts at szStartDirectory and goes up until it finds a folder that contains the given sub folder structure.
  /// Returns an empty string if nothing is found. Otherwise the returned path concatenated with szSubPath will be a valid, existing path.
  ezString FindFolderWithSubPath(const char* szStartDirectory, const char* szSubPath) const;
  
  /// \brief Adds a data directory as a hard dependency to the project. Should be used by plugins to ensure their required data is available.
  /// The path must be relative to the PROJECT folder.
  void AddPluginDataDirDependency(const char* szRelativePath, const char* szRootName = nullptr);

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
  void SlotQueuedGuiCreateOrOpenProject(bool bCreate);

private:
  void UpdateGlobalStatusBarMessage();

  void DocumentManagerRequestHandler(ezDocumentManager::Request& r);
  void DocumentManagerEventHandler(const ezDocumentManager::Event& r);
  void DocumentEventHandler(const ezDocumentEvent& e);
  void DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e);
  void ProjectRequestHandler(ezToolsProjectRequest& r);
  void ProjectEventHandler(const ezToolsProjectEvent& r);
  void EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e);

  void LoadEditorPreferences();
  void LoadProjectPreferences();
  void DetectAvailableEditorPlugins();
  void DetectAvailableEnginePlugins();
  void ReadEditorPluginsToBeLoaded();
  void ReadEnginePluginConfig();
  void ReadTagRegistry();

  void SetupDataDirectories();
  void SetStyleSheet();
  void CreatePanels();

  bool m_bSafeMode;

  ezSet<ezString> s_RestartRequiredReasons;
  ezSet<ezString> s_ReloadProjectRequiredReasons;

  ezPluginSet s_EditorPlugins;
  ezPluginSet s_EnginePlugins;

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

  // *** Progress Bar ***
  ezProgress* m_pProgressbar;
  ezQtProgressbar* m_pQtProgressbar;
};