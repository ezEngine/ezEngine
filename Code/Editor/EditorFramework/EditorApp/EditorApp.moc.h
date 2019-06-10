#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/EditorApp/WhatsNew.h>
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
#include <ToolsFoundation/Basics/RecentFilesList.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class QMainWindow;
class QWidget;
class ezProgress;
class ezQtProgressbar;
class ezQtEditorApp;
class QStringList;

struct EZ_EDITORFRAMEWORK_DLL ezEditorAppEvent
{
  enum class Type
  {
    BeforeApplyDataDirectories, ///< Sent after data directory config was loaded, but before it is applied. Allows to add custom
                                ///< dependencies at the right moment.
    ReloadResources,            ///< Sent when 'ReloadResources' has been triggered (and a message was sent to the engine)
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
      Headless = EZ_BIT(0),
      SafeMode = EZ_BIT(1),
      NoRecent = EZ_BIT(2),
      Debug = EZ_BIT(3),
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

  ezEvent<const ezEditorAppEvent&> m_Events;

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
  ezStatus ExecuteTool(const char* szTool, const QStringList& arguments, ezUInt32 uiSecondsTillTimeout,
    ezLogInterface* pLogOutput = nullptr, ezLogMsgType::Enum LogLevel = ezLogMsgType::WarningMsg);

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
  bool IsInSafeMode() const { return m_bSafeMode; }
  /// \brief Returns true if StartupEditor was called with true. This is the case in an EditorProcessor.
  bool IsInHeadlessMode() const { return m_bHeadless; }

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
  void StartupEditor(ezBitflags<StartupFlags> flags, const char* szUserDataFolder = nullptr);
  void ShutdownEditor();
  ezInt32 RunEditor();
  void DeInitQt();

  void LoadEditorPlugins();
  void UnloadEditorPlugins();

  ezRecentFilesList& GetRecentProjectsList() { return s_RecentProjects; }
  ezRecentFilesList& GetRecentDocumentsList() { return s_RecentDocuments; }

  ezEditorEngineProcessConnection* GetEngineViewProcess() { return s_pEngineViewProcess; }

  void ShowSettingsDocument();
  void CloseSettingsDocument();

  const ezWhatsNewText& GetWhatsNew() const { return m_WhatsNew; }

  void CloseProject();
  void OpenProject(const char* szProject);

  void GuiCreateDocument();
  void GuiOpenDocument();

  void GuiCreateProject();
  void GuiOpenProject();

  void OpenDocumentQueued(const char* szDocument, const ezDocumentObject* pOpenContext = nullptr);
  ezDocument* OpenDocument(const char* szDocument, ezBitflags<ezDocumentFlags> flags, const ezDocumentObject* pOpenContext = nullptr);
  ezDocument* CreateDocument(const char* szDocument, ezBitflags<ezDocumentFlags> flags, const ezDocumentObject* pOpenContext = nullptr);

  ezResult CreateOrOpenProject(bool bCreate, const char* szFile);

  /// \brief Adds a data directory as a hard dependency to the project. Should be used by plugins to ensure their required data is
  /// available. The path must be relative to the SdkRoot folder.
  void AddPluginDataDirDependency(const char* szSdkRootRelativePath, const char* szRootName = nullptr, bool bWriteable = false);

  const ezApplicationFileSystemConfig& GetFileSystemConfig() const { return m_FileSystemConfig; }
  const ezApplicationPluginConfig& GetEnginePluginConfig() const { return m_EnginePluginConfig; }

  void SetFileSystemConfig(const ezApplicationFileSystemConfig& cfg);

  bool MakeDataDirectoryRelativePathAbsolute(ezStringBuilder& sPath) const;
  bool MakeDataDirectoryRelativePathAbsolute(ezString& sPath) const;
  bool MakePathDataDirectoryRelative(ezStringBuilder& sPath) const;
  bool MakePathDataDirectoryRelative(ezString& sPath) const;

  bool MakePathDataDirectoryParentRelative(ezStringBuilder& sPath) const;
  bool MakeParentDataDirectoryRelativePathAbsolute(ezStringBuilder& sPath, bool bCheckExists) const;

  void AddRuntimePluginDependency(const char* szEditorPluginName, const char* szRuntimeDependency);

  ezStatus SaveTagRegistry();

  /// \brief Reads the known input slots from disk and adds them to the existing list.
  ///
  /// All input slots to be exposed by the editor are stored in 'Shared/Tools/ezEditor/InputSlots'
  /// as txt files. Each line names one input slot.
  void GetKnownInputSlots(ezDynamicArray<ezString>& slots) const;

Q_SIGNALS:
  void IdleEvent();

private:
  ezString BuildDocumentTypeFileFilter(bool bForCreation);

  void GuiCreateOrOpenDocument(bool bCreate);
  void GuiCreateOrOpenProject(bool bCreate);

private Q_SLOTS:
  void SlotTimedUpdate();
  void SlotQueuedCloseProject();
  void SlotQueuedOpenProject(QString sProject);
  void SlotQueuedOpenDocument(QString sProject, void* pOpenContext);
  void SlotQueuedGuiCreateOrOpenProject(bool bCreate);
  void SlotSaveSettings();

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
  void SaveAllOpenDocuments();

  void ValidateEnginePluginConfig();

  void ReadTagRegistry();

  void SetupDataDirectories();
  void SetStyleSheet();
  void CreatePanels();

  bool m_bSafeMode;
  bool m_bHeadless;
  bool m_bSavePreferencesAfterOpenProject;

  ezSet<ezString> s_RestartRequiredReasons;
  ezSet<ezString> s_ReloadProjectRequiredReasons;

  ezPluginSet s_EditorPlugins;
  ezPluginSet s_EnginePlugins;

  void SaveRecentFiles();
  void LoadRecentFiles();

  ezRecentFilesList s_RecentProjects;
  ezRecentFilesList s_RecentDocuments;

  QApplication* s_pQtApplication;
  ezLongOpControllerManager m_LongOpControllerManager;
  ezEditorEngineProcessConnection* s_pEngineViewProcess;
  QTimer* m_pTimer;

  ezLogWriter::HTML m_LogHTML;

  ezApplicationFileSystemConfig m_FileSystemConfig;
  ezApplicationPluginConfig m_EnginePluginConfig;

  ezMap<ezString, ezSet<ezString>> m_AdditionalRuntimePluginDependencies;

  // *** Recent Paths ***
  ezString m_sLastDocumentFolder;
  ezString m_sLastProjectFolder;

  // *** Progress Bar ***
  ezProgress* m_pProgressbar;
  ezQtProgressbar* m_pQtProgressbar;

  ezWhatsNewText m_WhatsNew;
};

EZ_DECLARE_FLAGS_OPERATORS(ezQtEditorApp::StartupFlags);
