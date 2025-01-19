#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/CheckVersion.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <QFileDialog>
#include <ToolsFoundation/Application/ApplicationServices.h>

EZ_IMPLEMENT_SINGLETON(ezQtEditorApp);

ezEvent<const ezEditorAppEvent&> ezQtEditorApp::m_Events;

ezQtEditorApp::ezQtEditorApp()
  : m_SingletonRegistrar(this)
  , m_RecentProjects(20)
  , m_RecentDocuments(100)
{
  m_bSavePreferencesAfterOpenProject = false;
  m_pVersionChecker = EZ_DEFAULT_NEW(ezQtVersionChecker);

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
  ezInt32 ret = m_pQtApplication->exec();
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

  RestartEngineProcessIfPluginsChanged(false);

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
    if (m_pVersionChecker->IsLatestNewer())
    {
      ezQtUiServices::GetSingleton()->MessageBoxInformation(
        ezFmt("<html>A new version is available: {}<br><br>Your version is: {}<br><br>Please check the <A "
              "href=\"https://github.com/ezEngine/ezEngine/releases\">Releases</A> for details.</html>",
          m_pVersionChecker->GetKnownLatestVersion(), m_pVersionChecker->GetOwnVersion()));
    }
    else
    {
      ezStringBuilder tmp("You have the latest version: \n");
      tmp.Append(m_pVersionChecker->GetOwnVersion());

      ezQtUiServices::GetSingleton()->MessageBoxInformation(tmp);
    }
  }

  if (m_pVersionChecker->IsLatestNewer())
  {
    ezQtUiServices::GetSingleton()->ShowGlobalStatusBarMessage(
      ezFmt("New version '{}' available, please update.", m_pVersionChecker->GetKnownLatestVersion()));
  }
}

void ezQtEditorApp::EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case ezEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (auto pTypeMsg = ezDynamicCast<const ezUpdateReflectionTypeMsgToEditor*>(e.m_pMsg))
      {
        ezPhantomRttiManager::RegisterType(pTypeMsg->m_desc);
      }
      if (auto pDynEnumMsg = ezDynamicCast<const ezDynamicStringEnumMsgToEditor*>(e.m_pMsg))
      {
        auto& dynEnum = ezDynamicStringEnum::CreateDynamicEnum(pDynEnumMsg->m_sEnumName);
        for (auto& sEnumValue : pDynEnumMsg->m_EnumValues)
        {
          dynEnum.AddValidValue(sEnumValue);
        }
        dynEnum.SortValues();
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
    m_pVersionChecker->Check(true);
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
        if (pWnd->SaveDocument().Failed())
          return;
      }
      // There might be no window for this document.
      else
      {
        pDoc->SaveDocument().LogFailure();
      }
    }
  }
}

bool ezQtEditorApp::IsProgressBarProcessingEvents() const
{
  return m_pQtProgressbar != nullptr && m_pQtProgressbar->IsProcessingEvents();
}

void ezQtEditorApp::OnDemandDynamicStringEnumLoad(ezStringView sEnumName, ezDynamicStringEnum& e)
{
  ezStringBuilder sFile;
  sFile.SetFormat(":project/Editor/{}.txt", sEnumName);

  // enums loaded this way are user editable
  e.SetStorageFile(sFile);
  e.ReadFromStorage();

  m_DynamicEnumStringsToClear.Insert(sEnumName);
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

ezStatus ezQtEditorApp::MakeRemoteProjectLocal(ezStringBuilder& inout_sFilePath)
{
  // already a local project?
  if (inout_sFilePath.EndsWith_NoCase("ezProject"))
    return ezStatus(EZ_SUCCESS);

  {
    ezStringBuilder tmp = inout_sFilePath;
    tmp.AppendPath("ezProject");

    if (ezOSFile::ExistsFile(tmp))
    {
      inout_sFilePath = tmp;
      return ezStatus(EZ_SUCCESS);
    }
  }

  EZ_LOG_BLOCK("Open Remote Project", inout_sFilePath.GetData());

  ezStringBuilder sRedirFile = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder(inout_sFilePath);
  sRedirFile.AppendPath("LocalCheckout.txt");

  // read redirection file, if available
  {
    ezOSFile file;
    if (file.Open(sRedirFile, ezFileOpenMode::Read).Succeeded())
    {
      ezDataBuffer content;
      file.ReadAll(content);

      const ezStringView sContent((const char*)content.GetData(), content.GetCount());

      if (sContent.EndsWith_NoCase("ezProject") && ezOSFile::ExistsFile(sContent))
      {
        inout_sFilePath = sContent;
        return ezStatus(EZ_SUCCESS);
      }
    }
  }

  ezString sName;
  ezString sType;
  ezString sUrl;
  ezString sProjectFile;

  // read the info about the remote project from the OpenDDL config file
  {
    ezOSFile file;
    if (file.Open(inout_sFilePath, ezFileOpenMode::Read).Failed())
    {
      return ezStatus(ezFmt("Remote project file '{}' doesn't exist.", inout_sFilePath));
    }

    ezDataBuffer content;
    file.ReadAll(content);

    ezMemoryStreamContainerWrapperStorage<ezDataBuffer> storage(&content);
    ezMemoryStreamReader reader(&storage);

    ezOpenDdlReader ddl;
    if (ddl.ParseDocument(reader).Failed())
    {
      return ezStatus("Error in remote project DDL config file");
    }

    if (auto pRoot = ddl.GetRootElement())
    {
      if (auto pProject = pRoot->FindChildOfType("RemoteProject"))
      {
        if (auto pName = pProject->FindChildOfType(ezOpenDdlPrimitiveType::String, "Name"))
        {
          sName = pName->GetPrimitivesString()[0];
        }
        if (auto pType = pProject->FindChildOfType(ezOpenDdlPrimitiveType::String, "Type"))
        {
          sType = pType->GetPrimitivesString()[0];
        }
        if (auto pUrl = pProject->FindChildOfType(ezOpenDdlPrimitiveType::String, "Url"))
        {
          sUrl = pUrl->GetPrimitivesString()[0];
        }
        if (auto pProjectFile = pProject->FindChildOfType(ezOpenDdlPrimitiveType::String, "ProjectFile"))
        {
          sProjectFile = pProjectFile->GetPrimitivesString()[0];
        }
      }
    }
  }

  if (sType.IsEmpty() || sName.IsEmpty())
  {
    return ezStatus(ezFmt("Remote project '{}' DDL configuration is invalid.", inout_sFilePath));
  }

  ezQtUiServices::GetSingleton()->MessageBoxInformation("This is a 'remote' project, meaning the data is not yet available on your machine.\n\nPlease select a folder where the project should be downloaded to.");

  static QString sPreviousFolder = ezOSFile::GetUserDocumentsFolder().GetData();

  QString sSelectedDir = QFileDialog::getExistingDirectory(QApplication::activeWindow(), QLatin1String("Choose Folder"), sPreviousFolder, QFileDialog::Option::ShowDirsOnly | QFileDialog::Option::DontResolveSymlinks);

  if (sSelectedDir.isEmpty())
  {
    return ezStatus("");
  }

  sPreviousFolder = sSelectedDir;
  ezStringBuilder sTargetDir = sSelectedDir.toUtf8().data();

  // if it is a git repository, clone it
  if (sType == "git" && !sUrl.IsEmpty())
  {
    QStringList args;
    args << "clone";
    args << ezMakeQString(sUrl);
    args << ezMakeQString(sName);

    QProcess proc;
    proc.setWorkingDirectory(sTargetDir.GetData());
    proc.setProcessChannelMode(QProcess::MergedChannels);

    ezProgressRange progress("Downloading Project", true);
    bool bRecursion = false;

    QObject::connect(&proc, &QProcess::readyReadStandardOutput, [&]()
      {
        if (bRecursion)
          return;

        bRecursion = true;

        auto data = proc.readAllStandardOutput();
        ezStringBuilder str = data.toStdString().c_str();
        if (const char* szPercent = str.FindLastSubString("%"))
        {
          str.SetSubString_FromTo(szPercent - 3, szPercent);
          str.Trim();

          ezInt32 p;
          if (ezConversionUtils::StringToInt(str, p).Succeeded())
          {
            progress.SetCompletion(p / 100.0f);
          }
        }

        if (progress.WasCanceled())
        {
          proc.close();
        }

        bRecursion = false;
        //
      });

    QApplication::setOverrideCursor(Qt::WaitCursor);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
    proc.start("git.exe", args);
#else
    proc.start("git", args);
#endif

    if (!proc.waitForStarted())
    {
      return ezStatus(ezFmt("Running 'git' to download the remote project failed."));
    }

    proc.waitForFinished(60 * 1000);
    QApplication::restoreOverrideCursor();

    if (proc.exitStatus() != QProcess::ExitStatus::NormalExit)
    {
      return ezStatus(ezFmt("Failed to git clone the remote project '{}' from '{}'", sName, sUrl));
    }

    ezLog::Success("Cloned remote project '{}' from '{}' to '{}'", sName, sUrl, sTargetDir);

    inout_sFilePath.SetFormat("{}/{}/{}", sTargetDir, sName, sProjectFile);

    // write redirection file
    {
      ezOSFile file;
      if (file.Open(sRedirFile, ezFileOpenMode::Write).Succeeded())
      {
        file.Write(inout_sFilePath.GetData(), inout_sFilePath.GetElementCount()).AssertSuccess();
      }
    }

    return ezStatus(EZ_SUCCESS);
  }

  return ezStatus(ezFmt("Unknown remote project type '{}' or invalid URL '{}'", sType, sUrl));
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
  if (m_StartupFlags.IsAnySet(StartupFlags::Background | StartupFlags::Headless | StartupFlags::UnitTest))
    return;

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
  if (m_StartupFlags.IsAnySet(StartupFlags::Background | StartupFlags::Headless | StartupFlags::UnitTest))
    return;

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
  if (m_bWroteCrashIndicatorFile)
  {
    // orderly shutdown -> make sure the crash indicator file is gone
    ezStringBuilder sTemp = ezOSFile::GetTempDataFolder("ezEditor");
    sTemp.AppendPath("ezEditorCrashIndicator");
    ezOSFile::DeleteFile(sTemp).IgnoreResult();
    m_bWroteCrashIndicatorFile = false;
  }

  ezStringBuilder app;
  app = ezOSFile::GetApplicationDirectory();
  app.AppendPath("ezEditor");
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  app.Append(".exe");
#endif
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

  if (ezCommandLineUtils::GetGlobalInstance()->HasOption("-renderer"))
  {
    ezStringBuilder sRenderer = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer");
    args << "-renderer";
    args << sRenderer.GetData();
  }

  QProcess proc;
  proc.startDetached(QString::fromUtf8(app, app.GetElementCount()), args);
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
  msg.m_sPayload = "ReloadAllResources";
  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void ezQtEditorApp::OpenDemoDocument()
{
  auto* pCurator = ezAssetCurator::GetSingleton();
  auto assets = pCurator->GetKnownAssets();

  ezStringBuilder sBestDoc;

  for (auto it = assets->GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->m_Path.GetDataDirRelativePath().GetFileName().IsEqual_NoCase("Main"))
    {
      sBestDoc = it.Value()->m_Path.GetAbsolutePath();
      break;
    }
  }

  if (!sBestDoc.IsEmpty())
  {
    SlotQueuedOpenDocument(sBestDoc.GetData(), nullptr);
  }
}
