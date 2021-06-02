#include <EditorFrameworkPCH.h>

#include <Actions/CommonAssetActions.h>
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
#include <Foundation/Utilities/CommandLineOptions.h>
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
#include <QSplashScreen>
#include <QSvgRenderer>
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
    ezCommonAssetActions::RegisterActions();

    ezActionMapManager::RegisterActionMap("SettingsTabMenuBar").IgnoreResult();
    ezProjectActions::MapActions("SettingsTabMenuBar");
    ezStandardMenus::MapActions("SettingsTabMenuBar", ezStandardMenuTypes::Panels);

    ezActionMapManager::RegisterActionMap("AssetBrowserToolBar").IgnoreResult();
    ezAssetActions::MapActions("AssetBrowserToolBar", false);

    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezFileBrowserAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtFilePropertyWidget(); }).IgnoreResult();
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezAssetBrowserAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtAssetPropertyWidget(); }).IgnoreResult();
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezDynamicEnumAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtDynamicEnumPropertyWidget(); }).IgnoreResult();
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezDynamicStringEnumAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtDynamicStringEnumPropertyWidget(); }).IgnoreResult();
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezExposedParametersAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtExposedParametersPropertyWidget(); }).IgnoreResult();
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezGameObjectReferenceAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtGameObjectReferencePropertyWidget(); }).IgnoreResult();

    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezSphereManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezSphereManipulatorAdapter); }).IgnoreResult();
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezCapsuleManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezCapsuleManipulatorAdapter); }).IgnoreResult();
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezBoxManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezBoxManipulatorAdapter); }).IgnoreResult();
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezConeAngleManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezConeAngleManipulatorAdapter); }).IgnoreResult();
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezConeLengthManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezConeLengthManipulatorAdapter); }).IgnoreResult();
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezNonUniformBoxManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezNonUniformBoxManipulatorAdapter); }).IgnoreResult();
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezTransformManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezTransformManipulatorAdapter); }).IgnoreResult();

    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezBoxVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezBoxVisualizerAdapter); }).IgnoreResult();
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezSphereVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezSphereVisualizerAdapter); }).IgnoreResult();
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezCapsuleVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezCapsuleVisualizerAdapter); }).IgnoreResult();
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezCylinderVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezCylinderVisualizerAdapter); }).IgnoreResult();
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezDirectionVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezDirectionVisualizerAdapter); }).IgnoreResult();
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezConeVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezConeVisualizerAdapter); }).IgnoreResult();
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezCameraVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezCameraVisualizerAdapter); }).IgnoreResult();

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
    ezCommonAssetActions::UnregisterActions();

    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezFileBrowserAttribute>()).IgnoreResult();
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezAssetBrowserAttribute>()).IgnoreResult();
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezDynamicEnumAttribute>()).IgnoreResult();
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezDynamicStringEnumAttribute>()).IgnoreResult();
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezGameObjectReferenceAttribute>()).IgnoreResult();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezCommandLineOptionBool opt_Safe("_Editor", "-safe", "In safe-mode the editor minimizes the risk of crashing, for instance by not loading previous projects and scenes.", false);
ezCommandLineOptionBool opt_NoRecent("_Editor", "-noRecent", "Disables automatic loading of recent projects and documents.", false);
ezCommandLineOptionBool opt_Debug("_Editor", "-debug", "Enables debug-mode, which makes the editor wait for a debugger to attach, and disables risky features, such as recent file loading.", false);

void ezQtEditorApp::StartupEditor()
{
  ezBitflags<StartupFlags> startupFlags;

  startupFlags.AddOrRemove(StartupFlags::SafeMode, opt_Safe.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified));
  startupFlags.AddOrRemove(StartupFlags::NoRecent, opt_NoRecent.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified));
  startupFlags.AddOrRemove(StartupFlags::Debug, opt_Debug.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified));

  StartupEditor(startupFlags);
}

void ezQtEditorApp::StartupEditor(ezBitflags<StartupFlags> startupFlags, const char* szUserDataFolder)
{
  EZ_PROFILE_SCOPE("StartupEditor");

  m_StartupFlags = startupFlags;

  auto* pCmd = ezCommandLineUtils::GetGlobalInstance();

  if (!IsInHeadlessMode())
  {
    SetupAndShowSplashScreen();

    m_pProgressbar = EZ_DEFAULT_NEW(ezProgress);
    m_pQtProgressbar = EZ_DEFAULT_NEW(ezQtProgressbar);

    ezProgress::SetGlobalProgressbar(m_pProgressbar);
    m_pQtProgressbar->SetProgressbar(m_pProgressbar);
  }

  // custom command line arguments
  {
    // Make sure to disable the fileserve plugin
    pCmd->InjectCustomArgument("-fs_off");
  }

  const bool bNoRecent = m_StartupFlags.IsAnySet(StartupFlags::UnitTest | StartupFlags::SafeMode | StartupFlags::Headless | StartupFlags::NoRecent);

  ezString sApplicationName = pCmd->GetStringOption("-appname", 0, "ezEditor");
  ezApplicationServices::GetSingleton()->SetApplicationName(sApplicationName);

  QLocale::setDefault(QLocale(QLocale::English));

  s_pEngineViewProcess = new ezEditorEngineProcessConnection;

  s_pEngineViewProcess->SetWaitForDebugger(m_StartupFlags.IsSet(StartupFlags::Debug));
  s_pEngineViewProcess->SetRenderer(pCmd->GetStringOption("-renderer", 0, ""));

  m_LongOpControllerManager.Startup(&s_pEngineViewProcess->GetCommunicationChannel());

  QCoreApplication::setOrganizationDomain("www.ezEngine.net");
  QCoreApplication::setOrganizationName("ezEngine Project");
  QCoreApplication::setApplicationName(ezApplicationServices::GetSingleton()->GetApplicationName());
  QCoreApplication::setApplicationVersion("1.0.0");

  if (!IsInHeadlessMode())
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
  ezQtUiServices::s_Events.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::UiServicesEvents, this));

  ezStartup::StartupCoreSystems();

  // prevent restoration of window layouts when in safe mode
  ezQtDocumentWindow::s_bAllowRestoreWindowLayout = !IsInSafeMode();

  {
    EZ_PROFILE_SCOPE("Filesystem");
    ezFileSystem::DetectSdkRootDirectory().IgnoreResult();

    const ezString sAppDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
    ezString sUserData = ezApplicationServices::GetSingleton()->GetApplicationUserDataFolder();
    if (!ezStringUtils::IsNullOrEmpty(szUserDataFolder))
    {
      sUserData = szUserDataFolder;
    }
    // make sure these folders exist
    ezFileSystem::CreateDirectoryStructure(sAppDir).IgnoreResult();
    ezFileSystem::CreateDirectoryStructure(sUserData).IgnoreResult();

    ezFileSystem::AddDataDirectory("", "AbsPaths", ":", ezFileSystem::AllowWrites).IgnoreResult();             // for absolute paths
    ezFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", ezFileSystem::AllowWrites).IgnoreResult();     // writing to the binary directory
    ezFileSystem::AddDataDirectory(sAppDir, "AppData", "app").IgnoreResult();                                  // app specific data
    ezFileSystem::AddDataDirectory(sUserData, "AppData", "appdata", ezFileSystem::AllowWrites).IgnoreResult(); // for writing app user data
  }

  {
    EZ_PROFILE_SCOPE("Logging");
    ezString sApplicationID = pCmd->GetStringOption("-appid", 0, "ezEditor");
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

  if (!IsInHeadlessMode())
  {
    ezActionManager::LoadShortcutAssignment();

    LoadRecentFiles();

    CreatePanels();

    ShowSettingsDocument();

    connect(&m_VersionChecker, &ezQtVersionChecker::VersionCheckCompleted, this, &ezQtEditorApp::SlotVersionCheckCompleted);

    m_VersionChecker.Initialize();
    m_VersionChecker.Check(false);
  }

  LoadEditorPlugins();

  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();

  if (pCmd->GetStringOptionArguments("-project") > 0)
  {
    for (ezUInt32 doc = 0; doc < pCmd->GetStringOptionArguments("-documents"); ++doc)
    {
      m_DocumentsToOpen.PushBack(pCmd->GetStringOption("-documents", doc));
    }

    CreateOrOpenProject(false, pCmd->GetAbsolutePathOption("-project")).IgnoreResult();
  }
  else if (!bNoRecent && !m_StartupFlags.IsSet(StartupFlags::Debug) && pPreferences->m_bLoadLastProjectAtStartup)
  {
    if (!s_RecentProjects.GetFileList().IsEmpty())
    {
      CreateOrOpenProject(false, s_RecentProjects.GetFileList()[0].m_File).IgnoreResult();
    }
  }
  else if (!IsInHeadlessMode() && !IsInSafeMode())
  {
    if (ezQtContainerWindow::GetContainerWindow())
    {
      ezQtContainerWindow::GetContainerWindow()->ScheduleRestoreWindowLayout();
    }
  }

  connect(m_pTimer, SIGNAL(timeout()), this, SLOT(SlotTimedUpdate()), Qt::QueuedConnection);
  m_pTimer->start(1);

  if (m_StartupFlags.AreNoneSet(StartupFlags::Headless | StartupFlags::UnitTest) && !ezToolsProject::GetSingleton()->IsProjectOpen())
  {
    GuiOpenDashboard();
  }
}

void ezQtEditorApp::ShutdownEditor()
{
  m_pTimer->stop();
  ezToolsProject::CloseProject();

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
  ezQtUiServices::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::UiServicesEvents, this));

  ezQtUiServices::GetSingleton()->SaveState();

  CloseSettingsDocument();

  if (!IsInHeadlessMode())
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


void ezQtEditorApp::SetupAndShowSplashScreen()
{
  EZ_ASSERT_DEV(m_pSplashScreen == nullptr, "Splash screen shouldn't exist already.");

  QSvgRenderer svgRenderer(QString(":/Splash/Splash/splash.svg"));

  const qreal PixelRatio = qApp->primaryScreen()->devicePixelRatio();

  // TODO: When migrating to Qt 5.15 or newer this should have a fixed square size and
  // let the aspect ratio mode of the svg renderer handle the difference
  QPixmap splashPixmap(QSize(187, 256) * PixelRatio);
  splashPixmap.fill(Qt::transparent);
  {
    QPainter painter;
    painter.begin(&splashPixmap);
    svgRenderer.render(&painter);
    painter.end();
  }

  splashPixmap.setDevicePixelRatio(PixelRatio);

  m_pSplashScreen = new QSplashScreen(splashPixmap);
  m_pSplashScreen->setMask(splashPixmap.mask());
  m_pSplashScreen->setWindowFlag(Qt::WindowStaysOnTopHint, true);

  m_pSplashScreen->show();
}

void ezQtEditorApp::CloseSplashScreen()
{
  if (!m_pSplashScreen)
    return;

  m_pSplashScreen->deleteLater();
  m_pSplashScreen = nullptr;
}
