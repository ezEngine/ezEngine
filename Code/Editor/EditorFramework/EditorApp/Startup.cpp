#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/GameObjectContextActions.h>
#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorFramework/CodeGen/CodeEditorPreferencesWidget.moc.h>
#include <EditorFramework/CodeGen/CompilerPreferencesWidget.moc.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/CheckVersion.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/EditorApp/StackTraceLogParser.h>
#include <EditorFramework/GUI/DynamicDefaultStateProvider.h>
#include <EditorFramework/GUI/ExposedParametersDefaultStateProvider.h>
#include <EditorFramework/Manipulators/BoneManipulatorAdapter.h>
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
#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <EditorFramework/Panels/LongOpsPanel/LongOpsPanel.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/DynamicEnumPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/ExposedBoneWidget.moc.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/FileBrowserPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/GameObjectReferencePropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/RttiTypeStringPropertyWidget.moc.h>
#include <EditorFramework/Visualizers/BoxVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CameraVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CapsuleVisualizerAdapter.h>
#include <EditorFramework/Visualizers/ConeVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CylinderVisualizerAdapter.h>
#include <EditorFramework/Visualizers/DirectionVisualizerAdapter.h>
#include <EditorFramework/Visualizers/SphereVisualizerAdapter.h>
#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/ETW.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <QSvgRenderer>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ads/DockManager.h>

void ezCompilerPreferences_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);
void ezCodeEditorPreferences_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, EditorFrameworkMain)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "GuiFoundation",
    "PropertyGrid",
    "ManipulatorAdapterRegistry",
    "DefaultState"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezDefaultState::RegisterDefaultStateProvider(ezExposedParametersDefaultStateProvider::CreateProvider);
    ezDefaultState::RegisterDefaultStateProvider(ezDynamicDefaultStateProvider::CreateProvider);
    ezProjectActions::RegisterActions();
    ezAssetActions::RegisterActions();
    ezViewActions::RegisterActions();
    ezViewLightActions::RegisterActions();
    ezGameObjectContextActions::RegisterActions();
    ezGameObjectDocumentActions::RegisterActions();
    ezGameObjectSelectionActions::RegisterActions();
    ezQuadViewActions::RegisterActions();
    ezTransformGizmoActions::RegisterActions();
    ezTranslateGizmoAction::RegisterActions();
    ezCommonAssetActions::RegisterActions();

    // Default Asset Menu Bar
    // All asset menu bar mappings should derive from this to allow for actions to be defined that show up in every asset document editor's menu bar.
    {
      const char* szMenuBar = "AssetMenuBar";
      ezActionMapManager::RegisterActionMap(szMenuBar);
      ezStandardMenus::MapActions(szMenuBar, ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
      ezProjectActions::MapActions(szMenuBar);
      ezDocumentActions::MapMenuActions(szMenuBar);
      ezAssetActions::MapMenuActions(szMenuBar);
      ezCommandHistoryActions::MapActions(szMenuBar);
    }

    // Default Asset Toolbar
    // All asset toolbar mappings should derive from this to allow for actions to be defined that show up in every asset document editor's tool bar.
    {
      const char* szToolbar = "AssetToolbar";
      ezActionMapManager::RegisterActionMap(szToolbar);

      ezDocumentActions::MapToolbarActions(szToolbar);
      ezCommandHistoryActions::MapActions(szToolbar, "");
      ezAssetActions::MapToolBarActions(szToolbar, true);
    }

    // Default Asset View Toolbar
    // All asset view toolbar mappings should derive from this or its derived "SimpleAssetViewToolbar" to allow for actions to be defined that show up in every asset document editor's view toolbar.
    {
      ezActionMapManager::RegisterActionMap("AssetViewToolbar");
      // Convenience mapping that adds the most common view settings:
      const char* szSimpleViewToolbar = "SimpleAssetViewToolbar";
      ezActionMapManager::RegisterActionMap(szSimpleViewToolbar, "AssetViewToolbar");
      ezViewActions::MapToolbarActions(szSimpleViewToolbar, ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
      ezViewLightActions::MapToolbarActions(szSimpleViewToolbar);
    }

    ezActionMapManager::RegisterActionMap("SettingsTabMenuBar");
    ezStandardMenus::MapActions("SettingsTabMenuBar", ezStandardMenuTypes::Default);
    ezProjectActions::MapActions("SettingsTabMenuBar");

    ezActionMapManager::RegisterActionMap("AssetBrowserToolBar");
    ezAssetActions::MapToolBarActions("AssetBrowserToolBar", false);

    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezFileBrowserAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtFilePropertyWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezExternalFileBrowserAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtExternalFilePropertyWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezAssetBrowserAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtAssetPropertyWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezDynamicEnumAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtDynamicEnumPropertyWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezDynamicStringEnumAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtDynamicStringEnumPropertyWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezExposedParametersAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtExposedParametersPropertyWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezGameObjectReferenceAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtGameObjectReferencePropertyWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezExposedBone>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtExposedBoneWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezCompilerPreferences>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtCompilerPreferencesWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezCodeEditorPreferences>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtCodeEditorPreferencesWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezImageSliderUiAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtPropertyEditorSliderWidget(); });
    ezQtPropertyGridWidget::GetFactory().RegisterCreator(ezGetStaticRTTI<ezRttiTypeStringAttribute>(), [](const ezRTTI* pRtti)->ezQtPropertyWidget* { return new ezQtRttiTypeStringPropertyWidget(); });

    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezSphereManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezSphereManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezCapsuleManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezCapsuleManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezBoxManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezBoxManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezConeAngleManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezConeAngleManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezConeLengthManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezConeLengthManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezNonUniformBoxManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezNonUniformBoxManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezTransformManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezTransformManipulatorAdapter); });
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezBoneManipulatorAttribute>(), [](const ezRTTI* pRtti)->ezManipulatorAdapter* { return EZ_DEFAULT_NEW(ezBoneManipulatorAdapter); });

    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezBoxVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezBoxVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezSphereVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezSphereVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezCapsuleVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezCapsuleVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezCylinderVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezCylinderVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezDirectionVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezDirectionVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezConeVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezConeVisualizerAdapter); });
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezCameraVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezCameraVisualizerAdapter); });

    ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezCompilerPreferences_PropertyMetaStateEventHandler);
    ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezCodeEditorPreferences_PropertyMetaStateEventHandler);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezDefaultState::UnregisterDefaultStateProvider(ezExposedParametersDefaultStateProvider::CreateProvider);
    ezDefaultState::UnregisterDefaultStateProvider(ezDynamicDefaultStateProvider::CreateProvider);
    ezProjectActions::UnregisterActions();
    ezAssetActions::UnregisterActions();
    ezViewActions::UnregisterActions();
    ezViewLightActions::UnregisterActions();
    ezGameObjectContextActions::UnregisterActions();
    ezGameObjectDocumentActions::UnregisterActions();
    ezGameObjectSelectionActions::UnregisterActions();
    ezQuadViewActions::UnregisterActions();
    ezTransformGizmoActions::UnregisterActions();
    ezTranslateGizmoAction::UnregisterActions();
    ezCommonAssetActions::UnregisterActions();

    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezFileBrowserAttribute>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezExternalFileBrowserAttribute>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezAssetBrowserAttribute>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezDynamicEnumAttribute>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezDynamicStringEnumAttribute>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezGameObjectReferenceAttribute>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezExposedParametersAttribute>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezExposedBone>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezCompilerPreferences>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezCodeEditorPreferences>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezImageSliderUiAttribute>());
    ezQtPropertyGridWidget::GetFactory().UnregisterCreator(ezGetStaticRTTI<ezRttiTypeStringAttribute>());

    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezSphereManipulatorAttribute>());
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezCapsuleManipulatorAttribute>());
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezBoxManipulatorAttribute>());
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezConeAngleManipulatorAttribute>());
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezConeLengthManipulatorAttribute>());
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezNonUniformBoxManipulatorAttribute>());
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezTransformManipulatorAttribute>());
    ezManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezBoneManipulatorAttribute>());

    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezBoxVisualizerAttribute>());
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezSphereVisualizerAttribute>());
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezCapsuleVisualizerAttribute>());
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezCylinderVisualizerAttribute>());
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezDirectionVisualizerAttribute>());
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezConeVisualizerAttribute>());
    ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(ezGetStaticRTTI<ezCameraVisualizerAttribute>());

    ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezCompilerPreferences_PropertyMetaStateEventHandler);
    ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezCodeEditorPreferences_PropertyMetaStateEventHandler);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezCommandLineOptionBool opt_Safe("_Editor", "-safe", "In safe-mode the editor minimizes the risk of crashing, for instance by not loading previous projects and scenes.", false);
ezCommandLineOptionBool opt_NoRecent("_Editor", "-noRecent", "Disables automatic loading of recent projects and documents.", false);

void ezQtEditorApp::StartupEditor()
{
  {
    ezStringBuilder sTemp = ezOSFile::GetTempDataFolder("ezEditor");
    sTemp.AppendPath("ezEditorCrashIndicator");

    if (ezOSFile::ExistsFile(sTemp))
    {
      ezOSFile::DeleteFile(sTemp).IgnoreResult();

      if (ezQtUiServices::GetSingleton()->MessageBoxQuestion("It seems the editor ran into problems last time.\n\nDo you want to run it in safe mode, to deactivate automatic project loading and document restoration?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
      {
        opt_Safe.GetOptions(sTemp);
        ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument(sTemp);
      }
    }
  }

  ezBitflags<StartupFlags> startupFlags;

  startupFlags.AddOrRemove(StartupFlags::SafeMode, opt_Safe.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified));
  startupFlags.AddOrRemove(StartupFlags::NoRecent, opt_NoRecent.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified));

  StartupEditor(startupFlags);
}

void ezQtEditorApp::StartupEditor(ezBitflags<StartupFlags> startupFlags, const char* szUserDataFolder)
{
  EZ_PROFILE_SCOPE("StartupEditor");

  QCoreApplication::setOrganizationDomain("www.ezengine.net");
  QCoreApplication::setOrganizationName("ezEngine Project");
  QCoreApplication::setApplicationName(ezApplication::GetApplicationInstance()->GetApplicationName().GetData());
  QCoreApplication::setApplicationVersion("1.0.0");

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

  const ezString sApplicationName = pCmd->GetStringOption("-appname", 0, ezApplication::GetApplicationInstance()->GetApplicationName());
  ezApplication::GetApplicationInstance()->SetApplicationName(sApplicationName);

  QLocale::setDefault(QLocale(QLocale::English));

  m_pEngineViewProcess = new ezEditorEngineProcessConnection;

  m_LongOpControllerManager.Startup(&m_pEngineViewProcess->GetCommunicationChannel());

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
  ezQtUiServices::s_Events.AddEventHandler(ezMakeDelegate(&ezQtEditorApp::UiServicesEvents, this));

  ezStartup::StartupCoreSystems();

  // prevent restoration of window layouts when in safe mode
  ezQtDocumentWindow::s_bAllowRestoreWindowLayout = !IsInSafeMode();

  {
    // Make sure that we have at least 4 worker threads for short running and 4 worker threads for long running tasks.
    // Otherwise the Editor might deadlock during asset transform.
    ezInt32 iLongThreads = ezMath::Max(4, (ezInt32)ezTaskSystem::GetNumAllocatedWorkerThreads(ezWorkerThreadType::LongTasks));
    ezInt32 iShortThreads = ezMath::Max(4, (ezInt32)ezTaskSystem::GetNumAllocatedWorkerThreads(ezWorkerThreadType::ShortTasks));
    ezTaskSystem::SetWorkerThreadCount(iShortThreads, iLongThreads);
  }

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

    ezFileSystem::AddDataDirectory("", "AbsPaths", ":", ezDataDirUsage::AllowWrites).IgnoreResult();             // for absolute paths
    ezFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", ezDataDirUsage::AllowWrites).IgnoreResult();     // writing to the binary directory
    ezFileSystem::AddDataDirectory(sAppDir, "AppData", "app").IgnoreResult();                                    // app specific data
    ezFileSystem::AddDataDirectory(sUserData, "AppData", "appdata", ezDataDirUsage::AllowWrites).IgnoreResult(); // for writing app user data
  }

  {
    EZ_PROFILE_SCOPE("Logging");
    ezInt32 iApplicationID = pCmd->GetIntOption("-appid", 0);
    ezStringBuilder sLogFile;
    sLogFile.SetFormat(":appdata/Log_{0}.htm", iApplicationID);
    m_LogHTML.BeginLog(sLogFile, sApplicationName);

    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLoggingEvent::Handler(&ezLogWriter::HTML::LogMessageHandler, &m_LogHTML));
    ezGlobalLog::AddLogWriter(ezETW::LogMessageHandler);
  }
  ezUniquePtr<ezTranslatorFromFiles> pTranslatorEn = EZ_DEFAULT_NEW(ezTranslatorFromFiles);
  m_pTranslatorFromFiles = pTranslatorEn.Borrow();

  // ezUniquePtr<ezTranslatorFromFiles> pTranslatorDe = EZ_DEFAULT_NEW(ezTranslatorFromFiles);

  pTranslatorEn->AddTranslationFilesFromFolder(":app/Localization/en");
  // pTranslatorDe->LoadTranslationFilesFromFolder(":app/Localization/de");

  ezTranslationLookup::AddTranslator(EZ_DEFAULT_NEW(ezTranslatorMakeMoreReadable));
  // ezTranslationLookup::AddTranslator(EZ_DEFAULT_NEW(ezTranslatorLogMissing));
  ezTranslationLookup::AddTranslator(std::move(pTranslatorEn));
  // ezTranslationLookup::AddTranslator(std::move(pTranslatorDe));

  LoadEditorPreferences();
  ezCppProject::LoadPreferences();

  ezQtUiServices::GetSingleton()->LoadState();

  if (!IsInHeadlessMode())
  {
    ezActionManager::LoadShortcutAssignment();

    LoadRecentFiles();

    ShowSettingsDocument();

    CreatePanels();

    if (!IsInUnitTestMode())
    {
      connect(m_pVersionChecker.Borrow(), &ezQtVersionChecker::VersionCheckCompleted, this, &ezQtEditorApp::SlotVersionCheckCompleted, Qt::QueuedConnection);

      m_pVersionChecker->Initialize();
      m_pVersionChecker->Check(false);
    }
  }

  LoadEditorPlugins();
  CloseSplashScreen();

  m_bIsRunning = true;
  {
    ezEditorAppEvent e;
    e.m_Type = ezEditorAppEvent::Type::EditorStarted;
    m_Events.Broadcast(e);
  }


  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();

  if (pCmd->GetStringOptionArguments("-newproject") > 0)
  {
    CreateOrOpenProject(true, pCmd->GetAbsolutePathOption("-newproject")).IgnoreResult();
  }
  else if (pCmd->GetStringOptionArguments("-project") > 0)
  {
    for (ezUInt32 doc = 0; doc < pCmd->GetStringOptionArguments("-documents"); ++doc)
    {
      m_DocumentsToOpen.PushBack(pCmd->GetStringOption("-documents", doc));
    }

    CreateOrOpenProject(false, pCmd->GetAbsolutePathOption("-project")).IgnoreResult();
  }
  else if (!bNoRecent && pPreferences->m_bLoadLastProjectAtStartup)
  {
    if (!m_RecentProjects.GetFileList().IsEmpty())
    {
      CreateOrOpenProject(false, m_RecentProjects.GetFileList()[0].m_File).IgnoreResult();
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

  if (m_bWroteCrashIndicatorFile)
  {
    QTimer::singleShot(1000, [this]()
      {
        ezStringBuilder sTemp = ezOSFile::GetTempDataFolder("ezEditor");
        sTemp.AppendPath("ezEditorCrashIndicator");
        ezOSFile::DeleteFile(sTemp).IgnoreResult();
        m_bWroteCrashIndicatorFile = false;
        //
      });
  }

  if (m_StartupFlags.AreNoneSet(StartupFlags::Headless | StartupFlags::UnitTest) && !ezToolsProject::GetSingleton()->IsProjectOpen())
  {
    GuiOpenDashboard();
  }

  ezStackTraceLogParser::Register();
}

void ezQtEditorApp::ShutdownEditor()
{
  m_bIsRunning = false;
  ezStackTraceLogParser::Unregister();

  ezToolsProject::SaveProjectState();

  m_pTimer->stop();

  ezToolsProject::CloseProject();

  m_LongOpControllerManager.Shutdown();

  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::EngineProcessMsgHandler, this));
  ezToolsProject::s_Requests.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::ProjectRequestHandler, this));
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::ProjectEventHandler, this));
  ezDocument::s_EventsAny.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentEventHandler, this));
  ezDocumentManager::s_Requests.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentManagerRequestHandler, this));
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtEditorApp::DocumentManagerEventHandler, this));
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
      delete pPanel;
    }
  }


  QCoreApplication::sendPostedEvents();
  qApp->processEvents();

  delete m_pEngineViewProcess;

  // Unload potential plugin referenced clipboard data to prevent crash on shutdown.
  QApplication::clipboard()->clear();
  ezPlugin::UnloadAllPlugins();

  if (m_bWroteCrashIndicatorFile)
  {
    // orderly shutdown -> make sure the crash indicator file is gone
    ezStringBuilder sTemp = ezOSFile::GetTempDataFolder("ezEditor");
    sTemp.AppendPath("ezEditorCrashIndicator");
    ezOSFile::DeleteFile(sTemp).IgnoreResult();
    m_bWroteCrashIndicatorFile = false;
  }

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

  ezQtContainerWindow* pMainWnd = ezQtContainerWindow::GetContainerWindow();
  ads::CDockManager* pDockManager = pMainWnd->GetDockManager();

  ezQtApplicationPanel* pAssetBrowserPanel = new ezQtAssetBrowserPanel(pDockManager);
  ezQtApplicationPanel* pAssetCuratorPanel = new ezQtAssetCuratorPanel(pDockManager);
  ezQtApplicationPanel* pLogPanel = new ezQtLogPanel(pDockManager);
  ezQtApplicationPanel* pCVarPanel = new ezQtCVarPanel(pDockManager);
  ezQtApplicationPanel* pLongOpsPanel = new ezQtLongOpsPanel(pDockManager);

  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pAssetBrowserPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pAssetCuratorPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pLogPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pCVarPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pLongOpsPanel);

  // by default these panels can be hidden
  pCVarPanel->toggleView(false);
  pLongOpsPanel->toggleView(false);

  pAssetBrowserPanel->raise();
}

ezCommandLineOptionBool opt_NoSplashScreen("_Editor", "-NoSplash", "Disables the editor splash-screen", false);

void ezQtEditorApp::SetupAndShowSplashScreen()
{
  EZ_ASSERT_DEV(m_pSplashScreen == nullptr, "Splash screen shouldn't exist already.");

  if (m_StartupFlags.IsAnySet(ezQtEditorApp::StartupFlags::UnitTest))
    return;

  if (opt_NoSplashScreen.GetOptionValue(ezCommandLineOption::LogMode::Never))
    return;

  bool bShowSplashScreen = true;

  // preferences are not yet available here
  {
    QSettings s;
    s.beginGroup("EditorPreferences");
    bShowSplashScreen = s.value("ShowSplashscreen", true).toBool();
    s.endGroup();
  }

  if (!bShowSplashScreen)
    return;

  // QSvgRenderer svgRenderer(QString(":/Splash/Splash/splash.svg"));

  // const qreal PixelRatio = qApp->primaryScreen()->devicePixelRatio();

  //// TODO: When migrating to Qt 5.15 or newer this should have a fixed square size and
  //// let the aspect ratio mode of the svg renderer handle the difference
  // QPixmap splashPixmap(QSize(187, 256) * PixelRatio);
  // splashPixmap.fill(Qt::transparent);
  //{
  //   QPainter painter;
  //   painter.begin(&splashPixmap);
  //   svgRenderer.render(&painter);
  //   painter.end();
  // }

  QPixmap splashPixmap(QString(":/Splash/Splash/splash.png"));

  // splashPixmap.setDevicePixelRatio(PixelRatio);

  m_pSplashScreen = new QSplashScreen(splashPixmap);
  m_pSplashScreen->setMask(splashPixmap.mask());

  // Don't set always on top if a debugger is attached to prevent it being stuck over the debugger.
  if (!ezSystemInformation::IsDebuggerAttached())
  {
    m_pSplashScreen->setWindowFlag(Qt::WindowStaysOnTopHint, true);
  }
  m_pSplashScreen->show();
}

void ezQtEditorApp::CloseSplashScreen()
{
  if (!m_pSplashScreen)
    return;

  EZ_ASSERT_DEBUG(QThread::currentThread() == this->thread(), "CloseSplashScreen must be called from the main thread");
  QSplashScreen* pLocalSplashScreen = m_pSplashScreen;
  m_pSplashScreen = nullptr;

  pLocalSplashScreen->finish(ezQtContainerWindow::GetContainerWindow());
  // if the deletion is done 'later', the splashscreen can end up as the parent window of other things
  // like messageboxes, and then the deletion will make the app crash
  delete pLocalSplashScreen;
}
