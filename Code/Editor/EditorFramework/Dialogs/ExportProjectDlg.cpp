#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/ExportProjectDlg.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/String.h>
#include <QFileDialog>

struct ezPathPattern
{
  enum MatchType : ezUInt8
  {
    Exact,
    StartsWith,
    EndsWith,
    Contains
  };

  MatchType m_MatchType = MatchType::Exact;
  ezString m_sString;

  void Configure(ezStringView text)
  {
    text.Trim(" \t\r\n");

    const bool bStart = text.StartsWith("*");
    const bool bEnd = text.EndsWith("*");

    text.Trim("*");
    m_sString = text;

    if (bStart && bEnd)
      m_MatchType = MatchType::Contains;
    else if (bStart)
      m_MatchType = MatchType::EndsWith;
    else if (bEnd)
      m_MatchType = MatchType::StartsWith;
    else
      m_MatchType = MatchType::Exact;
  }

  bool Matches(ezStringView text) const
  {
    switch (m_MatchType)
    {
      case MatchType::Exact:
        return text.IsEqual_NoCase(m_sString.GetView());
      case MatchType::StartsWith:
        return text.StartsWith_NoCase(m_sString);
      case MatchType::EndsWith:
        return text.EndsWith_NoCase(m_sString);
      case MatchType::Contains:
        return text.FindSubString_NoCase(m_sString) != nullptr;
    }

    EZ_ASSERT_NOT_IMPLEMENTED;
    return false;
  }
};

struct ezPathPatternFilter
{
  ezDynamicArray<ezPathPattern> m_ExcludePatterns;
  ezDynamicArray<ezPathPattern> m_IncludePatterns;

  bool PassesFilters(ezStringView text) const
  {
    for (const auto& filter : m_IncludePatterns)
    {
      // if any include pattern matches, that overrides the exclude patterns
      if (filter.Matches(text))
        return true;
    }

    for (const auto& filter : m_ExcludePatterns)
    {
      // no include pattern matched, but any exclude pattern matches -> filter out
      if (filter.Matches(text))
        return false;
    }

    // no filter matches at all -> include by default
    return true;
  }

  void AddFilter(ezStringView sText, bool bIncludeFilter)
  {
    ezStringBuilder text = sText;
    text.MakeCleanPath();
    text.Trim(" \t\r\n");

    if (text.IsEmpty() || text.StartsWith("//"))
      return;

    if (!text.StartsWith("*") && !text.StartsWith("/"))
      text.Prepend("/");

    if (bIncludeFilter)
      m_IncludePatterns.ExpandAndGetRef().Configure(text);
    else
      m_ExcludePatterns.ExpandAndGetRef().Configure(text);
  }

  ezResult ReadConfigFile(const char* szFile)
  {
    ezStringBuilder content;

    ezPreprocessor pp;
    pp.SetPassThroughLine(false);
    pp.SetPassThroughPragma(false);
    if (pp.Process(szFile, content, true, true).Failed())
      return EZ_FAILURE; // TODO: error reporting

    ezDynamicArray<ezStringView> lines;

    content.Split(false, lines, "\n", "\r");

    bool bIncludeFilter = false;

    for (auto line : lines)
    {
      if (line.IsEqual_NoCase("[INCLUDE]"))
      {
        bIncludeFilter = true;
        continue;
      }

      if (line.IsEqual_NoCase("[EXCLUDE]"))
      {
        bIncludeFilter = false;
        continue;
      }

      AddFilter(line, bIncludeFilter);
    }

    return EZ_SUCCESS;
  }
};


bool ezQtExportProjectDlg::s_bTransformAll = true;

ezQtExportProjectDlg::ezQtExportProjectDlg(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

  ezProjectPreferencesUser* pPref = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();

  Destination->setText(pPref->m_sExportFolder.GetData());
}

void ezQtExportProjectDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  TransformAll->setChecked(s_bTransformAll);
}

void ezQtExportProjectDlg::on_BrowseDestination_clicked()
{
  QString sPath = QFileDialog::getExistingDirectory(this, QLatin1String("Select output directory"), Destination->text());

  if (!sPath.isEmpty())
  {
    Destination->setText(sPath);
    ezProjectPreferencesUser* pPref = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();
    pPref->m_sExportFolder = sPath.toUtf8().data();
  }
}

void ezQtExportProjectDlg::on_ExportProjectButton_clicked()
{
  // TODO:
  // filter out unused runtime/game plugins
  // better logic to filter out input assets
  // blacklist / whitelist for files per data directory
  // output log to UI
  // asset profile
  // copy inputs into resource: RML files
  // code cleanup

  if (TransformAll->isChecked())
  {
    ezStatus stat = ezAssetCurator::GetSingleton()->TransformAllAssets(ezTransformFlags::None);

    if (stat.Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxStatus(stat, "Asset transform failed");
      return;
    }
  }

  ezProgressRange mainProgress("Export Project", 6, true);
  mainProgress.SetStepWeighting(0, 0.05f); // Preparing output folder
  mainProgress.SetStepWeighting(1, 0.10f); // Scanning data directories
  mainProgress.SetStepWeighting(2, 0.10f); // Filtering files
  mainProgress.SetStepWeighting(3, 0.05f); // Gathering binaries
  mainProgress.SetStepWeighting(4, 0.01f); // Writing data directory config
  // mainProgress.SetStepWeighting(5, 0.0f); // Copying files

  const ezString szDstFolder = Destination->text().toUtf8().data();

  {
    mainProgress.BeginNextStep("Preparing output folder");

    if (ezOSFile::DeleteFolder(szDstFolder).Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to remove destination folder:\n'{}'", szDstFolder));
      return;
    }

    if (ezOSFile::CreateDirectoryStructure(szDstFolder).Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to create destination folder:\n'{}'", szDstFolder));
      return;
    }
  }

  ezStringBuilder sTemp;

  ezOSFile logFile;

  sTemp.Set(szDstFolder, "/ExportLog.txt");
  if (logFile.Open(sTemp, ezFileOpenMode::Write).Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to create log file '{0}'", sTemp));
    return;
  }

  const auto dataDirs = ezQtEditorApp::GetSingleton()->GetFileSystemConfig();

  ezStringBuilder sPath, sRelPath, sStartPath;

  struct DataDirInfo
  {
    ezString m_sTargetDirPath;
    ezString m_sTargetDirRootName;
    ezSet<ezString> m_Files;
  };

  ezMap<ezString, DataDirInfo> fileList;
  // ezSet<ezString> assetInputs;

  auto logToFile = [&](const char* msg, const char* msg2)
  {
    logFile.Write(msg, ezStringUtils::GetStringElementCount(msg)).AssertSuccess();
    logFile.Write(msg2, ezStringUtils::GetStringElementCount(msg2)).AssertSuccess();
    logFile.Write("\n", 1).AssertSuccess();
  };

  ezPathPatternFilter filter;

  if (filter.ReadConfigFile("Project.ezExportFilter").Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxInformation(ezFmt("The config file 'Project.ezExportFilter' does not exist or is invalid.\n\nUsing 'Common.ezExportFilter' instead."));

    if (filter.ReadConfigFile("Common.ezExportFilter").Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("The config file 'Common.ezExportFilter' does not exist or is invalid.\n\nCanceling operation.."));

      return;
    }
  }

  {
    ezDynamicArray<ezString> addFiles;

    for (auto pMan : ezDocumentManager::GetAllDocumentManagers())
    {
      if (auto pAssMan = ezDynamicCast<ezAssetDocumentManager*>(pMan))
      {
        pAssMan->GetAdditionalOutputs(addFiles);
      }
    }

    if (ezFileSystem::ResolveSpecialDirectory(">project", sStartPath).Succeeded())
    {
      DataDirInfo& ddInfo = fileList[sStartPath];
      ezSet<ezString>& ddFileList = ddInfo.m_Files;

      for (const auto& file : addFiles)
      {
        ddFileList.Insert(file);
      }
    }
  }

  {
    mainProgress.BeginNextStep("Scanning data directories");

    ezProgressRange progress("Scanning data directories", dataDirs.m_DataDirs.GetCount(), true);

    ezUInt32 uiDataDirNumber = 1;

    for (const auto& dataDir : dataDirs.m_DataDirs)
    {
      progress.BeginNextStep(dataDir.m_sDataDirSpecialPath);

      if (ezFileSystem::ResolveSpecialDirectory(dataDir.m_sDataDirSpecialPath, sStartPath).Failed())
      {
        ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to get special directory '{0}'", dataDir.m_sDataDirSpecialPath));
        return;
      }

      sStartPath.Trim("/\\");
      const ezUInt32 uiStrip = sStartPath.GetElementCount();

      DataDirInfo& ddInfo = fileList[sStartPath];
      ezSet<ezString>& ddFileList = ddInfo.m_Files;

      if (!dataDir.m_sRootName.IsEmpty())
      {
        sTemp.Set("Data/", dataDir.m_sRootName);

        ddInfo.m_sTargetDirRootName = dataDir.m_sRootName;
        ddInfo.m_sTargetDirPath = sTemp;
      }
      else
      {
        sTemp.Format("Data/Extra{}", uiDataDirNumber);

        ddInfo.m_sTargetDirPath = sTemp;
      }

      ezFileSystemIterator it;
      for (it.StartSearch(sStartPath, ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
      {
        if (ezProgress::GetGlobalProgressbar()->WasCanceled())
          return;

        it.GetStats().GetFullPath(sPath);

        sRelPath = sPath;
        sRelPath.Shrink(uiStrip, 0);

        if (it.GetStats().m_bIsDirectory)
        {
          if (!filter.PassesFilters(sRelPath))
          {
            it.SkipFolder();
          }
          else
          {
            it.Next();
          }

          continue;
        }

        if (!filter.PassesFilters(sRelPath))
        {
          it.Next();
          continue;
        }

        // ezDocumentTypeDescriptor* pDocDesc;
        // if (ezDocumentManager::FindDocumentTypeFromPath(sPath, false, pDocDesc).Succeeded())
        //{
        //   if (ezAssetDocumentManager* pAssetMan = ezDynamicCast<ezAssetDocumentManager*>(pDocDesc->m_pManager))
        //   {
        //     pAssetMan->GetRelativeOutputFileName(
        //   }
        // }

        auto asset = ezAssetCurator::GetSingleton()->FindSubAsset(sPath);
        if (asset.isValid())
        {
          // redirect to asset output
          if (asset->m_bMainAsset)
          {
            ezAssetDocumentManager* pAssetMan = ezStaticCast<ezAssetDocumentManager*>(asset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_pManager);

            ezStringBuilder sRelFile = pAssetMan->GetRelativeOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, sStartPath, asset->m_pAssetInfo->m_sAbsolutePath, nullptr);

            sRelFile.Prepend("AssetCache/");
            ddFileList.Insert(sRelFile);

            for (const ezString& outputTag : asset->m_pAssetInfo->m_Info->m_Outputs)
            {
              sRelFile = pAssetMan->GetRelativeOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, sStartPath, asset->m_pAssetInfo->m_sAbsolutePath, outputTag);

              sRelFile.Prepend("AssetCache/");
              ddFileList.Insert(sRelFile);
            }
          }

          // ignore all asset files
          it.Next();
          continue;
        }

        ddFileList.Insert(sRelPath);
        it.Next();
      }
    }
  }

  {
    mainProgress.BeginNextStep("Gathering binaries");

    // ezProgressRange range("Gathering binaries", true);

    sPath = ezOSFile::GetApplicationDirectory();
    sPath.MakeCleanPath();
    sPath.Trim("/\\");
    const ezUInt32 uiStrip = sPath.GetElementCount();

    DataDirInfo& ddInfo = fileList[sPath];
    ezSet<ezString>& ddFileList = ddInfo.m_Files;

    ddInfo.m_sTargetDirPath = "Bin";
    ddInfo.m_sTargetDirRootName = "-"; // don't add to data dir config

    ddFileList.Insert("Player.exe");

    ezFileSystemIterator it;
    for (it.StartSearch(sPath, ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
    {
      if (ezProgress::GetGlobalProgressbar()->WasCanceled())
        return;

      const ezStringBuilder fileName = it.GetStats().m_sName;

      if (it.GetStats().m_bIsDirectory)
      {
        if (fileName == "iconengines" ||
            fileName == "imageformats" ||
            fileName == "platforms")
        {
          it.SkipFolder();
          continue;
        }

        it.Next();
        continue;
      }

      if (fileName.HasExtension("exe") ||
          fileName.HasExtension("pdb") ||
          fileName.HasExtension("loaded"))
      {
        it.Next();
        continue;
      }

      if (fileName.HasExtension("dll"))
      {
        if (fileName.StartsWith("ezEditorPlugin") ||
            fileName.StartsWith("ezEnginePlugin") ||
            fileName.StartsWith("ezSharedPlugin") ||
            fileName.StartsWith("Qt5") ||
            fileName.StartsWith("Qt6"))
        {
          it.Next();
          continue;
        }
      }

      it.GetStats().GetFullPath(sPath);
      sPath.Shrink(uiStrip, 0);
      ddFileList.Insert(sPath);
      it.Next();
    }
  }

  // write data dir config file
  {
    mainProgress.BeginNextStep("Writing data directory config");

    ezApplicationFileSystemConfig cfg;

    for (auto itDir = fileList.GetIterator(); itDir.IsValid(); ++itDir)
    {
      const auto& info = itDir.Value();

      if (info.m_sTargetDirRootName == "-")
        continue;

      sPath.Set(">sdk/", info.m_sTargetDirPath);

      auto& ddc = cfg.m_DataDirs.ExpandAndGetRef();
      ddc.m_sDataDirSpecialPath = sPath;
      ddc.m_sRootName = info.m_sTargetDirRootName;
    }

    sPath.Set(szDstFolder, "/Data/project/DataDirectories.ddl");
    if (cfg.Save(sPath).Failed())
    {
      if (cfg.Save(sPath).Failed())
        ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to write data directory config file '{0}'", sPath));
      return;
    }
  }

  ezDynamicArray<ezString> sceneFiles;

  {
    mainProgress.BeginNextStep("Copying files");

    ezUInt32 uiTotalFiles = 0;
    for (auto itDir = fileList.GetIterator(); itDir.IsValid(); ++itDir)
      uiTotalFiles += itDir.Value().m_Files.GetCount();

    ezProgressRange range("Copying files", uiTotalFiles, true);

    logToFile("Output folder: ", szDstFolder);

    for (auto itDir = fileList.GetIterator(); itDir.IsValid(); ++itDir)
    {
      logToFile("Source sub-folder: ", itDir.Key());
      logToFile("Destination sub-folder: ", itDir.Value().m_sTargetDirPath);

      for (auto itFile = itDir.Value().m_Files.GetIterator(); itFile.IsValid(); ++itFile)
      {
        if (ezProgress::GetGlobalProgressbar()->WasCanceled())
          return;

        range.BeginNextStep(itFile.Key());

        sPath.Set(itDir.Key(), "/", itFile.Key());
        sTemp.Set(szDstFolder, "/", itDir.Value().m_sTargetDirPath, "/", itFile.Key());

        if (ezOSFile::CopyFile(sPath, sTemp).Succeeded())
        {
          logToFile(" -> ", itFile.Key());

          if (sTemp.EndsWith_NoCase(".ezObjectGraph") && sTemp.FindSubString_NoCase("scene"))
          {
            sceneFiles.PushBack(sTemp);
          }
        }
        else
        {
          logToFile(" Copy failed:", itFile.Key());
        }
      }
    }
  }

  // write .bat files
  for (const auto& sf : sceneFiles)
  {
    ezStringBuilder cmd;
    cmd.Format("start Bin/Player.exe -scene \"{}", sf);

    ezStringBuilder bat;
    bat.Format("{}/Launch {}.bat", szDstFolder, ezPathUtils::GetFileName(sf));

    ezOSFile file;
    if (file.Open(bat, ezFileOpenMode::Write).Succeeded())
    {
      file.Write(cmd.GetData(), cmd.GetElementCount()).AssertSuccess();
    }
  }

  accept();
}
