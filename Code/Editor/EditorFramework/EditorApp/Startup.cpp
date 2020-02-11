#include <EditorFrameworkPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/GameObjectContextActions.h>
#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Manipulators/BoxManipulatorAdapter.h>
#include <EditorFramework/Manipulators/CapsuleManipulatorAdapter.h>
#include <EditorFramework/Manipulators/ConeAngleManipulatorAdapter.h>
#include <EditorFramework/Manipulators/ConeLengthManipulatorAdapter.h>
#include <EditorFramework/Manipulators/ManipulatorAdapterRegistry.h>
#include <EditorFramework/Manipulators/NonUniformBoxManipulatorAdapter.h>
#include <EditorFramework/Manipulators/SphereManipulatorAdapter.h>
#include <EditorFramework/Manipulators/TransformManipulatorAdapter.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetCuratorPanel.moc.h>
#include <EditorFramework/Panels/CVarPanel/CVarPanel.moc.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <EditorFramework/Panels/LongOpsPanel/LongOpsPanel.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/DynamicEnumPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/FileBrowserPropertyWidget.moc.h>
#include <EditorFramework/Visualizers/BoxVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CameraVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CapsuleVisualizerAdapter.h>
#include <EditorFramework/Visualizers/ConeVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CylinderVisualizerAdapter.h>
#include <EditorFramework/Visualizers/DirectionVisualizerAdapter.h>
#include <EditorFramework/Visualizers/SphereVisualizerAdapter.h>
#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <PropertyGrid/GameObjectReferencePropertyWidget.moc.h>
#include <QClipboard>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <ads/DockManager.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, EditorFrameworkMain)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "GuiFoundation",
    "PropertyGrid",
    "ManipulatorAdapterRegistry"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezProjectActions::RegisterActions();
    ezAssetActions::RegisterActions();
    ezViewActions::RegisterActions();
    ezGameObjectContextActions::RegisterActions();
    ezGameObjectDocumentActions::RegisterActions();
    ezGameObjectSelectionActions::RegisterActions();
    ezQuadViewActions::RegisterActions();
    ezTransformGizmoActions::RegisterActions();
    ezTranslateGizmoAction::RegisterActions();

    ezActionMapManager::RegisterActionMap("SettingsTabMenuBar");
    ezProjectActions::MapActions("SettingsTabMenuBar");
    ezStandardMenus::MapActions("SettingsTabMenuBar", ezStandardMenuTypes::Panels);

    ezActionMapManager::RegisterActionMap("AssetBrowserToolBar");
    ezAssetActions::MapActions("AssetBrowserToolBar", false);

    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezFileBrowserAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtFilePropertyWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezAssetBrowserAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtAssetPropertyWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezDynamicEnumAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtDynamicEnumPropertyWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezDynamicStringEnumAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtDynamicStringEnumPropertyWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezExposedParametersAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtExposedParametersPropertyWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezGameObjectReferenceAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtGameObjectReferencePropertyWidget(); });

    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezSphereManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezSphereManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezCapsuleManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezCapsuleManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezBoxManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezBoxManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezConeAngleManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezConeAngleManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezConeLengthManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezConeLengthManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezNonUniformBoxManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezNonUniformBoxManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezTransformManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezTransformManipulatorAdapter); });

    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezBoxVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezBoxVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezSphereVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezSphereVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezCapsuleVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezCapsuleVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezCylinderVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezCylinderVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezDirectionVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezDirectionVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezConeVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezConeVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezCameraVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezCameraVisualizerAdapter); });

  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezProjectActions::UnregisterActions();
    ezAssetActions::UnregisterActions();
    ezViewActions::UnregisterActions();
    ezGameObjectContextActions::UnregisterActions();
    ezGameObjectDocumentActions::UnregisterActions();
    ezGameObjectSelectionActions::UnregisterActions();
    ezQuadViewActions::UnregisterActions();
    ezTransformGizmoActions::UnregisterActions();
    ezTranslateGizmoAction::UnregisterActions();

    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezFileBrowserAttribute>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezAssetBrowserAttribute>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezDynamicEnumAttribute>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezDynamicStringEnumAttribute>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezGameObjectReferenceAttribute>());
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on


void ezQtEditorApp::StartupEditor()
{
  ezBitflags<StartupFlags> flags;
  if (ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-safe"))
    flags.Add(StartupFlags::SafeMode);
  if (ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-norecent"))
    flags.Add(StartupFlags::NoRecent);
  if (ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-debug"))
    flags.Add(StartupFlags::Debug);

  StartupEditor(flags);
}

void ezQtEditorApp::StartupEditor(ezBitflags<StartupFlags> flags, const char* szUserDataFolder)
{
  EZ_PROFILE_SCOPE("StartupEditor");

  m_bUnitTestMode = flags.IsSet(StartupFlags::UnitTest);

  m_bHeadless = flags.IsSet(StartupFlags::Headless);
  if (!m_bHeadless)
  {
    m_pProgressbar = EZ_DEFAULT_NEW(ezProgress);
    m_pQtProgressbar = EZ_DEFAULT_NEW(ezQtProgressbar);

    ezProgress::SetGlobalProgressbar(m_pProgressbar);
    m_pQtProgressbar->SetProgressbar(m_pProgressbar);
  }

  // custom command line arguments
  {
    // Make sure to disable the fileserve plugin
    ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-fs_off");
  }

  m_bSafeMode = flags.IsSet(StartupFlags::SafeMode);
  const bool bNoRecent = m_bUnitTestMode || m_bSafeMode || m_bHeadless || flags.IsSet(StartupFlags::NoRecent);

  ezString sApplicationName = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-appname", 0, "ezEditor");
  ezApplicationServices::GetSingleton()->SetApplicationName(sApplicationName);

  QLocale::setDefault(QLocale(QLocale::English));

  s_pEngineViewProcess = new ezEditorEngineProcessConnection;

  s_pEngineViewProcess->SetWaitForDebugger(flags.IsSet(StartupFlags::Debug));

  m_LongOpControllerManager.Startup(&s_pEngineViewProcess->GetCommunicationChannel());

  QCoreApplication::setOrganizationDomain("www.ezEngine.net");
  QCoreApplication::setOrganizationName("ezEngine Project");
  QCoreApplication::setApplicationName(ezApplicationServices::GetSingleton()->GetApplicationName());
  QCoreApplication::setApplicationVersion("1.0.0");

  if (!m_bHeadless)
  {
    EZ_PROFILE_SCOPE("ezQtContainerWindow");
    SetStyleSheet();

    ezQtContainerWindow* pContainer = new ezQtContainerWindow();
    pContainer->show();
  }

  ezDocumentManager::s_Requests.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentManagerRequestHandler, this));
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentManagerEventHandler, this));
  ezDocument::s_EventsAny.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentEventHandler, this));
  ezToolsProject::s_Requests.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::ProjectRequestHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::ProjectEventHandler, this));
  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::EngineProcessMsgHandler, this));
  ezQtDocumentWindow::s_Events.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentWindowEventHandler, this));

  ezStartup::StartupCoreSystems();

  {
    EZ_PROFILE_SCOPE("Filesystem");
    const ezString sAppDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
    ezString sUserData = ezApplicationServices::GetSingleton()->GetApplicationUserDataFolder();
    if (!ezStringUtils::IsNullOrEmpty(szUserDataFolder))
    {
      sUserData = szUserDataFolder;
    }
    // make sure these folders exist
    ezFileSystem::CreateDirectoryStructure(sAppDir);
    ezFileSystem::CreateDirectoryStructure(sUserData);

    ezFileSystem::AddDataDirectory("", "AbsPaths", ":", ezFileSystem::AllowWrites);             // for absolute paths
    ezFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", ezFileSystem::AllowWrites);     // writing to the binary directory
    ezFileSystem::AddDataDirectory(sAppDir, "AppData", "app");                                  // app specific data
    ezFileSystem::AddDataDirectory(sUserData, "AppData", "appdata", ezFileSystem::AllowWrites); // for writing app user data
  }

  {
    EZ_PROFILE_SCOPE("Logging");
    ezString sApplicationID = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-appid", 0, "ezEditor");
    ezStringBuilder sLogFile;
    sLogFile.Format(":appdata/Log_{0}.htm", sApplicationID);
    m_LogHTML.BeginLog(sLogFile, sApplicationID);

    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLoggingEvent::Handler(&ezLogWriter::HTML::LogMessageHandler, &m_LogHTML));
  }
  ezUniquePtr<ezTranslatorFromFiles> pTranslatorEn = EZ_DEFAULT_NEW(ezTranslatorFromFiles);
  m_pTranslatorFromFiles = pTranslatorEn.Borrow();

  // ezUniquePtr<ezTranslatorFromFiles> pTranslatorDe = EZ_DEFAULT_NEW(ezTranslatorFromFiles);

  pTranslatorEn->AddTranslationFilesFromFolder(":app/Localization/en");
  // pTranslatorDe->LoadTranslationFilesFromFolder(":app/Localization/de");

  ezTranslationLookup::AddTranslator(EZ_DEFAULT_NEW(ezTranslatorLogMissing));
  ezTranslationLookup::AddTranslator(std::move(pTranslatorEn));
  // ezTranslationLookup::AddTranslator(std::move(pTranslatorDe));

  LoadEditorPreferences();

  ezQtUiServices::GetSingleton()->LoadState();

  if (!m_bHeadless)
  {
    ezActionManager::LoadShortcutAssignment();

    m_WhatsNew.Load(":app/WhatsNew.txt");

    LoadRecentFiles();

    CreatePanels();

    ShowSettingsDocument();
  }

  LoadEditorPlugins();

  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();

  if (!bNoRecent && !m_bUnitTestMode && pPreferences->m_bLoadLastProjectAtStartup && !m_WhatsNew.HasChanged())
  {
    // first open the project, so that the data directory list is read
    if (!s_RecentProjects.GetFileList().IsEmpty())
    {
      CreateOrOpenProject(false, s_RecentProjects.GetFileList()[0].m_File);
    }
  }
  else if (!m_bHeadless)
  {
    if (ezQtContainerWindow::GetContainerWindow())
    {
      ezQtContainerWindow::GetContainerWindow()->ScheduleRestoreWindowLayout();
    }
  }

  connect(m_pTimer, SIGNAL(timeout()), this, SLOT(SlotTimedUpdate()), Qt::QueuedConnection);
  m_pTimer->start(1);
}

void ezQtEditorApp::ShutdownEditor()
{
  ezToolsProject::CloseProject();

  if (!m_bHeadless && !m_bUnitTestMode)
  {
    m_WhatsNew.StoreLastRead();
  }

  SaveSettings();

  m_LongOpControllerManager.Shutdown();

  ezToolsProject::CloseProject();

  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::EngineProcessMsgHandler, this));
  ezToolsProject::s_Requests.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::ProjectRequestHandler, this));
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::ProjectEventHandler, this));
  ezDocument::s_EventsAny.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentEventHandler, this));
  ezDocumentManager::s_Requests.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentManagerRequestHandler, this));
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentManagerEventHandler, this));
  ezQtDocumentWindow::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentWindowEventHandler, this));

  ezQtUiServices::GetSingleton()->SaveState();

  CloseSettingsDocument();

  if (!m_bHeadless)
  {
    delete ezQtContainerWindow::GetContainerWindow();
  }
  // HACK to figure out why the panels are not always properly destroyed together with the ContainerWindows
  // if you run into this, please try to figure this out
  // every ezQtApplicationPanel actually registers itself with a container window in its constructor
  // there its Qt 'parent' is set to the container window (there is only one)
  // that means, when the application is shut down, all ezQtApplicationPanel instances should get deleted by their parent
  // ie. the container window
  // however, SOMETIMES this does not happen
  // it seems to be related to whether a panel has been opened/closed (ie. shown/hidden), and maybe also with the restored state
  {
    const auto& Panels = ezQtApplicationPanel::GetAllApplicationPanels();
    ezUInt32 uiNumPanels = Panels.GetCount();

    EZ_ASSERT_DEBUG(uiNumPanels == 0, "Not all panels have been cleaned up correctly");

    for (ezUInt32 i = 0; i < uiNumPanels; ++i)
    {
      ezQtApplicationPanel* pPanel = Panels[i];
      QObject* pParent = pPanel->parent();
      delete pPanel;
    }
  }


  QCoreApplication::sendPostedEvents();
  qApp->processEvents();

  delete s_pEngineViewProcess;

  // Unload potential plugin referenced clipboard data to prevent crash on shutdown.
  QApplication::clipboard()->clear();
  UnloadEditorPlugins();

  // make sure no one tries to load any further images in parallel
  ezQtImageCache::GetSingleton()->StopRequestProcessing(true);

  ezTranslationLookup::Clear();

  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLoggingEvent::Handler(&ezLogWriter::HTML::LogMessageHandler, &m_LogHTML));
  m_LogHTML.EndLog();

  EZ_DEFAULT_DELETE(m_pQtProgressbar);
  EZ_DEFAULT_DELETE(m_pProgressbar);
}



void ezQtEditorApp::CreatePanels()
{
  EZ_PROFILE_SCOPE("CreatePanels");
  ezQtApplicationPanel* pAssetBrowserPanel = new ezQtAssetBrowserPanel();
  ezQtApplicationPanel* pLogPanel = new ezQtLogPanel();
  ezQtApplicationPanel* pLongOpsPanel = new ezQtLongOpsPanel();
  ezQtApplicationPanel* pCVarPanel = new ezQtCVarPanel();
  ezQtApplicationPanel* pAssetCuratorPanel = new ezQtAssetCuratorPanel();

  ezQtContainerWindow* pMainWnd = ezQtContainerWindow::GetContainerWindow();
  ads::CDockManager* pDockManager = pMainWnd->GetDockManager();
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pAssetBrowserPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pLogPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pAssetCuratorPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pCVarPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pLongOpsPanel);

  pAssetBrowserPanel->raise();
}
