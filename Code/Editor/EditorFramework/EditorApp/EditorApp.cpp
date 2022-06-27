#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

EZ_IMPLEMENT_SINGLETON(ezQtEditorApp);

ezEvent<const ezEditorAppEvent&> ezQtEditorApp::m_Events;

ezQtEditorApp::ezQtEditorApp()
  : m_SingletonRegistrar(this)
  , s_RecentProjects(5)
  , s_RecentDocuments(50)
{
  m_bSavePreferencesAfterOpenProject = false;

  ezApplicationServices::GetSingleton()->SetApplicationName("ezEditor");

  m_pTimer = new QTimer(nullptr);
}

ezQtEditorApp::~ezQtEditorApp()
{
  delete m_pTimer;
  m_pTimer = nullptr;

  CloseSplashScreen();
}

ezInt32 ezQtEditorApp::RunEditor()
{
  ezInt32 ret = s_pQtApplication->exec();
  return ret;
}

void ezQtEditorApp::SlotTimedUpdate()
{
  if (ezToolsProject::IsProjectOpen())
  {
    if (ezEditorEngineProcessConnection::GetSingleton())
      ezEditorEngineProcessConnection::GetSingleton()->Update();

    ezAssetCurator::GetSingleton()->MainThreadTick(true);
  }
  ezTaskSystem::FinishFrameTasks();

  // Close the splash screen when we get to the first idle event
  CloseSplashScreen();

  Q_EMIT IdleEvent();

  RestartEngineProcessIfPluginsChanged();

  m_pTimer->start(1);
}

void ezQtEditorApp::SlotSaveSettings()
{
  SaveSettings();
}

void ezQtEditorApp::SlotVersionCheckCompleted(bool bNewVersionReleased, bool bForced)
{
  // Close the splash screen so it doesn't become the parent window of our message boxes.
  CloseSplashScreen();

  if (bForced || bNewVersionReleased)
  {
    if (m_VersionChecker.IsLatestNewer())
    {
      ezQtUiServices::GetSingleton()->MessageBoxInformation(
        ezFmt("<html>A new version is available: {}<br><br>Your version is: {}<br><br>Please check the <A "
              "href=\"https://github.com/ezEngine/ezEngine/releases\">Releases</A> for details.</html>",
          m_VersionChecker.GetKnownLatestVersion(), m_VersionChecker.GetOwnVersion()));
    }
    else
    {
      ezStringBuilder tmp("You have the latest version: \n");
      tmp.Append(m_VersionChecker.GetOwnVersion());

      ezQtUiServices::GetSingleton()->MessageBoxInformation(tmp);
    }
  }

  if (m_VersionChecker.IsLatestNewer())
  {
    ezQtUiServices::GetSingleton()->ShowGlobalStatusBarMessage(
      ezFmt("New version '{}' available, please update.", m_VersionChecker.GetKnownLatestVersion()));
  }
}

void ezQtEditorApp::EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case ezEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<ezUpdateReflectionTypeMsgToEditor>())
      {
        const ezUpdateReflectionTypeMsgToEditor* pMsg = static_cast<const ezUpdateReflectionTypeMsgToEditor*>(e.m_pMsg);
        ezPhantomRttiManager::RegisterType(pMsg->m_desc);
      }
      else if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<ezProjectReadyMsgToEditor>())
      {
        // This message is waited upon (blocking) but does not contain any data.
      }
    }
    break;

    case ezEditorEngineProcessConnection::Event::Type::ProcessRestarted:
      StoreEnginePluginModificationTimes();
      break;

    default:
      return;
  }
}

void ezQtEditorApp::UiServicesEvents(const ezQtUiServices::Event& e)
{
  if (e.m_Type == ezQtUiServices::Event::Type::CheckForUpdates)
  {
    m_VersionChecker.Check(true);
  }
}

void ezQtEditorApp::SaveAllOpenDocuments()
{
  for (auto pMan : ezDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : pMan->ezDocumentManager::GetAllOpenDocuments())
    {
      ezQtDocumentWindow* pWnd = ezQtDocumentWindow::FindWindowByDocument(pDoc);
      // Layers for example will share a window with the scene document and the window will always save the scene.
      if (pWnd && pWnd->GetDocument() == pDoc)
      {
        if (pWnd->SaveDocument().m_Result.Failed())
          return;
      }
      // There might be no window for this document.
      else
      {
        pDoc->SaveDocument();
      }
    }
  }
}

bool ezQtEditorApp::IsProgressBarProcessingEvents() const
{
  return m_pQtProgressbar != nullptr && m_pQtProgressbar->IsProcessingEvents();
}

void ezQtEditorApp::OnDemandDynamicStringEnumLoad(const char* szEnumName, ezDynamicStringEnum& e)
{
  ezStringBuilder sFile;
  sFile.Format(":project/Editor/{}.txt", szEnumName);

  // enums loaded this way are user editable
  e.SetStorageFile(sFile);
  e.ReadFromStorage();

  m_DynamicEnumStringsToClear.Insert(szEnumName);
}

bool ContainsPlugin(const ezDynamicArray<ezApplicationPluginConfig::PluginConfig>& all, const char* szPlugin)
{
  for (const ezApplicationPluginConfig::PluginConfig& one : all)
  {
    if (one.m_sAppDirRelativePath == szPlugin)
      return true;
  }

  return false;
}

ezResult ezQtEditorApp::AddBundlesInOrder(ezDynamicArray<ezApplicationPluginConfig::PluginConfig>& order, const ezPluginBundleSet& bundles, const ezString& start, bool bEditor, bool bEditorEngine, bool bRuntime) const
{
  const ezPluginBundle& bundle = bundles.m_Plugins.Find(start).Value();

  for (const ezString& req : bundle.m_RequiredBundles)
  {
    auto it = bundles.m_Plugins.Find(req);

    if (!it.IsValid())
    {
      ezLog::Error("Plugin bundle '{}' has a dependency on bundle '{}' which does not exist.", start, req);
      return EZ_FAILURE;
    }

    EZ_SUCCEED_OR_RETURN(AddBundlesInOrder(order, bundles, req, bEditor, bEditorEngine, bRuntime));
  }

  if (bRuntime)
  {
    for (const ezString& dll : bundle.m_RuntimePlugins)
    {
      if (!ContainsPlugin(order, dll))
      {
        ezApplicationPluginConfig::PluginConfig& p = order.ExpandAndGetRef();
        p.m_sAppDirRelativePath = dll;
        p.m_bLoadCopy = bundle.m_bLoadCopy;
      }
    }
  }

  if (bEditorEngine)
  {
    for (const ezString& dll : bundle.m_EditorEnginePlugins)
    {
      if (!ContainsPlugin(order, dll))
      {
        ezApplicationPluginConfig::PluginConfig& p = order.ExpandAndGetRef();
        p.m_sAppDirRelativePath = dll;
        p.m_bLoadCopy = bundle.m_bLoadCopy;
      }
    }
  }

  if (bEditor)
  {
    for (const ezString& dll : bundle.m_EditorPlugins)
    {
      if (!ContainsPlugin(order, dll))
      {
        ezApplicationPluginConfig::PluginConfig& p = order.ExpandAndGetRef();
        p.m_sAppDirRelativePath = dll;
        p.m_bLoadCopy = bundle.m_bLoadCopy;
      }
    }
  }

  return EZ_SUCCESS;
}

bool ezQtEditorApp::ExistsPluginSelectionStateDDL(const char* szProjectDir /*= ":project"*/)
{
  ezStringBuilder path = szProjectDir;
  path.MakeCleanPath();

  if (path.EndsWith_NoCase("/ezProject"))
    path.PathParentDirectory();

  path.AppendPath("Editor/PluginSelection.ddl");

  return ezOSFile::ExistsFile(path);
}

void ezQtEditorApp::WritePluginSelectionStateDDL(const char* szProjectDir /*= ":project"*/)
{
  ezStringBuilder path = szProjectDir;
  path.AppendPath("Editor/PluginSelection.ddl");

  ezFileWriter file;
  file.Open(path).AssertSuccess();

  ezOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  m_PluginBundles.WriteStateToDDL(ddl);
}

void ezQtEditorApp::CreatePluginSelectionDDL(const char* szProjectFile, const char* szTemplate)
{
  ezStringBuilder sPath = szProjectFile;
  sPath.PathParentDirectory();

  for (auto it : m_PluginBundles.m_Plugins)
  {
    ezPluginBundle& bundle = it.Value();

    bundle.m_bSelected = bundle.m_EnabledInTemplates.Contains(szTemplate);
  }

  WritePluginSelectionStateDDL(sPath);
}

void ezQtEditorApp::LoadPluginBundleDlls(const char* szProjectFile)
{
  ezStringBuilder sPath = szProjectFile;
  sPath.PathParentDirectory();
  sPath.AppendPath("Editor/PluginSelection.ddl");

  ezFileReader file;
  if (file.Open(sPath).Succeeded())
  {
    ezOpenDdlReader ddl;
    if (ddl.ParseDocument(file).Failed())
    {
      ezLog::Error("Syntax error in plugin bundle file '{}'", sPath);
    }
    else
    {
      auto pState = ddl.GetRootElement()->FindChildOfType("PluginState");
      while (pState)
      {
        if (auto pName = pState->FindChildOfType(ezOpenDdlPrimitiveType::String, "ID"))
        {
          const ezString sID = pName->GetPrimitivesString()[0];

          bool bExisted = false;
          auto itPlug = m_PluginBundles.m_Plugins.FindOrAdd(sID, &bExisted);

          if (!bExisted)
          {
            ezPluginBundle& bundle = itPlug.Value();
            bundle.m_bMissing = true;
            bundle.m_sDisplayName = sID;
            bundle.m_sDescription = "This plugin bundle is referenced by the project, but doesn't exist. Please check that all plugins are built correctly and their respective *.ezPluginBundle files are copied to the binary directory.";
            bundle.m_LastModificationTime = ezTimestamp::CurrentTimestamp();
          }
        }

        pState = pState->GetSibling();
      }

      m_PluginBundles.ReadStateFromDDL(ddl);
    }
  }

  ezDynamicArray<ezApplicationPluginConfig::PluginConfig> order;

  // first all the mandatory bundles
  for (auto it : m_PluginBundles.m_Plugins)
  {
    if (!it.Value().m_bMandatory)
      continue;

    if (AddBundlesInOrder(order, m_PluginBundles, it.Key(), true, false, false).Failed())
    {
      ezQtUiServices::MessageBoxWarning("The mandatory plugin bundles have non-existing dependencies. Please make sure all plugins are properly built and the ezPluginBundle files correctly reference each other.");

      return;
    }
  }

  // now the non-mandatory bundles
  for (auto it : m_PluginBundles.m_Plugins)
  {
    if (it.Value().m_bMandatory || !it.Value().m_bSelected)
      continue;

    if (AddBundlesInOrder(order, m_PluginBundles, it.Key(), true, false, false).Failed())
    {
      ezQtUiServices::MessageBoxWarning("The plugin bundles have non-existing dependencies. Please make sure all plugins are properly built and the ezPluginBundle files correctly reference each other.");

      return;
    }
  }

  ezSet<ezString> NotLoaded;
  for (const ezApplicationPluginConfig::PluginConfig& it : order)
  {
    if (ezPlugin::LoadPlugin(it.m_sAppDirRelativePath, it.m_bLoadCopy ? ezPluginLoadFlags::LoadCopy : ezPluginLoadFlags::Default).Failed())
    {
      NotLoaded.Insert(it.m_sAppDirRelativePath);
    }
  }

  if (!NotLoaded.IsEmpty())
  {
    ezStringBuilder s = "The following plugins could not be loaded. Scenes may not load correctly.\n\n";

    for (auto it = NotLoaded.GetIterator(); it.IsValid(); ++it)
    {
      s.AppendFormat(" '{0}' \n", it.Key());
    }

    ezQtUiServices::MessageBoxWarning(s);
  }
}

void ezQtEditorApp::LaunchEditor(const char* szProject, bool bCreate)
{
  ezStringBuilder app;
  app = ezOSFile::GetApplicationDirectory();
  app.AppendPath("Editor.exe");
  app.MakeCleanPath();

  // TODO: pass through all command line arguments ?

  QStringList args;
  args << "-nosplash";
  args << (bCreate ? "-newproject" : "-project");
  args << QString::fromUtf8(szProject);

  if (m_StartupFlags.IsSet(StartupFlags::SafeMode))
    args << "-safe";
  if (m_StartupFlags.IsSet(StartupFlags::NoRecent))
    args << "-noRecent";
  if (m_StartupFlags.IsSet(StartupFlags::Debug))
    args << "-debug";

  QProcess proc;
  proc.startDetached(QString::fromUtf8(app), args);
}

const ezApplicationPluginConfig ezQtEditorApp::GetRuntimePluginConfig(bool bIncludeEditorPlugins) const
{
  ezApplicationPluginConfig cfg;

  ezHybridArray<ezString, 16> order;
  for (auto it : m_PluginBundles.m_Plugins)
  {
    if (it.Value().m_bMandatory || it.Value().m_bSelected)
    {
      AddBundlesInOrder(cfg.m_Plugins, m_PluginBundles, it.Key(), false, bIncludeEditorPlugins, true).IgnoreResult();
    }
  }

  return cfg;
}

void ezQtEditorApp::ReloadEngineResources()
{
  ezSimpleConfigMsgToEngine msg;
  msg.m_sWhatToDo = "ReloadResources";
  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
