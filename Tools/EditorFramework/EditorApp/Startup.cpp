#include <PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/DynamicEnumPropertyWidget.moc.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <PropertyGrid/FileBrowserPropertyWidget.moc.h>
#include <PropertyGrid/DynamicStringEnumPropertyWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <CoreUtils/Other/Progress.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <CoreUtils/Localization/TranslationLookup.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <QClipboard>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <Panels/LogPanel/LogPanel.moc.h>
#include <Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Manipulators/ManipulatorAdapterRegistry.h>
#include <EditorFramework/Manipulators/SphereManipulatorAdapter.h>
#include <EditorFramework/Manipulators/CapsuleManipulatorAdapter.h>
#include <EditorFramework/Manipulators/BoxManipulatorAdapter.h>
#include <EditorFramework/Manipulators/ConeManipulatorAdapter.h>
#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <EditorFramework/Visualizers/BoxVisualizerAdapter.h>
#include <EditorFramework/Visualizers/SphereVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CapsuleVisualizerAdapter.h>
#include <EditorFramework/Visualizers/DirectionVisualizerAdapter.h>
#include <EditorFramework/Visualizers/ConeVisualizerAdapter.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, EditorFrameworkMain)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "GuiFoundation",
    "PropertyGrid",
    "ManipulatorAdapterRegistry"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezProjectActions::RegisterActions();
    ezAssetActions::RegisterActions();
    ezViewActions::RegisterActions();

    ezActionMapManager::RegisterActionMap("SettingsTabMenuBar");
    ezProjectActions::MapActions("SettingsTabMenuBar");
    ezStandardMenus::MapActions("SettingsTabMenuBar", ezStandardMenuTypes::Panels);
    
    ezActionMapManager::RegisterActionMap("AssetBrowserToolBar");
    ezAssetActions::MapActions("AssetBrowserToolBar", false);

    ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezFileBrowserAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtFilePropertyWidget(); });
    ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezAssetBrowserAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtAssetPropertyWidget(); });
    ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezDynamicEnumAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtDynamicEnumPropertyWidget(); });
    ezPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezDynamicStringEnumAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtDynamicStringEnumPropertyWidget(); });

    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezSphereManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezSphereManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezCapsuleManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezCapsuleManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezBoxManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezBoxManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezConeManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezConeManipulatorAdapter); });

    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezBoxVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezBoxVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezSphereVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezSphereVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezCapsuleVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezCapsuleVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezDirectionVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezDirectionVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezConeVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezConeVisualizerAdapter); });
  }

  ON_CORE_SHUTDOWN
  {
    ezProjectActions::UnregisterActions();
    ezAssetActions::UnregisterActions();
    ezViewActions::UnregisterActions();

    ezPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezFileBrowserAttribute>());
    ezPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezAssetBrowserAttribute>());
    ezPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezDynamicEnumAttribute>());
    ezPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezDynamicStringEnumAttribute>());
  }

EZ_END_SUBSYSTEM_DECLARATION


void ezQtEditorApp::StartupEditor(bool bHeadless)
{
  m_bHeadless = bHeadless;
  if (!bHeadless)
  { 
    // ezUniquePtr does not work with forward declared classes :-(
    m_pProgressbar = EZ_DEFAULT_NEW(ezProgress);
    m_pQtProgressbar = EZ_DEFAULT_NEW(ezQtProgressbar);

    ezProgress::SetGlobalProgressbar(m_pProgressbar);
    m_pQtProgressbar->SetProgressbar(m_pProgressbar);
  }

  m_bSafeMode = ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-safe");
  const bool bNoRecent = m_bSafeMode || bHeadless || ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-norecent");

  ezString sApplicationName = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-appname", 0, "ezEditor");
  ezApplicationServices::GetSingleton()->SetApplicationName(sApplicationName);

  QLocale::setDefault(QLocale(QLocale::English));

  s_pEngineViewProcess = new ezEditorEngineProcessConnection;

  s_pEngineViewProcess->SetWaitForDebugger(ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-debug"));

  QCoreApplication::setOrganizationDomain("www.ezEngine.net");
  QCoreApplication::setOrganizationName("ezEngine Project");
  QCoreApplication::setApplicationName(ezApplicationServices::GetSingleton()->GetApplicationName());
  QCoreApplication::setApplicationVersion("1.0.0");

  if (!bHeadless)
  {
    SetStyleSheet();

    ezContainerWindow* pContainer = new ezContainerWindow();
    pContainer->show();
  }

  ezDocumentManager::s_Requests.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentManagerRequestHandler, this));
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentManagerEventHandler, this));
  ezDocument::s_EventsAny.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentEventHandler, this));
  ezToolsProject::s_Requests.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::ProjectRequestHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::ProjectEventHandler, this));
  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::EngineProcessMsgHandler, this));
  ezQtDocumentWindow::s_Events.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentWindowEventHandler, this));

  ezStartup::StartupCore();

  const ezString sAppDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
  const ezString sUserData = ezApplicationServices::GetSingleton()->GetApplicationUserDataFolder();

  // make sure these folders exist
  ezOSFile::CreateDirectoryStructure(sAppDir);
  ezOSFile::CreateDirectoryStructure(sUserData);

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

  ezFileSystem::AddDataDirectory("", "AbsPaths", ":", ezFileSystem::AllowWrites); // for absolute paths
  ezFileSystem::AddDataDirectory(ezOSFile::GetApplicationDirectory(), "AppBin", "bin", ezFileSystem::AllowWrites); // writing to the binary directory
  ezFileSystem::AddDataDirectory(sAppDir, "AppData", "app"); // app specific data
  ezFileSystem::AddDataDirectory(sUserData, "AppData", "appdata", ezFileSystem::AllowWrites); // for writing app user data

  m_LogHTML.BeginLog(":appdata/Log_Editor.htm", "ezEditor");

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLoggingEvent::Handler(&ezLogWriter::HTML::LogMessageHandler, &m_LogHTML));

  ezUniquePtr<ezTranslatorFromFiles> pTranslatorEn = EZ_DEFAULT_NEW(ezTranslatorFromFiles);
  ezUniquePtr<ezTranslatorFromFiles> pTranslatorDe = EZ_DEFAULT_NEW(ezTranslatorFromFiles);

  pTranslatorEn->SetSearchPath("Localization/en");
  pTranslatorDe->SetSearchPath("Localization/de");

  ezTranslationLookup::AddTranslator(EZ_DEFAULT_NEW(ezTranslatorLogMissing));
  ezTranslationLookup::AddTranslator(std::move(pTranslatorEn));
  //ezTranslationLookup::AddTranslator(std::move(pTranslatorDe));

  ezTranslatorFromFiles::AddTranslationFile("ezEditorBasics.txt");

  LoadEditorPreferences();

  ezUIServices::GetSingleton()->LoadState();

  if (!bHeadless)
  {
    ezActionManager::LoadShortcutAssignment();

    LoadRecentFiles();

    CreatePanels();
  }

  LoadEditorPlugins();

  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();

  if (!bNoRecent && pPreferences->m_bLoadLastProjectAtStartup)
  {
    // first open the project, so that the data directory list is read
    if (!s_RecentProjects.GetFileList().IsEmpty())
    {
      CreateOrOpenProject(false, s_RecentProjects.GetFileList()[0]);
    }
  }

  if (!bHeadless)
  {
    if (ezQtDocumentWindow::GetAllDocumentWindows().IsEmpty())
    {
      ShowSettingsDocument();
    }
  }
}

void ezQtEditorApp::ShutdownEditor()
{
  SaveSettings();

  ezToolsProject::CloseProject();

  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::EngineProcessMsgHandler, this));
  ezToolsProject::s_Requests.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::ProjectRequestHandler, this));
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::ProjectEventHandler, this));
  ezDocument::s_EventsAny.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentEventHandler, this));
  ezDocumentManager::s_Requests.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentManagerRequestHandler, this));
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentManagerEventHandler, this));
  ezQtDocumentWindow::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentWindowEventHandler, this));

  ezUIServices::GetSingleton()->SaveState();

  CloseSettingsDocument();

  if (!m_bHeadless)
  {
    while (!ezContainerWindow::GetAllContainerWindows().IsEmpty())
    {
      delete ezContainerWindow::GetAllContainerWindows()[0];
    }
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
  ezQtImageCache::StopRequestProcessing(true);

  ezTranslationLookup::Clear();

  m_LogHTML.EndLog();

  EZ_DEFAULT_DELETE(m_pQtProgressbar);
  EZ_DEFAULT_DELETE(m_pProgressbar);
}



void ezQtEditorApp::CreatePanels()
{
  new ezQtLogPanel();
  new ezQtAssetBrowserPanel();
}


