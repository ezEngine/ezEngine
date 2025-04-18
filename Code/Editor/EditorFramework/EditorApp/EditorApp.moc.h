#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
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
template <typename T>
class QList;
using QStringList = QList<QString>;
class ezTranslatorFromFiles;
class ezDynamicStringEnum;
class QSplashScreen;
class ezQtVersionChecker;

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
    using StorageType = ezUInt8;
    enum Enum
    {
      Headless = EZ_BIT(0),   ///< The app does not do any rendering.
      SafeMode = EZ_BIT(1),   ///< '-safe' : Prevent automatic loading of projects, scenes, etc. to minimize risk of crashing.
      NoRecent = EZ_BIT(2),   ///< '-norecent' : Do not modify recent file lists. Used for modes such as tests, where the user does not do any interactions.
      UnitTest = EZ_BIT(3),   ///< Specified when the process is running as a unit test
      Background = EZ_BIT(4), ///< This process is an editor processor background process handling IPC tasks of the editor parent process.
      Default = 0,
    };

    struct Bits
    {
      StorageType Headless : 1;
      StorageType SafeMode : 1;
      StorageType NoRecent : 1;
      StorageType UnitTest : 1;
      StorageType Background : 1;
    };
  };

public:
  ezQtEditorApp();
  ~ezQtEditorApp();

  static ezEvent<const ezEditorAppEvent&> m_Events;

  //
  // External Tools
  //

  /// \brief Searches for an external tool.
  ///
  /// Either uses one from the precompiled tools folder, or from the currently compiled binaries, depending where it finds one.
  /// If the editor preference is set to use precompiled tools, that folder is preferred, otherwise the other folder is preferred.
  ezString FindToolApplication(const char* szToolName);

  /// \brief Executes an external tool as found by FindToolApplication().
  ///
  /// The applications output is parsed and forwarded to the given log interface. A custom log level is applied first.
  /// If the tool cannot be found or it takes longer to execute than the allowed timeout, the function returns failure.
  ezStatus ExecuteTool(const char* szTool, const QStringList& arguments, ezUInt32 uiSecondsTillTimeout, ezLogInterface* pLogOutput = nullptr, ezLogMsgType::Enum logLevel = ezLogMsgType::WarningMsg, const char* szCWD = nullptr);

  /// \brief Creates the string with which to run Fileserve for the currently open project.
  ezString BuildFileserveCommandLine() const;

  /// \brief Launches Fileserve with the settings for the current project.
  void RunFileserve();

  /// \brief Launches ezInspector.
  void RunInspector();

  /// \brief Launches Tracy.
  void RunTracy();

  //
  //
  //

  /// \brief Returns whether we are between StartupEditor and ShutdownEditor.
  bool IsRunning() const { return m_bIsRunning; }

  /// \brief Can be set via the command line option '-safe'. In this mode the editor will not automatically load recent documents
  bool IsInSafeMode() const { return m_StartupFlags.IsSet(StartupFlags::SafeMode); }

  /// \brief Returns true if the the app shouldn't display anything. This is the case in an EditorProcessor.
  bool IsInHeadlessMode() const { return m_StartupFlags.IsSet(StartupFlags::Headless); }

  /// \brief Returns true if the editor is started in run in test mode.
  bool IsInUnitTestMode() const { return m_StartupFlags.IsSet(StartupFlags::UnitTest); }

  /// \brief Returns true if the editor is started in run in background mode.
  bool IsBackgroundMode() const { return m_StartupFlags.IsSet(StartupFlags::Background); }

  const ezPluginBundleSet& GetPluginBundles() const { return m_PluginBundles; }
  ezPluginBundleSet& GetPluginBundles() { return m_PluginBundles; }

  void AddRestartRequiredReason(const char* szReason);
  const ezSet<ezString>& GetRestartRequiredReasons() { return m_RestartRequiredReasons; }

  void AddReloadProjectRequiredReason(const char* szReason);
  const ezSet<ezString>& GetReloadProjectRequiredReason() { return m_ReloadProjectRequiredReasons; }

  void SaveSettings();

  /// \brief Writes a file containing all the currently open documents
  void SaveOpenDocumentsList();

  /// \brief Reads the list of last open documents in the current project.
  ezRecentFilesList LoadOpenDocumentsList();

  void InitQt(int iArgc, char** pArgv);
  void StartupEditor();
  void StartupEditor(ezBitflags<StartupFlags> startupFlags, const char* szUserDataFolder = nullptr);
  void ShutdownEditor();
  ezInt32 RunEditor();
  void DeInitQt();

  void LoadEditorPlugins();

  ezRecentFilesList& GetRecentProjectsList() { return m_RecentProjects; }
  ezRecentFilesList& GetRecentDocumentsList() { return m_RecentDocuments; }

  ezEditorEngineProcessConnection* GetEngineViewProcess() { return m_pEngineViewProcess; }

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

  void OpenDocumentQueued(ezStringView sDocument, const ezDocumentObject* pOpenContext = nullptr);
  ezDocument* OpenDocument(ezStringView sDocument, ezBitflags<ezDocumentFlags> flags, const ezDocumentObject* pOpenContext = nullptr);
  ezDocument* CreateDocument(ezStringView sDocument, ezBitflags<ezDocumentFlags> flags, const ezDocumentObject* pOpenContext = nullptr);

  ezResult CreateOrOpenProject(bool bCreate, ezStringView sFile);

  /// \brief If this project is remote, ie coming from another repository that is not checked-out by default, make sure it exists locally on disk.
  ///
  /// Adjusts inout_sFilePath from pointing to an ezRemoteProject file to a ezProject file, if necessary.
  /// If the project is already local, it always succeeds.
  /// If checking out fails or is user canceled, the function returns failure.
  ezStatus MakeRemoteProjectLocal(ezStringBuilder& inout_sFilePath);

  bool ExistsPluginSelectionStateDDL(const char* szProjectDir = ":project");
  void WritePluginSelectionStateDDL(const char* szProjectDir = ":project");
  void CreatePluginSelectionDDL(const char* szProjectFile, const char* szTemplate);
  void LoadPluginBundleDlls(const char* szProjectFile);
  void DetectAvailablePluginBundles(ezStringView sSearchDirectory);

  /// \brief Launches a new instance of the editor to open the given project.
  void LaunchEditor(const char* szProject, bool bCreate);

  /// \brief Adds a data directory as a hard dependency to the project. Should be used by plugins to ensure their required data is
  /// available. The path must be relative to the SdkRoot folder.
  void AddPluginDataDirDependency(const char* szSdkRootRelativePath, const char* szRootName = nullptr, bool bWriteable = false);

  const ezApplicationFileSystemConfig& GetFileSystemConfig() const { return m_FileSystemConfig; }
  const ezApplicationPluginConfig GetRuntimePluginConfig(bool bIncludeEditorPlugins) const;

  void SetFileSystemConfig(const ezApplicationFileSystemConfig& cfg);

  bool MakeDataDirectoryRelativePathAbsolute(ezStringBuilder& ref_sPath) const;
  bool MakeDataDirectoryRelativePathAbsolute(ezString& ref_sPath) const;
  bool MakePathDataDirectoryRelative(ezStringBuilder& ref_sPath) const;
  bool MakePathDataDirectoryRelative(ezString& ref_sPath) const;

  bool MakePathDataDirectoryParentRelative(ezStringBuilder& ref_sPath) const;
  bool MakeParentDataDirectoryRelativePathAbsolute(ezStringBuilder& ref_sPath, bool bCheckExists) const;

  ezStatus SaveTagRegistry();

  /// \brief Reads the known input slots from disk and adds them to the existing list.
  ///
  /// All input slots to be exposed by the editor are stored in 'Shared/Tools/ezEditor/InputSlots'
  /// as txt files. Each line names one input slot.
  void GetKnownInputSlots(ezDynamicArray<ezString>& slots) const;

  /// \brief Instructs the engine to reload its resources
  void ReloadEngineResources();

  void RestartEngineProcessIfPluginsChanged(bool bForce);
  void SetStyleSheet();

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
  void ProjectRequestHandler(ezToolsProjectRequest& r);
  void ProjectEventHandler(const ezToolsProjectEvent& r);
  void EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e);
  void UiServicesEvents(const ezQtUiServices::Event& e);

  void SetupNewProject();
  void LoadEditorPreferences();
  void LoadProjectPreferences();
  void StoreEnginePluginModificationTimes();
  bool CheckForEnginePluginModifications();
  void SaveAllOpenDocuments();

  void ReadTagRegistry();

  void SetupDataDirectories();
  void CreatePanels();

  void SetupAndShowSplashScreen();
  void CloseSplashScreen();

  void OpenDemoDocument();

  ezResult AddBundlesInOrder(ezDynamicArray<ezApplicationPluginConfig::PluginConfig>& order, const ezPluginBundleSet& bundles, const ezString& start, bool bEditor, bool bEditorEngine, bool bRuntime) const;

  bool m_bSavePreferencesAfterOpenProject;
  bool m_bLoadingProjectInProgress = false;
  bool m_bAnyProjectOpened = false;
  bool m_bWroteCrashIndicatorFile = false;
  bool m_bIsRunning = false;

  ezBitflags<StartupFlags> m_StartupFlags;
  ezDynamicArray<ezString> m_DocumentsToOpen;

  ezSet<ezString> m_RestartRequiredReasons;
  ezSet<ezString> m_ReloadProjectRequiredReasons;

  ezPluginBundleSet m_PluginBundles;

  void SaveRecentFiles();
  void LoadRecentFiles();

  ezRecentFilesList m_RecentProjects;
  ezRecentFilesList m_RecentDocuments;

  int m_iArgc = 0;
  QApplication* m_pQtApplication = nullptr;
  ezLongOpControllerManager m_LongOpControllerManager;
  ezEditorEngineProcessConnection* m_pEngineViewProcess;
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
  void OnDemandDynamicStringEnumLoad(ezStringView sEnumName, ezDynamicStringEnum& e);

  ezUniquePtr<ezQtVersionChecker> m_pVersionChecker;
};

EZ_DECLARE_FLAGS_OPERATORS(ezQtEditorApp::StartupFlags);
