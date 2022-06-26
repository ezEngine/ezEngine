#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorFramework/EditorApp/CheckVersion.moc.h>
#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <QApplication>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Utilities/RecentFilesList.h>

class QMainWindow;
class QWidget;
class ezProgress;
class ezQtProgressbar;
class ezQtEditorApp;
class QStringList;
class ezTranslatorFromFiles;
class ezDynamicStringEnum;
class QSplashScreen;

struct EZ_EDITORFRAMEWORK_DLL ezEditorAppEvent
{
  enum class Type
  {
    BeforeApplyDataDirectories, ///< Sent after data directory config was loaded, but before it is applied. Allows to add custom
                                ///< dependencies at the right moment.
    ReloadResources,            ///< Sent when 'ReloadResources' has been triggered (and a message was sent to the engine)
    EditorStarted,              ///< Editor has finished all initialization code and will now load the recent project.
  };

  Type m_Type;
};

class EZ_EDITORFRAMEWORK_DLL ezQtEditorApp : public QObject
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtEditorApp);

public:
  struct StartupFlags
  {
    typedef ezUInt8 StorageType;
    enum Enum
    {
      Headless = EZ_BIT(0), ///< The app does not do any rendering.
      SafeMode = EZ_BIT(1), ///< '-safe' : Prevent automatic loading of projects, scenes, etc. to minimize risk of crashing.
      NoRecent = EZ_BIT(2), ///< '-norecent' : Do not modify recent file lists. Used for modes such as tests, where the user does not do any interactions.
      Debug = EZ_BIT(3),    ///< '-debug' : Tell the engine process to wait for a debugger to attach.
      UnitTest = EZ_BIT(4), ///< Specified when the process is running as a unit test
      Default = 0,
    };

    struct Bits
    {
      StorageType Headless : 1;
      StorageType SafeMode : 1;
      StorageType NoRecent : 1;
      StorageType Debug : 1;
    };
  };

public:
  ezQtEditorApp();
  ~ezQtEditorApp();

  static ezEvent<const ezEditorAppEvent&> m_Events;

  //
  // External Tools
  //

  /// \brief Returns the folder in which the tools binaries can be found. If enabled in the preferences, it uses the pre-compiled tools,
  /// otherwise the currently compiled ones. If bForceUseCustomTools is true, it always returns the folder in which custom compiled tools
  /// are stored (app binary dir)
  ezString GetExternalToolsFolder(bool bForceUseCustomTools = false);

  /// \brief Searches for an external tool by calling GetExternalToolsFolder(). Falls back to the currently compiled tools, if a tool cannot
  /// be found in the precompiled folder.
  ezString FindToolApplication(const char* szToolName);

  /// \brief Executes an external tool as found by FindToolApplication().
  ///
  /// The applications output is parsed and forwarded to the given log interface. A custom log level is applied first.
  /// If the tool cannot be found or it takes longer to execute than the allowed timeout, the function returns failure.
  ezStatus ExecuteTool(const char* szTool, const QStringList& arguments, ezUInt32 uiSecondsTillTimeout, ezLogInterface* pLogOutput = nullptr, ezLogMsgType::Enum LogLevel = ezLogMsgType::WarningMsg, const char* szCWD = nullptr);

  /// \brief Creates the string with which to run Fileserve for the currently open project.
  ezString BuildFileserveCommandLine() const;

  /// \brief Launches Fileserve with the settings for the current project.
  void RunFileserve();

  /// \brief Launches ezInspector.
  void RunInspector();

  //
  //
  //

  /// \brief Can be set via the command line option '-safe'. In this mode the editor will not automatically load recent documents
  bool IsInSafeMode() const { return m_StartupFlags.IsSet(StartupFlags::SafeMode); }

  /// \brief Returns true if the the app shouldn't display anything. This is the case in an EditorProcessor.
  bool IsInHeadlessMode() const { return m_StartupFlags.IsSet(StartupFlags::Headless); }

  /// \brief Returns true if the editor is started is run in test mode.
  bool IsInUnitTestMode() const { return m_StartupFlags.IsSet(StartupFlags::UnitTest); }

  const ezPluginBundleSet& GetPluginBundles() const { return m_PluginBundles; }
  ezPluginBundleSet& GetPluginBundles() { return m_PluginBundles; }

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
  void StartupEditor(ezBitflags<StartupFlags> startupFlags, const char* szUserDataFolder = nullptr);
  void ShutdownEditor();
  ezInt32 RunEditor();
  void DeInitQt();

  void LoadEditorPlugins();

  ezRecentFilesList& GetRecentProjectsList() { return s_RecentProjects; }
  ezRecentFilesList& GetRecentDocumentsList() { return s_RecentDocuments; }

  ezEditorEngineProcessConnection* GetEngineViewProcess() { return s_pEngineViewProcess; }

  void ShowSettingsDocument();
  void CloseSettingsDocument();

  void CloseProject();
  ezResult OpenProject(const char* szProject, bool bImmediate = false);

  void GuiCreateDocument();
  void GuiOpenDocument();

  void GuiOpenDashboard();
  void GuiOpenDocsAndCommunity();
  bool GuiCreateProject(bool bImmediate = false);
  bool GuiOpenProject(bool bImmediate = false);

  void OpenDocumentQueued(const char* szDocument, const ezDocumentObject* pOpenContext = nullptr);
  ezDocument* OpenDocument(const char* szDocument, ezBitflags<ezDocumentFlags> flags, const ezDocumentObject* pOpenContext = nullptr);
  ezDocument* CreateDocument(const char* szDocument, ezBitflags<ezDocumentFlags> flags, const ezDocumentObject* pOpenContext = nullptr);

  ezResult CreateOrOpenProject(bool bCreate, const char* szFile);

  void WritePluginSelectionStateDDL(const char* szProjectDir = ":project");
  void CreatePluginSelectionDDL(const char* szProjectFile, const char* szTemplate);
  void LoadPluginBundleDlls(const char* szProjectFile);

  /// \brief Launches a new instance of the editor to open the given project.
  void LaunchEditor(const char* szProject, bool bCreate);

  /// \brief Adds a data directory as a hard dependency to the project. Should be used by plugins to ensure their required data is
  /// available. The path must be relative to the SdkRoot folder.
  void AddPluginDataDirDependency(const char* szSdkRootRelativePath, const char* szRootName = nullptr, bool bWriteable = false);

  const ezApplicationFileSystemConfig& GetFileSystemConfig() const { return m_FileSystemConfig; }
  const ezApplicationPluginConfig GetRuntimePluginConfig(bool bIncludeEditorPlugins) const;

  void SetFileSystemConfig(const ezApplicationFileSystemConfig& cfg);

  bool MakeDataDirectoryRelativePathAbsolute(ezStringBuilder& sPath) const;
  bool MakeDataDirectoryRelativePathAbsolute(ezString& sPath) const;
  bool MakePathDataDirectoryRelative(ezStringBuilder& sPath) const;
  bool MakePathDataDirectoryRelative(ezString& sPath) const;

  bool MakePathDataDirectoryParentRelative(ezStringBuilder& sPath) const;
  bool MakeParentDataDirectoryRelativePathAbsolute(ezStringBuilder& sPath, bool bCheckExists) const;

  ezStatus SaveTagRegistry();

  /// \brief Reads the known input slots from disk and adds them to the existing list.
  ///
  /// All input slots to be exposed by the editor are stored in 'Shared/Tools/ezEditor/InputSlots'
  /// as txt files. Each line names one input slot.
  void GetKnownInputSlots(ezDynamicArray<ezString>& slots) const;

  /// \brief Instructs the engine to reload its resources
  void ReloadEngineResources();

Q_SIGNALS:
  void IdleEvent();

private:
  ezString BuildDocumentTypeFileFilter(bool bForCreation);

  void GuiCreateOrOpenDocument(bool bCreate);
  bool GuiCreateOrOpenProject(bool bCreate);

private Q_SLOTS:
  void SlotTimedUpdate();
  void SlotQueuedCloseProject();
  void SlotQueuedOpenProject(QString sProject);
  void SlotQueuedOpenDocument(QString sProject, void* pOpenContext);
  void SlotQueuedGuiOpenDashboard();
  void SlotQueuedGuiOpenDocsAndCommunity();
  void SlotQueuedGuiCreateOrOpenProject(bool bCreate);
  void SlotSaveSettings();
  void SlotVersionCheckCompleted(bool bNewVersionReleased, bool bForced);

private:
  void UpdateGlobalStatusBarMessage();

  void DocumentManagerRequestHandler(ezDocumentManager::Request& r);
  void DocumentManagerEventHandler(const ezDocumentManager::Event& r);
  void DocumentEventHandler(const ezDocumentEvent& e);
  void DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e);
  void ProjectRequestHandler(ezToolsProjectRequest& r);
  void ProjectEventHandler(const ezToolsProjectEvent& r);
  void EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e);
  void UiServicesEvents(const ezQtUiServices::Event& e);

  void LoadEditorPreferences();
  void LoadProjectPreferences();
  void DetectAvailablePluginBundles();
  void StoreEnginePluginModificationTimes();
  bool CheckForEnginePluginModifications();
  void RestartEngineProcessIfPluginsChanged();
  void SaveAllOpenDocuments();

  void ReadTagRegistry();

  void SetupDataDirectories();
  void SetStyleSheet();
  void CreatePanels();

  void SetupAndShowSplashScreen();
  void CloseSplashScreen();

  ezResult AddBundlesInOrder(ezDynamicArray<ezApplicationPluginConfig::PluginConfig>& order, const ezPluginBundleSet& bundles, const ezString& start, bool bEditor, bool bEditorEngine, bool bRuntime) const;

  bool m_bSavePreferencesAfterOpenProject;
  bool m_bLoadingProjectInProgress = false;
  bool m_bAnyProjectOpened = false;

  ezBitflags<StartupFlags> m_StartupFlags;
  ezDynamicArray<ezString> m_DocumentsToOpen;

  ezSet<ezString> s_RestartRequiredReasons;
  ezSet<ezString> s_ReloadProjectRequiredReasons;

  ezPluginBundleSet m_PluginBundles;

  void SaveRecentFiles();
  void LoadRecentFiles();

  ezRecentFilesList s_RecentProjects;
  ezRecentFilesList s_RecentDocuments;

  QApplication* s_pQtApplication = nullptr;
  ezLongOpControllerManager m_LongOpControllerManager;
  ezEditorEngineProcessConnection* s_pEngineViewProcess;
  QTimer* m_pTimer = nullptr;

  QSplashScreen* m_pSplashScreen = nullptr;

  ezLogWriter::HTML m_LogHTML;

  ezTime m_LastPluginModificationCheck;
  ezApplicationFileSystemConfig m_FileSystemConfig;

  // *** Recent Paths ***
  ezString m_sLastDocumentFolder;
  ezString m_sLastProjectFolder;

  // *** Progress Bar ***
public:
  bool IsProgressBarProcessingEvents() const;

private:
  ezProgress* m_pProgressbar = nullptr;
  ezQtProgressbar* m_pQtProgressbar = nullptr;

  // *** Localization ***
  ezTranslatorFromFiles* m_pTranslatorFromFiles = nullptr;

  // *** Dynamic Enum Strings ***
  ezSet<ezString> m_DynamicEnumStringsToClear;
  void OnDemandDynamicStringEnumLoad(const char* szEnum, ezDynamicStringEnum& e);

  ezQtVersionChecker m_VersionChecker;
};

EZ_DECLARE_FLAGS_OPERATORS(ezQtEditorApp::StartupFlags);
